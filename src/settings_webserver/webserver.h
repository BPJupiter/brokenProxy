#ifndef WEBSERVER_H
#define WEBSERVER_H

extern int g_proxy_pid;

int webserver_start(uint16 port, int proxy_pid);
void webserver_shutdown(void);

#endif
