#ifndef BASE_ARENA_H
#define BASE_ARENA_H

//////////////////
// Arena Types

#define ARENA_HEADER_SIZE 128

typedef u64 ArenaFlags;
enum {
    ArenaFlag_NoChain    = (1<<0),
    ArenaFlag_LargePages = (1<<1)
};

typedef struct Arena_Params Arena_Params;
struct Arena_Params {
    ArenaFlags flags;
    u64 reserve_size;
    u64 commit_size;
    void *optional_backing_buffer;
    char *allocation_site_file;
    int allocation_site_line;
    char *name;
};

typedef struct Arena Arena;
struct Arena {
    Arena *prev;
    Arena *current;
    ArenaFlags flags;
    u64 commit_size;
    u64 reserve_size;
    u64 base_pos;
    u64 pos;
    u64 commit;
    u64 reserve;
    char *allocation_site_file;
    int allocation_site_line;
    char *name;
#if ARENA_FREE_LIST
    Arena *free_last;
#endif
#if ARENA_TABLE_DEBUG
    struct Arena_Table_Node *table_node;
#endif
};
StaticAssert(sizeof(Arena) <= ARENA_HEADER_SIZE, arena_header_size_check);

typedef struct Temp Temp;
struct Temp {
    Arena *arena;
    u64 pos;
};

//////////////////////
// Arena functions

global u64 arena_default_reserve_size = Megabytes(64);
global u64 arena_default_commit_size = Kilobytes(64);
global ArenaFlags arena_default_flags = 0;

// arena creation/destruction
internal Arena *arena_alloc_(Arena_Params *params);
#define arena_alloc(...) arena_alloc_(&(Arena_Params){.reserve_size = arena_default_reserve_size, .commit_size = arena_default_commit_size, .flags = arena_default_flags, .allocation_site_file = __FILE__, .allocation_site_line = __LINE__, __VA_ARGS__})
internal void   arena_release(Arena *arena);

// arena push/pop/pos core functions
internal void *arena_push(Arena *arena, u64 size, u64 align, bool32 zero);
internal u64   arena_pos(Arena *arena);
internal void  arena_pop_to(Arena *arena, u64 pos);

// arena push/pop helpers
internal void arena_clear(Arena *arena);
internal void arena_pop(Arena *arena, u64 amount);

// temporary arena scopes
internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

// helper macros
#define push_array_no_zero_aligned(a, T, c, align) (T *)arena_push((a), sizeof(T) * (c), (align), (0))
#define push_array_aligned(a, T, c, align) (T *)arena_push((a), sizeof(T) * (c), (align), (1))
#define push_array_no_zero(a, T, c) push_array_no_zero_aligned(a, T, c, Max(8, AlignOf(T)))
#define push_array(a, T, c) push_array_aligned(a, T, c, Max(8, AlignOf(T)))

#endif // BASE_ARENA_H

