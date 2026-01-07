#include "shared_context.h"
#include <stdio.h>

typedef struct SharedContext
{
    pthread_mutex_t lock;
    double rtt_cutoff;
    short (*resolve_cb)(char *hostname, unsigned char ***answer_index);
    double (*tracert_cb)(const char *ip, char *out_buf, size_t out_size);
    double (*ping_cb)(const char *ip);
} SharedContext;

SharedContext gContext;

static void lock(void);
static void unlock(void);

void sharedContext_init(void)
{
    pthread_mutex_init(&gContext.lock, NULL);

    gContext.rtt_cutoff = 1000.0;
    gContext.resolve_cb = NULL;
    gContext.tracert_cb = NULL;
    gContext.ping_cb = NULL;
}

void sharedContext_destroy(void)
{
    pthread_mutex_destroy(&gContext.lock);
}

void sharedContext_set_rtt_cutoff(double val)
{
    lock();
    gContext.rtt_cutoff = val;
    unlock();
}

double sharedContext_get_rtt_cutoff(void)
{
    double result;
    lock();
    result = gContext.rtt_cutoff;
    unlock();
    return result;
}

void sharedContext_set_resolve_cb(short (*cb)(char *, unsigned char ***))
{
    lock();
    gContext.resolve_cb = cb;
    unlock();
}

short sharedContext_exec_resolve_cb(char *hostname, unsigned char ***answer_index)
{
    short (*loc_cb)(char *, unsigned char ***);

    lock();
    loc_cb = gContext.resolve_cb;
    unlock();

    if (loc_cb != NULL)
    {
        return loc_cb(hostname, answer_index);
    }
    return -1;
}

void sharedContext_set_tracert_cb(double (*cb)(const char *, char *, size_t))
{
    lock();
    gContext.tracert_cb = cb;
    unlock();
}

double sharedContext_exec_tracert_cb(const char *ip, char *out_buf, size_t out_size)
{
    double (*loc_cb)(const char *, char *, size_t);

    lock();
    loc_cb = gContext.tracert_cb;
    unlock();

    if (loc_cb != NULL)
    {
        return loc_cb(ip, out_buf, out_size);
    }
    return -1.0;
}

void sharedContext_set_ping_cb(double (*cb)(const char *))
{
    lock();
    gContext.ping_cb = cb;
    unlock();
}

double sharedContext_exec_ping_cb(const char *ip)
{
    double (*loc_cb)(const char *);

    lock();
    loc_cb = gContext.ping_cb;
    unlock();

    if (loc_cb != NULL)
    {
        return loc_cb(ip);
    }

    return -1.0;
}

static void lock(void)
{
    pthread_mutex_lock(&gContext.lock);
}

static void unlock(void)
{
    pthread_mutex_unlock(&gContext.lock);
}
