#include <pthread.h>
#include <stdlib.h>

#ifdef C_MEMORY_DEBUG
#include <signal.h>
#include <stdio.h>
#include "Callisto/callisto.h"
#endif

#include "dnslookup.h"
#include "proxy.h"
#include "traceroute.h"

#define PROXY_PORT 8080
#define UNUSED(x) (void)(x)

double rtt_cutoff = 9999.0f;

double no_traceroute(const char *ip, char *out_buf, size_t out_size)
{
    UNUSED(ip);
    UNUSED(out_buf);
    UNUSED(out_size);
    return 0.0f;
}

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
    /*  TODO: Set "ping mode" where requests are limited to a given ms of latency, */
    /* else they get dropped. */
    /*  I suggest 80ms for limiting requests to NZ/AUS. All other connections seem */
    /*  to be of reliably higher latency */
    int proxy_port = PROXY_PORT;

    if (argc > 1)
        proxy_port = atoi(argv[1]);

    if (argc > 2)
        rtt_cutoff = atof(argv[2]);

#if defined(C_MEMORY_DEBUG)
    c_debug_memory_init(c_mem_mutex_lock, c_mem_mutex_unlock, c_mem_mutex_create());
    signal(SIGINT, signal_handler);
#endif

#if defined(NO_TRACERT)
    dns_init(no_traceroute);
    proxy_init(dns_resolve, no_traceroute);
#elif defined(PING)
    dns_init(ping);
    proxy_init(dns_resolve, ping);
#else
    dns_init(traceroute);
    proxy_init(dns_resolve, traceroute);
#endif

    proxy_start(proxy_port);
    return 0;
}
