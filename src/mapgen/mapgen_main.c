
/////////////////
// Build Options

#define BUILD_CONSOLE_INTERFACE 1

/////////////
// Includes

#include "base/base_inc.h"
#include "mapgen_inc.h"

#include "base/base_inc.c"
#include "mapgen_inc.c"

// Entry Point

#if !defined(FBTE_INCLUDE_FBT_EARCUT_H)
# define FBT_EARCUT_IMPLEMENTATION
# include "third_party/fbt/fbt_earcut.h"
#endif

internal void entry_point(Cmd_Line *cmdline)
{
    map_arena = arena_alloc(.reserve_size = Gigabytes(64), .commit_size = Megabytes(64));
    map_state = push_array(map_arena, MAP_State, 1);

    // extract paths
    String8 build_dir_path = get_process_info()->binary_path;
    String8 project_dir_path = str8_chop_last_slash(build_dir_path);
    String8 data_dir_path = str8f(map_arena, "%S/data", project_dir_path);

    // parse .shp data
    u8 *base = map_world_shp_bytes.str;
    u64 off = 0;
    if (map_world_shp_bytes.size >= 100 &&
        map_shp_read_be_s32(base, &off) == 9994)
    {
        off += 5*sizeof(u32); // unused
        u32 file_length        = (u32)map_shp_read_be_s32(base, &off);
        u32 version            = (u32)map_shp_read_le_u32(base, &off);
        MAP_SHP_ShapeKind kind = map_shp_read_le_s32(base, &off);
        map_state->map_rect.x0 = map_shp_read_le_f64(base, &off); 
        map_state->map_rect.y0 = map_shp_read_le_f64(base, &off); 
        map_state->map_rect.x1 = map_shp_read_le_f64(base, &off); 
        map_state->map_rect.y1 = map_shp_read_le_f64(base, &off);
        off += 4*sizeof(f64); // min/max z, min/max m
        fprintf(stderr,
                "dimensions of map are %.2f,%.2f and %.2f,%.2f\n",
                map_state->map_rect.x0, map_state->map_rect.y0,
                map_state->map_rect.x1, map_state->map_rect.y1);
        fprintf(stderr, "map consists of %.*s features\n", str8_varg(map_shp_string_from_shape_kind(kind)));

        for (; off + 8 < map_world_shp_bytes.size ;)
        {
            u32 record_number = (u32)map_shp_read_be_s32(base, &off);
            u32 content_length = (u32)map_shp_read_be_s32(base, &off);
            
            u64 next_record_off = off + (content_length * 2);
            // we only see polygons in this file so I'm just going to hardcode it
            MAP_SHP_ShapeKind feature_kind = map_shp_read_le_s32(base, &off);
            switch(feature_kind)
            {
                default:{}break;
                case MAP_SHP_ShapeKind_Null: {
                    {
                    }
                } break;
                case MAP_SHP_ShapeKind_Polygon: {
                    {
                        MAP_Feature_Node *feature = push_array(map_arena, MAP_Feature_Node, 1);
                        SLLQueuePush(map_state->features.first, map_state->features.last, feature);
                        map_state->features.count += 1;

                        feature->v.bounding_box.x0 = map_shp_read_le_f64(base, &off);
                        feature->v.bounding_box.y0 = map_shp_read_le_f64(base, &off);
                        feature->v.bounding_box.x1 = map_shp_read_le_f64(base, &off);
                        feature->v.bounding_box.y1 = map_shp_read_le_f64(base, &off);
                        feature->v.indices.count = map_shp_read_le_u32(base, &off);
                        feature->v.indices.v = push_array(map_arena, u32, feature->v.indices.count);
                        feature->v.vertices.count = map_shp_read_le_u32(base, &off);
                        feature->v.vertices.v = push_array(map_arena, f64, 2*feature->v.vertices.count);
                        printf("This feature has %llu vertices and %llu parts\n",
                               feature->v.vertices.count,
                               feature->v.indices.count);
                        for (u64 idx = 0; idx < feature->v.indices.count; idx += 1)
                        {
                            feature->v.indices.v[idx] = map_shp_read_le_u32(base, &off);
                        }
                        for (u64 idx = 0; idx < 2*feature->v.vertices.count; idx += 2)
                        {
                            feature->v.vertices.v[idx] = map_shp_read_le_f64(base, &off);
                            feature->v.vertices.v[idx + 1] = map_shp_read_le_f64(base, &off);
                        }
                    }
                } break;
            }

            off = next_record_off;
        }

        for (MAP_Feature_Node *n = map_state->features.first; n != 0; n = n->next)
        {
            FBTE_Data *data = fbte_load_from_memory((char *)n->v.vertices.v,
                                                    n->v.vertices.count,
                                                    sizeof(n->v.vertices.v[0]),
                                                    n->v.indices.v,
                                                    n->v.indices.count,
                                                    0);

            n->v.vertices.v = push_array(map_arena, f64, data->vertex_count*data->vertex_comp);
            MemoryCopy(n->v.vertices.v,
                       data->vertex_data,
                       data->vertex_count*data->vertex_comp*sizeof(f64));
            n->v.vertices.count = data->vertex_count;
            map_state->vertex_data.count += n->v.vertices.count;
            
            n->v.indices.v = push_array(map_arena, u32, data->index_count);
            MemoryCopy(n->v.indices.v,
                       data->index_data,
                       data->index_count*sizeof(u32));
            n->v.indices.count = data->index_count;
            map_state->index_data.count += n->v.indices.count;
            
            fbte_earcut_free(data);
        }

        map_state->vertex_data.v = push_array(map_arena, f64, 2*map_state->vertex_data.count);
        map_state->index_data.v  = push_array(map_arena, u32, map_state->index_data.count);
        u64 vertex_data_off = 0;
        u64 index_data_off = 0;
        u64 index_base = 0;
        for (MAP_Feature_Node *n = map_state->features.first; n != 0; n = n->next)
        {
            for (u64 j = 0; j < n->v.indices.count; j++)
            {
                map_state->index_data.v[index_data_off + j] = n->v.indices.v[j] + (u32)index_base;
            }
            MemoryCopy(&map_state->vertex_data.v[vertex_data_off], n->v.vertices.v, 2*n->v.vertices.count*sizeof(f64));
            vertex_data_off += 2*n->v.vertices.count;
            index_data_off += n->v.indices.count;
            index_base += n->v.vertices.count;
        }

        FILE *v = fopen("../data/generated/map_vertices.vb", "wb");
        fwrite(map_state->vertex_data.v, sizeof(f64), vertex_data_off, v);
        fclose(v);
        FILE *i = fopen("../data/generated/map_indices.ib", "wb");
        fwrite(map_state->index_data.v, sizeof(u32), index_data_off, i);
        fclose(i);
    }
}
