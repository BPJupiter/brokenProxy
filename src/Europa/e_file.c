#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "europa.h"

#ifdef _WIN32
#include <string.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32

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

void europa_pwd(char *output, uint32 output_size)
{
    if (getcwd(output, output_size) == NULL)
    {
        printf("Error getting module filename\n");
        return;
    }
}

#endif

#ifdef _WIN32
void europa_goto_dir(char *dir, uint32 dir_len, char *pwd, uint32 *pwd_len)
{

}
#else
void europa_goto_dir(char *dir, uint32 dir_len, char *pwd, uint32 *pwd_len)
{

}
#endif
