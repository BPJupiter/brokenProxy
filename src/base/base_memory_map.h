
#ifndef BASE_MEMORY_MAP_H
#define BASE_MEMORY_MAP_H

///////////////////////
// Memory Map Types

typedef struct Memory_Map_Range Memory_Map_Range;
struct Memory_Map_Range {
    Rng1u64 vaddr_range;
    void *base;
};

typedef struct Memory_Map_Range_Node Memory_Map_Range_Node;
struct Memory_Map_Range_Node {
    Memory_Map_Range_Node *next;
    Memory_Map_Range v;
};

typedef struct Memory_Map Memory_Map;
struct Memory_Map {
    Memory_Map_Range_Node *first_range;
    Memory_Map_Range_Node *last_range;
};

///////////////////////////
// Memory Map Functions

internal void memory_map_push(Arena *arena, Memory_Map *map, Rng1u64 vaddr_range, void *data);
internal u64  memory_map_read(Memory_Map *map, Rng1u64 range, void *dst);
#define memory_map_read_struct(map, vaddr, ptr) memory_map_read((map), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))

#endif // BASE_MEMORY_MAP_H
