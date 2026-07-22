
#ifndef NET_ADDRESS_H
#define NET_ADDRESS_H

typedef struct Net_Socket Net_Socket;
struct Net_Socket {
    u64 u64[1];
};

typedef enum Net_AddressType {
    Net_AddressType_Ipv4,
    Net_AddressType_Ipv6,
    Net_AddressType_COUNT
} Net_AddressType;

typedef struct Net_Address Net_Address;
struct Net_Address {
    union
    {
        u32 v4;
        u128 v6;
    } ip;
    Net_AddressType address_type;
    u16 port;
};

typedef enum Net_HandleKind {
    Net_HandleKind_StreamingServer,
    Net_HandleKind_StreamingConnection,
    Net_HandleKind_PacketPeer,
    Net_HandleKind_COUNT,
} Net_HandleKind;

typedef struct Net_Handle Net_Handle;
struct Net_Handle {
    Net_Socket socket;
    Net_Address address;
    Net_HandleKind kind;
    Ring *read_buffer;
    Ring *write_buffer;
    bool32 is_connected;
};

///////////////////////
// Network Functions

internal Net_Handle  net_handle_alloc(Arena *arena, Net_HandleKind kind, etc);
internal u64         net_handle_send(Net_Handle *handle);
internal u64         net_handle_receive(Net_Handle *handle, Net_Address *from);

//////////////////
// Data Packing

internal bool32        net_handle_pack(Net_Handle *handle, u64 size, void *ptr);
#define net_handle_pack_struct(handle, ptr) net_handle_pack((handle), sizeof(*(ptr)), (ptr))
internal bool32        net_handle_unpack(Net_Handle *handle, u64 size, void *ptr);
#define net_handle_unpack_struct(handle, ptr) net_handle_unpack((handle), sizeof(*(ptr)), (ptr))



#endif // NET_ADDRESS_H
