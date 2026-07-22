
internal u64 *push_u64(Arena *arena, u64 value)
{
    u64 *result = push_array_no_zero(arena, u64, 1);
    *result = value;
    return result;
}

internal u32 *push_array_copy_u32(Arena *arena, u32 *v, u64 count)
{
    u32 *result = push_array_no_zero(arena, u32, count);
    MemoryCopyTyped(result, v, count);
    return result;
}

internal u64 *push_array_copy_u64(Arena *arena, u64 *v, u64 count)
{
    u64 *result = push_array_no_zero(arena, u64, count);
    MemoryCopyTyped(result, v, count);
    return result;
}

internal Arena **alloc_arena_many(Arena *arena, u64 count, u64 *sizes)
{
    Arena **result = push_array(arena, Arena *, count);
    u64  total_size = sum_array_u64(count, sizes);
    u8  *buffer     = push_array(arena, u8, total_size + (ARENA_HEADER_SIZE * count));
    u64  offset     = 0;
    for EachIndex(i, count) {
        if (sizes[i] > 0) {
            void *backing_buffer = buffer + offset;
            result[i] = arena_alloc_(&(Arena_Params){ .flags = ArenaFlag_NoChain, .optional_backing_buffer = backing_buffer, .reserve_size = ARENA_HEADER_SIZE + sizes[i], .commit_size = ARENA_HEADER_SIZE + sizes[i] });
            offset += ARENA_HEADER_SIZE;
            offset += sizes[i];
        }
    }
    return result;
}

internal Arena **alloc_arena_array_(Arena *arena, u64 count, u64 *counts, u64 element_size)
{
    Temp scratch = scratch_begin(&arena, 1);
    u64 *sizes = push_array(scratch.arena, u64, count);
    for EachIndex(i, count) { sizes[i] = counts[i] * element_size; }
    Arena **result = alloc_arena_many(arena, count, sizes);
    scratch_end(scratch);
    return result;
}

internal Arena **alloc_fixed_size_arena_array(Arena *arena, u64 count, u64 res, u64 cmt)
{
    u64 data_size = sizeof(count) + sizeof(Arena *) * count;
    u8 *data = push_array_no_zero(arena, u8, data_size);
    u64 *count_ptr = (u64 *)data;
    Arena **arr = (Arena **)(count_ptr + 1);
    *count_ptr = count;
    
    for EachIndex(i, count) {
        Arena *fixed_arena = arena_alloc_(&(Arena_Params){ .reserve_size = res, .commit_size = cmt });
        arr[i] = fixed_arena;
    }
    
    return arr;
}
