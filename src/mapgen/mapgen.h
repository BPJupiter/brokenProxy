#ifndef MAPGEN_H
#define MAPGEN_H

/////////////
// Main Types

typedef struct MAP_Feature MAP_Feature;
struct MAP_Feature
{
    Rng2f32 bounding_box;
    u32_Array indices;
};

typedef struct MAP_Feature_Node MAP_Feature_Node;
struct MAP_Feature_Node
{
    MAP_Feature_Node *next;
    MAP_Feature v;
};

typedef struct MAP_Feature_List MAP_Feature_List;
struct MAP_Feature_List
{
    MAP_Feature_Node *first;
    MAP_Feature_Node *last;
    u64 count;
};

typedef struct MAP_State MAP_State;
struct MAP_State
{
    MAP_Feature_List features;
};

/////////////
// Globals

global Arena *map_arena = 0;
global MAP_State *map_state = 0;

#endif // MAPGEN_H
