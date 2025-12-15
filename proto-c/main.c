#include <stdlib.h>

#include "traceroute.h"
#include "dnslookup.h"
#include "proxy.h"

#define PROXY_PORT 8080

int no_traceroute(const char* ip, char* out_buf, size_t out_size) {}

unsigned short get_ip_addresses(unsigned char* hostname, unsigned char*** answer_index)
{
  #ifdef ROOT_SERVER
  return dns_resolve(hostname, T_A, ROOT_SERVER, answer_index);
  #else
  return dns_resolve(hostname, T_A, E, answer_index);
  #endif
}

int main(int argc, char *argv[])
{
  //TODO: Set "ping mode" where requests are limited to a given ms of latency, else they get dropped.
  // I suggest 80ms for limiting requests to NZ/AUS. All other connections seem to be of reliably higher latency
  int proxy_port = PROXY_PORT;
  double rtt_cutoff = 9999.0f;

  if (argc > 1)
    proxy_port = atoi(argv[1]);

  if (argc > 2)
    rtt_cutoff = atof(argv[2]);

  #if defined(NO_TRACERT)
  dns_init(no_traceroute, rtt_cutoff);
  proxy_init(get_ip_addresses, no_traceroute, rtt_cutoff);
  #elif defined(PING)
  dns_init(ping, rtt_cutoff);
  proxy_init(get_ip_addresses, ping, rtt_cutoff);
  #else
  dns_init(traceroute, rtt_cutoff);
  proxy_init(get_ip_addresses, traceroute, rtt_cutoff);
  #endif
  proxy_start(proxy_port);
  return 0;
}
