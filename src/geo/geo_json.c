
// Third Party Includes
#include "third_party/cjson/cJSON.c"

// Generated Code
#include "generated/geo.meta.c"

/////////////
// Helpers

internal void geo_feature_list_push_node(GEO_Feature_List *list, GEO_Feature_Node *n)
{
    SLLQueuePush(list->first, list->last, n);
    list->count += 1;
}

internal GEO_Feature_Node * geo_feature_list_push(Arena *arena, GEO_Feature_List *list, GEO_Feature feature)
{
    GEO_Feature_Node *n = push_array(arena, GEO_Feature_Node, 1);
    MemoryCopyStruct(&n->v, &feature);
    n->v.indices.v = push_array(arena, u32, n->v.indices.count);
    MemoryCopy(&n->v.indices.v, &feature.indices.v, n->v.indices.count);
    geo_feature_list_push_node(list, n);
    return n;
}

internal void geo_feature_list_concat(GEO_Feature_List *list, GEO_Feature_List *to_concat)
{
    if (to_concat->first)
    {
        if (list->first)
        {
            list->last->next = to_concat->first;
            list->last       = to_concat->last;
        }
        else
        {
            list->first = to_concat->first;
            list->last  = to_concat->last;
        }
        list->count += to_concat->count;
        MemoryZeroStruct(to_concat);
    }
}

internal GEO_Feature_Array geo_feature_array_from_list(Arena *arena, GEO_Feature_List *list)
{
    GEO_Feature_Array arr = {0};
    arr.count             = list->count;
    arr.v                 = push_array_no_zero(arena, GEO_Feature, arr.count);
    u64 idx = 0;
    for (GEO_Feature_Node *n = list->first; n != 0; n = n->next)
    {
        arr.v[idx] = n->v;
        idx += 1;
    }
    return arr;
}

/////////////////
// Core Functions

internal GEO_Mesh_Set geo_mesh_set_from_geojson(Arena *arena, cJSON *geojson)
{
    /*
    cJSON *features = cJSON_GetObjectItemCaseSensitive(geojson, "features");
    cJSON *feature = 0;
    u64 feature_count = (u64)cJSON_GetArraySize(features);

    GEO_Feature_Array *features = push_array(arena, GEO_Feature_Mesh, feature_count);

    // figure out how many vertices we need to store
    u64 vertex_count = 0;
    cJSON_ArrayForEach(feature, features)
    {
        cJSON *geometry = cJSON_GetObjectItemCaseSensitive(feature, "geometry");
        GEO_Type type = geo_type_from_string(str8_cstring(cJSON_GetObjectItemCaseSensitive(geometry, "type")->valuestring));
        cJSON *coordinates = cJSON_GetObjectItemCaseSensitive(geometry, "coordinates");
        switch (type)
        {
            case GEO_Type_MultiPolygon: {
                {
                    cJSON *polygon = 0;
                    cJSON_ArrayForEach(polygon, coordinates)
                    {
                        cJSON *ring = 0;
                        cJSON_ArrayForEach(ring, polygon)
                        {
                            vertex_count += (u64)(cJSON_GetArraySize(ring));
                        }
                    }
                }
            } break;
            case GEO_Type_Polygon: {
                {
                    cJSON *ring = 0;
                    cJSON_ArrayForEach(ring, coordinates)
                    {
                        vertex_count += (u64)(cJSON_GetArraySize(ring));
                    }
                }
            } break;
        }
    }
    // allocate memory
    Vec3f32 *vertices = push_array(arena, Vec3f32, vertex_count);

    // traverse tree again and extract coordinates
    cJSON_ArrayForEach(feature, features)
    {
        Rng2f32 bbox = rng_2f32(vec_2f32(neg_inf32, neg_inf32), vec2f32(neg_inf32, neg_inf32));
        u64 feature_vertex_base = 0;
        u64_Array ring_beginnings = {0};

        cJSON *geometry = cJSON_GetObjectItemCaseSensitive(feature, "geometry");
        GEO_Type type = geo_type_from_string(str8_cstring(cJSON_GetObjectItemCaseSensitive(geometry, "type")->valuestring));
        cJSON *coordinates = cJSON_GetObjectItemCaseSensitive(geometry, "coordinates");
        switch (type)
        {
            case GEO_Type_MultiPolygon: {
                {
                }
            } break;
            case GEO_Type_Polygon: {
                {
                    cJSON *ring = 0;

                    cJSON_ArrayForEach(ring, coordinates)
                    {
                        cJSON *point = 0;
                        cJSON_ArrayForEach(point, ring)
                        {
                            f64 longitude = cJSON_GetArrayItem(point, 0)->valuedouble;
                            f64 latitude = cJSON_GetArrayItem(point, 1)->valuedouble;
                        }
                    }
                }
            } break;
        }
    }
    */
}

