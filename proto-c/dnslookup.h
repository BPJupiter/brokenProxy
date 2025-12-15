#ifndef DNSLOOKUP_H
#define DNSLOOKUP_H

typedef enum RootServerIndex {
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M
} RootServerIndex;

#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 //Canonical name
#define T_SOA 6 //Start of authority zone
#define T_PTR 12 //Domain name pointer
#define T_MX 15 //Mail server
#define T_AAAA 28 //IPv6 address 

short dns_resolve(unsigned char* host, int query_type, RootServerIndex root_server, unsigned char*** answer_index);

#ifdef DNS_DEBUG
extern void print_response_info(void*);
extern void print_response_contents(void*, void*, void*, void*, void*);
#endif

#endif
