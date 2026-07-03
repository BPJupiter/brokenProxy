#ifndef __NETWORK_TOOLS_H__
#define __NETWORK_TOOLS_H__

#include "Clay/clay.h"
#include "Styx/styx.h"

extern boolean verify_latency(const char *ip, uint32 rtt_ns);
extern boolean verify_cable(const char *ip);
extern boolean verify_traceroute_ips(const char *ip);

extern boolean dns_resolve_iterative_root_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6, uint32 max_rtt_ns);

#endif

