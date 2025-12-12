#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <stddef.h>

int traceroute(const char* address, char* output, size_t output_len);

#endif
