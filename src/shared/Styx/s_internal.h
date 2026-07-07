
#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET VSocket;
#define INVALID_VSOCKET INVALID_SOCKET
#else
typedef int VSocket;
#define INVALID_VSOCKET -1
#endif

#define STYX_MINIMUM_WRITE_SPACE 1024

typedef enum {
    S_TYPE_UINT8,
    S_TYPE_INT8,
    S_TYPE_UINT16,
    S_TYPE_INT16,
    S_TYPE_UINT32,
    S_TYPE_INT32,
    S_TYPE_UINT64,
    S_TYPE_INT64,
    S_TYPE_FLOAT,
    S_TYPE_DOUBLE,
    S_TYPE_RAW,
    S_TYPE_STRING,
    S_TYPE_STRUCT,
    S_TYPE_COUNT
} STypes;

extern uint     styx_unpack_buffer_get(SHandle *handle);
extern boolean     styx_pack_buffer_clear(SHandle *handle);
extern void     styx_handle_clear(SHandle *handle, uint type);
extern size_t     styx_network_stream_receive(SHandle *handle, uint8 *buffer, size_t length);
extern size_t     styx_network_stream_send(SHandle *handle, uint8 *buffer, size_t length);
