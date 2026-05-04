#ifndef WEBSERVER_H
#define WEBSERVER_H

extern int g_proxy_pid;

int webserver_start(int port, int proxy_pid);
void webserver_shutdown(void);

#endif
