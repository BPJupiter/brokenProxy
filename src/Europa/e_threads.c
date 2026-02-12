#include <stdlib.h>
#include <stdio.h>
#include "Europa/europa.h"

#ifdef _WIN32
#include <windows.h>
#include <winnt.h>
#include <winreg.h>
#include <mmsystem.h>
#include <process.h>
#include "Clay/clay.h"

void *europa_mutex_create(void)
{
    CRITICAL_SECTION *mutex;
    mutex = malloc(sizeof *mutex);
    InitializeCriticalSection(mutex);
    return mutex;
}

void europa_mutex_destroy(void *mutex)
{
    DeleteCriticalSection(mutex);
    free(mutex);
}

void europa_mutex_lock(void *mutex)
{
    EnterCriticalSection(mutex);
}

boolean europa_mutex_lock_try(void *mutex)
{
    return TryEnterCriticalSection(mutex);
}

void europa_mutex_unlock(void *mutex)
{
    LeaveCriticalSection(mutex);
}
#else
#include <pthread.h>
#include <unistd.h>
#include "Clay/clay.h"

void *europa_mutex_create(void)
{
    pthread_mutex_t *mutex;
    mutex = malloc(sizeof *mutex);
    pthread_mutex_init(mutex, NULL);
    return mutex;
}

void europa_mutex_destroy(void *mutex)
{
    pthread_mutex_destroy(mutex);
    free(mutex);
}

void europa_mutex_lock(void *mutex)
{
    pthread_mutex_lock(mutex);
}

boolean europa_mutex_lock_try(void *mutex)
{
    return pthread_mutex_trylock(mutex);
}

void europa_mutex_unlock(void *mutex)
{
    pthread_mutex_unlock(mutex);
}

#endif

/* Signals */
#ifdef _WIN32

void *europa_signal_create(void)
{
    CONDITION_VARIABLE *p;
    p = malloc(sizeof *p);
    InitializeConditionVariable(p);
    return p;
}

void europa_signal_destroy(void *signal)
{
    free(signal);
}

boolean europa_signal_wait(void *signal, void *mutex)
{
    SleepConditionVariableCS(signal, mutex, INFINITE);
    return TRUE;
}

boolean europa_signal_activate(void *signal)
{
    WakeConditionVariable(signal);
    return TRUE;
}

boolean europa_signal_activate_all(void *signal)
{
    WakeAllConditionVariable(signal);
    return TRUE;
}

#else

void *europa_signal_create(void)
{
    pthread_cond_t *p;
    p = malloc(sizeof *p);
    pthread_cond_init(p, NULL);
    return p;
}

void europa_signal_destroy(void *signal)
{
    pthread_cond_destroy(signal);
    free(signal);
}

boolean europa_signal_wait(void *signal, void *mutex)
{
    pthread_cond_wait(signal, mutex);
    return TRUE;
}

boolean europa_signal_activate(void *signal)
{
    pthread_cond_signal(signal);
    return TRUE;
}

boolean europa_signal_activate_all(void *signal)
{
    pthread_cond_broadcast(signal);
    return TRUE;
}

#endif

/* Threads */

typedef struct
{
    void (*func)(void *data);
    void *data;
}EuropaThreadParams;

#ifdef _WIN32

DWORD WINAPI e_win32_thread(LPVOID param)
{
    EuropaThreadParams *thread_param;
    thread_param = (EuropaThreadParams *)param;
    thread_param->func(thread_param->data);
    free(thread_param);
    return 0;
}

#pragma pack(push, 8)
typedef struct
{
    DWORD dwType; /* Must be 0x1000 */
    LPCSTR szName; /* Pointer to name (in usr addr space) */
    DWORD dwThreadID; /* Thread ID (-1=caller thread)*/
    DWORD dwFlags; /* Reserved, must be 0 */
}EuropaThreadNameParam;
#pragma pack(pop)

EuropaThread europa_thread(void (*func)(void *data), void *data, char *name)
{
    EuropaThreadNameParam info;
    EuropaThreadParams *thread_param;
    HANDLE thread_h;
    DWORD dwThreadID;

    thread_param = malloc(sizeof *thread_param);
    thread_param->func = func;
    thread_param->data = data;

    thread_h = CreateThread(NULL, 0, e_win32_thread, thread_param, 0, &info.dwThreadID);

    return thread_h;
}

void europa_join(EuropaThread thread)
{
    if (thread)
    {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

#else

void *e_thread_thread(void *param)
{
    EuropaThreadParams *thread_param;
    thread_param = (EuropaThreadParams *)param;
    thread_param->func(thread_param->data);
    free(thread_param);
    return NULL;
}

EuropaThread europa_thread(void (*func)(void *data), void *data, char *name)
{
    pthread_t thread_id;
    EuropaThreadParams *thread_param;

    UNUSED(name);

    thread_param = malloc(sizeof *thread_param);
    thread_param->func = func;
    thread_param->data = data;

    pthread_create(&thread_id, NULL, e_thread_thread, thread_param);
    /*
       if (name != NULL)
        pthread_setname_np(thread_id, name);
     */

    return thread_id;
}

void europa_join(EuropaThread thread)
{
    pthread_join(thread, NULL);
}

#endif

/* Execute */

#ifdef _WIN32

void europa_sleepi(uint seconds, uint nano_seconds)
{
    Sleep(nano_seconds / 1000000 + seconds * 1000);
}

#else

void europa_sleepi(uint seconds, uint nano_seconds)
{
    struct timespec times;
    times.tv_sec = seconds;
    times.tv_nsec = nano_seconds;
    nanosleep(&times, NULL);
}

#endif

void europa_sleepd(double time)
{
    europa_sleepi((uint)time, (time - (double)((uint)time)) * 1000000000);
}

#ifdef _WIN32

boolean europa_execute(char *command)
{
    STARTUPINFO startup_info;
    PROCESS_INFORMATION process_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    ZeroMemory(&process_info, sizeof(process_info));
    return CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info);
}

#else

boolean europa_execute(const char *command)
{
    uint id;
    id = fork();
    if (id == 0)
        return execl(command, NULL);
    return FALSE;
}

#endif
