#ifndef DNSLOOKUP_H
#define DNSLOOKUP_H

#include <stddef.h>

typedef enum RootServerIndex
{
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
    M,
    RSI_COUNT
} RootServerIndex;

#ifndef _ARPA_NAMESER_H_

#define T_A 1     /* Ipv4 address */
#define T_NS 2    /* Nameserver */
#define T_CNAME 5 /* Canonical name */
#define T_SOA 6   /* Start of authority zone */
#define T_PTR 12  /* Domain name pointer */
#define T_MX 15   /* Mail server */
#define T_AAAA 28 /* IPv6 address */

#endif

short dns_resolve(char *host, unsigned char ***answer_index);

#ifdef DNS_DEBUG
extern void print_response_info(void *);
extern void print_response_contents(void *, void *, void *, void *, void *);
#endif

#endif
