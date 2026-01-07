#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include <pthread.h>

void sharedContext_init(void); /* Creates a shared context */
void sharedContext_destroy(void); /* Destroys a shared context */

void sharedContext_set_rtt_cutoff(double val); /* */
double sharedContext_get_rtt_cutoff(void); /* */

void sharedContext_set_resolve_cb(short (*cb)(char *, unsigned char ***)); /* */
short sharedContext_exec_resolve_cb(char *hostname, unsigned char ***answer_index); /* */
void sharedContext_set_tracert_cb(double (*cb)(const char *, char *, size_t)); /* */
double sharedContext_exec_tracert_cb(const char *ip, char *out_buf, size_t out_size); /* */
void sharedContext_set_ping_cb(double (*cb)(const char *)); /* */
double sharedContext_exec_ping_cb(const char *ip); /* */

#endif
