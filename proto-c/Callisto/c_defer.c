#include "callisto.h"

#ifdef C_MEMORY_DEBUG
#undef defer
#endif
#define MAX_DEFERRED 1024

typedef struct {
  void (*func)(void*);
  void* arg;
} DeferredCall;

DeferredCall deferredCalls[MAX_DEFERRED];
int deferredCount = 0;

void defer(void (*func)(void*), void* arg)
{
  if (deferredCount < MAX_DEFERRED)
  {
    deferredCalls[deferredCount].func = func;
    deferredCalls[deferredCount].arg = arg;
    deferredCount++;
  }
}

void run_deferred()
{
  int i;
  for (i = deferredCount - 1; i >= 0; i--)
  {
    deferredCalls[i].func(deferredCalls[i].arg);
  }
  deferredCount = 0;
}
