#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "context/context.h"
#include "verify/verify.h"
#include "cjson/cJSON.h"
#include "memory_usage.h"
#include "proxy.h"
#include "termcolor.h"

#define PROXY_HOST "127.0.0.1"
#define BUFFER_SIZE 16384
#define MAX_CLIENTS 8
#define PROJECT_ROOT_FOLDER_NAME "brokenProxy"


static int proxy_printf(const char *format, ...);
#define printf proxy_printf

typedef struct
{
    VSocket client_handle;
    StyxSockaddrInet client_addr;
} client_info_t;

typedef struct
{
    VSocket source_handle;
    VSocket dest_handle;
} tunnel_args_t;

struct settings
{
    char pingEnabled;
    char trEnabled;
    char locDNSEnabled;
    double rtt;
};

static int host_port_from_url(const char *url, char *host, int *port);
static void tunnel_data(void *arg);
static void handle_client(void *arg);
static void update_proxy_settings(void);

static void *thread_count_mutex = NULL;
static int active_thread_count = 0;

static void *settings_mod_mutex = NULL;
static int settings_modified = 1;

static boolean shutdown_flag = FALSE;

/* must be global such that shutdown can close() */
VSocket gServer_handle;

static int host_port_from_url(const char *url, char *host, int *port)
{
    const char *http_pos = strstr(url, "http://");
    const char *https_pos = strstr(url, "https://");

    if (http_pos != NULL)
    {
        const char *temp = http_pos + 7;
        const char *slash = strchr(temp, '/');
        size_t host_len = slash ? (size_t)(slash - temp) : strlen(temp);
        strncpy(host, temp, host_len);
        host[host_len] = '\0';
        *port = 80;
        return 0;
    }
    else if (https_pos != NULL)
    {
        const char *temp = https_pos + 8;
        const char *slash = strchr(temp, '/');
        size_t host_len = slash ? (size_t)(slash - temp) : strlen(temp);
        strncpy(host, temp, host_len);
        host[host_len] = '\0';
        *port = 443;
        return 0;
    }
    else
    {
        /* No protocol, parse as host:port */
        const char *colon = strchr(url, ':');
        if (colon)
        {
            int host_len = colon - url;
            strncpy(host, url, host_len);
            host[host_len] = '\0';
            *port = atoi(colon + 1);
        }
        else
        {
            strcpy(host, url);
            *port = 80;
        }
        return 0;
    }
}

static int send_all(VSocket socket, const char *buf, uint len)
{
    uint total_sent = 0;
    uint bytes_left = len;
    int n;

    while (total_sent < len)
    {
        n = send(socket, buf + total_sent, bytes_left, 0);
        if (n == -1) { return -1; }
        total_sent += n;
        bytes_left -= n;
    }
    return 0;
}

/* Tunnel data from source to destination */
static void tunnel_data(void *arg)
{
    tunnel_args_t *args = (tunnel_args_t *)arg;
    char buffer[BUFFER_SIZE] = {0};
    struct timeval timeout = {0};
    int sent;

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    setsockopt(args->source_handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1)
    {
        int n = recv(args->source_handle, buffer, BUFFER_SIZE, 0);
        if (n <= 0)
        {
            break;
        }

        sent = 0;
        while (sent < n)
        {
            int s = send(args->dest_handle, buffer + sent, n - sent, 0);
            if (s <= 0)
            {
                return;
            }
            sent += s;
        }
    }

    return;
}

/* Handle individual client connection */
static void handle_client(void *arg)
{
    client_info_t *client_info = (client_info_t *)arg;
    VSocket client_handle = client_info->client_handle;
    VSocket remote_handle = -1;

    char buffer[BUFFER_SIZE] = {0};
    char host[512] = {0};
    char first_line[1024] = {0};
    char method[16], url_buf[512], version[16];
    char *url = NULL;
    char *first_line_end = NULL;

    DnsResult dns_result = {0};
    char *destination_ip = NULL;
    struct sockaddr_in remote_addr;

    int first_line_len;
    int current_thread_count;
    int bytes_recvd;
    int port = 80;
    double rtt_cutoff;

    sharedContext_getVariable(SCV_MAX_RTT, &rtt_cutoff);

    europa_mutex_lock(thread_count_mutex);
    active_thread_count++;
    current_thread_count = active_thread_count;
    europa_mutex_unlock(thread_count_mutex);

    printf("\x1b[33m[Info]\x1b[0m ThreadCount: %d\n", current_thread_count);
    printf("Thread %ld: %s\n", (long)arg, get_memory_usage_str_kb());

    /* Receive initial request */
    bytes_recvd = recv(client_handle, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_recvd <= 0)
    {
        goto cleanup;
    }
    buffer[bytes_recvd] = '\0';
    
    /* ----------------------------------------------------- */
    printf("\nBUFFER:\n%s...\n", buffer);
    return;

    /* Parse first line - split by newline */
    first_line_end = strchr(buffer, '\n');
    if (!first_line_end)
    {
        goto cleanup;
    }

    first_line_len = first_line_end - buffer;
    strncpy(first_line, buffer, first_line_len);
    first_line[first_line_len] = '\0';

    /* Parse: METHOD URL VERSION */
    if (sscanf(first_line, "%15s %511s %15s", method, url_buf, version) != 3)
    {
        goto cleanup;
    }

    url = url_buf;

    /* Extract host and port from URL */
    if (host_port_from_url(url, host, &port) != 0)
    {
        goto cleanup;
    }

    /* Resolve hostname using custom DNS resolver */
    sharedContext_callback_execute_dnsResolve(&dns_result, host);

    if (dns_result.status == DNS_ERR_RTT_EXHAUSTED)
    {
        printf("DNS Resultion failed due to rtt cutoff of %.2lf ms! Packet dropped!\n", rtt_cutoff);
        goto cleanup;
    }

    if (dns_result.nAns == 0)
    {
        printf("%d\n", dns_result.status);
        printf("\x1b[31m[Error]\x1b[0m Failed to resolve hostname: %s\n", host);
        goto cleanup;
    }

    destination_ip = dns_result.answers[0];
    printf("%s resolved to %s\n", host, destination_ip);

    /* VERIFICATION STEP */
	if (!verify_latency(destination_ip))
	{
		printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
		DnsResult_free(&dns_result);
		goto cleanup;
	}
	if (!verify_cable(destination_ip))
	{
		printf("Request uses disabled cable! Packet dropped!\n");
		DnsResult_free(&dns_result);
		goto cleanup;
	}

    /* Connect to remote server */
    remote_handle = styx_socket_create(1, 0);
    if (!styx_socket_assert(remote_handle, "Couldn't create remote socket "))
    {
        DnsResult_free(&dns_result);
        goto cleanup;
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr.s_addr = inet_addr(destination_ip);

    if (connect(remote_handle, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
    {
        printf("\x1b[31m[Error]\x1b[0m Failed to connect to %s:%d - %s\n", destination_ip, port, strerror(errno));
        DnsResult_free(&dns_result);
        goto cleanup;
    }

    printf("Accepted request for: %s:%d\n", host, port);

    /* Handle CONNECT method (HTTPS tunnel) */
    if (strstr(first_line, "CONNECT") != NULL)
    {
        const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client_handle, response, strlen(response), 0);
        printf("Client established connection!\n");
    }
    else
    {
        /* Regular HTTP - forward the initial request */
        send(remote_handle, buffer, bytes_recvd, 0);
        printf("Remote socket sent request!\n");
    }

    DnsResult_free(&dns_result);

    /* Create bidirectional tunnel */
    {
        EuropaThread client_to_remote_thread, remote_to_client_thread;

        tunnel_args_t *c2r_args = malloc(sizeof(*c2r_args));
        tunnel_args_t *r2c_args = malloc(sizeof(*r2c_args));

        talos_malloc_assert(c2r_args);
        talos_malloc_assert(r2c_args);

        c2r_args->source_handle = client_handle;
        c2r_args->dest_handle = remote_handle;

        r2c_args->source_handle = remote_handle;
        r2c_args->dest_handle = client_handle;

        client_to_remote_thread = europa_thread(tunnel_data, c2r_args, NULL);
        remote_to_client_thread = europa_thread(tunnel_data, r2c_args, NULL);

        /* Wait for both threads to complete */
        europa_join(client_to_remote_thread);
        europa_join(remote_to_client_thread);

        free(c2r_args);
        free(r2c_args);
    }

cleanup:
    if (styx_socket_assert(client_handle, NULL))
        styx_socket_destroy(client_handle);
    if (styx_socket_assert(remote_handle, NULL))
        styx_socket_destroy(remote_handle);
    free(client_info);

    if (NULL != thread_count_mutex)
    {
        europa_mutex_lock(thread_count_mutex);
        active_thread_count--;
        europa_mutex_unlock(thread_count_mutex);
    }

    return;
}

static void update_proxy_settings(void)
{
    FILE *fp;
    char buffer[BUFFER_SIZE];
    cJSON *json;
    cJSON *settings;

    cJSON *pingObj;
    cJSON *tracertObj;
    cJSON *localDNSObj;

    cJSON *pingEnabled;
    cJSON *maxLatency;

    cJSON *trEnabled;

    cJSON *locDNSEnabled;

    fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, "settings/settings.json", "rb");
    if (fp == NULL)
    {
        talos_print_error("Error opening settings.json file!");
        return;
    }

    fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("CJSON Error: %s: line %d file %s\n", error_ptr, __LINE__, __FILE__);
        }
        cJSON_Delete(json);
        return;
    }

    settings = cJSON_GetObjectItemCaseSensitive(json, "settings");
    pingObj = cJSON_GetObjectItemCaseSensitive(settings, "ping");
    tracertObj = cJSON_GetObjectItemCaseSensitive(settings, "traceroute");
    localDNSObj = cJSON_GetObjectItemCaseSensitive(settings, "localDNS");
    maxLatency = cJSON_GetObjectItemCaseSensitive(pingObj, "maxLatency");
    pingEnabled = cJSON_GetObjectItemCaseSensitive(pingObj, "pingEnabled");
    trEnabled = cJSON_GetObjectItemCaseSensitive(tracertObj, "trEnabled");
    locDNSEnabled = cJSON_GetObjectItemCaseSensitive(localDNSObj, "locDNSEnabled");

    if (!cJSON_IsObject(settings)
        || (!cJSON_IsObject(pingObj)
            || !cJSON_IsNumber(maxLatency)
            || !cJSON_IsBool(pingEnabled))
        || (!cJSON_IsObject(tracertObj)
            || !cJSON_IsBool(trEnabled))
        || (!cJSON_IsObject(localDNSObj)
            || !cJSON_IsBool(locDNSEnabled))
        )
    {
        printf("Error: JSON structure invalid!\n");
        cJSON_Delete(json);
        return;
    }

    sharedContext_setVariable(SCV_MAX_RTT, &maxLatency->valuedouble);
    sharedContext_callback_toggle_ping(cJSON_IsTrue(pingEnabled));
    sharedContext_callback_toggle_traceroute(cJSON_IsTrue(trEnabled));
    sharedContext_callback_toggle_dnsResolve(!cJSON_IsTrue(locDNSEnabled));

    cJSON_Delete(json);
    return;
}

int proxy_start(int proxy_port)
{
    struct sockaddr_in addr;
    int opt;

#ifndef _WIN32
    /* Ignore SIGPIPE - prevents crash when writing to closed socket */
    signal(SIGPIPE, SIG_IGN);
#endif

    gServer_handle = styx_socket_create(TRUE, 0);
    if (!styx_socket_assert(gServer_handle, "Server socket creation: "))
        exit(1);

    /* Set socket options */
    opt = 1;
    setsockopt(gServer_handle, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(PROXY_HOST);
    addr.sin_port = htons(proxy_port);

    if (bind(gServer_handle, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        styx_print_error("Bind failed. ");
        exit(1);
    }

    if (listen(gServer_handle, MAX_CLIENTS) < 0)
    {
        styx_print_error("Listen failed");
        exit(1);
    }

    sharedContext_callback_toggle_dnsResolve(RT_FULL);
    settings_mod_mutex = europa_mutex_create();
    thread_count_mutex = europa_mutex_create();

    printf("Proxy server listening on %s:%d\n", PROXY_HOST, proxy_port);

    /* Accept connections and spawn threads */
    while (1)
    {
        EuropaThread thread = 0;
        client_info_t *client_info = NULL;
        socklen_t client_len;

        if (shutdown_flag)
        {
            break;
        }

        printf("Memory (KB): %s\n", get_memory_usage_str_kb());

        client_info = malloc(sizeof(*client_info));
        client_len = sizeof(client_info->client_addr);

        talos_malloc_assert(client_info);

        europa_mutex_lock(settings_mod_mutex);
        if (settings_modified)
        {
            update_proxy_settings();
            settings_modified = 0;
        }
        europa_mutex_unlock(settings_mod_mutex);

        client_info->client_handle = accept(gServer_handle, (struct sockaddr *)&client_info->client_addr.Ipv4, &client_len);

        if (!styx_socket_assert(client_info->client_handle, "Accept error"))
        {
            free(client_info);
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_info->client_addr.Ipv4.sin_addr), ntohs(client_info->client_addr.Ipv4.sin_port));

        thread = europa_thread(handle_client, client_info, NULL);
        if (thread == 0)
        {
            talos_print_error("Thread creation");
            styx_socket_destroy(client_info->client_handle);
            free(client_info);
        }
    }

    while (active_thread_count > 0)
        europa_sleepi(0, 10000000);

    europa_mutex_destroy(thread_count_mutex);
    thread_count_mutex = NULL;

    return 0;
}

void proxy_shutdown()
{
    shutdown_flag = TRUE;
    shutdown(gServer_handle, 2);
    styx_socket_destroy(gServer_handle);
    /* wait for all threads to finish but this one and the main thread */
    while (active_thread_count > 1)
        europa_sleepi(0, 10000000);
    sharedContext_destroy();
    europa_mutex_destroy(settings_mod_mutex);
    settings_mod_mutex = NULL;
}

static int proxy_printf(const char *format, ...)
{
    va_list args;
    char msg[1024] = "\x1b[35m[Proxy]\x1b[0m ";
    int ret;

    strcat(msg, format);

    va_start(args, format);
    ret = vprintf(msg, args);
    va_end(args);
    fflush(stdout);

    return ret;
}
