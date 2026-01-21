#include <stdlib.h>
#include <signal.h>
#include <stdio.h>


#include "Clay/clay.h"
#include "Europa/europa.h"
#include "shared_context.h"
#include "proxy.h"

#define PROXY_PORT 13406
#define UNUSED(x) (void)(x)

void signal_handler(int sig)
{
    printf("\nCaught signal %d\n", sig);
    c_debug_mem_print(1);
    exit(sig);
}

int main(int argc, char *argv[])
{
    int proxy_port = PROXY_PORT;
    double rtt_cutoff;

    if (argc > 1)
    {
        rtt_cutoff = atof(argv[1]);
        sharedContext_setVariable(SCV_MAX_RTT, &rtt_cutoff);
    }

#if defined(C_MEMORY_DEBUG)
    c_debug_memory_init(europa_mutex_lock, europa_mutex_unlock, europa_mutex_create());
    signal(SIGINT, signal_handler);
#endif
    sharedContext_init();

    proxy_start(proxy_port);
    sharedContext_destroy();
    return 0;
}
