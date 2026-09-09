
///////////////////
// Build Options

#define BUILD_TITLE "TIDE"
#define OS_FEATURE_GRAPHICAL 1

#define WM_INIT_MANUAL  1
#define FP_INIT_MANUAL  1
#define R_INIT_MANUAL   1
#define FNT_INIT_MANUAL 1
#define TI_INIT_MANUAL  1

#define ARENA_TABLE_DEBUG BUILD_DEBUG

//////////////
// Includes

#include "base/base_inc.h"
#include "win32/win32_inc.h"
#include "artifact_cache/artifact_cache.h"
#include "mdesk/mdesk.h"
#include "net/net_inc.h"
#include "dns/dns_inc.h"
#include "window_manager/window_manager_inc.h"
#include "shell/shell_inc.h"
#include "config/config_inc.h"
#include "content/content.h"
#include "file_stream/file_stream.h"
#include "text/text.h"
#include "mutable_text/mutable_text.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
//#include "geo/geo_inc.h"
#include "tide_inc.h"

#include "base/base_inc.c"
#include "artifact_cache/artifact_cache.c"
#include "mdesk/mdesk.c"
#include "net/net_inc.c"
#include "dns/dns_inc.c"
#include "window_manager/window_manager_inc.c"
#include "shell/shell_inc.c"
#include "config/config_inc.c"
#include "content/content.c"
#include "file_stream/file_stream.c"
#include "text/text.c"
#include "mutable_text/mutable_text.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
//#include "geo/geo_inc.c"
#include "tide_inc.c"

////////////////////////////////
// Top Level Execution Types

typedef enum ExecMode {
    ExecMode_Normal,
    ExecMode_Help,
} ExecMode;

///////////////////////////
// Per-Frame Entry Point

internal bool32 frame(void)
{
    ti_frame();
    return ti_state->quit;
}

/////////////////
// Entry Point

void entry_point(Cmd_Line *cmdline)
{
    Temp scratch = scratch_begin(0, 0);

    // unpack command line arguments
    ExecMode exec_mode = ExecMode_Normal;
    if (cmd_line_has_flag(cmdline, str8_lit("?")) ||
        cmd_line_has_flag(cmdline, str8_lit("help")))
    {
        exec_mode = ExecMode_Help;
    }

    // init log
    g_logs_folder = cmd_line_string(cmdline, str8_lit("logs"));
    if (g_logs_folder.size == 0)
    {
        String8 user_program_logs_data_path = get_process_info()->user_program_logs_data_path;
        g_logs_folder = push_str8f(scratch.arena, "%S/app/logs", user_program_logs_data_path);
    }
    make_directory(g_logs_folder);

    switch (exec_mode)
    {
        default:
        case ExecMode_Normal: {
            // manual layer initialisation
            {
                wm_init();
                fp_init();
                r_init(cmdline);
                fnt_init();
                ti_init(cmdline);
            }

            // Main application loop
            {
                for (bool32 quit = false; !quit;)
                {
                    // update
                    quit = update();
                }
            }
        } break;

        case ExecMode_Help: {
            {
                sh_message(0,
                           str8_lit("The TIDE Proxy - Help"),
                           str8_lit("The following options may be used when starting the application from the command line:\n\n"
                                    "--user:<path>\n"
                                    "Use to specify the location of a user file which should be used. User files are used to store settings for users, inclulding window and panel setups and visual settings. If this file does not exist, it will be created as necessary. This file will be autosaved as user-related changes are math.\n\n"
                                    "--project:<path>\n"
                                    "Use to specify the location of a project file which should be used. Project files are used to store settings for workspace. If this file does not exist, it will be created as necessary. This file will be autosaved as project-related changed are made.\n\n"
                                    "--logs:<path>\n"
                                    "Overrides default path to the folder with logs.\n"
                                    ));
            }
        } break;
    }

    scratch_end(scratch);
}
