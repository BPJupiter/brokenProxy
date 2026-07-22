#ifndef LINUX_BASE_H
#define LINUX_BASE_H

///////////////
// Includes

#include <dirent.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <features.h>
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

pid_t gettid(void);
int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char *name, size_t size);

typedef struct tm tm;
typedef struct timespec timespec;

///////////////////////////////////////////
// Linux Call Interruption Retry Helper

#define LNX_RETRY_ON_EINTR(expr)                       \
    (__extension__({                                   \
            __typeof__(expr) __ret;                    \
            do {                                       \
                __ret = (expr);                        \
            } while ((__ret == -1) && errno == EINTR); \
            __ret;                                     \
        }))

////////////////////
// File Iterator

typedef struct LNX_File_Iter LNX_File_Iter;
struct LNX_File_Iter {
    DIR *dir;
    struct dirent *dp;
    String8 path;
};
StaticAssert(sizeof(Member(File_Iter, memory)) >= sizeof(LNX_File_Iter), lnx_file_iter_size_check);

//////////////////////////////
// Safe Call Handler Chain

typedef struct LNX_Safe_Call_Chain LNX_Safe_Call_Chain;
struct LNX_Safe_Call_Chain {
    LNX_Safe_Call_Chain *next;
    Thread_Entry_Point_Function_Type *fail_handler;
    void *ptr;
};

///////////////
// Entities

typedef enum LNX_EntityKind {
    LNX_EntityKind_Thread,
    LNX_EntityKind_Mutex,
    LNX_EntityKind_RWMutex,
    LNX_EntityKind_ConditionVariable,
    LNX_EntityKind_Barrier,
} LNX_EntityKind;

typedef struct LNX_Entity LNX_Entity;
struct LNX_Entity {
    LNX_Entity *next;
    LNX_EntityKind kind;
    union
    {
        struct {
            pthread_t handle;
            Thread_Entry_Point_Function_Type *func;
            void *ptr;
        } thread;
        pthread_mutex_t mutex_handle;
        pthread_rwlock_t rwmutex_handle;
        struct {
            pthread_cond_t cond_handle;
            pthread_mutex_t rwlock_mutex_handle;
        } cv;
        pthread_barrier_t barrier;
    };
};

////////////
// State

typedef struct LNX_State LNX_State;
struct LNX_State {
    Arena *arena;
    System_Info system_info;
    Process_Info process_info;
    pthread_mutex_t entity_mutex;
    Arena *entity_arena;
    LNX_Entity *entity_free;
    u64 default_env_count;
    char **default_env;
};

//////////////
// Globals

global LNX_State lnx_state = {0};
thread_static LNX_Safe_Call_Chain *lnx_safe_call_chain = 0;

//////////////
// Helpers

internal Date_Time       lnx_date_time_from_tm(tm in, u32 msec);
internal tm              lnx_tm_from_date_time(Date_Time dt);
internal timespec        lnx_timespec_from_date_time(Date_Time dt);
internal Dense_Time      lnx_dense_time_from_timespec(timespec in);
internal File_Properties lnx_file_properties_from_stat(struct stat *s);
internal void            lnx_safe_call_sig_handler(int x);

///////////////
// Entities

internal LNX_Entity *lnx_entity_alloc(LNX_EntityKind kind);
internal void        lnx_entity_release(LNX_Entity *entity);

/////////////////////////
// thread entry point

internal void *lnx_thread_entry_point(void *ptr);

#endif // LINUX_BASE_H
