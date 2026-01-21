#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "europa.h"

#ifdef _WIN32
#include <string.h>
#include <windows.h>

void europa_pwd(char *output, uint32 output_size)
{
    uint length;
    char *last_slash;
    if (GetModuleFileName(NULL, output, output_size) == 0)
    {
        printf("Error getting module filename\n");
        return;
    }

    length = strlen(output);
    last_slash = &output[length - 1];

    while (*last_slash != '\\')
    {
        *last_slash = '\0';
        last_slash--;
    }
}

#else
#include <unistd.h>

void europa_pwd(char *output, uint32 output_size)
{
    if (getcwd(output, output_size) == NULL)
    {
        printf("Error getting module filename\n");
        return;
    }
}

#endif

