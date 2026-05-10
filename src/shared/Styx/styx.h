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

/* -------- Data Packing --------
 * Functions used to pack data into a stream. If STYX_DEBUG is not defined, the param "name" will be removed with the macro.
 * In debug mode (where both STYX_DEBUG is defined AND the bug mode of the stream has been set to TRUE styxDebugModeSet)
 * the name and data type will be checked to make sure they match the name and type that was packed.
 * If this test fails the application will terminate on unpack and write out a Error raport to standard out.*/

extern void     styxPackUint8(SHandle *handle, uint8 value, char *name);
extern void     styxPackInt8(SHandle *handle, int8 value, char *name);
extern void     styxPackUint16(SHandle *handle, uint16 value, char *name);
extern void     styxPackInt16(SHandle *handle, int16 value, char *name);
extern void     styxPackUint32(SHandle *handle, uint32 value, char *name);
extern void     styxPackInt32(SHandle *handle, int32 value, char *name);
extern void     styxPackUint64(SHandle *handle, uint64 value, char *name);
extern void     styxPackInt64(SHandle *handle, int64 value, char *name);
extern void     styxPackFloat(SHandle *handle, float value, char *name);
extern void     styxPackDouble(SHandle *handle, double value, char *name);
extern boolean  styxPackString(SHandle *handle, char *value, char *name);
extern uint64   styxPackRaw(SHandle *handle, uint8 *data, uint64 length, char *name);

/* -------- Data Unpacking --------
 * Functions used to unpack data in to a stram. If STYX_DEBUG is not defined, the param "name" will be removed with a macro.
 * In debug mode (where both STYX_DEBUG is defined AND the bug mode of the stream has been set to TRUE styxDebugModeSet)
 * the name, and data type will be checked to make sure they match the name and type that was packed.
 * If this test fails the application will terminate and write out a Error raport to standard out.*/

extern uint8    styxUnpackUint8(SHandle *handle, char *name);
extern int8     styxUnpackInt8(SHandle *handle, char *name);
extern uint16   styxUnpackUint16(SHandle *handle, char *name);
extern int16    styxUnpackInt16(SHandle *handle, char *name);
extern uint32   styxUnpackUint32(SHandle *handle, char *name);
extern int32    styxUnpackInt32(SHandle *handle, char *name);
extern uint64   styxUnpackUint64(SHandle *handle, char *name);
extern int64    styxUnpackInt64(SHandle *handle, char *name);
extern float    styxUnpackFloat(SHandle *handle, char *name);
extern double   styxUnpackDouble(SHandle *handle, char *name);
extern boolean  styxUnpackString(SHandle *handle, char *value, uint buffer_size, char *name);
extern char    *styxUnpackStringWithOwnership(SHandle *handle, char *name);
extern uint64   styxUnpackRaw(SHandle *handle, uint8 *buffer, uint64 buffer_len, char *name);

#include "s_debug.h"


#endif /* STYX_H */
