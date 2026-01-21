#include "Clay/clay.h"

#ifndef STYX_H
#define STYX_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Ws2tcpip.h>
typedef SOCKADDR_INET StyxSockaddrInet;
typedef SOCKET VSocket;
#else
#include <netinet/in.h>
#include <arpa/inet.h>
typedef union
{
    struct sockaddr_in Ipv4;
    struct sockaddr_in6 Ipv6;
    uint16 si_family;
}StyxSockaddrInet;
typedef int VSocket;

#endif

typedef struct
{
    uint32 ip;
    uint16 port;
}StyxNetworkAddress;

/* -------- Raw Networking -------- */

extern VSocket  styx_socket_create(boolean stream, uint16 port);
extern void     styx_socket_destroy(VSocket sock);
extern void     styx_socket_set_timeout(VSocket sock, uint32 seconds);

extern void     styx_print_error(const char *msg);
extern boolean  styx_socket_assert(VSocket sock, const char *msg);

extern boolean  styx_network_address_lookup(StyxNetworkAddress *address, const char *dns_name, uint16 default_port);
extern boolean  styx_network_address_compare(StyxNetworkAddress *a, StyxNetworkAddress *b);
extern void     styx_local_dns_server_get(char *dest, uint32 dest_buf_len);

extern uint32   styx_string_to_ipv4(char *path);
extern void     styx_ipv4_to_string(char *dest, StyxNetworkAddress *address);
extern char *styx_ipv4_to_string_alloc(StyxNetworkAddress *address);


#endif /* STYX_H */
