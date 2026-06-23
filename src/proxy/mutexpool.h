#ifndef MUTEXPOOL_H
#define MUTEXPOOL_H

typedef struct MutexPool MutexPool;

extern MutexPool *mutex_pool_init(void)
extern void      *mutex_pool_mutex_get(MutexPool *mpool, uint midx)
extern uint       mutex_pool_mutex_add(MutexPool *mpool)
extern void       mutex_pool_mutex_remove(MutexPool *mpool, uint midx)
extern void       mutex_pool_free(MutexPool *mpool)

#endif
