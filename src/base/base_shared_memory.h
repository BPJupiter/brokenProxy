
#ifndef BASE_SHARED_MEMORY_H
#define BASE_SHARED_MEMORY_H

typedef struct Shared_Memory Shared_Memory;
struct Shared_Memory {
    u64 u64[1];
};

/////////////////////////////////
// @per_os_impl Shared Memory

internal Shared_Memory shared_memory_alloc(u64 size, String8 name);
internal Shared_Memory shared_memory_open(String8 name);
internal void          shared_memory_close(Shared_Memory handle);
internal void         *shared_memory_view_open(Shared_Memory handle, Rng1u64 range);
internal void          shared_memory_view_close(Shared_Memory handle, void *ptr, Rng1u64 range);

#endif // BASE_SHARED_MEMORY_H
