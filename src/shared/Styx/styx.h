#include "Clay/clay.h"
#include <sys/types.h>

#ifndef STYX_H
#define STYX_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Ws2tcpip.h>
typedef SOCKADDR_INET StyxSockaddrInet;
typedef SOCKET VSocket;
#define INVALID_VSOCKET INVALID_SOCKET
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
#define INVALID_VSOCKET -1
#endif

typedef struct
{
    union {
        uint32 v4;
        uint8 v6[16]; 
    } ip;
    uint16 port;
    boolean is_ipv6;
}StyxNetworkAddress;



/* -------- Raw Networking -------- 
 * -------- DEPRICATED -------- */

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
extern char     *styx_ipv4_to_string_alloc(StyxNetworkAddress *address);

typedef enum {
    S_HT_STREAMING_SERVER,
    S_HT_STREAMING_CONNECTION,
    S_HT_PACKET_PEER,
    S_HT_FILE_READ,
    S_HT_FILE_WRITE,
    S_HT_BUFFER
}SHandleType;

#include "s_internal.h"

/* -------- TCP Networking --------
Handles the creation and management of TCP streams. */

extern SHandle *styxNetworkStreamAddressCreate(const char *host_name, uint16 port, boolean ipv6);
extern int      styxNetworkStreamSendForce(SHandle *handle);
extern SHandle *styxNetworkStreamWaitForConnection(SHandle *listen, StyxNetworkAddress *from);
extern boolean  styxNetworkStreamConnected(SHandle *handle);

/* -------- UDP Networking --------
Handles the creation and management of UDP datagrams. */

extern boolean  styxNetworkAddressLookup(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *ipv6);
extern boolean  styxNetworkAddressCompare(StyxNetworkAddress *a, StyxNetworkAddress *b);
extern SHandle *styxNetworkDatagramCreate(uint16 port, boolean ipv6);
extern int      styxNetworkDatagramSend(SHandle *handle, StyxNetworkAddress *to);
extern int      styxNetworkReceive(SHandle *handle, StyxNetworkAddress *from);

/* -------- File Management --------
Handles the creation and management of binary file handles. */

extern SHandle *styxFileLoad(char *path);
extern SHandle *styxFileSave(char *path);
extern uint64   styxFileSize(SHandle *handle);
extern void     styxFilePositionGet(SHandle *handle, uint64 pos);
extern uint64   styxFilePositionSet(SHandle *handle);

/* -------- Memory Buffers --------
Handles the creation and management of in memory FIFO channels. */

extern SHandle *styxBufferCreate(void);
extern void    *styxBufferGet(SHandle *handle, uint32 *size);
extern void     styxBufferSet(SHandle *handle, void *data, uint32 size);

/* -------- Handle Utils --------
*/

extern SHandleType styxType(SHandle *handle);
extern void     styxFree(SHandle *handle);
extern int      styxNetworkWait(SHandle **handles, boolean *read, boolean *write, uint handle_count, uint micro_seconds);


#endif /* STYX_H */
