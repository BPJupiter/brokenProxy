#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "Europa/europa.h"

typedef enum
{
    RT_QUICK = 0,
    RT_FULL = 1,
    RT_LOCAL, /* UNIMPLEMENTED */
    RT_NONE,
    RT_COUNT
} ResolveType;

/* -------- Callback return types -------- */

typedef enum DnsStatus
{
    DNS_OK,
    DNS_ERR_GENERIC,
    DNS_ERR_RTT_EXHAUSTED,
    DNS_ERR_TIMEOUT,
    DNS_ERR_RECURSION_LIMIT,
    DNS_ERR_FAIL_SYSCALL
} DnsStatus;

typedef struct TracertResult
{
    uint hopCount;
    uint *hopRtt;
    char **hopAddress;
} TracertResult;

typedef struct PingResult
{
    double rtt;
} PingResult;

extern void TracertResult_free(TracertResult *tracert_result);
/*extern void PingResult_free(PingResult *ping_result); */

extern void sharedContext_init(void); /* Creates a shared context */
extern void sharedContext_destroy(void); /* Destroys a shared context */

extern boolean sharedContext_var_get_maxRtt(double *value);
extern boolean sharedContext_var_set_maxRtt(const double *value);

extern boolean sharedContext_callback_set_dnsResolve(ResolveType type);
extern boolean sharedContext_callback_set_traceroute(boolean enabled);
extern boolean sharedContext_callback_set_ping(boolean enabled);

extern boolean sharedContext_callback_isEnabled_traceroute(void);
extern boolean sharedContext_callback_isEnabled_ping(void);

extern boolean sharedContext_callback_execute_traceroute(TracertResult *tracert_result, const char *ip);
extern boolean sharedContext_callback_execute_ping(PingResult *ping_result, const char *ip);

#endif
