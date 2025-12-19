#ifndef PROXY_H
#define PROXY_H

void proxy_init(unsigned short (*hostname_resolver)(unsigned char* hostname, unsigned char*** answer_index), double (*tracert)(const char* ip, char* out_buf, size_t out_size), double rtt_cutoff);
int proxy_start(int port);

#endif
