
#include "window_manager.c"
#if WM_STUB
# include "window_manager_stub.c"
#elif OS_WINDOWS
# include "win32/window_manager/win32_window_manager.c"
#elif OS_LINUX
# include "linux/window_manager/linux_window_manager.c"
#else
# error Window manager layer not implemented for this operating system. (Try defining WM_SUB=1).
#endif
