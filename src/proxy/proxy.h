#ifndef PROXY_H
#define PROXY_H

#include <stddef.h>
#include "Clay/clay.h"

int proxy_start(uint16 port);
void proxy_shutdown(void);

#endif
