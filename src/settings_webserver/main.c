#include "Clay/clay.h"
#include "Europa/europa.h"
#include "webserver.h"

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
	int proxy_pid = -1;

	if (argc > 1) {
		webserver_port = atoi(argv[1]);
	}

	c_debug_memory_init(europa_mutex_lock, europa_mutex_unlock, europa_mutex_create());

	europa_pwd_to_root("brokenProxy");

	signal(SIGINT, signal_handler);

	proxy_pid = europa_execute("bin/proxy.exe");
	if (proxy_pid == 0) {
		printf("FAILED TO LAUNCH PROXY PROCESS.\n");
		exit(1);
	}
	printf("Proxy launched.\n");

	webserver_start(webserver_port, proxy_pid);

	c_debug_mem_print(0);
	return 0;
}