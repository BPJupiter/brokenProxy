#include "Talos/talos.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#else
#include <stdio.h>
#endif

void talos_print_error(const char *msg)
{
#ifdef _WIN32
    DWORD err_code = GetLastError();
    char *err_msg = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&err_msg,
        0,
        NULL
        );

    if (msg != NULL && *msg != '\0')
    {
        fprintf(stderr, "%s: %s\n", msg, err_msg ? err_msg : "Unknown Error");
    }
    else
    {
        fprintf(stderr, "%s\n", err_msg ? err_msg : "Unknown Error");
    }

    if (err_msg)
        LocalFree(err_msg);
#else
    perror(msg);
#endif
}

