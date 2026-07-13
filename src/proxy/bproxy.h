#ifndef BPROXY_H
#define BPROXY_H

#include "Clay/clay.h"
#include "Styx/styx.h"
#include "vepoll.h"

#define MAX_CONNECTIONS 256
#define MAX_EVENTS  256

typedef enum {
    CS_DISCONNCETED = 0,
    CS_CLIENT_SENDING_METHODS,
    CS_SERVER_SENDING_METHOD,
    CS_CLIENT_SENDING_REQUEST,
    CS_SERVER_SENDING_REPLY,
    CS_CONNECTED
} ConnectionState;

typedef struct {
    SHandle *handle;
    boolean write_pending;
} Connection;

typedef struct {
    Connection clients[MAX_CONNECTIONS];
    Connection targets[MAX_CONNECTIONS];
    ConnectionState states[MAX_CONNECTIONS];
    boolean used[MAX_CONNECTIONS];
    int free_list[MAX_CONNECTIONS];
    int free_list_count;
    int next_empty_slot;
    uint count;
} Connections;

typedef struct {
    boolean do_ping;
    boolean do_traceroute;
    boolean use_local_dns;
    uint32 max_rtt_ns;
} BProxySettings;

typedef struct BProxyHandle {
    BProxySettings settings;
    char *settings_filename;
    
    SHandle *server_handle;
    uint16 port;
    Connections connections;
    
    struct epoll_event events[MAX_EVENTS];
    VHandle epoll_handle;
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
