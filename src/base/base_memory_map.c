// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Memory Map Functions

internal void memory_map_push(Arena *arena, Memory_Map *map, Rng1u64 vaddr_range, void *data)
{
    Memory_Map_Range_Node *n = push_array(arena, Memory_Map_Range_Node, 1);
    n->v.vaddr_range = vaddr_range;
    n->v.base = data;
    SLLQueuePush(map->first_range, map->last_range, n);
}

internal u64 memory_map_read(Memory_Map *map, Rng1u64 range, void *dst)
{
    u64 dst_vaddr = range.min;
    {
        for(Memory_Map_Range_Node *n = map->first_range; n != 0; n = n->next) {
            if(contains_1u64(n->v.vaddr_range, dst_vaddr)) {
                u64 src_off = dst_vaddr - n->v.vaddr_range.min;
                u64 num_bytes_possible = n->v.vaddr_range.max - dst_vaddr;
                u64 num_bytes_needed = range.max - dst_vaddr;
                u64 num_bytes_to_read = Min(num_bytes_needed, num_bytes_possible);
                MemoryCopy((u8 *)dst + (dst_vaddr - range.min), (u8 *)n->v.base + src_off, num_bytes_to_read);
                dst_vaddr += num_bytes_to_read;
            }
        }
    }
    u64 bytes_read = (dst_vaddr - range.min);
    return bytes_read;
}
