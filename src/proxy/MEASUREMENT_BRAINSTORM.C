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
    MT_PING,
    MT_TRACEROUTE,
    MT_COUNT
} MeasurementType;

typedef enum {
    MF_NIL = (1 << 0),
    MF_PING = (1 << 1),
    MF_TRACEROUTE = (1 << 2),
    MF_MAX_FLAGS = (1 << 31)
} MeasurementFlags;

typedef enum {
    RB_NIL = 0,
    RB_DNS,
    RB_HOP,
    RB_HOST
} ReachabilityBlame;

typedef enum {
    RT_NIL = 0,
    RT_CABLE_BROKEN,
    RT_EXCEEDS_MAX_LATENCY,
    RT_REACHABLE
} ReachabilityType;

typedef struct {
    union {
        uint32 v4;
        uint8 v6[16];
    } address; /* objective*/
    char domain[256]; /* objective */
    boolean is_ipv6; /* objective */
    float rtt_min; /* subjective */
    float rtt_avg;
    float rtt_max;
    float rtt_stddev;
    uint8 rtt_samples;
    time_t measurement_last_performed[MT_COUNT];

    midx hops[32]; /* objective */
    uint hop_count; /* objective */

    MeasurementFlags measurements_performed; /* objective */
    struct {
        ReachabilityType type;
        struct {
            ReachabilityBlame reason;
            midx host;
        } blame;
    } reachability;
} IpMeasurement;

/* Frances:
 * Do I even bother with this? Given how fast an SQLite in-memory database can be.
 * Turns out there is the SQLite Online Backup API that allows syncing between in-memory and on-disk databases.
 * Multi-producer, single consumer queue. Dozens of reader threads, one writer thread, reader threads queue data for the writer thread to batch.
 * SQLite WAL mode?
 * Would end up puttings this in data/data.h
*/
typedef struct {
    IpMeasurement    ip_measurements[MAX_MEASUREMENTS];
    boolean            used[MAX_MEASUREMENTS]; /* may want to move away from C89 to allow for 1 bit array ? Or just use an int array with bitmasking */
    midx            next_empty_slot; /* 0 slot is nil */
    
} Measurements;

midx measurement_add(IpMeasurement measurement);
void meausrement_rem(midx idx);
IpMeasurement *measurement_get(midx idx);

/* END OF BRAINSTORMING!!!! */

