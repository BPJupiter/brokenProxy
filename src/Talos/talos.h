#include "Clay/clay.h"

#ifndef TALOS_H
#define TALOS_H

#define talos_malloc_assert(x) do {if (!_talos_malloc_assert(x, __FILE__, __LINE__)) abort();} while (0);
extern boolean _talos_malloc_assert(void *p, const char *file, uint32 line);

extern void talos_print_error(const char *msg);

#endif
