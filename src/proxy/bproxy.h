#ifndef BPROXY_H
#define BPROXY_H

#include "Clay/clay.h"

typedef struct BProxyHandle BProxyHandle;

#ifdef __cplusplus
extern "C" {
#endif

extern BProxyHandle *bp_init(uint16 port, const char *settings_filename);
extern int           bp_start_blocking(BProxyHandle *proxy);
extern boolean       bp_update(BProxyHandle *proxy);
extern void          bp_reload_settings(BProxyHandle *proxy); // Todo: use europa settings handle?
extern int     bp_cntl(int optype, int arg);
extern int     bp_block_ip_address(const char *ip);
extern int     bp_allow_ip_address(const char *ip);
extern boolean bp_ip_address_is_blocked(const char *ip);
extern void    bp_destroy(BProxyHandle *proxy);

#ifdef __cplusplus
}
#endif

#endif
