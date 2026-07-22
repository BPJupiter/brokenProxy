#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

//@per_os_impl Platform Memory Allocation

internal void *reserve_memory(u64 size);
internal bool32 commit_memory(void *ptr, u64 size);
internal void decommit_memory(void *ptr, u64 size);
internal void release_memory(void *ptr, u64 size);

internal void *reserve_memory_large(u64 size);
internal bool32 commit_memory_large(void *ptr, u64 size);

#endif // BASE_MEMORY_H
