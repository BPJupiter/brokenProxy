
/////////////
// Globals

global u32 handle_buffer_size[] = {
    [Net_HandleKind_StreamingServer]     = 4096,
    [Net_HandleKind_StreamingConnection] = 4096,
    [Net_HandleKind_PacketPeer]          = 1500
};

///////////////////////
// Network Functions

internal Net_Handle net_handle_alloc(Arena *arena, Net_HandleKind kind, Net_Adrress address)
{
    Net_Handle handle = {0};
    handle.address = address;
    handle.read_buffer  = make_ring(arena, handle_buffer_size[kind]);
    handle.write_buffer = make_ring(arena, handle_buffer_size[kind]);
    handle.is_connected = false;

    return handle;
}

internal u64 net_handle_send(Net_Handle *handle)
{
    
}

internal u64 net_handle_receive(Net_Handle *handle, Net_Address *from)
{
}

//////////////////
// Data Packing

internal bool32 net_handle_pack(Net_Handle *handle, u64 size, void *ptr)
{
    return ring_try_write(&handle->write_buffer, size, ptr);
}

internal bool32 net_handle_unpack(Net_Handle *handle, u64 size, void *ptr)
{
    bool32 is_tcp = false;
    u32 size_recieved = 0;
    if (handle.kind == Net_HandleKind_StreamingServer)     is_tcp = true;
    if (handle.kind == Net_HandleKind_StreamingConnection) is_tcp = true;
    if (is_tcp) {
        size_recieved = 
    }
    return ring_try_read(&handle->read_buffer, size, ptr);
}

