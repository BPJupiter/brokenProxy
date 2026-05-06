#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
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
    if (mutex == NULL) return NULL;
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
    if (p == NULL) return NULL;
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

EuropaThread europa_thread_create(void (*func)(void *data), void *data, char *name)
{
    EuropaThreadNameParam info;
    EuropaThreadParams *thread_param;
    HANDLE thread_h;
    DWORD dwThreadID;

    thread_param = malloc(sizeof *thread_param);
    if (thread_param == NULL) return NULL;
    thread_param->func = func;
    thread_param->data = data;

    thread_h = CreateThread(NULL, 0, e_win32_thread, thread_param, 0, &info.dwThreadID);

    return thread_h;
}

void europa_thread_join(EuropaThread thread)
{
    if (thread)
    {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

void europa_thread_detach(EuropaThread thread)
{
    if (thread) {
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

EuropaThread europa_thread_create(void (*func)(void *data), void *data, char *name)
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

void europa_thread_join(EuropaThread thread)
{
    pthread_join(thread, NULL);
}

void europa_thread_detach(EuropaThread thread)
{
    pthread_detach(thread);
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

int europa_execute(char *command)
{
    STARTUPINFO startup_info;
    PROCESS_INFORMATION process_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    ZeroMemory(&process_info, sizeof(process_info));

    if (CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startup_info, &process_info)) {
        int child_pid = (int)process_info.dwProcessId;

        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
        
        return child_pid;
    }

    return 0;
}

void europa_process_shutdown(int pid)
{
    if (pid > 0) {
        GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
    }
}

void europa_process_terminate(int pid)
{
    if (pid > 0) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProc) {
            TerminateProcess(hProc, 0);
            CloseHandle(hProc);
        }
    }
}

void europa_process_reload(int pid)
{
    if (pid > 0) {
        char event_name[128];
        snprintf(event_name, sizeof(event_name), "Global\\EuropaReload_%d", pid);

        HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, event_name);
        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
        else {
            printf("Could not find proxy reload event for PID %d\n", pid);
        }
    }
}

#else

int europa_execute(const char *command)
{
    int id;
    id = fork();
    if (id == 0) {
        execl(command, command, (char *)NULL);
        exit(1);
    }
    else if (id > 0) {
        return id;
    }
    return 0;
}

void europa_process_shutdown(int pid)
{
    if (pid > 0) kill(pid, SIGTERM);
}

void europa_process_terminate(int pid)
{
    if (pid > 0) kill(pid, SIGKILL);
}

void europa_process_reload(int pid)
{
    if (pid > 0) kill(pid, SIGUSR1);
}

#endif
