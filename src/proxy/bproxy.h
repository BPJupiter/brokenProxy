#ifndef BPROXY_H
#define BPROXY_H

#include "Clay/clay.h"

typedef struct BProxyHandle BProxyHandle;

extern BProxyHandle *bp_init(uint16 port);
extern int           bp_start(BProxyHandle *proxy);
extern int     bp_cntl(int optype, int arg);
extern int     bp_block_ip_address(const char *ip);
extern int     bp_allow_ip_address(const char *ip);
extern boolean bp_ip_address_is_blocked(const char *ip);
extern void    bp_destroy(BProxyHandle *proxy);

#endif
