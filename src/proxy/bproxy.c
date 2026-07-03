#include "bproxy.h"

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "network_tools.h"
#include "memory_usage.h"

#include <signal.h>
#include <stdarg.h>
#include <errno.h>

#if defined(_WIN32)
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#define BUFFER_SIZE (1 << 16)
#define UDP_HEADER_IPV4_LEN 10
#define IP_SIZE 4
#define MAX_CLIENTS 256
#define PROJECT_ROOT_FOLDER_NAME "brokenProxy"

enum socks {
    RESERVED = 0x00,
    VERSION4 = 0x04,
    VERSION5 = 0x05
};

enum socks_auth_methods {
    NOAUTH = 0x00,
    GSSAPI = 0x01,
    USERPASS = 0x02,
    NOMETHOD = 0xff
};

enum socks_auth_userpass {
    AUTH_OK = 0x00,
    AUTH_VERSION = 0x01,
    AUTH_FAIL = 0xff
};

enum socks_command {
    CONNECT = 0x01,
    BIND = 0x02,
    UDP_ASSOCIATE = 0x03
};

enum socks_command_type {
    IPV4 = 0x01,
    DOMAIN = 0x03,
    IPV6 = 0x04
};

enum socks_status {
    OK = 0x00,
    GENERAL_FAILURE = 0x01,
    NOT_ALLOWED = 0x02,
    NETWORK_UNREACHABLE = 0x03,
    HOST_UNREACHABLE = 0x04,
    CONNECTION_REFUSED = 0x05,
    TTL_EXPIRED = 0x06,
    UNSUPPORTED_CMD = 0x07,
    UNSUPPORTED_ADDRESS = 0x08
};

typedef struct {
    struct {
        SHandle *client;
        SHandle *target;
        boolean active;
    } pairs[MAX_CLIENTS];
    uint count;
    void *mutex;
} HandleSet;

typedef struct {
    boolean do_ping;
    boolean do_traceroute;
    boolean use_local_dns;
    uint32 max_rtt_ns;
} BProxySettings;

typedef struct BProxyHandle {
    BProxySettings settings;
    HandleSet handles;
    EThreadPool threadpool;
    SHandle *server_handle;
    char *settings_filename;
    uint16 port;
} BProxyHandle;

static void client_thread(SHandle *client_handle);
static boolean target_addr_get_from_buffer(BProxyHandle *proxy, SHandle *client_handle, StyxNetworkAddress *target_addr, char *target_ip_str, char *target_domain);
static int transfer_data(SHandle *from_handle, SHandle *to_handle);
static void pipe_handle_data_thread(void *arg);

static int logln(const char *format, ...);
static void reload_config(BProxyHandle *proxy);

BProxyHandle *bp_init(uint16 port, const char *settings_filename)
{
    BProxyHandle *proxy = calloc(1, sizeof(BProxyHandle));
    proxy->threadpool = europa_threadpool_init(8);
    proxy->handles.mutex = europa_mutex_create();
    proxy->settings_filename = malloc(strlen(settings_filename) + 1);
    strcpy(proxy->settings_filename, settings_filename);
    proxy->port = port;
    proxy->server_handle = styx_network_stream_address_create(NULL, proxy->port, FALSE);
    if (!proxy->server_handle) {
        logln("FATAL: Could not init server_handle!");
        exit(1);
    }
    if (!europa_threadpool_add_work(proxy->threadpool, pipe_handle_data_thread, proxy)) {
        logln("FATAL: Couldn't launch pipe thread!");
        exit(1);
    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    reload_config(proxy);
    logln("SOCKS5 Proxy setup on port: %d", proxy->port);
    return proxy;
}

void bp_destroy(BProxyHandle *proxy)
{
    if (proxy) {
        styx_free(proxy->server_handle);
        free(proxy->settings_filename);
        europa_mutex_destroy(proxy->handles.mutex);
        europa_threadpool_destroy(proxy->threadpool);
        free(proxy);
    }
}

int bp_start_blocking(BProxyHandle *proxy)
{
    while (1) {
        SHandle *client_handle = styx_network_stream_wait_for_connection(proxy->server_handle, NULL);
        if (client_handle) {
            void **args = malloc(sizeof(void *) * 2);
            args[0] = proxy;
            args[1] = client_handle;
            if (!europa_threadpool_add_work(proxy->threadpool, client_thread, args)) {
                logln("Failed to create new thread");
            }
        }
    }

    return 0;
}

boolean bp_update(BProxyHandle *proxy)
{
    SHandle *client_handle = styx_network_stream_wait_for_connection(proxy->server_handle, NULL);
    if (client_handle) {
        void **args = malloc(sizeof(void *) * 2);
        args[0] = proxy;
        args[1] = client_handle;
        if (!europa_threadpool_add_work(proxy->threadpool, client_thread, args)) {
            logln("Failed to create new thread");
        }
        return TRUE;
    }
    return FALSE;
}

void bp_reload_settings(BProxyHandle *proxy)
{
    reload_config(proxy);
}

static void client_thread(void **args)
{
    BProxyHandle *proxy = args[0];
    SHandle *client_handle = args[1];
    uint8 version, nmethods, command;
    boolean r = FALSE, w = FALSE;
    uint i;

    UNUSED(w);
    r = FALSE;
    while (!r) {
        r = TRUE;
        styx_network_wait(&client_handle, &r, NULL, 1, 1000);
    }

    version = styx_unpack_uint8(client_handle, "VERSION5");
    nmethods = styx_unpack_uint8(client_handle, "NMETHODS");
    for (i = 0; i < nmethods; i++) {
        (void)styx_unpack_uint8(client_handle, "METHODS");
    }

    if (version != VERSION5) {
        goto end;
    }

    styx_pack_uint8(client_handle, VERSION5, "VERSION5");
    styx_pack_uint8(client_handle, NOAUTH, "NOAUTH");
    if (styx_network_stream_send_force(client_handle) != 2) {
        logln("Failed to send SOCKS5 response");
        goto end;
    }

    r = FALSE;
    while (!r) {
        r = TRUE;
        styx_network_wait(&client_handle, &r, NULL, 1, 1000);
    }

    version = styx_unpack_uint8(client_handle, "VERSION5");
    if (version != VERSION5) {
        goto end;
    }

    command = styx_unpack_uint8(client_handle, "CMD");

    switch (command)
    {
        case CONNECT: {
            SHandle *target_handle = NULL;
            StyxNetworkAddress target_addr;
            uint8 rep = OK;

            char target_ip_str[64];
            char target_domain[256];

            logln("App requested TCP Connection");

            if (!target_addr_get_from_buffer(proxy, client_handle, &target_addr, target_ip_str, target_domain)) {
                if (client_handle) styx_free(client_handle);
                if (target_handle) styx_free(target_handle);
                goto end;
            }

            logln("TCP connection target: %s:%d (%s:%d)", target_domain, target_addr.port, target_ip_str, target_addr.port);

            if (proxy->settings.do_ping) {
                if (!verify_latency(target_ip_str, proxy->settings.max_rtt_ns)) {
                    logln("Request RTT Exceeded %d ms! Packet dropped!", proxy->settings.max_rtt_ns / 1000000);
                    rep = TTL_EXPIRED;
                }
            }

            if (proxy->settings.do_traceroute) {
                if (!verify_traceroute_ips(target_ip_str)) {
                    logln("Request passes through disabled ip! Packet dropped!");
                    rep = NETWORK_UNREACHABLE;
                }
            }

            if (!verify_cable(target_ip_str)) {
                logln("Request uses disabled cable! Packet dropped!");
                rep = NETWORK_UNREACHABLE;
            }

            target_handle = styx_network_stream_ip_create(target_addr);
            if (target_handle == NULL) {
                logln("Failed to connect to target!");
                rep = GENERAL_FAILURE;
            }

            styx_pack_uint8(client_handle, VERSION5, "VERSION5");
            styx_pack_uint8(client_handle, rep, "REP");
            styx_pack_uint8(client_handle, 0x00, "RSV");
            styx_pack_uint8(client_handle, IPV4, "ATYP");
            styx_pack_uint32(client_handle, target_addr.ip.v4, "BND.ADDR");
            styx_pack_uint16(client_handle, target_addr.port, "BND.PORT");
            styx_network_stream_send_force(client_handle);

            if (rep != OK) {
                if (client_handle) styx_free(client_handle);
                if (target_handle) styx_free(target_handle);
                goto end;
            }

            logln("Connection established");

            europa_mutex_lock(proxy->handles.mutex);
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (!proxy->handles.pairs[i].active) {
                    proxy->handles.pairs[i].client = client_handle;
                    proxy->handles.pairs[i].target = target_handle;
                    proxy->handles.pairs[i].active = TRUE;
                    proxy->handles.count++;
                    break;
                }
            }
            europa_mutex_unlock(proxy->handles.mutex);
            goto end;
            break;
        }
        case UDP_ASSOCIATE:
            logln("App requested UDP Association");
            /* TODO: Fix me */
            logln("This UDP Association is unsupported!");
        break;
        default:
            logln("Unsupported SOCKS command: 0x%02x!", command);
        break;
    }
end:
    free(args);
    return;
}

static boolean target_addr_get_from_buffer(BProxyHandle *proxy, SHandle *client_handle, StyxNetworkAddress *target_addr, char *target_ip_str, char *target_domain)
{
    /* TODO: DNS shouldn't happen here;
       DNS should happen in its own function
       after the target address is grabbed from the buffer.
    */
    uint i;
    uint8 atyp;

    (void)styx_unpack_uint8(client_handle, NULL);
    atyp = styx_unpack_uint8(client_handle, "ATYP");

    switch (atyp) {
        case IPV4:
            target_addr->is_ipv6 = FALSE;
            target_addr->ip.v4 = styx_unpack_uint32(client_handle, "DST.ADDR");
            target_addr->port = styx_unpack_uint16(client_handle, "DST.PORT");
        break;
        case DOMAIN:
        {
            uint8 domain_len = styx_unpack_uint8(client_handle, "LEN");

            for (i = 0; i < domain_len; i++) {
                target_domain[i] = styx_unpack_uint8(client_handle, NULL);
            }
            target_domain[domain_len] = '\0';

            target_addr->port = styx_unpack_uint16(client_handle, "DST.PORT");

            boolean dns_failed = FALSE;
            if (proxy->settings.use_local_dns) {
                if (!styx_network_address_lookup(target_addr, target_domain, target_addr->port, NULL)) dns_failed = TRUE;
            }
            else {
                if (!dns_resolve_iterative_root_func(target_addr, target_domain, target_addr->port, NULL, proxy->settings.max_rtt_ns)) dns_failed = TRUE;
            }

            if (dns_failed) {
                logln("DNS resolution failed for domain: %s!", target_domain);
                return FALSE;
            }
        }
        break;
        default:
            logln("Unsupported ATYP: 0x%02x!", atyp);
            return FALSE;
        break;
    }
    if (target_addr->is_ipv6) {
        inet_ntop(AF_INET6, &target_addr->ip.v6, target_ip_str, INET6_ADDRSTRLEN);
    }
    else {
        struct in_addr a;
        a.s_addr = htonl(target_addr->ip.v4);
        inet_ntop(AF_INET, &a, target_ip_str, INET_ADDRSTRLEN);
    }
    return TRUE;
}

static int transfer_data(SHandle *from_handle, SHandle *to_handle)
{
    uint8 buffer[8192];
    uint64 bytes_written;
    uint64 bytes_read;
    
    bytes_read = styx_unpack_raw(from_handle, buffer, sizeof(buffer), NULL);

    if (bytes_read <= 0) {
        return -1;
    }

    bytes_written = styx_pack_raw(to_handle, buffer, bytes_read, NULL);
    styx_network_stream_send_force(to_handle);

    return bytes_written;
}

static void pipe_handle_data_thread(BProxyHandle *proxy)
{
    for (;;) {
        SHandle *handles[MAX_CLIENTS * 2];
        boolean ready[MAX_CLIENTS * 2];
        uint pair_idxs[MAX_CLIENTS];
        uint i, n = 0, pairs = 0;
        int res;

        europa_mutex_lock(proxy->handles.mutex);
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!proxy->handles.pairs[i].active) {
                continue;
            }

            handles[n]         = proxy->handles.pairs[i].client;
            handles[n + 1]     = proxy->handles.pairs[i].target;
            ready[n] = ready[n + 1] = TRUE;
            n += 2;
            pair_idxs[pairs++] = i;
        }
        europa_mutex_unlock(proxy->handles.mutex);

        if (n == 0) {
            europa_sleepi(0, 1000000);
            continue;
        }

        res = 0;
        res = styx_network_wait(handles, ready, NULL, n, 100000);
        if (res <= 0) continue;

        europa_mutex_lock(proxy->handles.mutex);
        n = 0;
        for (i = 0; i < pairs; i++) {
            boolean closed = FALSE;
            if (!proxy->handles.pairs[pair_idxs[i]].active) {
                n += 2;
                continue;
            }
            
            if (ready[n])     closed |= transfer_data(handles[n], handles[n + 1]) < 0;
            if (ready[n + 1]) closed |= transfer_data(handles[n + 1], handles[n]) < 0;

            if (closed) {
                styx_free(proxy->handles.pairs[pair_idxs[i]].client);
                styx_free(proxy->handles.pairs[pair_idxs[i]].target);
                proxy->handles.pairs[pair_idxs[i]].active = FALSE;
                proxy->handles.count--;
            }
            n += 2;
        }
        europa_mutex_unlock(proxy->handles.mutex);
    }
}

static void reload_config(BProxyHandle *proxy)
{
    if (!europa_settings_load(proxy->settings_filename)) {
        printf("Creating settings file %s\n", proxy->settings_filename);
    }
    proxy->settings.do_ping = europa_setting_boolean_get("DO_PING", TRUE, "Do a ping measurement on every address touched by the Proxy.");
    proxy->settings.do_traceroute = europa_setting_boolean_get("DO_TRACEROUTE", FALSE, "Do a traceroute on every address touched by the Proxy.");
    proxy->settings.use_local_dns = europa_setting_boolean_get("USE_LOCAL_DNS", FALSE, "Use the local system DNS resolver rather than the Proxy's iterative one.");
    proxy->settings.max_rtt_ns = europa_setting_integer_get("MAX_LATENCY", 200, "Drop connections measured to have a latency greater than this value. Requires DO_PING or DO_TRACEROUTE to be enabled.") * 1000000;
    europa_settings_save(proxy->settings_filename);
}

static int logln(const char *format, ...)
{
    /* TODO: thread safety; stdout locking */
    va_list args;
    int ret;

    fputs("\x1b[35m[Proxy]\x1b[0m ", stdout);

    va_start(args, format);
    ret = vprintf(format, args);
    va_end(args);

    putchar('\n');
    fflush(stdout);

    return ret;
}
