#include "Clay/clay.h"
#include "Europa/europa.h"

#include <signal.h>

#define WEBSERVER_PORT 13406

void signal_handler(int sig)
{
	printf("\nCaught signal %d\n", sig);
    c_debug_mem_print(0);
    exit(sig);
}

int main(int argc, char **argv)
{
	int webserver_port = WEBSERVER_PORT;

	if (argc > 1)
	{
		webserver_port = atoi(argv[1]);
	}

	c_debug_memory_init(europa_mutex_lock, europa_mutex_unlock, europa_mutex_create());
	signal(SIGINT, signal_handler);

	webserver_start(webserver_port);

	c_debug_mem_print(0);
	return 0;
}