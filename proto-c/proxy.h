#ifndef PROXY_H
#define PROXY_H

void proxy_init(int (*hostname_resolver)(unsigned char* hostname, unsigned char*** answer_index), int (*tracert)(const char* ip, char* out_buf, size_t out_size));
int proxy_start(int port);

#endif
