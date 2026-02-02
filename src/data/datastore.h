#ifndef DATASTORE_H
#define DATASTORE_H

#include "Clay/clay.h"

typedef void EDBHandle;

extern void *gDatastore_lock; /* unused */
extern EDBHandle *gDatastore;

void datastore_init();
void datastore_destroy();

#endif /* DATASTORE_H */