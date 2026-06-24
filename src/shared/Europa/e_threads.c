#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
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

void europa_thread(void (*func)(void *data), void *data, char *name)
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
    CloseHandle(thread_h);
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

void europa_thread(void (*func)(void *data), void *data, char *name)
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

    pthread_detach(thread_id);
}

#endif

/* Threadpool */

static volatile int threads_keepalive;

typedef struct BSem {
    void *mutex;
    void *cond;
    int v;
} BSem;

typedef struct Job {
    struct Job *prev;
    void (*func)(void *arg);
    void *arg;
} Job;

typedef struct JobQueue {
    void *rwmutex;
    Job *front;
    Job *rear;
    BSem *has_jobs;
    int len;
} JobQueue;

typedef struct Thread {
    int id;
    struct ThreadPool *thpool_p;
} Thread;

typedef struct ThreadPool {
    Thread **threads;
    volatile uint num_threads_alive;
    volatile uint num_threads_working;
    void *thcount_lock_mutex;
    void *threads_all_idle_cond;
    BSem *running;
    JobQueue jobqueue;
} ThreadPool;

static int thread_init(ThreadPool *tp, Thread **thread_p, int id);
static void thread_do(Thread *thread);
static void thread_destroy(Thread *thread);

static int jobqueue_init(JobQueue *jq);
static void jobqueue_clear(JobQueue *jq);
static void jobqueue_push(JobQueue *jq, Job *newjob);
static struct Job *jobqueue_pull(JobQueue *jq);
static void jobqueue_destroy(JobQueue *jq);

static void bsem_init(BSem *bsem, int value);
static void bsem_reset(BSem *bsem);
static void bsem_close(BSem *bsem);
static void bsem_post(BSem *bsem);
static void bsem_post_all(BSem *bsem);
static void bsem_wait(BSem *bsem);

ThreadPool *europa_threadpool_init(uint num_threads)
{
    ThreadPool *tp;
    uint n;

    threads_keepalive = 1;

    tp = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (tp == NULL) {
        fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
        return NULL;
    }
    tp->num_threads_alive = 0;
    tp->num_threads_working = 0;

    if (jobqueue_init(&tp->jobqueue) == -1) {
        fprintf(stderr, "thpool_init(): Could not allocate memory for job queue\n");
        free(tp);
        return NULL;
    }

    tp->threads = (struct Thread **)malloc(num_threads * sizeof(struct Thread *));
    if (tp->threads == NULL) {
        fprintf(stderr, "thpool_init(): Could not allocate memory for threads\n");
        jobqueue_destroy(&tp->jobqueue);
        free(tp);
        return NULL;
    } 

    tp->running = (BSem *)malloc(sizeof(BSem));
    if (tp->running == NULL) {
        fprintf(stderr, "europa_threadpool_init(): Could not allocate for running bsem\n");
        jobqueue_destroy(&tp->jobqueue);
        free(tp->threads);
        free(tp);
        return NULL;
    }
    bsem_init(tp->running, 1);

    tp->thcount_lock_mutex = europa_mutex_create();
    tp->threads_all_idle_cond = europa_signal_create();
    

    for (n = 0; n < num_threads; n++) {
        thread_init(tp, &tp->threads[n], n);
    }

    while (tp->num_threads_alive != num_threads)
        ;

    return tp;
}

boolean europa_threadpool_add_work(ThreadPool *tp, void (*func)(void *), void* args)
{
    Job *newjob;

    newjob = (struct Job *)malloc(sizeof(struct Job));
    if (newjob == NULL) {
        fprintf(stderr, "europa_threadpool_add_work(): Could not allocate memory for new job\n");
        return FALSE;
    }

    newjob->func = func;
    newjob->arg = args;

    jobqueue_push(&tp->jobqueue, newjob);

    return TRUE;
}

void europa_threadpool_wait(ThreadPool *tp)
{
    europa_mutex_lock(tp->thcount_lock_mutex);
    while (tp->jobqueue.len || tp->num_threads_working) {
        europa_signal_wait(tp->threads_all_idle_cond, tp->thcount_lock_mutex);
    }
    europa_mutex_unlock(tp->thcount_lock_mutex);
}

void europa_threadpool_destroy(ThreadPool *tp)
{
    volatile int threads_total = tp->num_threads_alive;
    double timeout = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    int n;

    if (tp == NULL) {
        return;
    } 

    threads_keepalive = 0;

    time(&start);
    while (tpassed < timeout && tp->num_threads_alive) {
        bsem_post_all(tp->jobqueue.has_jobs);
        time(&end);
        tpassed = difftime(end, start);
    }

    while (tp->num_threads_alive) {
        bsem_post_all(tp->jobqueue.has_jobs);
        europa_sleepi(0, 1000000);
    }

    jobqueue_destroy(&tp->jobqueue);

    bsem_reset(tp->running);
    free(tp->running);

    for (n = 0; n < threads_total; n++) {
        thread_destroy(tp->threads[n]);
    }
    free(tp->threads);
    free(tp);
}

void europa_threadpool_pause(ThreadPool *tp)
{
    bsem_close(tp->running);
}

void europa_threadpool_resume(ThreadPool *tp)
{
    bsem_post(tp->running);
}

uint europa_threadpool_num_working(ThreadPool *tp)
{
    return tp->num_threads_working;
}



static int thread_init(ThreadPool *tp, Thread **threads, int id)
{
    *threads = (Thread *)malloc(sizeof(Thread));
    if (*threads == NULL) {
        fprintf(stderr, "thead_init(): Could not allocate memory!\n");
        return -1;
    }

    (*threads)->thpool_p = tp;
    (*threads)->id = id;

    europa_thread((void ( *)(void *))thread_do, (*threads), NULL);
    return 0;
}

static void thread_do(Thread *thread)
{
    ThreadPool *tp = thread->thpool_p;
    Job *job = NULL;

    europa_mutex_lock(tp->thcount_lock_mutex);
    tp->num_threads_alive++;
    europa_mutex_unlock(tp->thcount_lock_mutex);

    while (threads_keepalive) {
        bsem_wait(tp->jobqueue.has_jobs);

        if (!threads_keepalive) {
            break;
        }

        bsem_wait(tp->running);
        bsem_post(tp->running);

        europa_mutex_lock(tp->thcount_lock_mutex);
        tp->num_threads_working++;
        europa_mutex_unlock(tp->thcount_lock_mutex);

        job = jobqueue_pull(&tp->jobqueue);
        if (job) {
            job->func(job->arg);
            free(job);
        }

        europa_mutex_lock(tp->thcount_lock_mutex);
        tp->num_threads_working--;
        if (!tp->num_threads_working) {
            europa_signal_activate(tp->threads_all_idle_cond);
        }
        europa_mutex_unlock(tp->thcount_lock_mutex);
    }

    europa_mutex_lock(tp->thcount_lock_mutex);
    tp->num_threads_alive--;
    europa_mutex_unlock(tp->thcount_lock_mutex);
}

static void thread_destroy(Thread *thread)
{
    free(thread);
}



static int jobqueue_init(JobQueue *jq)
{
    jq->len = 0;
    jq->front = NULL;
    jq->rear = NULL;

    jq->has_jobs = (BSem *)malloc(sizeof(BSem));
    if (jq->has_jobs == NULL) {
        return -1;
    }

    jq->rwmutex = europa_mutex_create();
    bsem_init(jq->has_jobs, 0);

    return 0;
}

static void jobqueue_clear(JobQueue *jq)
{
    while (jq->len) {
        free(jobqueue_pull(jq));
    }

    jq->front = NULL;
    jq->rear = NULL;
    bsem_reset(jq->has_jobs);
    jq->len = 0;
}

static void jobqueue_push(JobQueue *jq, Job *newjob)
{
    europa_mutex_lock(jq->rwmutex);
    newjob->prev = NULL;

    switch (jq->len) {
        case 0:
            jq->front = newjob;
            jq->rear = newjob;
            break;
        default:
            jq->rear->prev = newjob;
            jq->rear = newjob;
            break;
    }
    jq->len++;

    bsem_post(jq->has_jobs);
    europa_mutex_unlock(jq->rwmutex);
}

static Job *jobqueue_pull(JobQueue *jq)
{
    Job *job = NULL;
    europa_mutex_lock(jq->rwmutex);
    job = jq->front;

    switch (jq->len) {
        case 0:
            break;
        case 1:
            jq->front = NULL;
            jq->rear = NULL;
            jq->len = 0;
            break;
        default:
            jq->front = job->prev;
            jq->len--;
            bsem_post(jq->has_jobs);
            break;
    }

    europa_mutex_unlock(jq->rwmutex);
    return job;
}

static void jobqueue_destroy(JobQueue *jq)
{
    jobqueue_clear(jq);
    free(jq->has_jobs);
}



static void bsem_init(BSem *bsem, int value)
{
    if (value < 0 || value > 1) {
        value = 0;
    }

    bsem->mutex = europa_mutex_create();
    bsem->cond = europa_signal_create();
    bsem->v = value;
}

static void bsem_reset(BSem *bsem)
{
    europa_mutex_destroy(bsem->mutex);
    europa_signal_destroy(bsem->cond);
    bsem_init(bsem, 0);
}

static void bsem_close(BSem *bsem)
{
    europa_mutex_lock(bsem->mutex);
    bsem->v = 0;
    europa_mutex_unlock(bsem->mutex);
}

static void bsem_post(BSem *bsem)
{
    europa_mutex_lock(bsem->mutex);
    bsem->v = 1;
    europa_signal_activate(bsem->cond);
    europa_mutex_unlock(bsem->mutex);
}

static void bsem_post_all(BSem *bsem)
{
    europa_mutex_lock(bsem->mutex);
    bsem->v = 1;
    europa_signal_activate_all(bsem->cond);
    europa_mutex_unlock(bsem->mutex);
}

static void bsem_wait(BSem *bsem)
{
    europa_mutex_lock(bsem->mutex);
    while (bsem->v != 1) {
        europa_signal_wait(bsem->cond, bsem->mutex);
    }
    bsem->v = 0;
    europa_mutex_unlock(bsem->mutex);
}

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
