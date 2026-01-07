#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <stddef.h>

double traceroute(const char *address, char *output, size_t output_len);
double ping(const char *ip);

#endif
