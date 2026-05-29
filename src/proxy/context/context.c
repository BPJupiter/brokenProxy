#include <stdio.h>

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "Styx/styx.h"
#include "context.h"

extern TracertResult traceroute(const char *ip);
extern PingResult ping(const char *ip);

extern boolean styx_network_address_lookup_default_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6);
extern boolean dns_resolve_iterative_root_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6);
/* -------- UNIMPLEMENTED ---------- */
extern boolean dns_resolve_iterative_local_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6);


typedef struct SharedContext
{
    /* variables */
    double maxRtt;

    /* callbacks */
    TracertResult (*tracert_cb)(const char *ip);
    PingResult (*ping_cb)(const char *ip);
} SharedContext;

static SharedContext gContexts[2];
static EAtomicInteger gActiveIndex;

void sharedContext_init(void)
{
    gContexts[0].maxRtt = 1000.0;
    gContexts[0].tracert_cb = NULL;
    gContexts[0].ping_cb = NULL;

    gContexts[1] = gContexts[0];

    europa_atomic_integer_init(gActiveIndex, 0);
}

void sharedContext_destroy(void)
{
    europa_atomic_integer_free(gActiveIndex);
    printf("Destroyed proxy context.\n");
}

boolean sharedContext_var_get_maxRtt(double *value)
{
    boolean success = FALSE;
    EAtomicInteger active = europa_atomic_integer_read(gActiveIndex);

    *value = gContexts[active].maxRtt;
    success = TRUE;

    return success;
}

boolean sharedContext_var_set_maxRtt(const double *value)
{
    boolean success = FALSE;
    EAtomicInteger active, inactive;

    active = europa_atomic_integer_read(gActiveIndex);
    inactive = (active == 0) ? 1 : 0;

    gContexts[inactive] = gContexts[active];

    gContexts[inactive].maxRtt = *value;
    success = TRUE;

    if (success) {
        europa_atomic_integer_write(gActiveIndex, inactive);
    }

    return success;
}

boolean sharedContext_callback_set_dnsResolve(ResolveType type)
{
    /*
    boolean success = TRUE;
    EAtomicInteger active, inactive;

    active = europa_atomic_integer_read(gActiveIndex);
    inactive = (active == 0) ? 1 : 0;

    gContexts[inactive] = gContexts[active];

    switch (type)
    {
        case RT_NONE:
            gContexts[inactive].dnsResolve_cb = NULL;
        break;
        case RT_QUICK:
            gContexts[inactive].dnsResolve_cb = dns_resolve_recursive_local;
        break;
        case RT_FULL:
            gContexts[inactive].dnsResolve_cb = dns_resolve_iterative_root;
        break;
        case RT_LOCAL:
            gContexts[inactive].dnsResolve_cb = dns_resolve_iterative_local;
        break;
        case RT_COUNT:
            gContexts[inactive].dnsResolve_cb = NULL;
            success = FALSE;
        break;
    }

    europa_atomic_integer_write(gActiveIndex, inactive);
    */
    boolean success = TRUE;

    switch (type)
    {
        case RT_NONE:
            styx_network_address_lookup = NULL;
        break;
        case RT_QUICK:
            styx_network_address_lookup = styx_network_address_lookup_default_func;
        break;
        case RT_FULL:
            styx_network_address_lookup = dns_resolve_iterative_root_func;
        break;
        case RT_LOCAL:
            styx_network_address_lookup = dns_resolve_iterative_local_func;
        break;
        case RT_COUNT:
            styx_network_address_lookup = NULL;
            success = FALSE;
        break;
    }

    return success;
}

boolean sharedContext_callback_set_traceroute(boolean enabled)
{
    boolean success = TRUE;
    EAtomicInteger active, inactive;

    active = europa_atomic_integer_read(gActiveIndex);
    inactive = (active == 0) ? 1 : 0;

    gContexts[inactive] = gContexts[active];
    gContexts[inactive].tracert_cb = enabled ? traceroute : NULL;

    europa_atomic_integer_write(gActiveIndex, inactive);

    return success;
}

boolean sharedContext_callback_set_ping(boolean enabled)
{
    boolean success = TRUE;
    EAtomicInteger active, inactive;

    active = europa_atomic_integer_read(gActiveIndex);
    inactive = (active == 0) ? 1 : 0;

    gContexts[inactive] = gContexts[active];
    gContexts[inactive].ping_cb = enabled ? ping : NULL;

    europa_atomic_integer_write(gActiveIndex, inactive);

    return success;
}

boolean sharedContext_callback_isEnabled_traceroute(void)
{
    EAtomicInteger active = europa_atomic_integer_read(gActiveIndex);
    return (NULL != gContexts[active].tracert_cb);
}

boolean sharedContext_callback_isEnabled_ping(void)
{
    EAtomicInteger active = europa_atomic_integer_read(gActiveIndex);
    return (NULL != gContexts[active].ping_cb);
}

boolean sharedContext_callback_execute_traceroute(TracertResult *tracert_result, const char *ip)
{
    TracertResult (*local_cb)(const char *) = NULL;
    TracertResult callback_result = { 0 };
    boolean success = TRUE;
    EAtomicInteger active;

    active = europa_atomic_integer_read(gActiveIndex);

    local_cb = gContexts[active].tracert_cb;

    if (local_cb != NULL) {
        callback_result = local_cb(ip);
        if (tracert_result != NULL) {
            *tracert_result = callback_result;
        }
    }
    else {
        success = FALSE;
    }

    return success;
}

boolean sharedContext_callback_execute_ping(PingResult *ping_result, const char *ip)
{
    PingResult (*local_cb)(const char *) = NULL;
    PingResult callback_result = { 0 };
    boolean success = TRUE;
    EAtomicInteger active;

    active = europa_atomic_integer_read(gActiveIndex);

    local_cb = gContexts[active].ping_cb;

    if (local_cb != NULL) {
        callback_result = local_cb(ip);
        if (ping_result != NULL) {
            *ping_result = callback_result;
        }
    }
    else {
        success = FALSE;
    }

    return success;
}
