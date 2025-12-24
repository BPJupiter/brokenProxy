#include "traceroute.h"
#include "termcolor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double traceroute(const char *address, char *output, size_t output_len)
{
  FILE *fp;
  char line[512];
  double latency = -1.0;

  char command[64] = "/bin/traceroute -I -n -q 1 -w 1 ";
  strcat(command, address);

  fp = popen(command, "r");
  if (fp == NULL)
  {
    printf("%s %s Failed to run traceroute command: %s\n", TR_TAG, ERR_TAG, command);
    return -1;
  }

  output[0] = '\0';
  size_t l = 0;
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
  char *gap1 = strstr(line, "  ");
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

  printf("%s Traceroute on %s : %.2lf ms\n", TR_TAG, address, latency);

  pclose(fp);

  return latency;
}

double ping(const char *address, char *output, size_t output_len)
{
  FILE *fp;
  char line[512];
  double latency = -1.0;

  char command[64] = "/bin/ping -c 1 -w 1 ";
  strcat(command, address);

  fp = popen(command, "r");
  if (fp == NULL)
  {
    printf("%s %s Failed to run ping command: %s\n", PING_TAG, ERR_TAG, command);
    return -1;
  }

  output[0] = '\0';
  size_t l = 0;
  while (fgets(line, sizeof(line), fp) != NULL)
  {
    l += strlen(line) + 1;
    if (l + 1 > output_len)
    {
      printf("%s %s Output buffer too small at: %s: %d", PING_TAG, ERR_TAG, __FILE__, __LINE__);
      output[0] = '\0';
      return -1;
    }
    strcat(output, line);
  }
  char *rtt_info = strstr(line, "= ");
  if (rtt_info != NULL)
  {
    char *slash = strstr(rtt_info, "/");
    if (slash != NULL)
    {
      *slash = '\0';
      latency = atof(rtt_info + 2);
    }
  }

  printf("%s Ping on %s : %.2lf ms\n", PING_TAG, address, latency);

  pclose(fp);

  return latency;
}
