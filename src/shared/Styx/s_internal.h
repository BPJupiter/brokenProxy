
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

typedef struct {
	union {
        uint32 v4;
        uint8 v6[16]; 
    } ip;
    uint16 port;
    VSocket socket;
    SHandleType type;
    uint8 *read_buffer;
    size_t read_buffer_used;
    size_t read_buffer_pos;
    size_t read_buffer_size;
    size_t read_marker;
    uint64 read_raw_progress;
    uint8 *write_buffer;
    size_t write_buffer_pos;
    size_t write_buffer_size;
    uint64 write_raw_progress;
    FILE *file;
    void *text_copy;
    char *file_name;
    boolean is_ipv6;
    boolean debug_descriptor;
    boolean debug_header;
    boolean connected;
} SHandle;

extern uint 	styx_unpack_buffer_get(SHandle *handle);
extern boolean 	styx_pack_buffer_clear(SHandle *handle);
extern void 	styx_handle_clear(SHandle *handle, uint type);
extern size_t 	styx_network_stream_receive(SHandle *handle, uint8 *buffer, size_t length);
extern size_t 	styx_network_stream_send(SHandle *handle, uint8 *buffer, size_t length);
