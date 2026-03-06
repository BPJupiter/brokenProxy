#include <stdio.h>

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "context.h"

extern TracertResult traceroute(const char *ip);
extern PingResult ping(const char *ip);

extern DnsResult dns_resolve_recursive_local(const char *host);
extern DnsResult dns_resolve_iterative_root(const char *host);
/* -------- UNIMPLEMENTED ---------- */
extern DnsResult dns_resolve_iterative_local(const char *host);


typedef struct SharedContext
{
    /* variables */
    void *rttCutoff_lock;
    double rttCutoff;

    /* callbacks */
    void *dnsResolve_lock;
    DnsResult (*dnsResolve_cb)(const char *hostname);
    void *tracert_lock;
    TracertResult (*tracert_cb)(const char *ip);
    void *ping_lock;
    PingResult (*ping_cb)(const char *ip);
} SharedContext;

SharedContext gContext;

static void lock(void *lock);
static void unlock(void *lock);

void sharedContext_init(void)
{
    gContext.rttCutoff_lock = europa_mutex_create();
    gContext.dnsResolve_lock = europa_mutex_create();
    gContext.tracert_lock = europa_mutex_create();
    gContext.ping_lock = europa_mutex_create();

    gContext.rttCutoff = 1000.0;
    gContext.dnsResolve_cb = NULL;
    gContext.tracert_cb = NULL;
    gContext.ping_cb = NULL;
}

void sharedContext_destroy(void)
{
    europa_mutex_destroy(gContext.rttCutoff_lock);
    europa_mutex_destroy(gContext.dnsResolve_lock);
    europa_mutex_destroy(gContext.tracert_lock);
    europa_mutex_destroy(gContext.ping_lock);
    printf("Destroyed sharedcontext.\n");
}

int sharedContext_getVariable(SharedContextVariable var, void *value)
{
    int success = 0;
    SharedContext tContext = {0};
    switch (var)
    {
        case SCV_MAX_RTT:
            lock(gContext.rttCutoff_lock);
            tContext.rttCutoff = gContext.rttCutoff;
            unlock(gContext.rttCutoff_lock);

            *(double *) value = tContext.rttCutoff;
        break;
        case SCV_VAR_COUNT:
            *(int *)value = SCV_VAR_COUNT;
        break;
    }
    return success;
}

int sharedContext_setVariable(SharedContextVariable var, const void *value)
{
    int success = 0;
    SharedContext tContext = {0};
    switch (var)
    {
        case SCV_MAX_RTT:
            tContext.rttCutoff = *(double *) value;
            lock(gContext.rttCutoff_lock);
            gContext.rttCutoff = tContext.rttCutoff;
            unlock(gContext.rttCutoff_lock);
        break;
        case SCV_VAR_COUNT:
            success = 1;
        break;
    }
    return success;
}

boolean sharedContext_callback_toggle_dnsResolve(ResolveType type)
{
    boolean success = TRUE;
    SharedContext tContext = { 0 };
    switch (type)
    {
        case RT_NONE:
            tContext.dnsResolve_cb = NULL;
        break;
        case RT_QUICK:
            tContext.dnsResolve_cb = dns_resolve_recursive_local;
        break;
        case RT_FULL:
            tContext.dnsResolve_cb = dns_resolve_iterative_root;
        break;
        case RT_LOCAL: /* UNIMPLEMENTED */
            tContext.dnsResolve_cb = dns_resolve_iterative_local;
        break;
        case RT_COUNT:
            tContext.dnsResolve_cb = NULL;
            success = FALSE;
        break;
    }
    lock(gContext.dnsResolve_lock);
    gContext.dnsResolve_cb = tContext.dnsResolve_cb;
    unlock(gContext.dnsResolve_lock);

    return success;
}

boolean sharedContext_callback_toggle_traceroute(boolean enabled)
{
    boolean success = TRUE;
    SharedContext tContext = { 0 };

    tContext.tracert_cb = enabled ? traceroute : NULL;

    lock(gContext.tracert_lock);
    gContext.tracert_cb = tContext.tracert_cb;
    unlock(gContext.tracert_lock);

    return success;
}

boolean sharedContext_callback_toggle_ping(boolean enabled)
{
    boolean success = TRUE;
    SharedContext tContext = { 0 };

    tContext.ping_cb = enabled ? ping : NULL;

    lock(gContext.ping_lock);
    gContext.ping_cb = tContext.ping_cb;
    unlock(gContext.ping_lock);

    return success;
}

boolean sharedContext_callback_isEnabled_traceroute()
{
    SharedContext tContext = { 0 };

    lock(gContext.tracert_lock);
    tContext.tracert_cb = gContext.tracert_cb;
    unlock(gContext.tracert_lock);

    return NULL != tContext.tracert_cb;
}

boolean sharedContext_callback_isEnabled_ping()
{
    SharedContext tContext = { 0 };

    lock(gContext.ping_lock);
    tContext.ping_cb = gContext.ping_cb;
    unlock(gContext.ping_lock);

    return NULL != tContext.ping_cb;
}

boolean sharedContext_callback_execute_dnsResolve(DnsResult *dns_result, const char *hostname)
{
    SharedContext tContext = { 0 };
    DnsResult callback_result = { 0 };
    boolean success = TRUE;

    lock(gContext.dnsResolve_lock);
    tContext.dnsResolve_cb = gContext.dnsResolve_cb;
    unlock(gContext.dnsResolve_lock);

    if (tContext.dnsResolve_cb != NULL)
    {
        callback_result = tContext.dnsResolve_cb(hostname);
        if (dns_result != NULL)
            *dns_result = callback_result;
    }
    else
        success = FALSE;

    return success;
}

boolean sharedContext_callback_execute_traceroute(TracertResult *tracert_result, const char *ip)
{
    SharedContext tContext = { 0 };
    TracertResult callback_result = { 0 };
    boolean success = TRUE;

    lock(gContext.tracert_lock);
    tContext.tracert_cb = gContext.tracert_cb;
    unlock(gContext.tracert_lock);

    if (tContext.tracert_cb != NULL)
    {
        callback_result = tContext.tracert_cb(ip);
        if (tracert_result != NULL)
            *tracert_result = callback_result;
    }
    else
        success = FALSE;

    return success;
}

boolean sharedContext_callback_execute_ping(PingResult *ping_result, const char *ip)
{
    SharedContext tContext = { 0 };
    PingResult callback_result = { 0 };
    boolean success = TRUE;

    lock(gContext.ping_lock);
    tContext.ping_cb = gContext.ping_cb;
    unlock(gContext.ping_lock);

    if (tContext.ping_cb != NULL)
    {
        callback_result = tContext.ping_cb(ip);
        if (ping_result != NULL)
            *ping_result = callback_result;
    }
    else
        success = FALSE;

    return success;
}

static void lock(void *lock)
{
    europa_mutex_lock(lock);
}

static void unlock(void *lock)
{
    europa_mutex_unlock(lock);
}
