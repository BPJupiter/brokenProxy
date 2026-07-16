#include "Clay/clay.h"

// DOES NOT COMPILE

/* 
* The below is a brainstorm for how I want to restructure my measurement data structure to:
* 1. Increase ease of use
* 2. Reduce data duplication
* 3. Easily map to SQL entries
*/

#define MAX_MEASUREMENTS (1 << 16)
typedef uint midx;

typedef enum {
    MT_LATENCY,
    MT_TRACEROUTE,
    MT_COUNT
} MeasurementType;

typedef enum {
    MF_LATENCY = (1 << 0),
    MF_TRACEROUTE = (1 << 1),
    MF_MAX_FLAGS = (1 << 31)
} MeasurementFlags;

typedef enum {
    FS_NOFAILURE = 0,
    FS_DNS,
    FS_ROUTING,
    FS_HANDSHAKE,
    FS_CONNECTION,
    FS_COUNT
} FailureStage;

typedef enum {
    FR_NOFAILURE = 0,
    FR_DNS_NXDOMAIN,
    FR_DNS_TIMEOUT, // Will need to change epoll to have a timeout value
    FR_DNS_SERVFAIL,
    FR_DNS_BADRECORD,
    FR_DNS_GENERAL,
    FR_ROUTING_NOROUTE,
    FR_ROUTING_HOSTUNREACHABLE,
    FR_ROUTING_TTLEXPIRED, // Looks like FR_HANDSHAKE_TIMEOUT
    FR_ROUTING_GENERAL,
    FR_HANDSHAKE_CONNREFUSED,
    FR_HANDSHAKE_TIMEOUT, // Looks like FR_ROUTING_TTLEXPIRED
    FR_HANDSHAKE_GENERAL,
    FR_CONNECTION_RESETBYPEER,
    FR_CONNECTION_BROKENPIPE,
    FR_CONNECTION_KEEPALIVETIMEOUT, // ?
    FR_CONNECTION_GENERAL
} FailureReason;

typedef struct {
    union {
        uint32 v4;
        uint8 v6[16];
    }       address;
    char    domain[256]; // spin into own array.
    boolean is_ipv6;
    float   rtt_min;
    float   rtt_avg;
    float   rtt_max;
    float   rtt_stddev;
    uint32  rtt_samples;
    time_t  timestamp;

    midx    hops[32];
    uint    hop_count;

    MeasurementFlags measurement_flags;
    struct {
        boolean is_reachable;
        struct {
            FailureStage failure_stage;
            FailureReason failure_reason;
            midx culprit;
        } blame;
    } reachability;
} IpMeasurement;

typedef struct {
    char uri[256];
} Domain;

/* Frances:
 * Do I even bother with this? Given how fast an SQLite in-memory database can be.
 * Turns out there is the SQLite Online Backup API that allows syncing between in-memory and on-disk databases.
 * Multi-producer, single consumer queue. Dozens of reader threads, one writer thread, reader threads queue data for the writer thread to batch.
 * SQLite WAL mode?
 * Would end up puttings this in data/data.h
*/
typedef struct {
    IpMeasurement    ip_measurements[MAX_MEASUREMENTS];
    boolean          used[MAX_MEASUREMENTS]; /* may want to move away from C89 to allow for 1 bit array ? Or just use an int array with bitmasking */
    midx             next_empty_slot; /* 0 slot is nil */
} Measurements;

midx measurement_add(IpMeasurement measurement);
void meausrement_rem(midx idx);
IpMeasurement *measurement_get(midx idx);

/* END OF BRAINSTORMING!!!! */

