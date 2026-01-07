#include <pthread.h>
#include <stdlib.h>

#ifdef C_MEMORY_DEBUG
#include <signal.h>
#include <stdio.h>
#include "Callisto/callisto.h"
#endif

#include "shared_context.h"
#include "proxy.h"

#define PROXY_PORT 13406
#define UNUSED(x) (void)(x)

#ifdef C_MEMORY_DEBUG
void c_mem_mutex_lock(void *mutex)
{
    pthread_mutex_lock((pthread_mutex_t *)mutex);
}

void c_mem_mutex_unlock(void *mutex)
{
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

void *c_mem_mutex_create()
{
    pthread_mutex_t *mutex;
    mutex = malloc(sizeof *mutex);
    pthread_mutex_init(mutex, NULL);
    return mutex;
}

void signal_handler(int sig)
{
    printf("\nCaught signal %d\n", sig);
    c_debug_mem_print(1);
    exit(sig);
}
#endif

int main(int argc, char *argv[])
{
    int proxy_port = PROXY_PORT;

    if (argc > 1)
        sharedContext_set_rtt_cutoff(atof(argv[1]));

#if defined(C_MEMORY_DEBUG)
    c_debug_memory_init(c_mem_mutex_lock, c_mem_mutex_unlock, c_mem_mutex_create());
    signal(SIGINT, signal_handler);
#endif
    sharedContext_init();

    proxy_start(proxy_port);
    sharedContext_destroy();
    return 0;
}
