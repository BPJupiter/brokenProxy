
// build options

#define BUILD_TITLE "TORMENT"

#define BUILD_TESTS 1
#define BUILD_CONSOLE_INTERFACE 1
#define OS_FEATURE_GRAPHICAL 0
#define WM_STUB 1


//////////////////////////////

#include "base/base_inc.h"
#include "base_ext/base_ext_inc.h"
#include "window_manager/window_manager_inc.h"
#include "torment.h"


#include "base/base_inc.c"
#include "base_ext/base_ext_inc.c"
#include "window_manager/window_manager_inc.c"
#include "torment.c"

#include "base/tests/base_tests.c"

internal bool32 frame(void) { return 0; }

/////////////////////////////

void entry_point(Cmd_Line *cmdline)
{
    t_entry_point(cmdline);
}
