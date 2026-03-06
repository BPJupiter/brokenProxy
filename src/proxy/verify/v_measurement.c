#include "Clay/clay.h"
#include "Europa/europa.h"
#include "context/context.h"
#include "data/datastore.h"
#include <string.h>

#include "verify.h"

typedef struct MeasurementInfo
{
    uint8 resultFlags;
    PingResult pingResult;
    TracertResult tracertResult;
} MeasurementInfo;

#define PING_RESULT 0x80
#define TRACERT_RESULT 0x40

/* multiple checks are done back-to-back */
static char current_ip[32] = "\0";
static MeasurementInfo current_measurement = {0};

boolean verify_latency(const char *ip)
{
    double rtt_cutoff, rtt_current;
    uint64 cm_size;

    if (!sharedContext_callback_isEnabled_ping())
        return TRUE;

    sharedContext_getVariable(SCV_MAX_RTT, &rtt_cutoff);
    if (strcmp(current_ip, ip) == 0 && (current_measurement.resultFlags & PING_RESULT) == PING_RESULT)
    {
        rtt_current = current_measurement.pingResult.rtt;
        return rtt_current < rtt_cutoff;
    }

    c_text_copy(32, current_ip, ip);
    memset(&current_measurement, 0, sizeof current_measurement);

    if (!europa_database_fetch(gDatastore, ip, strlen(ip), &current_measurement, &cm_size)
        || (current_measurement.resultFlags & PING_RESULT) != PING_RESULT)
    {
        sharedContext_callback_execute_ping(&current_measurement.pingResult, ip);
        current_measurement.resultFlags |= PING_RESULT;
        europa_database_store(gDatastore, ip, strlen(ip), &current_measurement, sizeof current_measurement);
    }

    rtt_current = current_measurement.pingResult.rtt;
    return rtt_current < rtt_cutoff;
}

boolean verify_cable(const char *ip)
{
    /* TODO: Map traceroute to cable */
    uint64 cm_size;

    if (!sharedContext_callback_isEnabled_traceroute())
        return TRUE;

    if (strcmp(current_ip, ip) == 0 && (current_measurement.resultFlags & TRACERT_RESULT) == TRACERT_RESULT)
    {
        return TRUE;
    }

    if (!europa_database_fetch(gDatastore, ip, strlen(ip), &current_measurement, &cm_size)
        || (current_measurement.resultFlags & TRACERT_RESULT) != TRACERT_RESULT)
    {
        sharedContext_callback_execute_traceroute(&current_measurement.tracertResult, ip);
        current_measurement.resultFlags |= TRACERT_RESULT;
        europa_database_store(gDatastore, ip, strlen(ip), &current_measurement, sizeof current_measurement);
    }

    return TRUE;
}
