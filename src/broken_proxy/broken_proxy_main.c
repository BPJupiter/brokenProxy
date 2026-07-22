
///////////////////
// Build Options

#define BUILD_TITLE "BROKEN PROXY"

#define BUILD_CONSOLE_INTERFACE 1
#define OS_FEATURE_GRAPHICAL 0
#define WM_STUB 1

//////////////
// Includes

#include "base/base_inc.h"
#include "base_ext/base_ext_inc.h"
#include "window_manager/window_manager_inc."
#include "broken_proxy.h"

#include "base/base_inc.c"
#include "base_ext/base_ext_inc.c"
#include "window_manager/window_manager_inc.c"
#include "broken_proxy.c"

internal bool32 frame(void) { return 0; }

/////////////////
// Entry Point

void entry_point(Cmd_Line *cmdline)
{
    bp_entry_point(cmdline);
}
