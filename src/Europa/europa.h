#if defined __cplusplus
extern "C" {
#endif

#ifndef EUROPA_H
#define EUROPA_H
#include <stdint.h>
#ifndef uint
typedef unsigned int uint;
#endif
#ifndef boolean
typedef unsigned char boolean;
#endif
#ifndef uint64
typedef uint64_t uint64;
#endif
#ifndef int64
typedef int64_t int64;
#endif
#ifndef uint32
typedef unsigned int uint32;
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
typedef HANDLE EuropaThread;
#else
    #include <pthread.h>
    #include <unistd.h>
typedef pthread_t EuropaThread;
#endif

/* -------- Multi Threading -------- */

extern void     *europa_mutex_create(void);         /* Creates a mutex. Unlocked upon creation */
extern void     europa_mutex_lock(void *mutex);     /* Lock mutex. If mutex already locked, thread will wait until unlock so it can lock */
extern boolean  europa_mutex_lock_try(void *mutex);     /* Thread will attempt to lock mutex. if mutex already locked, returns FALSE and fails. If mutex unlocked, it locks and returns TRUE */
extern void     europa_mutex_unlock(void *mutex);     /* unlocks mutex */
extern void     europa_mutex_destroy(void *mutex);     /* destroys mutex */

extern void     *europa_signal_create(void);         /* Creates a signal blocker */
extern void     europa_signal_destroy(void *signal);     /* Destroys signal blocker */
extern boolean  europa_signal_wait(void *signal, void *mutex);     /* Sets thread to wait on blocker for another thread to activate it */
extern boolean  europa_signal_activate(void *signal);     /* Activates blocker so one or more threads waiting on the signal will be released */
extern boolean  europa_signal_activate_all(void *signal);     /* Activates blocker so that all threads waiting on a signal will be released */
extern EuropaThread europa_thread(void (*func)(void *data), void *data, char *name);     /* Launches thread executing func pointer with data as args. Once function returns, thread is deleted. */
extern void     europa_join(EuropaThread thread);

/* -------- Timing -------- */

extern void     europa_current_time_get(uint32 *seconds, uint32 *fractions);
extern double   europa_delta_time_compute(uint new_seconds, uint new_fractions, uint old_seconds, uint old_fractions);

extern int64    europa_current_system_time_get();
extern void     europa_current_date_local(int64 time, uint *seconds, uint *minutes, uint *hours, uint *week_days, uint *month_days, uint *month, uint *year);

extern void     europa_sleepi(uint seconds, uint nano_seconds);
extern void     europa_sleepd(double time);

/* -------- Execution -------- */

extern boolean  europa_execute(const char *command);     /* Execute command on platform */

/* -------- File Traversal -------- */

extern void     europa_pwd(char *output, uint32 output_size);
extern void     europa_goto_dir(char *dir, uint32 dir_len, char *pwd, uint32 pwd_len);

/* -------- Database -------- */

typedef void EDBHandle;

extern EDBHandle *europa_database_open(const char *filename);
extern void      europa_database_close(EDBHandle **database);

extern boolean europa_database_store(EDBHandle *database, const void *key, int key_length, const void *data, uint64 data_length);
extern boolean europa_database_fetch(EDBHandle *database, const void *key, int key_length, void *buffer, uint64 *buffer_size);
extern boolean europa_database_delete(EDBHandle *database, const void *key, int key_length);

#if defined __cplusplus
}
#endif

#endif /* EUROPA_H */
