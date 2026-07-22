
#ifndef BASE_EXT_ARENA_H
#define BASE_EXT_ARENA_H

internal u64 *push_u64(Arena *arena, u64 value);
internal u32 *push_array_copy_u32(Arena *arena, u32 *v, u64 count);
internal u64 *push_array_copy_u64(Arena *arena, u64 *v, u64 count);
internal Arena **alloc_fixed_size_arena_array(Arena *arena, u64 count, u64 res, u64 cmt);
#define alloc_arena_array(_arena_, _arena_count_, _counts_, T) alloc_arena_array_(_arena_, _arena_count_, _counts_, sizeof(T))

#endif // BASE_EXT_ARENA_H
