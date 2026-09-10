
/////////////////
// Build Options

#define BUILD_CONSOLE_INTERFACE 1

/////////////
// Includes

#include "base/base_inc.h"
#include "mapgen.h"

#include "base/base_inc.c"
#include "mapgen.c"

// Entry Point

internal void entry_point(Cmd_Line *cmdline)
{
    map_arena = arena_alloc(.reserve_size = Gigabytes(64), .commit_size = Megabytes(64));
    map_state = push_array(map_arena, MAP_State, 1);

    // extract paths
    String8 build_dir_path = get_process_info()->binary_path;
    String8 project_dir_path = str8_chop_last_slash(build_dir_path);
    String8 data_dir_path = str8f(map_arena, "%S/data", project_dir_path);

    // parse .shp data
    if (map_world_shp_bytes.size >= 100 &&
        map_shp_read_be_s32(map_world_shp_bytes.str + 0) == 9994)
    {
        
    }
}
