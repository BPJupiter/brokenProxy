#include <stdlib.h>

#include "traceroute.h"
#include "dnslookup.h"
#include "proxy.h"

#define PROXY_PORT 8080

int no_traceroute(const char* ip, char* out_buf, size_t out_size) {}

int get_ip_addresses(unsigned char* hostname, unsigned char*** answer_index)
{
  return dns_resolve(hostname, T_A, J, answer_index);
}

int main(int argc, char *argv[])
{
  int proxy_port = PROXY_PORT;

  if (argc > 1)
    proxy_port = atoi(argv[1]);

  #ifdef NO_TRACERT
  dns_init(no_traceroute);
  proxy_init(get_ip_addresses, no_traceroute);
  #else
  dns_init(traceroute);
  proxy_init(get_ip_addresses, traceroute);
  #endif
  proxy_start(proxy_port);
  return 0;
}
