
#include "base_core.c"
#include "base_profile.c"
#include "base_memory.c"
#include "base_arena.c"
#include "base_math.c"
#include "base_strings.c"
#include "base_memory_map.c"
#include "base_hash.c"
#include "base_system.c"
#include "base_threads.c"
#include "base_profile.c"
#include "base_thread_context.c"
#include "base_files.c"
#include "base_shared_memory.c"
#include "base_processes.c"
#include "base_dynamic_libraries.c"
#include "base_command_line.c"
#include "base_markup.c"
#include "base_profile.c"
#include "base_log.c"
#include "base_test.c"
#include "base_entry_point.c"

#include "base_hash_table.c"

#if OS_WINDOWS
# include "win32/base/win32_base.c"
#elif OS_LINUX
# include "linux/base/linux_base.c"
#else
# error Operating system backed not found for the base layer.
#endif
