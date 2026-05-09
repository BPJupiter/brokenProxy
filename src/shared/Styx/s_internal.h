
#define STYX_MINIMUM_WRITE_SPACE 1024

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

extern uint 	styxUnpackBufferGet(SHandle *handle);
extern boolean 	styxPackBufferClear(SHandle *handle);
extern void 	styxHandleClear(SHandle *handle, uint type);
extern size_t 	styxNetworkStreamReceive(SHandle *handle, uint8 *buffer, size_t length);
extern size_t 	styxNetworkStreamSend(SHandle *handle, uint8 *buffer, size_t length);
