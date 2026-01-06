#include "callisto.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef C_MEMORY_DEBUG
#undef defer
#endif
#define MAX_DEFERRED 256

typedef struct
{
    void (*func)(void *);
    void *arg;
} DeferredCall;

struct _DeferredList
{
    DeferredCall *deferredCalls;
    int deferredCount;
};

DeferredList deferred_init(void)
{
    DeferredList deferredCallList;
    deferredCallList.deferredCalls = malloc(sizeof(DeferredCall) * MAX_DEFERRED);
    if (deferredCallList.deferredCalls == NULL)
    {
        perror("Could not initialise deferredCalls array\n.");
        deferredCallList.deferredCount = -1;
        return deferredCallList;
    }
    deferredCallList.deferredCount = 0;
    return deferredCallList;
}

void defer(DeferredList *deferredList, void (*func)(void *), void *arg)
{
    DeferredCall *deferredCalls = deferredList->deferredCalls;
    int *deferredCount = &(deferredList->deferredCount);
    if (*deferredCount < MAX_DEFERRED)
    {
        deferredCalls[*deferredCount].func = func;
        deferredCalls[*deferredCount].arg = arg;
        (*deferredCount)++;
    }
    else
    {
        printf("_DeferredList full! Could not defer function!\n");
    }
}

void deferred_run(DeferredList *deferredList)
{
    int i;
    DeferredCall *deferredCalls = deferredList->deferredCalls;
    int *deferredCount = &(deferredList->deferredCount);

    for (i = 0; i < *deferredCount; i++)
    {
        deferredCalls[i].func(deferredCalls[i].arg);
    }
    free(deferredCalls);
    *deferredCount = 0;
}
