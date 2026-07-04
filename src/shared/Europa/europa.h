#ifndef EUROPA_H
#define EUROPA_H
#include <stdlib.h>
#include <stdio.h>
#include "Clay/clay.h"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* -------- Multi Threading -------- */

extern void     *europa_mutex_create(void);         /* Creates a mutex. Unlocked upon creation */
extern void     europa_mutex_lock(void *mutex);     /* Lock mutex. If mutex already locked, thread will wait until unlock so it can lock */
extern boolean  europa_mutex_lock_try(void *mutex); /* Thread will attempt to lock mutex. if mutex already locked, returns FALSE and fails. If mutex unlocked, it locks and returns TRUE */
extern void     europa_mutex_unlock(void *mutex);   /* unlocks mutex */
extern void     europa_mutex_destroy(void *mutex);  /* destroys mutex */

extern void     *europa_signal_create(void);             /* Creates a signal blocker */
extern void     europa_signal_destroy(void *signal);     /* Destroys signal blocker */
extern boolean  europa_signal_wait(void *signal, void *mutex);     /* Sets thread to wait on blocker for another thread to activate it */
extern boolean  europa_signal_activate(void *signal);     /* Activates blocker so one or more threads waiting on the signal will be released */
extern boolean  europa_signal_activate_all(void *signal);     /* Activates blocker so that all threads waiting on a signal will be released */
extern void     europa_thread(void (*func)(void *data), void *data, char *name); /* Launches a thread that will execute the funtion pointer. The void pointer will be given as a parameter. The thread is detached upon creation and is deleted once it returns. */

typedef struct ThreadPool *EThreadPool;
extern EThreadPool europa_threadpool_init(uint num_threads); /* Initialize threadpool. Function will not return until all threads have initialized successfully. */
extern boolean     europa_threadpool_add_work(EThreadPool tp, void (*func)(void *), void *args); /* Add work to the job queue. If you want to add to work a function with more than one arguments then you can implement this with a pointer to a structure */
extern void        europa_threadpool_wait(EThreadPool tp); /* Wait for all queued jobs to finish, both queued and currently running. Once the queue is empty and all work is completed, the calling thread will continue. */
extern void        europa_threadpool_pause(EThreadPool tp); /* All threads paused whether idle or waiting. While the pool is paused, new work can be queued */
extern void        europa_threadpool_resume(EThreadPool tp); /* Unpauses all threads if they are paused; does nothing if they are not paused. */
extern void        europa_threadpool_destroy(EThreadPool tp); /* Waits for currently active threads to finish and then 'kill' the whole thread pool to free up memory. */
extern uint        europa_threadpool_num_working(EThreadPool tp); /* Show the number of currently working threads. */

/* -------- Atomics -------- */

typedef void * volatile EAtomicPointer; /* An atomic pointer */
typedef uint64 volatile EAtomicInteger; /* an atomic 64 bit integer */
typedef uint64 volatile EAtomicUsers; /* An atomic lock that lets multiple users read but only one write */
#include "e_atomics_internal.h"


/* -------- Timing -------- */

extern void     europa_current_time_get(uint32 *seconds, uint32 *fractions);
extern double   europa_delta_time_compute(uint new_seconds, uint new_fractions, uint old_seconds, uint old_fractions);

extern int64    europa_current_system_time_get(void);
extern void     europa_current_date_local(int64 time, uint *seconds, uint *minutes, uint *hours, uint *week_days, uint *month_days, uint *month, uint *year);

extern void     europa_sleepi(uint seconds, uint nano_seconds);
extern void     europa_sleepd(double time);

/* -------- Execution -------- */

extern int  europa_execute(char *command);     /* Execute command on platform */
extern void europa_process_shutdown(int pid); /* graceful */
extern void europa_process_terminate(int pid); /* kills immediately */
extern void europa_process_reload(int pid);

/* -------- Settings Storage --------
   XML settings system. An application can both set and get a settings value. If an application is using any of the "get" functions requesting a non-existent setting, then the setting will be automatically created using the given default value. */

extern boolean europa_setting_boolean_get(const char *setting, boolean default_value, const char *comment); /* Get boolean setting. The function will return default_value if the setting is not available, and create the setting in the setting list. */
extern void    europa_setting_boolean_set(const char *setting, boolean value, const char *comment);
extern int     europa_setting_integer_get(const char *setting, int default_value, const char *comment);
extern void    europa_setting_integer_set(const char *setting, int value, const char *comment);
extern double  europa_setting_double_get(const char *setting, double default_value, const char *comment);
extern void    europa_setting_double_set(const char *setting, double value, const char *comment);
extern char   *europa_setting_text_get(const char *setting, char *default_text, const char *comment);
extern void    europa_setting_text_set(const char *setting, char *text, const char *comment);
extern void    europa_settings_save(const char *file_name);
extern boolean europa_settings_load(const char *file_name);
extern boolean europa_setting_probe(const char *setting);

/* -------- File Traversal -------- */

#ifdef __WIN32
    #define EUROPA_DIR_ROOT_PATH "/"
    #define EUROPA_LIBRARY_EXTENSION "dll"
    #define EUROPA_DIR_SLASH '\\'
#else
    #define EUROPA_DIR_ROOT_PATH "/"
    #define EUROPA_LIBRARY_EXTENSION "lib"
    #define EUROPA_DIR_SLASH '/'
#endif
#define EUROPA_DIR_HOME_PATH "."

extern void     europa_get_pwd(char *output, uint32 output_size);
extern void     europa_set_pwd(char *new_dir);
extern int      europa_pwd_to_root(char *root_folder_name);
extern boolean  europa_path_search(char *file, boolean partial, char *path, boolean folders, uint number, char *out_buffer, uint out_buffer_size); /* Searches for a file recursively in a path, and writes its location to the out_buffer. If "partial" is set the search will also yeild results where the search string only makes up part of the file name.*/

typedef void EDir;

extern EDir *europa_path_dir_open(char *path);         /* Opens a path for traversal. If the path is not legal or not a directory the function will return NULL. */
extern boolean  europa_path_dir_next(EDir *d, char *file_name_buffer, uint buffer_size); /* Writes the name of the next member of the directory to file_name_buffer. Returns FALSE if there are no files left in the directory to write out. */
extern void     europa_path_dir_close(EDir *d); /* Closes a directory */

extern boolean  europa_path_is_dir(char *path); /* Returns True if the path is a valid directory. */

extern boolean  europa_path_file_stats(char *path, size_t *size, uint64 *create_time, uint64 *modify_time); /* Outputs stats about a file */
extern boolean  europa_path_volume_stats(char *path, size_t *block_size, size_t *free_size, size_t *used_size, size_t *total_size); /* Outputs information about a volume, its block size, and how many blocks are used and free. */

extern int      europa_path_rename(char *old_name, char *new_name); /* Rename a path */
extern int      europa_path_remove(char *path); /* Remove a file */
extern int      europa_path_make_dir(char *path); /* Creates a directory */
extern FILE *europa_path_open(char *path, char *mode);     /* Same as fopen but UTF-8 */
extern uint8 *europa_path_load(char *path, size_t *size);    /* Loads a file into a buffer. Writes the size of the buffer to "size" (optional). */

extern FILE *europa_project_root_fopen(const char *root_folder_name, const char *filename, char *perms);

#ifdef __cplusplus
}
#endif

#endif /* EUROPA_H */
