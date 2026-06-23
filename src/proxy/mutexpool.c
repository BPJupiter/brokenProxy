#include "mutexpool.h"

#include "Clay/clay.h"
#include "Europa/europa.h"

#define MAX_MUTEXES 64

typedef struct MutexPool {
    void *mutexes[MAX_MUTEXES];
    boolean used[MAX_MUTEXES];
    uint next_empty_slot;
} MutexPool;

MutexPool *mutex_pool_init(void)
{
    MutexPool *mpool = calloc(1, sizeof(MutexPool));
    if (!mpool) return NULL;
    mpool->mutexes[0] = europa_mutex_create();
    mpool->next_empty_slot = 1;
    return mpool;
}

void *mutex_pool_mutex_get(MutexPool *mpool, uint midx)
{
    if (midx > 0 && midx < MAX_MUTEXES && mpool->used[midx]) {
        return mpool->mutexes[midx];
    }
    else {
        if (europa_mutex_lock_try(mpool->mutexes[0])) {
            europa_mutex_unlock(mpool->mutexes[0]);
        }
        else {
            fprintf(stderr, "Nil mutex has been locked!!!!!!\n");
        }
        return mpool->mutexes[0];
    }
}

uint mutex_pool_mutex_add(MutexPool *mpool)
{
    uint slot = 0;
    if (mpool->next_empty_slot < MAX_MUTEXES) {
        slot = mpool->next_empty_slot++;
    }
    if (slot) {
        mpool->mutexes[slot] = europa_mutex_create();
        mpool->used[slot] = TRUE;
        return slot;
    }
    else {
        return 0;
    }
}

void mutex_pool_mutex_remove(MutexPool *mpool, uint midx)
{
    if (midx > 0 && midx < MAX_MUTEXES && mpool->used[midx]) {
        mpool->used[midx] = FALSE;
    }
    else {
        /* nothing to do with nil */
    }
}

void mutex_pool_free(MutexPool *mpool)
{
    uint i;
    if (mpool) {
        for (i = 0; i < mpool->next_empty_slot; i++) {
            europa_mutex_destroy(mpool->mutexes[i]);
        }
        free(mpool);
    }
}

