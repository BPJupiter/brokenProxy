#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include "Clay/clay.h"
#include "Europa/europa.h"

#if defined _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
typedef unsigned int uint;
#else
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

#include "styx.h"

void styx_socket_destroy(VSocket socket)
{
#if defined _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

#if defined _WIN32

boolean styx_socket_init_win32(void)
{
    static boolean initialised = FALSE;
    if (!initialised) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(1, 1), &wsa_data) != 0) {
            fprintf(stderr, "Styx Error: WSAStartup failed.\n");
            return FALSE;
        }
        initialised = TRUE;
    }
    return TRUE;
}

#endif

static VSocket styx_socket_create(boolean stream, uint16 port, boolean ipv6)
{
    VSocket s;
    int s_domain = ipv6 ? AF_INET6 : AF_INET;
    int s_type = stream ? SOCK_STREAM : SOCK_DGRAM;
    int s_protocol = stream ? IPPROTO_TCP : IPPROTO_UDP;

    int buffer_size = 1 << 20;
    const int option = 1;
    
#if defined _WIN32
    if (!styx_socket_init_win32()) {
        return INVALID_VSOCKET;
    }
#endif

    s = socket(s_domain, s_type, s_protocol);
    if (s == INVALID_VSOCKET) {
        fprintf(stderr, "Styx: Failed to create socket.\n");
        return INVALID_VSOCKET;
    }

    if (0) { /* FIX ME */
#if defined _WIN32
        unsigned long one = 1UL;
        if (ioctlsocket(s, FIONBIO, &one) != 0) {
            styx_socket_destroy(s);
            return INVALID_VSOCKET;
        }
#else
        int flags = fcntl(s, F_GETFL, 0);
        if (fcntl(s, F_SETFL, flags | O_NONBLOCK) != 0) {
            fprintf(stderr, "Styx: Couldn't make socket non-blocking!\n");
            styx_socket_destroy(s);
            return INVALID_VSOCKET;
        }
#endif
    }

    if (ipv6) {
        int no = 0;
        if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&no, sizeof(no)) != 0) {
            fprintf(stderr, "Styx: Couldn't set socket to dual stack mode\n");
        }
    }
    if (!stream) {
        if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &option, sizeof option) != 0) {
            fprintf(stderr, "Styx: Couldn't set broadcast option of socket to %d\n", option);
        }
    }
#if defined _WIN32
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option)) < 0) {
        fprintf(stderr, "Styx: Couldn't set to reuseaddr!\n");
    }
#else
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        fprintf(stderr, "Styx: Couldn't set to reuseaddr!\n");
    }
#endif
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof buffer_size) != 0) {
        fprintf(stderr, "Styx: Couldn't set send buffer size of socket to %d\n", buffer_size);
    }
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof buffer_size) != 0) {
        fprintf(stderr, "Styx: Couldn't set receive buffer size of socket to %d\n", buffer_size);
    }

    if (port != 0 || !stream) {
        if (ipv6) {
            struct sockaddr_in6 address6;
            memset(&address6, 0, sizeof(address6));
            address6.sin6_family = AF_INET6;
            address6.sin6_port = htons(port);
            address6.sin6_addr = in6addr_any;

            if (bind(s, (struct sockaddr *)&address6, sizeof(address6)) != 0) {
                fprintf(stderr, "Styx: Could not bind!\n");
                styx_socket_destroy(s);
                return INVALID_VSOCKET;
            }
        }
        else {
            struct sockaddr_in address4;
            memset(&address4, 0, sizeof(address4));
            address4.sin_family = AF_INET;
            address4.sin_port = htons(port);
            address4.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(s, (struct sockaddr *)&address4, sizeof(address4)) != 0) {
                fprintf(stderr, "Styx: Could not bind!\n");
                styx_socket_destroy(s);
                return INVALID_VSOCKET;
            }
        }

        if (stream) {
            if (listen(s, 5) < 0) {
#if defined _WIN32
                fprintf(stderr, "Styx: Failed to listen(), Winsock error %d\n", WSAGetLastError());
#else
                fprintf(stderr, "Styx: Failed to listen(), code %d (%s)\n", errno, strerror(errno));
#endif
            }
        }
    }

    
    return s;
}

boolean styx_network_address_lookup(StyxNetworkAddress *address, const char *dns_name, uint16 default_port, boolean *do_ipv6)
{
    char host[256];
    char port_str[16];
    struct addrinfo hints, *res, *p;
    const char *parsed_host = dns_name;
    size_t host_len;
    uint16 final_port = default_port;
    boolean found = FALSE;

    if (dns_name == NULL || address == NULL) {
        return FALSE;
    }

#if defined _WIN32
    if (!styx_socket_init_win32()) {
        return FALSE;
    }
#endif

    host_len = strlen(dns_name);

    if (dns_name[0] == '[') {
        const char *bracket_end = strchr(dns_name, ']');
        if (bracket_end != NULL) {
            parsed_host = dns_name + 1;
            host_len = bracket_end - parsed_host;

            if (*(bracket_end + 1) == ':') {
                final_port = (uint16)strtoul(bracket_end + 2, NULL, 10);
            }
        }
    }
    else {
        const char *first_colon = strchr(dns_name, ':');
        const char *last_colon = strrchr(dns_name, ':');

        if (last_colon != NULL && last_colon == first_colon) {
            host_len = last_colon - dns_name;
            final_port = (uint16)strtoul(last_colon + 1, NULL, 10);
        }
    }

    if (host_len >= sizeof(host)) {
        return FALSE;
    }

    memcpy(host, parsed_host, host_len);
    host[host_len]= '\0';
    snprintf(port_str, sizeof(port_str), "%u", final_port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    if (do_ipv6 == NULL) {
        hints.ai_family = AF_UNSPEC;
    }
    else {
        hints.ai_family = *do_ipv6 ? AF_INET6 : AF_INET;
    }

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return FALSE;
    }

    found = FALSE;
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            address->ip.v4 = ntohl(ipv4->sin_addr.s_addr);
            address->port = ntohs(ipv4->sin_port);
            address->is_ipv6 = FALSE;
            found = TRUE;
            break;
        }
        else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            memcpy(address->ip.v6, &ipv6->sin6_addr, 16);
            address->port = ntohs(ipv6->sin6_port);
            address->is_ipv6 = TRUE;
            found = TRUE;
            break;
        }
    }

    freeaddrinfo(res);
    return found;
}

boolean styx_network_address_compare(StyxNetworkAddress *a, StyxNetworkAddress *b)
{
    /* comparison of v6 union member encapsulates v4 */
    return a->ip.v6 == b->ip.v6 && a->port == b->port;
}

typedef struct {
    uint16 port;
    uint8 op;
    double timer;
} StyxMappedPort;

static void styx_send_port_mapping(boolean udp, uint16 port, boolean ipv6)
{
    UNUSED(udp);
    UNUSED(port);
    UNUSED(ipv6);
    /* IMPLEMENT ME */
}

SHandle *styx_network_stream_address_create(const char *host_name, uint16 port, boolean ipv6)
{
    SHandle *handle;
    VSocket socket;

    if (host_name != NULL) {
        StyxNetworkAddress ip;
        if (styx_network_address_lookup(&ip, host_name, port, &ipv6)) {
            return styx_network_stream_ip_create(ip);
        }
        return NULL;
    }

    socket = styx_socket_create(TRUE, port, ipv6);
    if (socket == INVALID_VSOCKET) {
        return NULL;
    }
    styx_send_port_mapping(FALSE, port, ipv6);

#if defined _WIN32
    {
        unsigned long one = 1UL;
        if (ioctlsocket(socket, FIONBIO, &one) != 0) {
            styx_socket_destroy(socket);
            return NULL;
        }
    }
#else
    {
        int flags = fcntl(socket, F_GETFL, 0);
        if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) != 0) {
            fprintf(stderr, "Styx: Couldn't make socket non-blocking!\n");
            styx_socket_destroy(socket);
            return NULL;
        }
    }
#endif

    handle = malloc(sizeof *handle);
    if (handle == NULL) {
        styx_socket_destroy(socket);
        return NULL;
    }

    styx_handle_clear(handle, S_HT_STREAMING_SERVER);
    handle->port = port;
    handle->socket = socket;
    handle->connected = TRUE;
    return handle;
}

SHandle *styx_network_stream_ip_create(StyxNetworkAddress ip)
{
    SHandle *handle;
    VSocket socket;
    int res;

    socket = styx_socket_create(TRUE, 0, ip.is_ipv6);
    if (socket == INVALID_VSOCKET) {
        return NULL;
    }

    if (ip.is_ipv6) {
        struct sockaddr_in6 address_in6;
        memset(&address_in6, 0, sizeof(address_in6));
        address_in6.sin6_family = AF_INET6;
        address_in6.sin6_port = htons(ip.port);
        memcpy(&address_in6.sin6_addr, ip.ip.v6, 16);

        res = connect(socket, (struct sockaddr *)&address_in6, sizeof(address_in6));
    }
    else {
        struct sockaddr_in address_in;
        memset(&address_in, 0, sizeof(address_in));
        address_in.sin_family = AF_INET;
        address_in.sin_port = htons(ip.port);
        address_in.sin_addr.s_addr = htonl(ip.ip.v4);

        res = connect(socket, (struct sockaddr *)&address_in, sizeof(address_in));
    }

    if (res < 0) {
#if defined _WIN32
        fprintf(stderr, "Styx: connect() failed, Winsock error %d\n", WSAGetLastError());
    #else
        fprintf(stderr, "Styx: connect() failed: %s\n", strerror(errno));
    #endif
        styx_socket_destroy(socket);
        return NULL;
    }
#if defined _WIN32
    {
        unsigned long one = 1UL;
        if (ioctlsocket(socket, FIONBIO, &one) != 0) {
            styx_socket_destroy(socket);
            return NULL;
        }
    }
#else
    {
        int flags = fcntl(socket, F_GETFL, 0);
        if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) != 0) {
            fprintf(stderr, "Styx: Couldn't make socket non-blocking!\n");
            styx_socket_destroy(socket);
            return NULL;
        }
    }
#endif

    handle = malloc(sizeof *handle);
    if (handle == NULL) {
        styx_socket_destroy(socket);
        return NULL;
    }

    styx_handle_clear(handle, S_HT_STREAMING_CONNECTION);

    handle->port = ip.port;
    handle->socket = socket;
    handle->connected = TRUE;
    return handle;
}

static void styx_network_stream_address_destroy(SHandle *handle)
{
    if (handle->read_buffer != NULL) {
        free(handle->read_buffer);
    }
    if (handle->write_buffer != NULL) {
        free(handle->write_buffer);
    }
    if (handle->file != NULL) {
        fclose(handle->file);
    }
    if (handle->text_copy != NULL) {
        fclose(handle->text_copy);
    }
}

SHandle *styx_network_stream_wait_for_connection(SHandle *listener, StyxNetworkAddress *from)
{
    SHandle *handle;
    VSocket socket;
    struct sockaddr_storage address;
#if defined _WIN32
    int length = sizeof(address);
#else
    socklen_t length = sizeof(address);
#endif
    boolean read = TRUE;
    boolean is_ipv6;

    if (listener == NULL || listener->socket == INVALID_VSOCKET) {
        return NULL;
    }

    styx_network_wait(&listener, &read, NULL, 1, 1);
    if (read) {
        if ((socket = accept(listener->socket, (struct sockaddr *)&address, &length)) >= 0) {
#if defined _WIN32
            {
                unsigned long one = 1UL;
                if (ioctlsocket(socket, FIONBIO, &one) != 0) {
                    styx_socket_destroy(socket);
                    return NULL;
                }
            }
#else
            {
                int flags = fcntl(socket, F_GETFL, 0);
                if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) != 0) {
                    fprintf(stderr, "Styx: Couldn't make socket non-blocking!\n");
                    styx_socket_destroy(socket);
                    return NULL;
                }
            }
#endif
            handle = malloc(sizeof *handle);
            if (handle == NULL) {
                styx_socket_destroy(socket);
                return NULL;
            }
            styx_handle_clear(handle, S_HT_STREAMING_CONNECTION);
            handle->debug_descriptor = listener->debug_descriptor;
            handle->socket = socket;

            if (address.ss_family == AF_INET6) {
                struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&address;
                memcpy(handle->ip.v6, &addr6->sin6_addr, 16);
                handle->port = ntohs(addr6->sin6_port);
                is_ipv6 = TRUE;
            }
            else if (address.ss_family == AF_INET) {
                struct sockaddr_in *addr4 = (struct sockaddr_in *)&address;
                handle->ip.v4 = ntohl(addr4->sin_addr.s_addr);
                handle->port = ntohs(addr4->sin_port);
                is_ipv6 = FALSE;
            }
            if (from != NULL) {
                from->is_ipv6 = is_ipv6;
                from->port = handle->port;

                if (is_ipv6) {
                    memcpy(from->ip.v6, handle->ip.v6, 16);
                }
                else {
                    from->ip.v4 = handle->ip.v4;
                }
            }
            return handle;
        }
    }
    return NULL;
}

boolean styx_network_stream_connected(SHandle *handle)
{
    return handle->connected;
}

SHandle *styx_network_datagram_create(uint16 port, boolean ipv6)
{
    SHandle *handle;
    VSocket socket;
    socket = styx_socket_create(FALSE, port, ipv6);
    if (socket == INVALID_VSOCKET) {
        return NULL;
    }
    handle = malloc(sizeof *handle);
    if (handle == NULL) {
        styx_socket_destroy(socket);
        return NULL;
    }
    memset(handle, 0, sizeof(*handle));
    handle->read_buffer = malloc((sizeof *handle->read_buffer) * 1500);
    if (handle->read_buffer == NULL) {
        free(handle);
        styx_socket_destroy(socket);
        return NULL;
    }
    handle->write_buffer = malloc((sizeof *handle->write_buffer) * 1500);
    if (handle->write_buffer == NULL) {
        free(handle->read_buffer);
        free(handle);
        styx_socket_destroy(socket);
        return NULL;
    }

    handle->port = port;
    handle->socket = socket;
    handle->type = S_HT_PACKET_PEER;
    handle->is_ipv6 = ipv6;

    handle->read_buffer_pos = 0;
    handle->read_buffer_size = 1500;
    handle->read_marker = -1;
    handle->read_raw_progress = 0;

    handle->write_buffer_pos = 0;
    handle->write_buffer_size = 1500;
    handle->write_raw_progress = 0;

    handle->file = NULL;
    handle->text_copy = NULL;
    handle->debug_descriptor = FALSE;

    return handle;
}

int styx_network_datagram_send(SHandle *handle, StyxNetworkAddress *to)
{
    int out;

    if (handle == NULL || to == NULL || handle->socket == INVALID_VSOCKET) {
        return -1;
    }

    if (!handle->is_ipv6 && to->is_ipv6) {
        return -1;
    }

    if (handle->is_ipv6) {
        struct sockaddr_in6 address_in6;
        memset(&address_in6, 0, sizeof(address_in6));
        address_in6.sin6_family = AF_INET6;
        address_in6.sin6_port = htons(to->port);

        if (to->is_ipv6) {
            memcpy(&address_in6.sin6_addr, to->ip.v6, 16);
        }
        else {
            uint32 ipv4 = htonl(to->ip.v4);
            address_in6.sin6_addr.s6_addr[10] = 0xFF;
            address_in6.sin6_addr.s6_addr[11] = 0xFF;

            memcpy(&address_in6.sin6_addr.s6_addr[12], &ipv4, 4);
        }

        out = sendto(handle->socket, handle->write_buffer, handle->write_buffer_pos, 0, (struct sockaddr *)&address_in6, sizeof(address_in6));
    }
    else {
        struct sockaddr_in address_in;
        memset(&address_in, 0, sizeof(address_in));
        address_in.sin_family = AF_INET;
        address_in.sin_port = htons(to->port);
        address_in.sin_addr.s_addr = htonl(to->ip.v4);

        out = sendto(handle->socket, handle->write_buffer, handle->write_buffer_pos, 0, (struct sockaddr *)&address_in, sizeof(struct sockaddr_in));
    }
    if (out < 0) {
#if defined _WIN32
        printf("Styx: sendto failed with error: %u\n", WSAGetLastError());
#endif
        handle->connected = FALSE;
    }

    handle->write_buffer_pos = 0;

    return out;
}

size_t styx_network_stream_send(SHandle *handle, uint8 *buffer, size_t length)
{
#if defined _WIN32
    int size;
#else
    ssize_t size;
#endif

    if (handle == NULL || buffer == NULL || handle->socket == INVALID_VSOCKET) {
        return 0;
    }

    if (length == 0) {
        return 0;
    }

#if defined _WIN32
    size = send(handle->socket, (const char *)buffer, (int)length, 0);
#else
    size = send(handle->socket, buffer, length, 0);
#endif

    if (size < 0) {
        int error;
#if defined _WIN32
        error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            return 0;
        }
#else
        error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK || error == 0) {
            return 0;
        }
#endif
        fprintf(stderr, "Styx: send error %u\n", error);
        handle->connected = FALSE;
        return 0;
    }

    return (size_t)size;
}

int styx_network_stream_send_force(SHandle *handle)
{
    size_t size;

    if (handle == NULL) {
        return 0;
    }

    if (handle->type == S_HT_STREAMING_CONNECTION) {
        if (handle->write_buffer_pos != 0 && handle->connected) {
            size = styx_network_stream_send(handle, handle->write_buffer, handle->write_buffer_pos);
            if (size == handle->write_buffer_pos) {
                handle->write_buffer_pos = 0;
            }
            else if (size != 0) {
                size_t remaining = handle->write_buffer_pos - size;
                memmove(handle->write_buffer, handle->write_buffer + size, remaining);
                handle->write_buffer_pos = remaining;
            }
            return (int)size;
        }
    }
    else if (handle->type == S_HT_FILE_WRITE) {
        styx_pack_buffer_clear(handle);
        return 0;
    }

    else {
        fprintf(stderr, "Styx: styxNetworkStreamSendForce FAILED %u %u %u\n", handle->type, S_HT_STREAMING_CONNECTION, handle->connected);
    }
    
    return 0;
}

int styx_network_wait(SHandle **handles, boolean *read, boolean *write, uint handle_count, uint microseconds)
{
    struct timeval tv;
    fd_set fd_read, fd_write;
    uint i;
    VSocket max = 0;
    int res;

    if (handles == NULL || handle_count == 0) {
        return -1;
    }

    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);

    for (i = 0; i < handle_count; i++) {
        if (handles[i] == NULL || !handles[i]->connected || handles[i]->socket == INVALID_VSOCKET) {
            if (read != NULL) {
                read[i] = FALSE;
            }
            if (write != NULL) {
                write[i] = FALSE;
            }
            continue;
        }
#ifndef _WIN32
        if (handles[i]->socket >= FD_SETSIZE) {
            fprintf(stderr, "Styx: Socket descriptor %d exceeds FD_SETSIZE limit!\n", (int)handles[i]->socket);
            if (read != NULL) {
                read[i] = FALSE;
            }
            if (write != NULL) {
                write[i] = FALSE;
            }
            continue;
        }
#endif
        if (read != NULL && read[i]) {
            FD_SET(handles[i]->socket, &fd_read);
            if (max < handles[i]->socket) {
                max = handles[i]->socket;
            }
        }
        if (write != NULL && write[i]) {
            FD_SET(handles[i]->socket, &fd_write);
            if (max < handles[i]->socket) {
                max = handles[i]->socket;
            }
        }
    }
    tv.tv_sec = microseconds / 1000000;
    tv.tv_usec = microseconds % 1000000;

    res = select((int)max + 1, &fd_read, &fd_write, NULL, &tv);

    if (res > 0) {
        for (i = 0; i < handle_count; i++) {
            if (handles[i] == NULL || !handles[i]->connected) {
                continue;
            }
            if (read != NULL && read[i]) {
                read[i] = FD_ISSET(handles[i]->socket, &fd_read);
            }
            if (write != NULL && write[i]) {
                write[i] = FD_ISSET(handles[i]->socket, &fd_write);
            }
        }
    }
    else {
        for (i = 0; i < handle_count; i++) {
            if (read != NULL && read[i]) {
                read[i] = FALSE;
            }
            if (write != NULL && write[i]) {
                write[i] = FALSE;
            }
        }
    }
    return res;
}

size_t styx_network_stream_receive(SHandle *handle, uint8 *buffer, size_t length)
{
#if defined _WIN32
    int get;
#else
    ssize_t get;
#endif

    if (handle == NULL || buffer == NULL || handle->socket == INVALID_VSOCKET) {
        return 0;
    }

#if defined _WIN32
    get = recv(handle->socket, (char *)buffer, (int)length, 0);
#else
    get = recv(handle->socket, buffer, length, 0);
#endif

    if (get == 0) {
        handle->connected = FALSE;
        return 0;
    }

    if (get < 0) {
        int error;
#if defined _WIN32
        error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            return 0;
        }
#else
        error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK || error == 0) {
            return 0;
        }
#endif
        fprintf(stderr, "Styx: recv error %d\n", error);
        handle->connected = FALSE;
        return 0;
    }

    return (size_t)get;
}

int styx_network_receive(SHandle *handle, StyxNetworkAddress *from)
{
    struct sockaddr_storage address;
#if defined _WIN32
    int from_length = sizeof(address);
    int len;
#else
    socklen_t from_length = sizeof(address);
    ssize_t len;
#endif

    if (handle == NULL || handle->socket == INVALID_VSOCKET || handle->read_buffer == NULL) {
        return -1;
    }

    memset(&address, 0, sizeof address);

#if defined _WIN32
    len = recvfrom(handle->socket, (char *)handle->read_buffer, handle->read_buffer_size, 0, (struct sockaddr *)&address, &from_length);
#else
    len = recvfrom(handle->socket, handle->read_buffer, handle->read_buffer_size, 0, (struct sockaddr *)&address, &from_length);
#endif

    if (len < 0) {
        int error;
#if defined _WIN32
        error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == EWOULDBLOCK) {
            return 0;
        }
#else
        error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK || error == 0) {
            return 0;
        }
#endif
        return -1;
    }
    handle->read_buffer_pos = 0;
    handle->read_marker = -1;

    if (len > 0) {
        handle->read_buffer_used = (uint)len;
        if (from != NULL) {
            if (address.ss_family == AF_INET6) {
                struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&address;

                if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
                    uint32 v4;
                    from->is_ipv6 = FALSE;
                    from->port = ntohs(addr6->sin6_port);

                    memcpy(&v4, &addr6->sin6_addr.s6_addr[12], 4);
                    from->ip.v4 = ntohl(v4);
                }
                else {
                    from->is_ipv6 = TRUE;
                    from->port = ntohs(addr6->sin6_port);
                    memcpy(from->ip.v6, &addr6->sin6_addr, 16);
                }
            }
            else if (address.ss_family == AF_INET) {
                struct sockaddr_in *addr4 = (struct sockaddr_in *)&address;
                from->is_ipv6 = FALSE;
                from->port = ntohs(addr4->sin_port);
                from->ip.v4 = ntohl(addr4->sin_addr.s_addr);
            }
        }
        return (int)len;
    }
    return 0;
}
