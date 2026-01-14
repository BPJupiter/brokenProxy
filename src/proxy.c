#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "shared_context.h"
#include "cjson/cJSON.h"
#include "memory_usage.h"
#ifdef C_MEMORY_DEBUG
#include "Callisto/callisto.h"
#endif

#define PROXY_HOST "127.0.0.1"
#define BUFFER_SIZE 16384
#define MAX_CLIENTS 8
#define SETTINGS_DIR "settings/settings.json"

static int proxy_printf(const char *format, ...);
#define printf proxy_printf

typedef unsigned char uchar;
typedef unsigned int uint;

typedef struct
{
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

typedef struct
{
    int source_fd;
    int dest_fd;
} tunnel_args_t;

struct settings
{
    char pingEnabled;
    char trEnabled;
    char locDNSEnabled;
    double rtt;
};

static void free_answers(unsigned char **answers, int n_ans);
static int host_port_from_url(const char *url, char *host, int *port);
static void send_local_file(char *filename, char *buffer, int client_fd);
static int get_content_length(const char *request);
static void set_json_settings(int client_fd, char *initial_buffer);
static void *tunnel_data(void *arg);
static void *handle_client(void *arg);
static void update_proxy_settings(void);

static pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_thread_count = 0;

static pthread_mutex_t settings_mod_mutex = PTHREAD_MUTEX_INITIALIZER;
static int settings_modified = 1;

static void free_answers(unsigned char **answers, int n_ans)
{
    int i;
    for (i = 0; i < n_ans; i++)
        free(answers[i]);
    free(answers);
}

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

static void send_local_file(char *filename, char *buffer, int client_fd)
{
    FILE *fp;

    char path[256] = {0};
    char extension[32] = {0};

    long content_length;
    size_t bytes_read;

    if (strcmp(filename, "/settings.json") == 0)
        strcpy(path, "settings/");
    else strcpy(path, "static/");

    strcat(path, filename + 1);
    if (strcmp(filename, "/") == 0)
        strcat(path, "index.html");

    strcpy(extension, "text/plain"); /* Default */
    if (strstr(path, ".html") != NULL)        strcpy(extension, "text/html");
    else if (strstr(path, ".css") != NULL)    strcpy(extension, "text/css");
    else if (strstr(path, ".json") != NULL)   strcpy(extension, "application/json");
    else if (strstr(path, ".js.map") != NULL) strcpy(extension, "application/json");
    else if (strstr(path, ".js") != NULL)     strcpy(extension, "text/javascript");
    else if (strstr(path, ".ico") != NULL)    strcpy(extension, "image/x-icon");

    fp = fopen(path, "rb");
    if (!fp)
    {
        printf("404 File not found: %s\n", path);
        return;
    }
    fseek(fp, 0L, SEEK_END);
    content_length = ftell(fp);
    rewind(fp);

    sprintf(buffer,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            extension, content_length);

    send(client_fd, buffer, strlen(buffer), 0);
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        send(client_fd, buffer, bytes_read, 0);
    }

    fclose(fp);
}

static int get_content_length(const char *request)
{
    const char *ptr = strstr(request, "Content-Length:");
    if (ptr)
    {
        return atoi(ptr + 15);
    }
    return 0;
}

static void set_json_settings(int client_fd, char *initial_buffer)
{
    FILE *fp;
    char buffer[BUFFER_SIZE];
    char *new_json;

    cJSON *file_json;
    cJSON *payload_json;

    cJSON *settings;
    cJSON *pingObj;
    cJSON *tracertObj;
    cJSON *localDNSObj;

    cJSON *incomingCables;

    const char *HTTP_200 =
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "OK";

    const char *HTTP_500 =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n";

    size_t bytes_read;

    char *body_start = strstr(initial_buffer, "\r\n\r\n");
    if (!body_start)
    {
        send(client_fd, HTTP_500, strlen(HTTP_500), 0);
        return;
    }
    body_start += 4;


    payload_json = cJSON_Parse(body_start);
    if (payload_json == NULL)
    {
        printf("Error: Failed to parse incoming POST JSON body.\n");
        send(client_fd, HTTP_500, strlen(HTTP_500), 0);
        return;
    }

    fp = fopen(SETTINGS_DIR, "r");
    if (fp == NULL)
    {
        perror("Error opening settings.json file!");
        cJSON_Delete(payload_json);
        send(client_fd, HTTP_500, strlen(HTTP_500), 0);
        return;
    }

    bytes_read = fread(buffer, 1, sizeof(buffer), fp);
    buffer[bytes_read] = '\0';
    fclose(fp);

    file_json = cJSON_Parse(buffer);
    if (file_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("CJSON Error: %s\n", error_ptr);
        }
        cJSON_Delete(payload_json);
        send(client_fd, HTTP_500, strlen(HTTP_500), 0);
        return;
    }

    settings = cJSON_GetObjectItemCaseSensitive(file_json, "settings");
    pingObj = cJSON_GetObjectItemCaseSensitive(settings, "ping");
    tracertObj = cJSON_GetObjectItemCaseSensitive(settings, "traceroute");
    localDNSObj = cJSON_GetObjectItemCaseSensitive(settings, "localDNS");

    if (!cJSON_IsObject(settings)
        || !cJSON_IsObject(pingObj)
        || !cJSON_IsObject(tracertObj)
        || !cJSON_IsObject(localDNSObj)
        )
    {
        printf("Error: JSON structure invalid!\n");
        cJSON_Delete(file_json);
        cJSON_Delete(payload_json);
        send(client_fd, HTTP_500, strlen(HTTP_500), 0);
        return;
    }

    cJSON_ReplaceItemInObject(pingObj, "maxLatency",
                              cJSON_CreateNumber(cJSON_GetObjectItem(payload_json, "maxLatency")->valuedouble));
    cJSON_ReplaceItemInObject(pingObj, "pingEnabled",
                              cJSON_CreateBool(cJSON_GetObjectItem(payload_json, "pingEnabled")->valueint));
    cJSON_ReplaceItemInObject(tracertObj, "trEnabled",
                              cJSON_CreateBool(cJSON_GetObjectItem(payload_json, "trEnabled")->valueint));
    cJSON_ReplaceItemInObject(localDNSObj, "locDNSEnabled",
                              cJSON_CreateBool(cJSON_GetObjectItem(payload_json, "locDNSEnabled")->valueint));

    incomingCables = cJSON_GetObjectItem(payload_json, "disabledCables");

    if (incomingCables != NULL && cJSON_IsArray(incomingCables))
    {
        if (cJSON_HasObjectItem(settings, "disabledCables"))
        {
            cJSON_DeleteItemFromObject(settings, "disabledCables");
        }
        cJSON_AddItemToObject(settings, "disabledCables", cJSON_Duplicate(incomingCables, 1));
    }

    new_json = cJSON_Print(file_json);
    fp = fopen(SETTINGS_DIR, "w");
    if (fp)
    {
        fputs(new_json, fp);
        fclose(fp);
        send(client_fd, HTTP_200, strlen(HTTP_200), 0);
    }
    else
    {send(client_fd, HTTP_500, strlen(HTTP_500), 0);}

    if (new_json)
    {
    #ifdef C_MEMORY_DEBUG
        c_no_debug_free(new_json);
    #else
        free(new_json);
    #endif
    }
    cJSON_Delete(file_json);
    cJSON_Delete(payload_json);
}

/* Tunnel data from source to destination */
static void *tunnel_data(void *arg)
{
    tunnel_args_t *args = (tunnel_args_t *)arg;
    char buffer[BUFFER_SIZE] = {0};
    struct timeval timeout = {0};
    int sent;

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    setsockopt(args->source_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1)
    {
        int n = recv(args->source_fd, buffer, BUFFER_SIZE, 0);
        if (n <= 0)
        {
            break;
        }

        sent = 0;
        while (sent < n)
        {
            int s = send(args->dest_fd, buffer + sent, n - sent, 0);
            if (s <= 0)
            {
                return NULL;
            }
            sent += s;
        }
    }

    return NULL;
}

/* Handle individual client connection */
static void *handle_client(void *arg)
{
    client_info_t *client_info = (client_info_t *)arg;
    argType a = {0};
    retType r = {0};
    int client_fd = client_info->client_fd;
    int remote_fd = -1;

    char buffer[BUFFER_SIZE] = {0};
    char host[512] = {0};
    char first_line[1024] = {0};
    char method[16], url_buf[512], version[16];
    char *url = NULL;
    char *first_line_end;

    unsigned char **answers;
    char *destination_ip;
    int n_ans;
    struct sockaddr_in remote_addr;

    int first_line_len;
    int current_thread_count;
    int bytes_recvd;
    int port = 80;
    double rtt_cutoff, latency;

    sharedContext_getVariable(SCV_MAX_RTT, &rtt_cutoff);

    pthread_mutex_lock(&thread_count_mutex);
    active_thread_count++;
    current_thread_count = active_thread_count;
    pthread_mutex_unlock(&thread_count_mutex);

    printf("\x1b[33m[Info]\x1b[0m ThreadCount: %d\n", current_thread_count);
    printf("Thread %ld: %s\n", (long)arg, get_memory_usage_str_kb());

    /* Receive initial request */
    bytes_recvd = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_recvd <= 0)
    {
        goto cleanup;
    }
    buffer[bytes_recvd] = '\0';

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
    if (sscanf(first_line, "%s %s %s", method, url_buf, version) != 3)
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
    a.resolve.hostname = host;
    a.resolve.answer_index = &answers;
    sharedContext_execCb(SCC_RESOLVE, &r, &a);
    n_ans = r.resolve;

    if (n_ans == (int)((unsigned short)-1))
    {
        printf("DNS RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
        goto cleanup;
    }

    if (n_ans == 0)
    {
        printf("\x1b[31m[Error]\x1b[0m Failed to resolve hostname: %s\n", host);
        goto cleanup;
    }

    destination_ip = (char *)answers[0];
    printf("%s resolved to %s\n", host, destination_ip);

    if (strcmp(destination_ip, "127.0.0.1") != 0)
    {
        a.ping.ip = destination_ip;
        sharedContext_execCb(SCC_PING, &r, &a);
        latency = r.ping;
        if (latency > rtt_cutoff)
        {
            printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
            free_answers(answers, n_ans);
            goto cleanup;
        }
    }

    /* Load proxy settings page if requested */
    /* This code sucks. But its short enough that i dont care. */
    if (strstr(first_line, "GET") != NULL)
    {
        send_local_file(host, buffer, client_fd);

        free_answers(answers, n_ans);
        goto cleanup;
    }

    if (strstr(first_line, "POST") != NULL)
    {
        pthread_mutex_lock(&settings_mod_mutex);
        set_json_settings(client_fd, buffer);
        settings_modified = 1;
        pthread_mutex_unlock(&settings_mod_mutex);

        free_answers(answers, n_ans);
        goto cleanup;
    }

    /* Connect to remote server */
    remote_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_fd < 0)
    {
        free_answers(answers, n_ans);
        goto cleanup;
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr.s_addr = inet_addr(destination_ip);

    if (connect(remote_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) <
        0)
    {
        printf("\x1b[31m[Error]\x1b[0m Failed to connect to %s:%d - %s\n",
               destination_ip, port, strerror(errno));
        free_answers(answers, n_ans);
        goto cleanup;
    }

    printf("Accepted request for: %s:%d\n", host, port);

    /* Handle CONNECT method (HTTPS tunnel) */
    if (strstr(first_line, "CONNECT") != NULL)
    {
        const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        printf("Client established connection!\n");
    }
    else
    {
        /* Regular HTTP - forward the initial request */
        send(remote_fd, buffer, bytes_recvd, 0);
        printf("Remote socket sent request!\n");
    }

    free_answers(answers, n_ans);

    /* Create bidirectional tunnel */
    {
        pthread_t client_to_remote_thread, remote_to_client_thread;

        tunnel_args_t *c2r_args = malloc(sizeof(*c2r_args));
        tunnel_args_t *r2c_args = malloc(sizeof(*r2c_args));

        c2r_args->source_fd = client_fd;
        c2r_args->dest_fd = remote_fd;

        r2c_args->source_fd = remote_fd;
        r2c_args->dest_fd = client_fd;

        pthread_create(&client_to_remote_thread, NULL, tunnel_data, c2r_args);
        pthread_create(&remote_to_client_thread, NULL, tunnel_data, r2c_args);

        /* Wait for both threads to complete */
        pthread_join(client_to_remote_thread, NULL);
        pthread_join(remote_to_client_thread, NULL);

        free(c2r_args);
        free(r2c_args);
    }

cleanup:
    if (client_fd >= 0)
        close(client_fd);
    if (remote_fd >= 0)
        close(remote_fd);
    free(client_info);

    pthread_mutex_lock(&thread_count_mutex);
    active_thread_count--;
    pthread_mutex_unlock(&thread_count_mutex);

    return NULL;
}

static void update_proxy_settings(void)
{
    FILE *fp;
    char buffer[16384];
    cJSON *json;
    cJSON *settings;

    cJSON *pingObj;
    cJSON *tracertObj;
    cJSON *localDNSObj;

    cJSON *pingEnabled;
    cJSON *maxLatency;

    cJSON *trEnabled;

    cJSON *locDNSEnabled;

    fp = fopen(SETTINGS_DIR, "r");
    if (fp == NULL)
    {
        perror("Error opening settings.json file!");
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
            printf("CJSON Error: %s\n", error_ptr);
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
    sharedContext_toggleCb(SCC_PING, cJSON_IsTrue(pingEnabled));
    sharedContext_toggleCb(SCC_TRACERT, cJSON_IsTrue(trEnabled));
    sharedContext_toggleCb(SCC_RESOLVE, !cJSON_IsTrue(locDNSEnabled));

    cJSON_Delete(json);
    return;
}

int proxy_start(int proxy_port)
{
    int server_fd;
    struct sockaddr_in addr;
    int opt;

    /* Ignore SIGPIPE - prevents crash when writing to closed socket */
    signal(SIGPIPE, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    /* Set socket options */
    opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(PROXY_HOST);
    addr.sin_port = htons(proxy_port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(1);
    }

    sharedContext_toggleCb(SCC_RESOLVE, 1);

    printf("Proxy server listening on %s:%d\n", PROXY_HOST, proxy_port);

    /* Accept connections and spawn threads */
    while (1)
    {
        pthread_t thread;
        pthread_attr_t attr;
        client_info_t *client_info;
        socklen_t client_len;
        printf("Memory (KB): %s\n", get_memory_usage_str_kb());
        client_info = malloc(sizeof(*client_info));
        client_len = sizeof(client_info->client_addr);

        pthread_mutex_lock(&settings_mod_mutex);
        if (settings_modified)
        {
            update_proxy_settings();
            settings_modified = 0;
        }
        pthread_mutex_unlock(&settings_mod_mutex);

        client_info->client_fd = accept(server_fd, (struct sockaddr *)&client_info->client_addr, &client_len);

        if (client_info->client_fd < 0)
        {
            perror("accept");
            free(client_info);
            continue;
        }

        printf("Accepted connection from %s:%d\n",
               inet_ntoa(client_info->client_addr.sin_addr),
               ntohs(client_info->client_addr.sin_port));

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread, &attr, handle_client, client_info) != 0)
        {
            perror("pthread_create");
            close(client_info->client_fd);
            free(client_info);
        }

        pthread_attr_destroy(&attr);
    }

    close(server_fd);
    return 0;
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
