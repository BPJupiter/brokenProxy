#ifndef BASE_THREADS_H
#define BASE_THREADS_H

// Thread types

typedef struct Thread Thread;
struct Thread {
    u64 u64[1];
};
typedef void Thread_Entry_Point_Function_Type(void *p);

// Synchronisation Primitive Types

typedef struct Mutex Mutex;
struct Mutex {
    u64 u64[1];
};

typedef struct RWMutex RWMutex;
struct RWMutex {
    u64 u64[1];
};

typedef struct CondVar CondVar;
struct CondVar {
    u64 u64[1];
};

typedef struct Semaphore Semaphore;
struct Semaphore {
    u64 u64[1];
};

typedef struct Barrier Barrier;
struct Barrier {
    u64 u64[1];
};

// Table Stripes

typedef struct Stripe Stripe;
struct Stripe {
    Arena *arena;
    RWMutex rw_mutex;
    CondVar cv;
    void *free;
};

typedef struct Stripe_Array Stripe_Array;
struct Stripe_Array {
    Stripe *v;
    u64 count;
};

////////////////////////////////
//~ rjf: Table Stripe Functions

internal Stripe_Array stripe_array_alloc(Arena *arena);
internal void stripe_array_release(Stripe_Array *stripes);
internal Stripe *stripe_from_slot_idx(Stripe_Array *stripes, u64 slot_idx);

////////////////////////////////
//~ rjf: Thread Info Helpers

internal void set_thread_name(String8 string);
internal void set_thread_namef(char *fmt, ...);

////////////////////////////////
//~ rjf: @per_os_impl Current Thread Info

internal u32 tid(void);
internal void set_platform_thread_name(String8 name);

////////////////////////////////
//~ rjf: @per_os_impl Thread Functions

internal Thread thread_launch(Thread_Entry_Point_Function_Type *f, void *p);
internal bool32 thread_join(Thread thread, u64 endt_us);
internal void thread_detach(Thread thread);

////////////////////////////////
//~ rjf: @per_os_impl Synchronization Primitive Functions

//- rjf: recursive mutexes
internal Mutex mutex_alloc(void);
internal void  mutex_release(Mutex mutex);
internal void  mutex_take(Mutex mutex);
internal void  mutex_drop(Mutex mutex);

//- rjf: reader/writer mutexes
internal RWMutex rw_mutex_alloc(void);
internal void    rw_mutex_release(RWMutex mutex);
internal void    rw_mutex_take(RWMutex mutex, bool32 write_mode);
internal void    rw_mutex_drop(RWMutex mutex, bool32 write_mode);
#define rw_mutex_take_r(m) rw_mutex_take((m), (0))
#define rw_mutex_take_w(m) rw_mutex_take((m), (1))
#define rw_mutex_drop_r(m) rw_mutex_drop((m), (0))
#define rw_mutex_drop_w(m) rw_mutex_drop((m), (1))

//- rjf: condition variables
internal CondVar   cond_var_alloc(void);
internal void      cond_var_release(CondVar cv);
// returns false on timeout, true on signal, (max_wait_ms = max_u64) -> no timeout
internal bool32       cond_var_wait(CondVar cv, Mutex mutex, u64 endt_us);
internal bool32       cond_var_wait_rw(CondVar cv, RWMutex mutex_rw, bool32 write_mode, u64 endt_us);
#define cond_var_wait_rw_r(cv, m, endt) cond_var_wait_rw((cv), (m), (0), (endt))
#define cond_var_wait_rw_w(cv, m, endt) cond_var_wait_rw((cv), (m), (1), (endt))
internal void      cond_var_signal(CondVar cv);
internal void      cond_var_broadcast(CondVar cv);

//- rjf: cross-process semaphores
internal Semaphore semaphore_alloc(u32 initial_count, u32 max_count, String8 name);
internal void      semaphore_release(Semaphore semaphore);
internal Semaphore semaphore_open(String8 name);
internal void      semaphore_close(Semaphore semaphore);
internal bool32       semaphore_take(Semaphore semaphore, u64 endt_us);
internal void      semaphore_drop(Semaphore semaphore);
internal void      semaphore_drop_count(Semaphore semaphore, u64 drop_count);

//- rjf: barriers
internal Barrier barrier_alloc(u64 count);
internal void barrier_release(Barrier barrier);
internal void barrier_wait(Barrier barrier);

//- rjf: scope macros
#define MutexScope(mutex) DeferLoop(mutex_take(mutex), mutex_drop(mutex))
#define RWMutexScope(mutex, write_mode) DeferLoop(rw_mutex_take((mutex), (write_mode)), rw_mutex_drop((mutex), (write_mode)))
#define MutexScopeR(mutex) DeferLoop(rw_mutex_take_r(mutex), rw_mutex_drop_r(mutex))
#define MutexScopeW(mutex) DeferLoop(rw_mutex_take_w(mutex), rw_mutex_drop_w(mutex))
#define MutexScopeRWPromote(mutex) DeferLoop((rw_mutex_drop_r(mutex), rw_mutex_take_w(mutex)), (rw_mutex_drop_w(mutex), rw_mutex_take_r(mutex)))

////////////////////////////////
//~ rjf: Platform-Abstracted Synchronization Primitive Functions

//- rjf: slow barriers
internal Barrier   slow_barrier_alloc(u64 count);
internal void      slow_barrier_release(Barrier barrier);
internal void      slow_barrier_wait(Barrier barrier);

////////////////////////////////
//~ rjf: @per_os_impl Safe Calls

internal void safe_call(Thread_Entry_Point_Function_Type *func, Thread_Entry_Point_Function_Type *fail_handler, void *ptr);

#endif // BASE_THREADS_H
