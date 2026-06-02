#include "Clay/clay.h"

#ifndef STYX_H
#define STYX_H

typedef struct StyxNetworkAddress
{
    union {
        uint32 v4;
        uint8 v6[16]; 
    } ip;
    uint16 port;
    struct StyxNetworkAddress *next;
    boolean is_ipv6;
}StyxNetworkAddress;

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

extern SHandle *styx_network_stream_address_create      (const char *host_name, uint16 port, boolean ipv6);
extern SHandle *styx_network_stream_ip_create           (StyxNetworkAddress ip);
extern int      styx_network_stream_send_force          (SHandle *handle);
extern SHandle *styx_network_stream_wait_for_connection  (SHandle *listen, StyxNetworkAddress *from);
extern boolean  styx_network_stream_connected          (SHandle *handle);

/* -------- UDP Networking --------
Handles the creation and management of UDP datagrams. */

extern boolean (*styx_network_address_lookup)(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6);
extern boolean  styx_network_address_compare   (StyxNetworkAddress *a, StyxNetworkAddress *b);
extern SHandle *styx_network_datagram_create   (uint16 port, boolean ipv6);
extern int      styx_network_datagram_send     (SHandle *handle, StyxNetworkAddress *to);
extern int      styx_network_receive          (SHandle *handle, StyxNetworkAddress *from);

/* -------- File Management --------
Handles the creation and management of binary file handles. */

extern SHandle *styx_file_load(char *path);
extern SHandle *styx_file_save(char *path);
extern uint64   styx_file_size(SHandle *handle);
extern void     styx_file_position_get(SHandle *handle, uint64 pos);
extern uint64   styx_file_position_set(SHandle *handle);

/* -------- Memory Buffers --------
Handles the creation and management of in memory FIFO channels. */

extern SHandle *styx_buffer_create(void); /* Creates a handle to a Styx FIFO Memory buffer. */
extern void    *styx_buffer_get(SHandle *handle, uint32 *size); /* Gives the user read direct memory access to the packed data. If the handle is set to debug mode it will contain debug header information. */
extern void     styx_buffer_set(SHandle *handle, void *data, uint32 size); /* Lets the user give a pointer to "data" of "size" number of bytes that will be added to the buffer. */

/* -------- Handle Utils --------
*/

extern SHandleType  styx_type        (SHandle *handle); /* Returns the type of a Styx handle */
extern void         styx_free        (SHandle *handle); /* Frees a handle */
extern int          styx_network_wait (SHandle **handles, boolean *read, boolean *write, uint handle_count, uint micro_seconds); /* Queries if network handles are tread to read and write. Can handle multiple handles as specified by the array "handles" and handle count. The function will block for at most miliseconds and wait for any resource to become available. Once a resource is available, it will return. */

/* -------- Data Packing --------
 * Functions used to pack data into a stream. If STYX_DEBUG is not defined, the param "name" will be removed with the macro.
 * In debug mode (where both STYX_DEBUG is defined AND the bug mode of the stream has been set to TRUE styxDebugModeSet)
 * the name and data type will be checked to make sure they match the name and type that was packed.
 * If this test fails the application will terminate on unpack and write out a Error raport to standard out.*/

extern void     styx_pack_uint8   (SHandle *handle, uint8 value, char *name);
extern void     styx_pack_int8    (SHandle *handle, int8 value, char *name);
extern void     styx_pack_uint16  (SHandle *handle, uint16 value, char *name);
extern void     styx_pack_int16   (SHandle *handle, int16 value, char *name);
extern void     styx_pack_uint32  (SHandle *handle, uint32 value, char *name);
extern void     styx_pack_int32   (SHandle *handle, int32 value, char *name);
extern void     styx_pack_uint64  (SHandle *handle, uint64 value, char *name);
extern void     styx_pack_int64   (SHandle *handle, int64 value, char *name);
extern void     styx_pack_float   (SHandle *handle, float value, char *name);
extern void     styx_pack_double  (SHandle *handle, double value, char *name);
extern boolean  styx_pack_string  (SHandle *handle, char *value, char *name); /* Do not use for non-null terminated text protocols e.g. HTTP */
extern uint64   styx_pack_raw     (SHandle *handle, uint8 *data, uint64 length, char *name);

/* -------- Data Unpacking --------
 * Functions used to unpack data in to a stram. If STYX_DEBUG is not defined, the param "name" will be removed with a macro.
 * In debug mode (where both STYX_DEBUG is defined AND the bug mode of the stream has been set to TRUE styxDebugModeSet)
 * the name, and data type will be checked to make sure they match the name and type that was packed.
 * If this test fails the application will terminate and write out a Error raport to standard out.*/

extern uint8    styx_unpack_uint8 (SHandle *handle, char *name);
extern int8     styx_unpack_int8  (SHandle *handle, char *name);
extern uint16   styx_unpack_uint16(SHandle *handle, char *name);
extern int16    styx_unpack_int16 (SHandle *handle, char *name);
extern uint32   styx_unpack_uint32(SHandle *handle, char *name);
extern int32    styx_unpack_int32 (SHandle *handle, char *name);
extern uint64   styx_unpack_uint64(SHandle *handle, char *name);
extern int64    styx_unpack_int64 (SHandle *handle, char *name);
extern float    styx_unpack_float (SHandle *handle, char *name);
extern double   styx_unpack_double(SHandle *handle, char *name);
extern boolean  styx_unpack_string(SHandle *handle, char *value, uint buffer_size, char *name);
extern char    *styx_unpack_string_with_ownership(SHandle *handle, char *name);
extern uint64   styx_unpack_raw   (SHandle *handle, uint8 *buffer, uint64 buffer_len, char *name);

#include "s_debug.h"


#endif /* STYX_H */
