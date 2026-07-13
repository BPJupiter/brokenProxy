#include "bproxy.h"

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "network_tools.h"
#define VEPOLL_IMPLEMENTATION
#include "vepoll.h"

#include <string.h>
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

static void handle_client(BProxyHandle *proxy, SHandle *client_handle);
static boolean target_addr_get_from_buffer(BProxyHandle *proxy, SHandle *client_handle, StyxNetworkAddress *target_addr, char *target_ip_str, char *target_domain);
static int transfer_data(SHandle *from_handle, SHandle *to_handle);
static int add_connection(Connections *connections, SHandle *handle);
static void remove_connection(VHandle epoll_handle, Connections *connections, int idx);
static void set_write_interest(VHandle epoll_handle, Connection *c, int idx, boolean value);

static int logln(const char *format, ...);
static void load_config(BProxyHandle *proxy);
static void save_config(BProxyHandle *proxy);

BProxyHandle *bp_init(uint16 port, const char *settings_filename)
{
    BProxyHandle *proxy = calloc(1, sizeof(BProxyHandle));
    
    (void)proxy->settings;
    proxy->settings_filename = malloc(strlen(settings_filename) + 1);
    strcpy(proxy->settings_filename, settings_filename);
    
    proxy->server_handle = styx_network_stream_address_create(NULL, port, FALSE);
    if (!proxy->server_handle) {
        logln("FATAL: Could not init server_handle!");
        exit(EXIT_FAILURE);
    }
    proxy->port = port;
    (void)proxy->connections.clients;
    (void)proxy->connections.targets;
    (void)proxy->connections.states;
    (void)proxy->connections.used;
    (void)proxy->connections.free_list;
    proxy->connections.free_list_count = 0;
    proxy->connections.next_empty_slot = 1;
    proxy->connections.count = 0;
    
    { // epoll setup
        struct epoll_event event;
        proxy->epoll_handle = epoll_create1(0);
        if (proxy->epoll_handle == INVALID_VEPOLL_HANDLE) {
            vepoll_perror("epoll_create1");
            exit(EXIT_FAILURE);
        }
        event.events = EPOLLIN;
        event.data.ptr = proxy->server_handle;
        if (epoll_ctl(proxy->epoll_handle,
                      EPOLL_CTL_ADD,
                      proxy->server_handle->socket,
                      &event) == VEPOLL_ERROR) {
            vepoll_perror("epoll_ctl: server_socket");
            exit(EXIT_FAILURE);
        }
    } // epoll setup
    
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    load_config(proxy);
    logln("SOCKS5 Proxy setup on port: %d", proxy->port);
    return proxy;
}

void bp_destroy(BProxyHandle *proxy)
{
    if (proxy) {
        vhandle_close(proxy->epoll_handle);
        styx_free(proxy->server_handle);
        free(proxy->settings_filename);
        free(proxy);
    }
}

int bp_start_blocking(BProxyHandle *proxy)
{
    while (1)
        bp_update(proxy);

    return 0;
}

boolean bp_update(BProxyHandle *proxy)
{
    int event_count = epoll_wait(proxy->epoll_handle, proxy->events, MAX_EVENTS, 0);
    struct epoll_event event;
    int e;
    if (event_count == VEPOLL_ERROR) {
        vepoll_perror("epoll_wait");
        return FALSE;
    }
    for (e = 0; e < event_count; e++) {
        int triggered_events = proxy->events[e].events;
        if (proxy->events[e].data.ptr == proxy->server_handle) {
            int client_idx;
            SHandle *client_handle = styx_network_stream_wait_for_connection(proxy->server_handle, NULL);
            if (!client_handle) return FALSE;
            // TODO: Edit the way styx handles waiting entirely.
            // styx_network_stream_wait_for_connection calls styx_network_wait
            // internally, which uses select().
            // Bad. Hopefully this works in the meantime
            client_idx = add_connection(&proxy->connections, client_handle);
            proxy->connections.states[client_idx] = CS_CLIENT_SENDING_METHODS;

            event.events = EPOLLIN | EPOLLRDHUP;
            event.data.u32 = client_idx;
            if (epoll_ctl(proxy->epoll_handle, EPOLL_CTL_ADD, client_handle->socket, &event) == VEPOLL_ERROR) {
                vepoll_perror("epoll_ctl: client_socket");
                remove_connection(proxy->epoll_handle, &proxy->connections, client_idx);
            }
        }
        else { // event on client handle
            int connection_idx = proxy->events[e].data.u32;
            int base_idx = (connection_idx < MAX_CONNECTIONS) ? connection_idx : connection_idx - MAX_CONNECTIONS;
            boolean connection_closed = FALSE;

            if (!proxy->connections.used[base_idx]) {
                continue;
            }

            if (triggered_events & EPOLLIN) { // data to read from client or target
                switch (proxy->connections.states[base_idx]) {
                case CS_CLIENT_SENDING_METHODS: { // handshake part 1
                    int client_idx = connection_idx;
                    SHandle *client_handle = proxy->connections.clients[client_idx].handle;
                    uint8 version, nmethods;
                    uint i;
                    int sent;
                    
                    version = styx_unpack_uint8(client_handle, "VERSION5");
                    if (version != VERSION5) {
                        logln("Client packet has a malformed socks5 header!");
                        remove_connection(proxy->epoll_handle, &proxy->connections, client_idx);
                        return FALSE;
                    }
                    nmethods = styx_unpack_uint8(client_handle, "NMETHODS");
                    for (i = 0; i < nmethods; i++) {
                        (void)styx_unpack_uint8(client_handle, "METHOD");
                    }
                    
                    proxy->connections.states[client_idx] = CS_SERVER_SENDING_METHOD;
                    set_write_interest(proxy->epoll_handle,
                                       &proxy->connections.clients[client_idx],
                                       client_idx,
                                       TRUE);
                    styx_pack_uint8(client_handle, VERSION5, "VERSION5");
                    styx_pack_uint8(client_handle, NOAUTH, "NOAUTH");
                } break; // handshake part 1
                case CS_CLIENT_SENDING_REQUEST: { // handshake part 3
                    int client_idx = connection_idx;
                    SHandle *client_handle = proxy->connections.clients[client_idx].handle;
                    uint8 version, command;

                    version = styx_unpack_uint8(client_handle, "VERSION5");
                    if (version != VERSION5) {
                        logln("Client request has malformed structure!");
                        remove_connection(proxy->epoll_handle, &proxy->connections, client_idx);
                        return FALSE;
                    }
                    command = styx_unpack_uint8(client_handle, "CMD");
                    switch (command) {
                    case CONNECT: {
                        SHandle *target_handle = NULL;
                        int target_idx;
                        StyxNetworkAddress target_addr;
                        uint8 rep = OK;

                        char target_ip_str[64];
                        char target_domain[256];

                        logln("App requested TCP Connection");

                        if (!target_addr_get_from_buffer(proxy, client_handle, &target_addr, target_ip_str, target_domain)) {
                            logln("Couldn't resolve address!");
                            remove_connection(proxy->epoll_handle, &proxy->connections, client_idx);
                            return FALSE;
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

                        if (rep == OK) {
                            proxy->connections.targets[client_idx].handle = target_handle;
                        }
                        target_idx = client_idx + MAX_CONNECTIONS;
                        event.events = EPOLLIN | EPOLLRDHUP;
                        event.data.u32 = target_idx;
                        if (epoll_ctl(proxy->epoll_handle, EPOLL_CTL_ADD, target_handle->socket, &event) == VEPOLL_ERROR) {
                            vepoll_perror("epoll_ctl: target_socket");
                            remove_connection(proxy->epoll_handle, &proxy->connections, client_idx);
                            return FALSE;
                        }

                        proxy->connections.states[client_idx] = CS_SERVER_SENDING_REPLY;
                        set_write_interest(proxy->epoll_handle,
                                           &proxy->connections.clients[client_idx],
                                           client_idx,
                                           TRUE);
                        styx_pack_uint8(client_handle, VERSION5, "VERSION5");
                        styx_pack_uint8(client_handle, rep, "REP");
                        styx_pack_uint8(client_handle, 0x00, "RSV");
                        styx_pack_uint8(client_handle, IPV4, "ATYP");
                        styx_pack_uint32(client_handle, target_addr.ip.v4, "BND.ADDR");
                        styx_pack_uint16(client_handle, target_addr.port, "BND.PORT");
                    } break;
                    case UDP_ASSOCIATE: {
                        logln("App requested UDP Association");
                        logln("This UDP Association is unsupported!");
                        remove_connection(proxy->epoll_handle, &proxy->connections, connection_idx);
                        return FALSE;
                    } break;
                    default: {
                        logln("Unsupported SOCKS command: 0x%02x!", command);
                        remove_connection(proxy->epoll_handle, &proxy->connections, connection_idx);
                        return FALSE;
                    } break;
                    }
                } break; //handeshake part 3
                case CS_CONNECTED: { // read data
                    if (connection_idx < MAX_CONNECTIONS) { // client to target
                        SHandle *client_handle = proxy->connections.clients[connection_idx].handle;
                        SHandle *target_handle = proxy->connections.targets[connection_idx].handle;
                        uint8 buffer[8192];
                        uint64 bytes_read = styx_unpack_raw(client_handle, buffer, sizeof(buffer), NULL);
                        if (bytes_read == 0) {
                            if (!styx_network_stream_connected(client_handle)) {
                                logln("Client disconnected.");
                                connection_closed = TRUE;
                            }
                        }
                        else {
                            styx_pack_raw(target_handle, buffer, (size_t)bytes_read, NULL);
                            set_write_interest(proxy->epoll_handle,
                                               &proxy->connections.targets[connection_idx],
                                               connection_idx + MAX_CONNECTIONS,
                                               TRUE);
                        }
                    }
                    else { // target to client
                        SHandle *target_handle = proxy->connections.targets[connection_idx - MAX_CONNECTIONS].handle;
                        SHandle *client_handle = proxy->connections.clients[connection_idx - MAX_CONNECTIONS].handle;
                        uint8 buffer[8192];
                        uint64 bytes_read = styx_unpack_raw(target_handle, buffer, sizeof(buffer), NULL);
                        if (bytes_read == 0) {
                            if (!styx_network_stream_connected(target_handle)) {
                                logln("Target disconnected.");
                                connection_closed = TRUE;
                            }
                        }
                        else {
                            styx_pack_raw(client_handle, buffer, (size_t)bytes_read, NULL);
                            set_write_interest(proxy->epoll_handle,
                                               &proxy->connections.clients[connection_idx - MAX_CONNECTIONS],
                                               connection_idx - MAX_CONNECTIONS,
                                               TRUE);
                        }
                    }
                } break; // read data
                }
            }

            if (!connection_closed && (triggered_events & EPOLLOUT)) {
                switch (proxy->connections.states[base_idx]) {
                case CS_SERVER_SENDING_METHOD: { // handshake part 2
                    SHandle *client_handle = proxy->connections.clients[connection_idx].handle;
                    if (styx_network_stream_send_force(client_handle) != 2) {
                        logln("Failed to send SOCKS5 response");
                        return FALSE;
                    }
                    proxy->connections.states[connection_idx] = CS_CLIENT_SENDING_REQUEST;
                    set_write_interest(proxy->epoll_handle,
                                       &proxy->connections.clients[connection_idx],
                                       connection_idx,
                                       FALSE);
                } break; // handshake part 2
                case CS_SERVER_SENDING_REPLY: { // handshake part 4
                    SHandle *client_handle = proxy->connections.clients[connection_idx].handle;
                    styx_network_stream_send_force(client_handle);
                    // if (rep != OK) {
                    //     remove_connection(proxy->epoll_handle, &proxy->connections, connection_idx);
                    //     return FALSE;
                    // }

                    logln("Connection established");
                    proxy->connections.states[connection_idx] = CS_CONNECTED;
                    set_write_interest(proxy->epoll_handle,
                                       &proxy->connections.clients[connection_idx],
                                       connection_idx,
                                       FALSE);
                } break; // handshake part 4
                case CS_CONNECTED: { // write data
                    if (connection_idx < MAX_CONNECTIONS) { // target to client
                        SHandle *client_handle = proxy->connections.clients[connection_idx].handle;
                        styx_network_stream_send_force(client_handle);
                        set_write_interest(proxy->epoll_handle,
                                           &proxy->connections.clients[connection_idx],
                                           connection_idx,
                                           FALSE);
                    }
                    else { // client to target
                        int target_slot = connection_idx - MAX_CONNECTIONS;
                        SHandle *target_handle = proxy->connections.targets[target_slot].handle;
                        styx_network_stream_send_force(target_handle);
                        set_write_interest(proxy->epoll_handle,
                                           &proxy->connections.targets[target_slot],
                                           connection_idx,
                                           FALSE);
                    }
                } break; // write data
                }
            }

            if (!connection_closed && (triggered_events & EPOLLRDHUP)) {
                connection_closed = TRUE;
                logln("EPOLLRDHUP");
            }

            if (!connection_closed && (triggered_events & EPOLLERR)) {
                connection_closed = TRUE;
                logln("EPOLLERR");
            }

            if (!connection_closed && (triggered_events & EPOLLHUP)) {
                connection_closed = TRUE;
                logln("EPOLLHUP");
            }

            if (connection_closed) {
                remove_connection(proxy->epoll_handle, &proxy->connections, base_idx);
            }
        }
    }
    return TRUE;
}

void bp_load_settings_from_disk(BProxyHandle *proxy)
{
    load_config(proxy);
}

void bp_save_settings_to_disk(BProxyHandle *proxy)
{
    save_config(proxy);
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

static int add_connection(Connections *connections, SHandle *client)
{
    int slot = 0;
    if (connections->free_list_count > 0) {
        slot = connections->free_list[--connections->free_list_count];
    }
    else if (connections->next_empty_slot < MAX_CONNECTIONS) {
        slot = connections->next_empty_slot++;
    }
    if (slot) {
        connections->clients[slot].handle = client;
        connections->used[slot] = TRUE;
        return slot;
    }
    else {
        return 0;
    }
}

static void remove_connection(VHandle epoll_handle, Connections *connections, int idx)
{
    if (idx > 0 && idx < MAX_CONNECTIONS && connections->used[idx]) {
        SHandle *client_handle = connections->clients[idx].handle;
        SHandle *target_handle = connections->targets[idx].handle;

        connections->used[idx] = FALSE;

        if (client_handle) {
            epoll_ctl(epoll_handle, EPOLL_CTL_DEL, client_handle->socket, NULL);
            styx_free(client_handle);
        }
        connections->clients[idx].handle = NULL;

        if (target_handle) {
            epoll_ctl(epoll_handle, EPOLL_CTL_DEL, target_handle->socket, NULL);
            styx_free(target_handle);
        }
        connections->targets[idx].handle = NULL;
        
        connections->free_list[connections->free_list_count++] = idx;
    }
    else {
        // nothing to do with nil?
    }
}

static void set_write_interest(VHandle epoll_handle, Connection *c, int idx, boolean value)
{
    if (c->write_pending == value) return;

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP | (value ? EPOLLOUT : 0);
    event.data.u32 = idx;
    epoll_ctl(epoll_handle, EPOLL_CTL_MOD, c->handle->socket, &event);
    c->write_pending = value;
}

static char *setting_desc[] =
    { "Do a ping measurement on every address touched by the Proxy.",
      "Do a traceroute measurement on every address touched by the Proxy.",
      "Use the local DNS resolver rather than the Proxy's custom iterative one.",
      "Drop connections measured to have a latency greater than this value. "
      "Requires DO_PING or DO_TRACEROUTE to have effect."
};

enum {
    DESC_PING = 0,
    DESC_TRACEROUTE,
    DESC_DNS,
    DESC_RTT
};

static void load_config(BProxyHandle *proxy)
{
    europa_settings_load(proxy->settings_filename);
    proxy->settings.do_ping =
        europa_setting_boolean_get("DO_PING",
                                   TRUE,
                                   setting_desc[DESC_PING]);
    proxy->settings.do_traceroute =
        europa_setting_boolean_get("DO_TRACEROUTE",
                                   FALSE,
                                   setting_desc[DESC_TRACEROUTE]);
    proxy->settings.use_local_dns =
        europa_setting_boolean_get("USE_LOCAL_DNS",
                                   FALSE,
                                   setting_desc[DESC_DNS]);
    proxy->settings.max_rtt_ns =
        europa_setting_integer_get("MAX_LATENCY",
                                   200,
                                   setting_desc[DESC_RTT]) * 1000000;
    europa_settings_save(proxy->settings_filename);
}

static void save_config(BProxyHandle *proxy)
{
    europa_setting_boolean_set("DO_PING",
                               proxy->settings.do_ping,
                               setting_desc[DESC_PING]);
    europa_setting_boolean_set("DO_TRACEROUTE",
                               proxy->settings.do_traceroute,
                               setting_desc[DESC_TRACEROUTE]);
    europa_setting_boolean_set("USE_LOCAL_DNS",
                               proxy->settings.use_local_dns,
                               setting_desc[DESC_DNS]);
    europa_setting_integer_set("MAX_LATENCY",
                               proxy->settings.max_rtt_ns / 1000000,
                               setting_desc[DESC_RTT]);
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
