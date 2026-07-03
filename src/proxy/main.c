#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "context/context.h"
#include "data/datastore.h"
#include "bproxy.h"

#define PROXY_PORT 13407

volatile sig_atomic_t g_reload_settings = 0;

void handle_shutdown(int sig)
{
    sharedContext_destroy();
    datastore_destroy();
    c_debug_mem_print(0);

    exit(0);
}

#ifdef _WIN32
unsigned __stdcall windows_reload_listener(void *arg)
{
    char event_name[128];
    snprintf(event_name, sizeof(event_name), "Global\\EuropaReload_%d", _getpid());

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, event_name);
    if (!hEvent) return 0;

    while (1) {
        if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0) {
            printf("\nReload event recieved!\n");
            g_reload_settings = 1;
        }
    }
    CloseHandle(hEvent);
    return 0;
}
#else
void handle_reload(int sig)
{
    UNUSED(sig);
    printf("\nReload signal recieved!\n");
    g_reload_settings = 1;
}
#endif

int main(int argc, char **argv)
{
    BProxyHandle *proxy;
    int proxy_port = PROXY_PORT;

    if (argc > 1)
    {
        proxy_port = atoi(argv[1]);
    }

    c_debug_memory_init(europa_mutex_lock, europa_mutex_unlock, europa_mutex_create());

#ifdef _WIN32
    signal(SIGBREAK, handle_shutdown);
    signal(SIGINT, handle_shutdown);

    _beginthreadex(NULL, 0, windows_reload_listener, NULL, 0, NULL);
#else
    signal(SIGTERM, handle_shutdown);
    signal(SIGINT, handle_shutdown);
    signal(SIGUSR1, handle_reload);
#endif

    sharedContext_init();
    datastore_init();

    proxy = bp_init(proxy_port);
    bp_start(proxy);
    bp_destroy(proxy);

    datastore_destroy();
    c_debug_mem_print(0);
    return 0;
}
