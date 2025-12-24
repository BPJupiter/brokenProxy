#ifndef PROXY_H
#define PROXY_H

#include <stddef.h>

void proxy_init(unsigned short (*hostname_resolver)(char *hostname, unsigned char ***answer_index),
                double (*tracert)(const char *ip, char *out_buf, size_t out_size));
int proxy_start(int port);

#endif
