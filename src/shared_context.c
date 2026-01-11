#include <stdio.h>

#include "shared_context.h"
#include "traceroute.h"
#include "dnslookup.h"

typedef struct SharedContext
{
    pthread_mutex_t lock_rtt;
    double rtt_cutoff;
    pthread_mutex_t lock_resolve;
    short (*resolve_cb)(const char *hostname, unsigned char ***answer_index);
    pthread_mutex_t lock_tracert;
    double (*tracert_cb)(const char *ip, char *out_buf, size_t out_size);
    pthread_mutex_t lock_ping;
    double (*ping_cb)(const char *ip);
} SharedContext;

SharedContext gContext;

static void lock(pthread_mutex_t *lock);
static void unlock(pthread_mutex_t *lock);

void sharedContext_init(void)
{
    pthread_mutex_init(&gContext.lock_rtt, NULL);
    pthread_mutex_init(&gContext.lock_resolve, NULL);
    pthread_mutex_init(&gContext.lock_tracert, NULL);
    pthread_mutex_init(&gContext.lock_ping, NULL);

    gContext.rtt_cutoff = 1000.0;
    gContext.resolve_cb = NULL;
    gContext.tracert_cb = NULL;
    gContext.ping_cb = NULL;
}

void sharedContext_destroy(void)
{
    pthread_mutex_destroy(&gContext.lock_rtt);
    pthread_mutex_destroy(&gContext.lock_resolve);
    pthread_mutex_destroy(&gContext.lock_tracert);
    pthread_mutex_destroy(&gContext.lock_ping);
}

int sharedContext_getVariable(SCVariable var, void *value)
{
    int success = 0;
    SharedContext tContext = {0};
    switch (var)
    {
        case SCV_MAX_RTT:
            lock(&gContext.lock_rtt);
            tContext.rtt_cutoff = gContext.rtt_cutoff;
            unlock(&gContext.lock_rtt);

            *(typeof(tContext.rtt_cutoff) *) value = tContext.rtt_cutoff;
        break;
        case SCV_VAR_COUNT:
            *(int *)value = SCV_VAR_COUNT;
        break;
    }
    return success;
}

int sharedContext_setVariable(SCVariable var, const void *value)
{
    int success = 0;
    SharedContext tContext = {0};
    switch (var)
    {
        case SCV_MAX_RTT:
            tContext.rtt_cutoff = *(typeof(tContext.rtt_cutoff) *) value;
            lock(&gContext.lock_rtt);
            gContext.rtt_cutoff = tContext.rtt_cutoff;
            unlock(&gContext.lock_rtt);
        break;
        case SCV_VAR_COUNT:
            success = 1;
        break;
    }
    return success;
}

int sharedContext_toggleCb(SCCallback cb, char enabled)
{
    int success = 0;
    SharedContext tContext = {0};
    switch (cb)
    {
        case SCC_RESOLVE:
            tContext.resolve_cb = enabled ? dns_resolve : quick_resolve;
            lock(&gContext.lock_resolve);
            gContext.resolve_cb = tContext.resolve_cb;
            unlock(&gContext.lock_resolve);
        break;
        case SCC_TRACERT:
            tContext.tracert_cb = enabled ? traceroute : NULL;
            lock(&gContext.lock_tracert);
            gContext.tracert_cb = tContext.tracert_cb;
            unlock(&gContext.lock_tracert);
        break;
        case SCC_PING:
            tContext.ping_cb = enabled ? ping : NULL;
            lock(&gContext.lock_ping);
            gContext.ping_cb = tContext.ping_cb;
            unlock(&gContext.lock_ping);
        break;
        case SCC_COUNT:
            success = 1;
        break;
    }
    return success;
}

int sharedContext_execCb(SCCallback cb, retType *result, argType *args)
{
    argType a = {0};
    retType r = {0};

    SharedContext tContext = {0};
    int success = 0;

    switch (cb)
    {
        case SCC_RESOLVE:
            lock(&gContext.lock_resolve);
            tContext.resolve_cb = gContext.resolve_cb;
            unlock(&gContext.lock_resolve);

            a = *args;
            if (tContext.resolve_cb != NULL)
            {
                r.resolve = tContext.resolve_cb(a.resolve.hostname, a.resolve.answer_index);
                if (result != NULL)
                    result->resolve = r.resolve;
            }
            else success = 1;
        break;
        case SCC_TRACERT:
            lock(&gContext.lock_tracert);
            tContext.tracert_cb = gContext.tracert_cb;
            unlock(&gContext.lock_tracert);

            a = *args;
            if (tContext.tracert_cb != NULL)
            {
                r.tracert = tContext.tracert_cb(a.tracert.ip, a.tracert.out_buf, a.tracert.out_size);
                if (result != NULL)
                    result->tracert = r.tracert;
            }
            else success = 1;
        break;
        case SCC_PING:
            lock(&gContext.lock_ping);
            tContext.ping_cb = gContext.ping_cb;
            unlock(&gContext.lock_ping);

            a = *args;
            if (tContext.ping_cb != NULL)
            {
                r.ping = tContext.ping_cb(a.ping.ip);
                if (result != NULL)
                    result->ping = r.ping;
            }
            else success = 1;
        break;
        case SCC_COUNT:
            success = 1;
        break;
    }
    return success;
}

static void lock(pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);
}

static void unlock(pthread_mutex_t *lock)
{
    pthread_mutex_unlock(lock);
}
