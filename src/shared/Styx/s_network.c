#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include "Clay/clay.h"
#include "Europa/europa.h"

#if defined _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "WSock32.lib")
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
#include <arpa/nameser.h>
#include <sys/wait.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <resolv.h>
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

boolean styx_socket_init_win32()
{
    static boolean initialised = FALSE;
    if (!initialised)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
        {
            fprintf(stderr, "Styx Error: WSAStartup failed.\n");
            return FALSE;
        }
        initialised = TRUE;
    }
    return TRUE;
}

#endif

VSocket styx_socket_create(boolean stream, uint16 port)
{
    struct sockaddr_in address;
    int buffer_size = 1 << 20;
    int option = TRUE;
    VSocket s;
#if defined _WIN32
    if (!styx_socket_init_win32())
        return INVALID_VSOCKET;
#endif
    if (stream)
    {
        if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            return INVALID_VSOCKET;
    }
    else
    {
        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            return INVALID_VSOCKET;
    }

    if (port != 0 || !stream)
    {
        memset(&address, 0, sizeof(struct sockaddr));
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(s, (struct sockaddr *)&address, sizeof(struct sockaddr)) != 0)
        {
            fprintf(stderr, "Styx Error: Failed to bind(), code %d (%s)\n", errno, strerror(errno));
            styx_socket_destroy(s);
            return INVALID_VSOCKET;
        }
        if (stream)
            if (listen(s, 8) < 0)
                fprintf(stderr, "Styx Error: Failed to listen(), code %d (%s)\n", errno, strerror(errno));
    }

    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &option, sizeof option) != 0) {
        /* fprintf(stderr, "Styx: Couldn't set broadcast option of socket to %d\n", option); */
    }
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof buffer_size) != 0) {
		fprintf(stderr, "Styx: Couldn't set send buffer size of socket to %d\n", buffer_size);
    }
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof buffer_size) != 0) {
		fprintf(stderr, "Styx: Couldn't set recieve buffer size of socket to %d\n", buffer_size);
    }
	return s;
}

void styx_socket_set_timeout(VSocket sock, uint32 seconds)
{
#ifdef _WIN32
    DWORD timeout = seconds * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof timeout);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof timeout);
#else
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof tv);
#endif
}

void styx_print_error(const char *msg)
{
#ifdef _WIN32
    int err_code = WSAGetLastError();
    char *err_msg = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&err_msg,
        0,
        NULL
        );

    if (msg != NULL && *msg != '\0')
    {
        fprintf(stderr, "%s: %s\n", msg, err_msg ? err_msg : "Unknown Error");
    }
    else
    {
        fprintf(stderr, "%s\n", err_msg ? err_msg : "Unknown Error");
    }

    if (err_msg)
        LocalFree(err_msg);
#else
    perror(msg);
#endif
}

boolean styx_network_address_lookup(StyxNetworkAddress *address, const char *dns_name, uint16 default_port)
{
    char *colon = NULL, *buf = NULL;
    boolean result = FALSE;
    const char *host = dns_name;
    struct addrinfo hints, *res = NULL;
    int status;

    address->port = default_port;

    if ((colon = strchr(dns_name, ':')) != NULL)
    {
        size_t hl = strlen(dns_name);
        if ((buf = malloc(hl + 1)) != NULL)
        {
            uint tp;
            c_text_copy(hl + 1, buf, dns_name);
            colon = buf + (colon - dns_name);
            *colon = '\0';
            host = buf;
            if (sscanf(colon + 1, "%u", &tp) == 1)
            {
                address->port = (unsigned short)tp;
                if (address->port != tp) {
                    goto cleanup;
                }
            }
            else {
                goto cleanup;
            }
        }
        else {
            goto cleanup;
        }
    }

#if defined _WIN32
    if (!styx_socket_init_win32())
    {
        printf("Cannot resolve DNS as WSA will not startup.\n");
        goto cleanup;
    }
#endif

    if (host == NULL) {
        printf("DNS name is null.\n");
        goto cleanup;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(host, NULL, &hints, &res);
    if (status != 0) {
        printf("getaddrinfo failed: %s: %s:%d\n", gai_strerror(status), __FILE__, __LINE__);
        goto cleanup;
    }

    if (res != NULL) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
        address->ip = ntohl(ipv4->sin_addr.s_addr);
        result = TRUE;
    }

cleanup:
    if (res != NULL) {
        freeaddrinfo(res);
    }
    if (buf != NULL) {
        free(buf);
    }

    return result;
}



void styx_local_dns_server_get(char *dest, uint32 dest_buf_len)
{
    dest[0] = '\0';

#ifdef _WIN32
    FIXED_INFO *fi;
    ULONG length;

    length = sizeof(fi);
    fi = malloc(sizeof(*fi));
    if (fi == NULL) return;

    if (GetNetworkParams(fi, &length) == ERROR_BUFFER_OVERFLOW);
    {
        free(fi);
        fi = malloc(length);
        if (fi == NULL) return;
    }

    if (GetNetworkParams(fi, &length) == NO_ERROR)
    {
        if (fi->DnsServerList.IpAddress.String[0] != 0)
        {
            c_text_copy(dest_buf_len, dest, fi->DnsServerList.IpAddress.String);
            dest[dest_buf_len - 1] = '\0';
        }
    }
    free(fi);
#else
    {
        struct __res_state dns = { 0 };

        if (res_ninit(&dns) == 0)
        {
            if (dns.nscount > 0)
            {
                struct in_addr addr = dns.nsaddr_list[0].sin_addr;
                char *ip_str = inet_ntoa(addr);

                if (ip_str != NULL)
                {
                    c_text_copy(dest_buf_len, dest, ip_str);
                    dest[dest_buf_len - 1] = '\0';
                }
            }
            res_nclose(&dns);
        }
    }
#endif
}

#ifdef _WIN32

boolean styx_socket_assert(VSocket sock, const char *msg)
{
    if (sock == INVALID_SOCKET)
    {
        if (NULL != msg)
            styx_print_error(msg);
        return FALSE;
    }
    return TRUE;
}

#else

boolean styx_socket_assert(VSocket sock, const char *msg)
{
    if (sock < 0)
    {
        if (NULL != msg)
            styx_print_error(msg);
        return FALSE;
    }
    return TRUE;
}

#endif

boolean styx_network_address_compare(StyxNetworkAddress *a, StyxNetworkAddress *b)
{
    return a->ip == b->ip && a->port == b->port;
}

uint32 styx_string_to_ipv4(char *path)
{
    uint32 ip = 0, i, pos = 0, value;
    for (i = 0; i < 4; i++)
    {
        value = 0;
        while (path[pos] != 0 && (path[pos] < '0' || path[pos] > '9'))
            pos++;
        if (path[pos] == 0)
            return 0;
        while (path[pos] != 0 && path[pos] >= '0' && path[pos] <= '9')
        {
            value *= 10;
            value += path[pos++] - '0';
        }

        ip |= value << ((3 - i) * 8);
    }
    return ip;
}

void styx_ipv4_to_string(char *dest, StyxNetworkAddress *address)
{
    uint8 bytes[4];

    bytes[0] = (address->ip >> 24) & 0xFF;
    bytes[1] = (address->ip >> 16) & 0xFF;
    bytes[2] = (address->ip >> 8) & 0xFF;
    bytes[3] = address->ip & 0xFF;

    sprintf(dest, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
}

char *styx_ipv4_to_string_alloc(StyxNetworkAddress *address)
{
    uint8 bytes[4];
    char *dest;

    bytes[0] = (address->ip >> 24) & 0xFF;
    bytes[1] = (address->ip >> 16) & 0xFF;
    bytes[2] = (address->ip >> 8) & 0xFF;
    bytes[3] = address->ip & 0xFF;

    dest = malloc((sizeof *dest) * 16);

    if (dest != NULL)
        sprintf(dest, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);

    return dest;
}
