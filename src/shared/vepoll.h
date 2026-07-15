#ifndef __VEPOLL_H__
#define __VEPOLL_H__

/* If you wish to compile on windows, wepoll.c and wepoll.h must be present. */

#ifdef _WIN32
#  include "wepoll.h"
typedef HANDLE VHandle;
#  define INVALID_VEPOLL_HANDLE NULL
#  define VEPOLL_ERROR -1
#else
#  include <sys/epoll.h>
#  include <unistd.h>
typedef int VHandle;
#  define INVALID_VEPOLL_HANDLE -1
#  define VEPOLL_ERROR -1
#endif

void vhandle_close(VHandle handle);
void vepoll_perror(const char *msg);

#endif /* __VEPOLL_H__ */

#ifdef VEPOLL_IMPLEMENTATION

void vhandle_close(VHandle handle)
#ifdef _WIN32
{
    epoll_close(handle);
}
#else
{
    close(handle);
}
#endif

void vepoll_perror(const char *msg)
#ifdef _WIN32
{
    int err_code = GetLastError();
    char *err_str = NULL;

    int size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                              | FORMAT_MESSAGE_FROM_SYSTEM
                              | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL,
                              err_code,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPSTR)&err_str,
                              0,
                              NULL);
    if (size > 0) {
        if (msg != NULL && *msg != '\0') {
            fprintf(stderr, "%s: Wepoll error (%d): %s\n", msg, err_code, err_str);
        }
        else {
            fprintf(stderr, "Wepoll error (%d): %s\n", err_code, err_str);
        }
        LocalFree(err_str);
    }
    else {
        fprintf(stderr, "Wepoll error (%d): (Failed to get message)\n", err_code);
    }
}
#else
{
    perror(msg);
}
#endif

#endif /* VEPOLL_IMPLEMENTATION */
