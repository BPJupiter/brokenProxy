#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Clay/clay.h"
#include "traceroute.h"
#include "termcolor.h"

#ifdef _WIN32
double traceroute(const char *address, char *output, size_t output_len)
{
    FILE *fp;
    char line[512] = { 0 };
    char last_hop_line[512] = { 0 };
    double latency = -1.0;
    size_t l = 0;

    char command[256] = "tracert -d -h 30 -w 1000 ";
    strcat(command, address);

    fp = _popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run traceroute command: %s\n", TR_TAG, ERR_TAG, command);
        return -1;
    }

    output[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        l += strlen(line) + 1;
        if (l + 1 > output_len)
        {
            printf("%s %s Output buffer too small at: %s: %d]n", TR_TAG, ERR_TAG, __FILE__, __LINE__);
            output[0] = '\0';
            return -1;
        }
        strcat(output, line);

        if (line[0] > '0' && line[0] <= '9' || line[1] >= '0' && line[1] <= '9')
            c_text_copy(sizeof(line), last_hop_line, line);
    }

    c_text_parse_double(last_hop_line, &latency);

    printf("%s Traceroute on %s : %.2f ms\n", TR_TAG, address, latency);

    _pclose(fp);

    return latency;

}
#else
double traceroute(const char *address, char *output, size_t output_len)
{
    FILE *fp;
    char line[512] = {0};
    char *gap1;
    double latency = -1.0;
    size_t l = 0;

    char command[128] = "/bin/traceroute -I -n -q 1 -w 1 ";
    strcat(command, address);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run traceroute command: %s\n", TR_TAG, ERR_TAG, command);
        return -1;
    }

    output[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        l += strlen(line) + 1;
        if (l + 1 > output_len)
        {
            printf("%s %s Output buffer too small at: %s: %d\n", TR_TAG, ERR_TAG, __FILE__, __LINE__);
            output[0] = '\0';
            return -1;
        }
        strcat(output, line);
    }
    gap1 = strstr(line, "  ");
    if (gap1 != NULL)
    {
        char *gap2 = strstr(gap1 + 2, "  ");
        if (gap2 != NULL)
        {
            char *ms = strstr(line, " ms");
            if (ms != NULL)
            {
                *ms = '\0';
                latency = atof(gap2 + 2);
            }
        }
    }

    printf("%s Traceroute on %s : %.2f ms\n", TR_TAG, address, latency);

    pclose(fp);

    return latency;
}
#endif

#ifdef _WIN32
double ping(const char *ip)
{
    FILE *fp;
    char line[512] = { 0 };
    int64 latency = -1;

    char command[256] = "ping -n 1 -w 1000 ";
    strcat(command, ip);

    fp = _popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run ping command: %s\n", PING_TAG, ERR_TAG, command);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL);

    c_text_parse_decimal(line + 14, &latency);

    printf("%s Ping on %s : %lld ms\n", PING_TAG, ip, latency);
    _pclose(fp);

    return (double)latency;
}
#else
double ping(const char *ip)
{
    FILE *fp;
    char line[512] = {0};
    double latency = -1.0;
    size_t l = 0;
    char *rtt_info;
    char output[1024] = {0};

    char command[128] = "/bin/ping -c 1 -w 1 ";
    strcat(command, ip);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("%s %s Failed to run ping command: %s\n", PING_TAG, ERR_TAG, command);
        return -1;
    }

    output[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        l += strlen(line) + 1;
        if (l + 1 > sizeof(output))
        {
            printf("%s %s Output buffer too small at: %s: %d", PING_TAG, ERR_TAG, __FILE__, __LINE__);
            output[0] = '\0';
            return -1;
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
            latency = atof(rtt_info + 2);
        }
    }

    printf("%s Ping on %s : %.2f ms\n", PING_TAG, ip, latency);

    pclose(fp);

    return latency;
}
#endif
