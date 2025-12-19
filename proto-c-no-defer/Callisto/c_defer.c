#include "callisto.h"
#include <pthread.h>

#ifdef C_MEMORY_DEBUG
#undef defer
#endif
#define MAX_DEFERRED 1024

typedef struct {
  void (*func)(void*);
  void* arg;
} DeferredCall;

static pthread_mutex_t deferredCalls_mutex = PTHREAD_MUTEX_INITIALIZER;
DeferredCall deferredCalls[MAX_DEFERRED];

static pthread_mutex_t deferredCount_mutex = PTHREAD_MUTEX_INITIALIZER;
int deferredCount = 0;

void defer(void (*func)(void*), void* arg)
{
  pthread_mutex_lock(&deferredCalls_mutex);
  pthread_mutex_lock(&deferredCount_mutex);
  if (deferredCount < MAX_DEFERRED)
  {
    deferredCalls[deferredCount].func = func;
    deferredCalls[deferredCount].arg = arg;
    deferredCount++;
  }
  pthread_mutex_unlock(&deferredCalls_mutex);
  pthread_mutex_unlock(&deferredCount_mutex);
}

void run_deferred()
{
  pthread_mutex_lock(&deferredCalls_mutex);
  pthread_mutex_lock(&deferredCount_mutex);
  int i;
  for (i = deferredCount - 1; i >= 0; i--)
  {
    deferredCalls[i].func(deferredCalls[i].arg);
  }
  deferredCount = 0;
  pthread_mutex_unlock(&deferredCalls_mutex);
  pthread_mutex_unlock(&deferredCount_mutex);
}
