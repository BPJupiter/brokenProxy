#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "Europa/europa.h"

typedef enum
{
    SCV_MAX_RTT, /**< (double) Maximum latency allowed while latency checking is enabled. */
    SCV_VAR_COUNT /**< Shared context variable count. */
} SCVariable;

typedef enum
{
    SCC_RESOLVE, /**< Resolves hostname to IPv4 address. Ret: (short) answer count. Params: hostname (const char *) hostname to be resolved., answer_index (unsigned char ***) the address of an array of IPv4 addresses in string format. */
    SCC_TRACERT, /**< Performs a traceroute on an IPv4 address. Ret: (double) latency of final hop in ms. Params: ip (const char *) IP address., out_buf (char *) Output buffer to store the results of the traceroute., out_size (size_t) Size of out_buf. */
    SCC_PING, /**< Pings a host. Ret: (double) latency in ms. Params: ip (const char *) IP address. */
    SCC_COUNT /**< Number of shared context callbacks. */
} SCCallback;

typedef enum
{
    RS_QUICK,
    RS_FULL,
    RS_LOCAL, /* UNIMPLEMENTED */
    RS_COUNT
} ResolveState;

typedef union
{
    struct resolve_args
    {
        const char *hostname;
        uint *n_ans;
    } resolve;

    struct tracert_args
    {
        const char *ip;
        char *out_buf;
        size_t out_size;
    } tracert;

    struct ping_args
    {
        const char *ip;
    } ping;

} argType;

typedef union
{
    char **resolve;
    double tracert;
    double ping;
} retType;

extern void sharedContext_init(void); /* Creates a shared context */
extern void sharedContext_destroy(void); /* Destroys a shared context */

extern int sharedContext_getVariable(SCVariable var, void *value);
extern int sharedContext_setVariable(SCVariable var, const void *value);

extern int sharedContext_toggleCb(SCCallback cb, char enabled);
extern int sharedContext_execCb(SCCallback cb, retType *result, argType *args);

#endif
