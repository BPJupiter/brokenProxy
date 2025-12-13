#include "traceroute.h"
#include "termcolor.h"

#include <stdio.h>
#include <string.h>

int traceroute(const char* address, char* output, size_t output_len)
{
  FILE *fp;
  char line[512];

  char command[] = "/bin/traceroute -I -n -q 1 -w 1 ";
  strcat(command, address);

  fp = popen(command, "r");
  if (fp == NULL)
  {
    printf("%s %s Failed to run traceroute command: %s\n", TR_TAG, ERR_TAG, command);
    return 1;
  }

  output[0] = '\0';
  size_t l = 0;
  while (fgets(line, sizeof(line), fp) != NULL)
  {
    l += strlen(line)+1;
    if (l+1 > output_len) {
      printf("%s %s Output buffer too small at: %s: %d\n", TR_TAG, ERR_TAG, __FILE__, __LINE__);
      output[0] = '\0';
      return 1;
    }
    strcat(output, line);
  }

  printf("%s Tracerotue on %s\n", TR_TAG, address);

  pclose(fp);

  return 0;
}
