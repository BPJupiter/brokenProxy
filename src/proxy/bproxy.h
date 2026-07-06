#ifndef BPROXY_H
#define BPROXY_H

#include "Clay/clay.h"
#include "Styx/styx.h"

#define MAX_CLIENTS 256

typedef struct {
    struct {
        SHandle *client;
        SHandle *target;
        boolean active;
    } pairs[MAX_CLIENTS];
    uint count;
} HandleSet;

typedef struct {
    boolean do_ping;
    boolean do_traceroute;
    boolean use_local_dns;
    uint32 max_rtt_ns;
} BProxySettings;

typedef struct BProxyHandle {
    BProxySettings settings;
    HandleSet connections;
    SHandle *server_handle;
    char *settings_filename;
    uint16 port;
} BProxyHandle;

#ifdef __cplusplus
extern "C" {
#endif

extern BProxyHandle *bp_init(uint16 port, const char *settings_filename);
extern int           bp_start_blocking(BProxyHandle *proxy);
extern boolean       bp_update(BProxyHandle *proxy);
extern void          bp_load_settings_from_disk(BProxyHandle *proxy); // Todo: use europa settings handle?
extern void          bp_save_settings_to_disk(BProxyHandle *proxy);
extern int     bp_cntl(int optype, int arg);
extern int     bp_block_ip_address(const char *ip);
extern int     bp_allow_ip_address(const char *ip);
extern boolean bp_ip_address_is_blocked(const char *ip);
extern void    bp_destroy(BProxyHandle *proxy);

#ifdef __cplusplus
}
#endif

#endif
