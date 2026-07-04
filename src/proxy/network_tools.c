#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Clay/clay.h"
#include "termcolor.h"

#include "network_tools.h"

#define NO_RTT 0xDEAD
#define NO_ADDRESS "\0"
#define PING_RESULT 0x80
#define TRACERT_RESULT 0x40

typedef struct TracertResult {
    uint hopCount;
    uint *hopRtt;
    char **hopAddress;
} TracertResult;

typedef struct MeasurementInfo
{
    uint8 result_flags;
    uint32 ping_result;
} MeasurementInfo;

/* multiple checks are done back-to-back */
static char current_ip[32] = "\0";
static MeasurementInfo current_measurement = {0};

static uint find_min(uint *arr, uint n);

void TracertResult_free(TracertResult *tracert_result)
{
    uint i;
    free(tracert_result->hopRtt);
    for (i = 0; i < tracert_result->hopCount; i++)
        free(tracert_result->hopAddress[i]);
    free(tracert_result->hopAddress);
    tracert_result->hopCount = 0;
    tracert_result->hopRtt = NULL;
    tracert_result->hopAddress = NULL;
}

#ifdef _WIN32
static TracertResult traceroute(const char *address)
{
    TracertResult result = { 0 };
    uint hopRtt_temp[30] = { 0 };
    char hopAddress_temp[30][64] = {0};
    uint i = 0;

    FILE *fp;
    char line[512] = { 0 };

    char command[256] = "tracert -d -h 30 -w 1000 ";
    strcat(command, address);

    fp = _popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run traceroute command: %s\n", TR_TAG, ERR_TAG, command);
        result.hopCount = 0;
        result.hopRtt = NULL;
        result.hopAddress = NULL;
        return result;
    }

    fgets(line, sizeof(line), fp); /* empty line */
    fgets(line, sizeof(line), fp); /* info line */
    fgets(line, sizeof(line), fp); /* empty line */

    for (i = 0; fgets(line, sizeof(line), fp) != NULL; i++)
    {
        uint rtt[3] = { 0, 0, 0 };
        char line_address[64] = NO_ADDRESS;
        uint j, k = 0;

        if (strlen(line) == 1) break;

        for (j = 3; line[j] != 0 && k < 3; j++)
        {
            if (line[j] == '*')
            {
                rtt[k] = NO_RTT;
                k++;
                continue;
            }
            if (line[j] < '0' || line[j] > '9')
            {
                continue;
            }
            rtt[k] *= 10;
            rtt[k] += line[j] - '0';
            if (line[j + 1] < '0' || line[j + 1] > '9')
                k++;
        }

        hopRtt_temp[i] = find_min(rtt, 3);

        if (strstr(line, "Request timed out.") == NULL)
        {
            char *ms1, *ms2, *star1, *star2, *p, *newline;
            ms1 = line;
            while (NULL != (ms2 = strstr(ms1, "ms"))) ms1 = ms2 + 2;
            star1 = line;
            while (NULL != (star2 = strstr(star1, "*"))) star1 = star2 + 1;

            p = ms1 > star1 ? ms1 : star1;
            while (*p == ' ') p++;

            newline = strchr(p, '\n');
            if (newline) *newline = '\0';
            c_text_copy(strlen(p) + 1, line_address, p);
        }

        c_text_copy(strlen(line_address) + 1, hopAddress_temp[i], line_address);
    }

    result.hopCount = i;
    result.hopRtt = malloc(i * (sizeof *result.hopRtt));
    result.hopAddress = malloc(i * (sizeof *result.hopAddress));
    for (i = 0; i < result.hopCount; i++)
    {
        result.hopRtt[i] = hopRtt_temp[i];
        result.hopAddress[i] = c_text_copy_allocate(hopAddress_temp[i]);
    }

    printf("%s %u hops to %s\n", TR_TAG, result.hopCount, address);

    _pclose(fp);

    return result;
}
#else
/* ignoring multiple addresses for a single hop, pretending all latencies are from the first given address on a line */
static TracertResult traceroute(const char *address)
{
    TracertResult result = { 0 };
    uint hopRtt_temp[30] = { 0 };
    char hopAddress_temp[30][64] = { 0 };
    uint i = 0;

    FILE *fp;
    char line[512] = { 0 };

    char command[128] = "/bin/traceroute -n -q 3 -w 1 ";
    strcat(command, address);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run traceroute command: %s\n", TR_TAG, ERR_TAG, command);
        result.hopCount = 0;
        result.hopRtt = NULL;
        result.hopAddress = NULL;
        return result;
    }

    fgets(line, sizeof(line), fp); /* info line */

    for (i = 0; fgets(line, sizeof(line), fp) != NULL; i++)
    {
        uint rtt[3] = { 0, 0, 0 };
        char address[64] = NO_ADDRESS;
        char *p1, *p2;
        uint j, k = 0;

        if ((p1 = strchr(line, '.')) == NULL)
        {
            c_text_copy(strlen(address) + 1, hopAddress_temp[i], address);
            hopRtt_temp[i] = NO_RTT;
            continue;
        }

        p1 -= 3;
        for (; *p1 < '0' || *p1 > '9'; p1++)
            ;
        if ((p2 = strchr(p1, ' ')) == NULL)
        {
            c_text_copy(strlen(address) + 1, hopAddress_temp[i], address);
            hopRtt_temp[i] = NO_RTT;
            continue;
        }
        *p2 = '\0';
        c_text_copy(strlen(p1) + 1, address, p1);
        p2++;

        for (j = 0; p2[j] != 0 && k < 3; j++)
        {
            if (p2[j] == '*')
            {
                rtt[k] = NO_RTT;
                k++;
                continue;
            }
            if (p2[j] < '0' || p2[j] > '9')
            {
                continue;
            }
            rtt[k] *= 10;
            rtt[k] += p2[j] - '0';
            if (p2[j + 1] < '0' || p2[j + 1] > '9')
            {
                j += 2;
                if (p2[j] >= '5') rtt[k] += 1;
                k++;
                for (; p2[j] >= '0' && p2[j] <= '9'; j++)
                    ;
            }
        }
        for (j = k; j < 3; j++)
            rtt[j] = NO_RTT;

        hopRtt_temp[i] = find_min(rtt, 3);
        c_text_copy(strlen(address) + 1, hopAddress_temp[i], address);
    }

    result.hopCount = i;
    result.hopRtt = malloc(i * (sizeof *result.hopRtt));
    result.hopAddress = malloc(i * (sizeof *result.hopAddress));
    for (i = 0; i < result.hopCount; i++)
    {
        result.hopRtt[i] = hopRtt_temp[i];
        result.hopAddress[i] = c_text_copy_allocate(hopAddress_temp[i]);
    }

    printf("%s %u hops to %s\n", TR_TAG, result.hopCount, address);

    pclose(fp);

    return result;
}
#endif

#ifdef _WIN32
static uint32 ping(const char *ip)
{
    FILE *fp;
    char line[512] = { 0 };
    uint64 result;

    char command[256] = "ping -n 1 -w 1000 ";
    strcat(command, ip);

    fp = _popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run ping command: %s\n", PING_TAG, ERR_TAG, command);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL);

    c_text_parse_decimal(line + 14, &result);

    printf("%s Ping on %s : %lld ms\n", PING_TAG, ip, result);
    _pclose(fp);

    result *= 1000000;
    return result;
}
#else
static uint32 ping(const char *ip)
{
    FILE *fp;
    char line[512] = {0};
    size_t l = 0;
    char *rtt_info;
    char output[1024] = {0};
    uint64 result;

    char command[128] = "/bin/ping -c 1 -w 1 ";
    strcat(command, ip);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run ping command: %s\n", PING_TAG, ERR_TAG, command);
        return 0;
    }

    output[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        l += strlen(line) + 1;
        if (l + 1 > sizeof(output))
        {
            printf("%s %s Output buffer too small at: %s: %d", PING_TAG, ERR_TAG, __FILE__, __LINE__);
            output[0] = '\0';
            return 0;
        }
        strcat(output, line);
    }
    rtt_info = strstr(line, "= ");
    if (rtt_info != NULL)
    {
        char *slash = strstr(rtt_info, "/");
        if (slash != NULL)
        {
            *slash = '\0';
            result = atoi(rtt_info + 2);
        }
    }

    printf("%s Ping on %s : %lld ms\n", PING_TAG, ip, result);

    pclose(fp);

    result *= 1000000;

    return result;
}
#endif

static uint find_min(uint *arr, uint n)
{
    uint min = arr[0];
    uint i;
    for (i = 1; i < n; i++)
    {
        if (arr[i] < min)
            min = arr[i];
    }

    return min;
}


boolean verify_latency(const char *ip, uint32 max_rtt_ns)
{
    uint32 rtt_current;
    uint64 cm_size;

    if (strcmp(current_ip, ip) == 0 && (current_measurement.result_flags & PING_RESULT) == PING_RESULT)
    {
        rtt_current = current_measurement.ping_result;
        return rtt_current < max_rtt_ns;
    }

    c_text_copy(32, current_ip, ip);
    memset(&current_measurement, 0, sizeof current_measurement);

    if ((current_measurement.result_flags & PING_RESULT) != PING_RESULT) {
        current_measurement.ping_result = ping(ip);
        current_measurement.result_flags |= PING_RESULT;
    }

    rtt_current = current_measurement.ping_result;
    return rtt_current < max_rtt_ns;
}

boolean verify_cable(const char *ip)
{
    return TRUE;
}

boolean verify_traceroute_ips(const char *dest_ip)
{
    return TRUE;
}
