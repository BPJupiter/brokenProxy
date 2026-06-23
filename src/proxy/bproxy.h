#ifndef BPROXY_H
#define BPROXY_H

#include "Clay/clay.h"

int bp_start(uint16 port);
int bp_block_ip_address(const char *ip);
int bp_allow_ip_address(const char *ip);
boolean bp_ip_address_is_blocked(const char *ip);
void proxy_shutdown(void);
void bp_shutdown(void);

#endif
