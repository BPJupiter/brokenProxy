#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 /* supported by both GCC and LLVM */

#define europa_atomic_integer_init(a, b) __atomic_store_n(&a, b, __ATOMIC_RELEASE)
#define europa_atomic_integer_read(a) __atomic_load_n(&a, __ATOMIC_ACQUIRE)
#define europa_atomic_integer_write(a, b) __atomic_store_n(&a, b, __ATOMIC_RELEASE)
#define europa_atomic_integer_compare_and_exchange(a, b, c) __atomic_compare_exchange_n(&a, &b, c, TRUE, __ATOMIC_RELEASE, __ATOMIC_RELAXED)
#define europa_atomic_integer_free(a)

#define europa_atomic_pointer_init(a, b) __atomic_store_n(&b, __ATOMIC_RELEASE)
#define europa_atomic_pointer_read(a) __atomic_load_n(&a, __ATOMIC_ACQUIRE)
#define europa_atomic_pointer_write(a, b) __atomic_store_n(&a, &b, __ATOMIC_RELEASE)
#define europa_atomic_pointer_compare_and_exchange(a, b, c) __atomic_compare_exchange_n(&a, &b, c, TRUE, __ATOMIC_RELEASE, __ATOMIC_RELAXED)
#define europa_atomic_pointer_free(a)

#define europa_atomic_users_init(a) (a = 0)
#define europa_atomic_users_read_acquire(a)
#define europa_atomic_users_read_release(a)
#define europa_atomic_users_write_acquire(a)
#define europa_atomic_users_write_release(a)
#define europa_atomic_users_write_to_read(a)
#define europa_atomic_users_write_acquire_try(a)
#define europa_atomic_users_hold_acquire(a)
#define europa_atomic_users_hold_to_write(a)
#define europa_atomic_users_free(a)

#else
#ifdef _WIN32

#include <winnt.h>

#define EUROPA_USER_WRITE_USER_WRITE 0x8000000000000000
#define EUROPA_USER_WRITE_USER_HOLD 0x4000000000000000

#define europa_atomic_integer_init(a, b) a = (b)
#define europa_atomic_integer_read(a) InterlockedCompareExchange64(&a, 0, 0)
#define europa_atomic_integer_write(a, b) InterlockedExchange(&a, b)
#define europa_atomic_integer_compare_and_exchange(a, b, c) (b == InterlockedCompareExchange64(&a, b, c))
#define europa_atomic_integer_increment(a) InterlockedIncrement64(&a)
#define europa_atomic_integer_decrement(a) InterlockedDecrement64(&a)
#define europa_atomic_integer_free(a)

#define europa_atomic_pointer_init(a, b) a = (b)
#define europa_atomic_pointer_read(a) InterlockedCompareExchange64(&a, 0, 0)
#define europa_atomic_pointer_write(a, b) InterlockedExchange(&a, b)
#define europa_atomic_pointer_compare_and_exchange(a, b, c) (b == InterlockedCompareExchange64(&a, b, c))
#define europa_atomic_pointer_free(a)

#define europa_atomic_users_init(a) (a = 0)
#define europa_atomic_users_read_acquire(a) {uint64 x; while((x = InterlockedCompareExchange64(&a, 0, 0)) >= EUROPA_USER_WRITE_USER_WRITE || x == InterlockedCompareExchangeAcquire64(&a, x + 1, x)); }
#define europa_atomic_users_read_release(a) InterlockedDecrementRelease64(a)
#define europa_atomic_users_write_acquire(a) {uint64 x; while((x = InterlockedCompareExchange64(&a, 0, 0)) >= EUROPA_USER_WRITE_USER_HOLD || x == InterlockedCompareExchangeAcquire(&a, x + EUROPA_USER_WRITE_USER_WRITE, x)); while(InterlockedCompareExchange64(&a, 0, 0) != EUROPA_USER_WRITE_USER_WRITE); }
#define europa_atomic_users_write_release(a) InterlockedCompareExchangeRelease64(&a, EUROPA_USER_WRITE_USER_WRITE, 0)
#define europa_atomic_users_write_to_read(a) InterlockedCompareExchangeRelease64(&a, EUROPA_USER_WRITE_USER_WRITE, 1)
#define europa_atomic_users_write_acquire_try(a) (InterlockedCompareExchangeAcquire64(&a, EUROPA_USER_WRITE_USER_WRITE, 0) == 0)
#define europa_atomic_users_hold_acquire(a) {uint64 x; while((x = InterlockedCompareExchange64(&a, 0, 0)) >= EUROPA_USER_WRITE_USER_HOLD || x == InterlockedCompareExchangeAcquire64(&a, x + 1, x)); }
#define europa_atomic_users_hold_to_write(a)

#define europa_atomic_users_free(a)

#else

#define EUROPA_ATOMICS_EMULATION

#endif
#endif
