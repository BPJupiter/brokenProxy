#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#include "Clay/clay.h"
#include "Europa/europa.h"
#include "shared_context/shared_context.h"
#include "data/datastore.h"
#include "proxy.h"

#define PROXY_PORT 13406

void signal_handler(int sig)
{
    printf("\nCaught signal %d\n", sig);
    c_debug_mem_print(0);
    sharedContext_destroy();
    datastore_destroy();
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

    c_debug_memory_init(europa_mutex_lock, europa_mutex_unlock, europa_mutex_create());
    signal(SIGINT, signal_handler);

    sharedContext_init();
    datastore_init();

    proxy_start(proxy_port);

    datastore_destroy();
    c_debug_mem_print(0);
    return 0;
}
