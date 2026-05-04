#ifndef DATASTORE_H
#define DATASTORE_H

#include "Europa/europa.h"

extern void *gDatastore_lock; /* unused */
extern EDBHandle *gDatastore;

void datastore_init(void);
void datastore_destroy(void);

#endif /* DATASTORE_H */
