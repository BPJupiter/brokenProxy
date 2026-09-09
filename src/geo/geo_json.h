#ifndef GEO_JSON_H
#define GEO_JSON_H

///////////////////////
// Third Party Includes
#define CJSON_HIDE_SYMBOLS
#include "third_party/cjson/cJSON.h"

/////////////////
// Generated Code
#include "generated/geo.meta.h"

/////////
// Types

typedef struct GEO_Feature GEO_Feature;
struct GEO_Feature
{
    Rng2f32 bbox;
    u32_Array indices;
};

typedef struct GEO_Feature_Node GEO_Feature_Node;
struct GEO_Feature_Node
{
    GEO_Feature_Node *next;
    GEO_Feature v;
};

typedef struct GEO_Feature_List GEO_Feature_List;
struct GEO_Feature_List
{
    GEO_Feature_Node *first;
    GEO_Feature_Node *last;
    u64 count;
};

typedef struct GEO_Feature_Array GEO_Feature_Array;
struct GEO_Feature_Array
{
    GEO_Feature *v;
    u64 count;
};

typedef struct GEO_Mesh_Set GEO_Mesh_Set;
struct GEO_Mesh_Set
{
    Vec3f32_Array vertices;
    GEO_Feature_Array features;
};

//////////
// Helpers

internal void geo_feature_list_push_node(GEO_Feature_List *list, GEO_Feature_Node *n);
internal GEO_Feature_Node * geo_feature_list_push(Arena *arena, GEO_Feature_List *list, GEO_Feature feature);
internal void geo_feature_list_concat(GEO_Feature_List *list, GEO_Feature_List *to_concat);
internal GEO_Feature_Array geo_feature_array_from_list(Arena *arena, GEO_Feature_List *list);

///////////////////
// Core Functions

internal GEO_Mesh_Set geo_mesh_set_from_geojson(Arena *arena, cJSON *geojson);

/////////////////////
// Utility Functions
internal bool32 gj_validate_object(cJSON *object, GEO_Type type);

#endif // GEOJSON_H
