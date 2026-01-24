#ifndef DNSLOOKUP_H
#define DNSLOOKUP_H

#include <stddef.h>
#include "Clay/clay.h"

/* #define DNS_DEBUG */

char **dns_resolve_recursive_local(const char *host, uint *n_ans);
char **dns_resolve_iterative_root(const char *host, uint *n_ans);
/* -------- UNIMPLEMENTED ---------- */
char **dns_resolve_iterative_local(const char *host, uint *n_ans);

#ifdef DNS_DEBUG
extern void print_response_info(void *);
extern void print_response_contents(void *, void *, void *, void *, void *);
#endif

#endif
