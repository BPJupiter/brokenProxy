#ifndef BASE_CORE_H
#define BASE_CORE_H

///////////////////////
// Foreign Includes

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

///////////////////////////
// Third Party Includes

#define STB_SPRINTF_DECORATE(name) bplib_##name
#define STB_SPRINTF_STATIC
#include "third_party/stb/stb_sprintf.h"

////////////////////////
// Codebase Keywords

#define internal      static
#define global        static
#define local_persist static

#if COMPILER_MSVC || (COMPILER_CLANG && OS_WINDOWS)
# pragma section(".rdata$", read)
# define read_only __declspec(allocate(".rdata$"))
#elif (COMPILER_CLANG && OS_LINUX)
# define read_only __attribute__((section(".rodata")))
#else
// NOTE(rjf): I don't know of a useful way to do this in GCC land.
// __attribute__((section(".rodata"))) looked promising, but it introduces a
// strange warning about malformed section attributes, and it doesn't look
// like writing to that section reliably produces access violations, strangely
// enough. (It does on Clang)
# define read_only
#endif

#if COMPILER_MSVC
# define thread_static __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
# define thread_static __thread
#else
# error thread_static not defined on this compiler.
#endif

#if COMPILER_MSVC
# define force_inline __forceinline
#elif COMPILER_CLANG || COMPILER_GCC
# define force_inline __attribute__((always_inline))
#else
# error force_inline is not defined for this compiler.
#endif

#if COMPILER_MSVC
# define no_inline __declspec(noinline)
#elif COMPILER_CLANG || COMPILER_GCC
# define no_inline __attribute__((noinline))
#else
# error no_inline not defined for this compiler.
#endif

/////////////////////////////
// Linkage Keyword Macros

#if OS_WINDOWS
# define shared_function C_LINKAGE __declspec(dllexport)
#else
# define shared_function C_LINKAGE
#endif

#if LANG_CPP
# define C_LINKAGE_BEGIN extern "C"{
# define C_LINKAGE_END }
# define C_LINKAGE extern "C"
#else
# define C_LINKAGE_BEGIN
# define C_LINKAGE_END
# define C_LINKAGE
#endif

////////////////////////////
// Optimisation Settings

#if COMPILER_MSVC
# define OPTIMIZE_BEGIN _Pragma("optimize(\"\", on)")
# define OPTIMIZE_END _Pragma("optimize(\"\", off)")
#elif COMPILER_CLANG
# define OPTIMIZE_BEGIN _Pragma("clang optimize on")
# define OPTIMIZE_END _Pragma("clang optimize off")
#elif COMPILER_GCC
# define OPTIMIZE_BEGIN _Pragma("GCC push_options") _Pragma("GCC optimize(\"O2\")")
# define OPTIMIZE_END _Pragma("GCC pop_options")
#else
# define OPTIMIZE_BEGIN
# define OPTIMIZE_END
#endif

#if COMPILER_MSVC && !BUILD_DEBUG
# define NO_OPTIMIZE_BEGIN _Pragma("optimize(\"\", off)")
# define NO_OPTIMIZE_END _Pragma("optimize(\"\", on)")
#elif COMPILER_CLANG && !BUILD_DEBUG
# define NO_OPTIMIZE_BEGIN _Pragma("clang optimize off")
# define NO_OPTIMIZE_END _Pragma("clang optimize on")
#elif COMPILER_GCC && !BUILD_DEBUG
# define NO_OPTIMIZE_BEGIN _Pragma("GCC push_options") _Pragma("GCC optimize(\"O0\")")
# define NO_OPTIMIZE_END _Pragma("GCC pop_options")
#else
# define NO_OPTIMIZE_BEGIN
# define NO_OPTIMIZE_END
#endif

///////////////
// Versions

#define Version(major, minor, patch) (u64)((((u64)(major) & 0xffff) << 32) | ((((u64)(minor) & 0xffff) << 16)) | ((((u64)(patch) & 0xffff) << 0)))
#define MajorFromVersion(version) (((version) & 0xffff00000000ull) >> 32)
#define MinorFromVersion(version) (((version) & 0x0000ffff0000ull) >> 16)
#define PatchFromVersion(version) (((version) & 0x00000000ffffull) >> 0)

////////////
// Units

#define Kilobytes(n) (((u64)(n)) << 10)
#define Megabytes(n) (((u64)(n)) << 20)
#define Gigabytes(n) (((u64)(n)) << 30)
#define Terabytes(n) (((u64)(n)) << 40)
#define Thousand(n)  ((n)*1000)
#define Million(n)   ((n)*1000000)
#define Billion(n)   ((n)*1000000000)

/////////////////////////////
// Branch Hredictor Hints

#if defined(__clang__)
# define Expect(expr, val) __builtin_except((expr), (val))
#else
# define Expect(expr, val) (expr)
#endif

#define Likely(expr)   Expect(expr,1)
#define Unlikely(expr) Expect(expr,0)

/////////////
// Clamps

#define Min(A,B) (((A)<(B))?(A):(B))
#define Max(A,B) (((A)>(B))?(A):(B))
#define ClampTop(A,X) Min(A,X)
#define ClampBot(X,B) Max(X,B)
#define Clamp(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

////////////////////////
// Type -> Alignment

#if COMPILER_MSVC
# define AlignOf(T) __alignof(T)
#elif COMPILER_CLANG
# define AlignOf(T) __alignof(T)
#elif COMPILER_GCC
# define AlignOf(T) __alignof__(T)
#else
# error AlignOf not defined for this compiler.
#endif

#if COMPILER_MSVC
# define AlignType(x) __declspec(align(x))
#elif COMPILER_CLANG || COMPILER_GCC
# define AlignType(x) __attribute__((aligned(x)))
#else
# error AlignType not defined for this compiler.
#endif

/////////////////////
// Member offsets

#define Member(T,m)                 (((T*)0)->m)
#define OffsetOf(T,m)               offsetof(T, m)
#define MemberFromOffset(T,ptr,off) (T)((((u8 *)ptr)+(off)))
#define MemberFromPtr(T,ptr,m)      (void*)((((u8 *)ptr)+OffsetOf(T,m)))
#define CastFromMember(T,m,ptr)     (T*)(((u8*)ptr) - OffsetOf(T,m))

////////////////////////////////
// For-loop construct macros

#define DeferLoop(begin, end)        for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define DeferLoopChecked(begin, end) for(int _i_ = 2 * !(begin); (_i_ == 2 ? ((end), 0) : !_i_); _i_ += 1, (end))

#define EachIndex(it, count)         (u64 it = 0; it < (count); it += 1)
#define EachElement(it, array)       (u64 it = 0; it < ArrayCount(array); it += 1)
#define EachEnumVal(type, it)        (type it = (type)0; it < type##_COUNT; it = (type)(it+1))
#define EachNonZeroEnumVal(type, it) (type it = (type)1; it < type##_COUNT; it = (type)(it+1))
#define EachInRange(it, range)       (u64 it = (range).min; it < (range).max; it += 1)
#define EachNode(it, T, first)       (T *it = first; it != 0; it = it->next)
#define EachBit(it, flags)           (u64 (_i_) = (flags), it = (flags) & -(flags); (_i_) != 0; (_i_) &= ((_i_) - 1), it = (flags) & -(flags))

//////////////////////////////
// Memory operation macros

#define MemoryCopy(dst, src, size)     memmove((dst), (src), (size))
#define MemorySet(dst, byte, size)     memset((dst), (byte), (size))
#define MemoryCompare(a, b, size)      memcmp((a), (b), (size))

#define MemoryCopyStruct(d,s)  MemoryCopy((d),(s),sizeof(*(d)))
#define MemoryCopyArray(d,s)   MemoryCopy((d),(s),sizeof(d))
#define MemoryCopyTyped(d,s,c) MemoryCopy((d),(s),sizeof(*(d))*(c))
#define MemoryCopyStr8(dst, s) MemoryCopy(dst, (s).str, (s).size)

#define MemoryZero(s,z)       memset((s),0,(z))
#define MemoryZeroStruct(s)   MemoryZero((s),sizeof(*(s)))
#define MemoryZeroArray(a)    MemoryZero((a),sizeof(a))
#define MemoryZeroTyped(m,c)  MemoryZero((m),sizeof(*(m))*(c))

#define MemoryMatch(a,b,z)     (MemoryCompare((a),(b),(z)) == 0)
#define MemoryMatchStruct(a,b)  MemoryMatch((a),(b),sizeof(*(a)))
#define MemoryMatchArray(a,b)   MemoryMatch((a),(b),sizeof(a))

#define MemoryIsZeroStruct(ptr) memory_is_zero((ptr), sizeof(*(ptr)))

//////////////
// Asserts

#if COMPILER_MSVC
# define Trap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
# define Trap() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define AssertAlways(x) do{if(!(x)) {Trap();}}while(0)
#if BUILD_DEBUG
# define Assert(x) AssertAlways(x)
#else
# define Assert(x) (void)(x)
#endif
#define InvalidPath        Assert(!"Invalid Path!")
#define NotImplemented     Assert(!"Not Implemented!")
#define NoOp               ((void)0)
#define StaticAssert(C, ID) global u8 Concat(ID, __LINE__)[(C)?1:-1]

//////////////
// Atomics

#if COMPILER_MSVC
# include <intrin.h>
# if ARCH_X64
#  define ins_atomic_u128_eval_cond_assign(x,k,c) (bool32)_InterlockedCompareExchange128((__int64 *)(x), ((__int64 *)&(k))[1], ((__int64 *)&(k))[0], (__int64 *)(c))
#  define ins_atomic_u64_eval(x)                  (u64)__iso_volatile_load64((__int64*)(x))
#  define ins_atomic_u64_inc_eval(x)              _InterlockedIncrement64((__int64 *)(x))
#  define ins_atomic_u64_dec_eval(x)              _InterlockedDecrement64((__int64 *)(x))
#  define ins_atomic_u64_eval_assign(x,c)         _InterlockedExchange64((__int64 *)(x), (c))
#  define ins_atomic_u64_add_eval(x,c)            _interlockedadd64((__int64 *)(x), (c))
#  define ins_atomic_u64_eval_cond_assign(x,k,c)  _InterlockedCompareExchange64((__int64 *)(x), (k), (c))
#  define ins_atomic_u32_eval(x)                  (u32)__iso_volatile_load32((__int32*)(x))
#  define ins_atomic_u32_inc_eval(x)              _InterlockedIncrement((long *)(x))
#  define ins_atomic_u32_dec_eval(x)              _InterlockedDecrement((long *)(x))
#  define ins_atomic_u32_eval_assign(x,c)         _InterlockedExchange((long *)(x), (c))
#  define ins_atomic_u32_eval_cond_assign(x,k,c)  _InterlockedCompareExchange((long *)(x), (k), (c))
#  define ins_atomic_u32_add_eval(x,c)            _interlockedadd((long *)(x), (c))
#  define ins_atomic_u8_eval_assign(x,c)          _InterlockedExchange8((char *)(x), (char)(c))
#  define ins_atomic_u8_or(x,c)                   _InterlockedOr8((char *)(x), (char)(c))
#  define ins_atomic_u32_or(x,c)                  _InterlockedOr((long *)(x), (long)(c))
# else
#  error Atomic intrinsics not defined for this compiler / architecture combination.
# endif
#elif COMPILER_CLANG || COMPILER_GCC
# include <immintrin.h>
# include <emmintrin.h>
#  define ins_atomic_u128_eval_cond_assign(x,k,c) (bool32)__atomic_compare_exchange_n((__int128 *)(x),(__int128 *)(c),*(__int128 *)(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)
#  define ins_atomic_u64_eval(x)                  __atomic_load_n((u64 *)(x), __ATOMIC_SEQ_CST)
#  define ins_atomic_u64_inc_eval(x)              __atomic_add_fetch((u64 *)(x), 1, __ATOMIC_SEQ_CST)
#  define ins_atomic_u64_dec_eval(x)              __atomic_sub_fetch((u64 *)(x), 1, __ATOMIC_SEQ_CST)
#  define ins_atomic_u64_eval_assign(x,c)         __atomic_exchange_n(x, c, __ATOMIC_SEQ_CST)
#  define ins_atomic_u64_add_eval(x,c)            __atomic_add_fetch((u64 *)(x), c, __ATOMIC_SEQ_CST)
#  define ins_atomic_u64_eval_cond_assign(x,k,c)  ({ u64 _new = (c); __atomic_compare_exchange_n((u64 *)(x),&_new,(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST); _new; })
#  define ins_atomic_u32_eval(x)                  __atomic_load_n(x, __ATOMIC_SEQ_CST)
#  define ins_atomic_u32_inc_eval(x)              __atomic_add_fetch((u32 *)(x), 1, __ATOMIC_SEQ_CST)
#  define ins_atomic_u32_dec_eval(x)              __atomic_sub_fetch((u32 *)(x), 1, __ATOMIC_SEQ_CST)
#  define ins_atomic_u32_add_eval(x,c)            __atomic_add_fetch((u32 *)(x), c, __ATOMIC_SEQ_CST)
#  define ins_atomic_u32_eval_assign(x,c)         __atomic_exchange_n((x), (c), __ATOMIC_SEQ_CST)
#  define ins_atomic_u32_eval_cond_assign(x,k,c)  ({ u32 _new = (c); __atomic_compare_exchange_n((u32 *)(x),&_new,(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST); _new; })
#  define ins_atomic_u8_eval_assign(x,c)          __atomic_exchange_n((x), (c), __ATOMIC_SEQ_CST)
#  define ins_atomic_u8_or(x,c)                   __atomic_fetch_or((u8 *)(x), (u8)(c), __ATOMIC_SEQ_CST)
#  define ins_atomic_u32_or(x,c)                  __atomic_fetch_or((u32 *)(x), (u32)(c), __ATOMIC_SEQ_CST)
#else
#  error Atomic intrinsics not defined for this compiler / architecture.
#endif

#if ARCH_64BIT
# define ins_atomic_ptr_eval_cond_assign(x,k,c) (void *)ins_atomic_u64_eval_cond_assign((u64 *)(x), (u64)(k), (u64)(c))
# define ins_atomic_ptr_eval_assign(x,c)        (void *)ins_atomic_u64_eval_assign((u64 *)(x), (u64)(c))
# define ins_atomic_ptr_eval(x)                 (void *)ins_atomic_u64_eval((u64 *)x)
#else
# error Atomic intrinsics for pointers not defined for this architecture.
#endif

//////////////////////////////////
// Linked List Building Macros

// Liniked List macro helpers
#define CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define SetNil(nil,p) ((p) = nil)

// Doubly Linked lists
#define DLLInsert_NPZ(nil,f,l,p,n,next,prev) (CheckNil(nil,f) ? \
((f) = (l) = (n), SetNil(nil,(n)->next), SetNil(nil,(n)->prev)) :\
CheckNil(nil,p) ? \
((n)->next = (f), (f)->prev = (n), (f) = (n), SetNil(nil,(n)->prev)) :\
((p)==(l)) ? \
((l)->next = (n), (n)->prev = (l), (l) = (n), SetNil(nil, (n)->next)) :\
(((!CheckNil(nil,p) && CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define DLLPushBack_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define DLLPushFront_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
((n) == (l) ? (l) = (l)->prev : (0)),\
(CheckNil(nil,(n)->prev) ? (0) :\
((n)->prev->next = (n)->next)),\
(CheckNil(nil,(n)->next) ? (0) :\
((n)->next->prev = (n)->prev)))

//- rjf: singly-linked, doubly-headed lists (queues)
#define SLLQueuePush_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
((f)=(l)=(n),SetNil(nil,(n)->next)):\
((l)->next=(n),(l)=(n),SetNil(nil,(n)->next)))
#define SLLQueuePushFront_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
((f)=(l)=(n),SetNil(nil,(n)->next)):\
((n)->next=(f),(f)=(n)))
#define SLLQueuePop_NZ(nil,f,l,next) ((f)==(l)?\
(SetNil(nil,f),SetNil(nil,l)):\
((f)=(f)->next))

//- rjf: singly-linked, singly-headed lists (stacks)
#define SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define SLLStackPop_N(f,next) ((f)=(f)->next)

//- rjf: doubly-linked-list helpers
#define DLLInsert_NP(f,l,p,n,next,prev) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack_NP(f,l,n,next,prev) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront_NP(f,l,n,next,prev) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove_NP(f,l,n,next,prev) DLLRemove_NPZ(0,f,l,n,next,prev)
#define DLLInsert(f,l,p,n) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack(f,l,n) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront(f,l,n) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove(f,l,n) DLLRemove_NPZ(0,f,l,n,next,prev)

//- rjf: singly-linked, doubly-headed list helpers
#define SLLQueuePush_N(f,l,n,next) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront_N(f,l,n,next) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop_N(f,l,next) SLLQueuePop_NZ(0,f,l,next)
#define SLLQueuePush(f,l,n) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront(f,l,n) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop(f,l) SLLQueuePop_NZ(0,f,l,next)

//- rjf: singly-linked, singly-headed list helpers
#define SLLStackPush(f,n) SLLStackPush_N(f,n,next)
#define SLLStackPop(f) SLLStackPop_N(f,next)

// Address Sanitizer markup

#if COMPILER_MSVC
# if defined(__SANITIZE_ADDRESS__)
#  define ASAN_ENABLED 1
#  define ASAN_NO_ADDR __declspec(no_sanitize_address)
# endif
#elif COMPILER_CLANG
# if defined(__has_feature)
#  if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#   define ASAN_ENABLED 1
#  endif
# endif
# define ASAN_NO_ADDR   __attribute__((no_sanitize("address")))
# define UBSAN_NO_ALIGN __attribute__((no_sanitize("alignment")))
#endif

#ifndef  ASAN_NO_ADDR
# define ASAN_NO_ADDR
#endif
#ifndef  UBSAN_NO_ALIGN
# define UBSAN_NO_ALIGN
#endif

#if ASAN_ENABLED
C_LINKAGE void __asan_poison_memory_region(void const volatile *addr, size_t size);
C_LINKAGE void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
# define AsanPoisonMemoryRegion(addr, size)   __asan_poison_memory_region((addr), (size))
# define AsanUnpoisonMemoryRegion(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
# define AsanPoisonMemoryRegion(addr, size)   ((void)(addr), (void)(size))
# define AsanUnpoisonMemoryRegion(addr, size) ((void)(addr), (void)(size))
#endif

//////////////////////////
// Misc. Helper Macros

#define Stringify_(S) #S
#define Stringify(S) Stringify_(S)

#define Concat_(A,B) A##B
#define Concat(A,B) Concat_(A,B)

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define CeilIntegerDiv(a,b) (((a) + (b) - 1)/(b))

#define Swap(T,a,b) do{T t__ = a; a = b; b = t__;}while(0)

#if ARCH_64BIT
# define IntFromPtr(ptr) ((u64)(ptr))
#elif ARCH_32BIT
# define IntFromPtr(ptr) ((u32)(ptr))
#else
# error Missing pointer-to-integer cast for this architecture.
#endif
#define PtrFromInt(i) (void*)(i)

#define Compose64Bit(a,b)  ((((u64)a) << 32) | ((u64)b))
#define Compose32Bit(a,b)  ((((u32)a) << 16) | ((u32)b))
#define AlignPow2(x,b)     (((x) + (b) - 1)&(~((b) - 1)))
#define AlignDownPow2(x,b) ((x)&(~((b) - 1)))
#define AlignPadPow2(x,b)  ((0-(x)) & ((b) - 1))
#define IsPow2(x)          ((x)!=0 && ((x)&((x)-1))==0)
#define IsPow2OrZero(x)    ((((x) - 1)&(x)) == 0)

#define ExtractBit(word, idx) (((word) >> (idx)) & 1)
#define Extract8(word, pos)   (((word) >> ((pos)*8))  & max_u8)
#define Extract16(word, pos)  (((word) >> ((pos)*16)) & max_u16)
#define Extract32(word, pos)  (((word) >> ((pos)*32)) & max_u32)

#if LANG_CPP
# define zero_struct {}
#else
# define zero_struct {0}
#endif

/////////////////
// Base types

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef u8       byte;
typedef s8       bool8;
typedef s16      bool16;
typedef s32      bool32;
typedef s64      bool64;
#undef true
#define true 1
#undef false
#define false 0
typedef float    f32;
typedef double   f64;
typedef void Void_Proc(void);
typedef union u128 u128;
union u128 {
    u8  u8[16];
    u16 u16[8];
    u32 u32[4];
    u64 u64[2];
    f32 f32[4];
    f64 f64[2];
};
typedef union u256 u256;
union u256 {
    u8  u8[32];
    u16 u16[16];
    u32 u32[8];
    u64 u64[4];
    u128 u128[2];
    f32 f32[8];
    f64 f64[4];
};
typedef union u512 u512;
union u512 {
    u8  u8[64];
    u16 u16[32];
    u32 u32[16];
    u64 u64[8];
    u128 u128[4];
    u256 u256[2];
    f32 f32[16];
    f64 f64[8];
};
#pragma pack(push, 1)
typedef struct u80 u80;
struct u80 {
    u64 int1_frac63;
    u16 sign1_exp15;
};
#pragma pack(pop)

////////////////////////////
// Basic type structures

// Arrays

typedef struct u16_Array u16_Array;
struct u16_Array {
    u16 *v;
    u64 count;
};

typedef struct u32_Array u32_Array;
struct u32_Array {
    u32 *v;
    u64 count;
};

typedef struct u64_Array u64_Array;
struct u64_Array {
    u64 *v;
    u64 count;
};

typedef struct u128_Array u128_Array;
struct u128_Array {
    u128 *v;
    u64 count;
};

///////////////////////////
// Basic Types & Spaces

typedef enum Dimension {
    Dimension_X,
    Dimension_Y,
    Dimension_Z,
    Dimension_W,
} Dimension;

typedef enum Side {
    Side_Invalid = -1,
    Side_Min,
    Side_Max,
    Side_COUNT,
} Side;
#define side_flip(s) ((Side)(!(s)))

typedef enum Axis2 {
    Axis2_Invalid = -1,
    Axis2_X,
    Axis2_Y,
    Axis2_COUNT,
} Axis;
#define axis2_flip(a) ((Axis2)(!(a)))

typedef enum Corner {
    Corner_Invalid = -1,
    Corner_00,
    Corner_01,
    Corner_10,
    Corner_11,
    Corner_COUNT
} Corner;

typedef enum Dir2 {
    Dir2_Invalid = -1,
    Dir2_Left,
    Dir2_Up,
    Dir2_Right,
    Dir2_Down,
    Dir2_COUNT
} Dir2;
#define axis2_from_dir2(d) (((d) & 1) ? Axis2_Y : Axis2_X)
#define side_from_dir2(d) (((d) < Dir2_Right) ? Side_Min : Side_Max)

//////////////////////////////////
// Toolchain/Environment enums

typedef enum OperatingSystem {
    OperatingSystem_Null,
    OperatingSystem_Windows,
    OperatingSystem_Linux,
    OperatingSystem_Mac,
    OperatingSystem_COUNT,
#if OS_WINDOWS
    OperatingSystem_CURRENT = OperatingSystem_Windows,
#elif OS_LINUX
    OperatingSystem_CURRENT = OperatingSystem_Linux,
#elif OS_MAC
    OperatingSystem_CURRENT = OperatingSystem_Mac,
#else
    OperatingSystem_CURRENT = OperatingSystem_Null,
#endif
} OperatingSystem;

typedef enum ExecutableImageKind {
    ExecutableImageKind_Null,
    ExecutableImageKind_CoffPe,
    ExecutableImageKind_Elf32,
    ExecutableImageKind_Elf64,
    ExecutableImageKind_Macho,
    ExecutableImageKind_COUNT
} ExecutableImageKind;

typedef enum Arch {
    Arch_Null,
    Arch_x64,
    Arch_x86,
    Arch_arm64,
    Arch_arm32,
    Arch_COUNT,
#if ARCH_X64
    Arch_CURRENT = Arch_x64,
#elif ARCH_X86
    Arch_CURRENT = Arch_x86,
#elif ARCH_ARM64
    Arch_CURRENT = Arch_arm64,
#elif ARCH_ARM32
    Arch_CURRENT = Arch_arm32,
#else
    Arch_CURRENT = Arch_Null,
#endif
} Arch;

typedef enum Compiler {
    Compiler_Null,
    Compiler_msvc,
    Compiler_gcc,
    Compiler_clang,
    Compiler_COUNT,
#if COMPILER_MSVC
    Compiler_CURRENT = Compiler_msvc,
#elif COMPILER_GCC
    Compiler_CURRENT = Compiler_gcc,
#elif COMPILER_CLANG
    Compiler_CURRENT = Compiler_clang,
#else
    Compiler_CURRENT = Compiler_Null,
#endif
} Compiler;

typedef enum Linker {
    Linker_Null,
    Linker_msvc,
    Linker_lld,
    Linker_COUNT,
} Linker;

///////////////////
// Access flags

typedef u32 AccessFlags;
enum
{
    AccessFlag_Read        = (1<<0),
    AccessFlag_Write       = (1<<1),
    AccessFlag_Execute     = (1<<2),
    AccessFlag_Append      = (1<<3),
    AccessFlag_ShareRead   = (1<<4),
    AccessFlag_ShareWrite  = (1<<5),
    AccessFlag_Inherited   = (1<<6),
};

/////////////////////////////////////
// Text 2D Croodinates and Ranges

typedef struct TxtPt TxtPt;
struct TxtPt {
    s64 line;
    s64 column;
};

typedef struct TxtRng TxtRng;
struct TxtRng {
    TxtPt min;
    TxtPt max;
};

//////////////////////////
// Globally Unique Ids

typedef union Guid Guid;
union Guid {
    struct {
        u32 data1;
        u16 data2;
        u16 data3;
        u8  data4[8];
    };
    u8 v[16];
};
StaticAssert(sizeof(Guid) == 16, g_guide_size_check);

//////////////////
// Machine Ops

typedef enum MachineOpResult
{
  MachineOpResult_Null,
  MachineOpResult_Ok,
  MachineOpResult_Fail,
  MachineOpResult_Maybe,
} MachineOpResult;

#define MACHINE_OP_REG_READ(name) MachineOpResult name(u64 reg_id, void *buffer, u64 buffer_max, void *ud)
typedef MACHINE_OP_REG_READ(MachineOp_RegRead);

#define MACHINE_OP_REG_WRITE(name) MachineOpResult name(u64 reg_id, void *value, u64 value_size, void *ud)
typedef MACHINE_OP_REG_WRITE(MachineOp_RegWrite);

#define MACHINE_OP_MEM_READ(name) MachineOpResult name(u64 addr, void *buffer, u64 buffer_size, void *ud)
typedef MACHINE_OP_MEM_READ(MachineOp_MemRead);

#define MACHINE_OP_MEM_WRITE(name) MachineOpResult name(u64 addr, void *value, u64 value_size, void *ud)
typedef MACHINE_OP_MEM_WRITE(MachineOp_MemWrite);

//////////////////////
// Basic constants

global u32 sign32     = 0x80000000;
global u32 exponent32 = 0x7F800000;
global u32 mantissa32 = 0x007FFFFF;

global f32   big_golden32 = 1.61803398875f;
global f32 small_golden32 = 0.61803398875f;

global f32 pi32 = 3.1415926535897f;

global f64 machine_epsilon64 = 4.94065645841247e-324;

global u64 max_u64 = 0xffffffffffffffffull;
global u32 max_u32 = 0xffffffff;
global u16 max_u16 = 0xffff;
global u8  max_u8  = 0xff;

global s64 max_s64 = (s64)0x7fffffffffffffffll;
global s32 max_s32 = (s32)0x7fffffff;
global s16 max_s16 = (s16)0x7fff;
global s8  max_s8  =  (s8)0x7f;

global s64 min_s64 = (s64)0x8000000000000000ll;
global s32 min_s32 = (s32)0x80000000;
global s16 min_s16 = (s16)0x8000;
global s8  min_s8  =  (s8)0x80;

global const u32 bitmask1  = 0x00000001;
global const u32 bitmask2  = 0x00000003;
global const u32 bitmask3  = 0x00000007;
global const u32 bitmask4  = 0x0000000f;
global const u32 bitmask5  = 0x0000001f;
global const u32 bitmask6  = 0x0000003f;
global const u32 bitmask7  = 0x0000007f;
global const u32 bitmask8  = 0x000000ff;
global const u32 bitmask9  = 0x000001ff;
global const u32 bitmask10 = 0x000003ff;
global const u32 bitmask11 = 0x000007ff;
global const u32 bitmask12 = 0x00000fff;
global const u32 bitmask13 = 0x00001fff;
global const u32 bitmask14 = 0x00003fff;
global const u32 bitmask15 = 0x00007fff;
global const u32 bitmask16 = 0x0000ffff;
global const u32 bitmask17 = 0x0001ffff;
global const u32 bitmask18 = 0x0003ffff;
global const u32 bitmask19 = 0x0007ffff;
global const u32 bitmask20 = 0x000fffff;
global const u32 bitmask21 = 0x001fffff;
global const u32 bitmask22 = 0x003fffff;
global const u32 bitmask23 = 0x007fffff;
global const u32 bitmask24 = 0x00ffffff;
global const u32 bitmask25 = 0x01ffffff;
global const u32 bitmask26 = 0x03ffffff;
global const u32 bitmask27 = 0x07ffffff;
global const u32 bitmask28 = 0x0fffffff;
global const u32 bitmask29 = 0x1fffffff;
global const u32 bitmask30 = 0x3fffffff;
global const u32 bitmask31 = 0x7fffffff;
global const u32 bitmask32 = 0xffffffff;

global const u64 bitmask33 = 0x00000001ffffffffull;
global const u64 bitmask34 = 0x00000003ffffffffull;
global const u64 bitmask35 = 0x00000007ffffffffull;
global const u64 bitmask36 = 0x0000000fffffffffull;
global const u64 bitmask37 = 0x0000001fffffffffull;
global const u64 bitmask38 = 0x0000003fffffffffull;
global const u64 bitmask39 = 0x0000007fffffffffull;
global const u64 bitmask40 = 0x000000ffffffffffull;
global const u64 bitmask41 = 0x000001ffffffffffull;
global const u64 bitmask42 = 0x000003ffffffffffull;
global const u64 bitmask43 = 0x000007ffffffffffull;
global const u64 bitmask44 = 0x00000fffffffffffull;
global const u64 bitmask45 = 0x00001fffffffffffull;
global const u64 bitmask46 = 0x00003fffffffffffull;
global const u64 bitmask47 = 0x00007fffffffffffull;
global const u64 bitmask48 = 0x0000ffffffffffffull;
global const u64 bitmask49 = 0x0001ffffffffffffull;
global const u64 bitmask50 = 0x0003ffffffffffffull;
global const u64 bitmask51 = 0x0007ffffffffffffull;
global const u64 bitmask52 = 0x000fffffffffffffull;
global const u64 bitmask53 = 0x001fffffffffffffull;
global const u64 bitmask54 = 0x003fffffffffffffull;
global const u64 bitmask55 = 0x007fffffffffffffull;
global const u64 bitmask56 = 0x00ffffffffffffffull;
global const u64 bitmask57 = 0x01ffffffffffffffull;
global const u64 bitmask58 = 0x03ffffffffffffffull;
global const u64 bitmask59 = 0x07ffffffffffffffull;
global const u64 bitmask60 = 0x0fffffffffffffffull;
global const u64 bitmask61 = 0x1fffffffffffffffull;
global const u64 bitmask62 = 0x3fffffffffffffffull;
global const u64 bitmask63 = 0x7fffffffffffffffull;
global const u64 bitmask64 = 0xffffffffffffffffull;

global const u32 bit1  = (1<<0);
global const u32 bit2  = (1<<1);
global const u32 bit3  = (1<<2);
global const u32 bit4  = (1<<3);
global const u32 bit5  = (1<<4);
global const u32 bit6  = (1<<5);
global const u32 bit7  = (1<<6);
global const u32 bit8  = (1<<7);
global const u32 bit9  = (1<<8);
global const u32 bit10 = (1<<9);
global const u32 bit11 = (1<<10);
global const u32 bit12 = (1<<11);
global const u32 bit13 = (1<<12);
global const u32 bit14 = (1<<13);
global const u32 bit15 = (1<<14);
global const u32 bit16 = (1<<15);
global const u32 bit17 = (1<<16);
global const u32 bit18 = (1<<17);
global const u32 bit19 = (1<<18);
global const u32 bit20 = (1<<19);
global const u32 bit21 = (1<<20);
global const u32 bit22 = (1<<21);
global const u32 bit23 = (1<<22);
global const u32 bit24 = (1<<23);
global const u32 bit25 = (1<<24);
global const u32 bit26 = (1<<25);
global const u32 bit27 = (1<<26);
global const u32 bit28 = (1<<27);
global const u32 bit29 = (1<<28);
global const u32 bit30 = (1<<29);
global const u32 bit31 = (1<<30);
global const u32 bit32 = (1<<31);

global const u64 bit33 = (1ull<<32);
global const u64 bit34 = (1ull<<33);
global const u64 bit35 = (1ull<<34);
global const u64 bit36 = (1ull<<35);
global const u64 bit37 = (1ull<<36);
global const u64 bit38 = (1ull<<37);
global const u64 bit39 = (1ull<<38);
global const u64 bit40 = (1ull<<39);
global const u64 bit41 = (1ull<<40);
global const u64 bit42 = (1ull<<41);
global const u64 bit43 = (1ull<<42);
global const u64 bit44 = (1ull<<43);
global const u64 bit45 = (1ull<<44);
global const u64 bit46 = (1ull<<45);
global const u64 bit47 = (1ull<<46);
global const u64 bit48 = (1ull<<47);
global const u64 bit49 = (1ull<<48);
global const u64 bit50 = (1ull<<49);
global const u64 bit51 = (1ull<<50);
global const u64 bit52 = (1ull<<51);
global const u64 bit53 = (1ull<<52);
global const u64 bit54 = (1ull<<53);
global const u64 bit55 = (1ull<<54);
global const u64 bit56 = (1ull<<55);
global const u64 bit57 = (1ull<<56);
global const u64 bit58 = (1ull<<57);
global const u64 bit59 = (1ull<<58);
global const u64 bit60 = (1ull<<59);
global const u64 bit61 = (1ull<<60);
global const u64 bit62 = (1ull<<61);
global const u64 bit63 = (1ull<<62);
global const u64 bit64 = (1ull<<63);

/////////////////
// Time types

typedef enum WeekDay {
    WeekDay_Sun,
    WeekDay_Mon,
    WeekDay_Tue,
    WeekDay_Wed,
    WeekDay_Thu,
    WeekDay_Fri,
    WeekDay_Sat,
    WeekDay_COUNT,
} WeekDay;

typedef enum Month {
    Month_Jan,
    Month_Feb,
    Month_Mar,
    Month_Apr,
    Month_May,
    Month_Jun,
    Month_Jul,
    Month_Aug,
    Month_Sep,
    Month_Oct,
    Month_Nov,
    Month_Dec,
    Month_COUNT,
} Month;

typedef struct Date_Time Date_Time;
struct Date_Time {
  u16 micro_sec; // [0,999]
  u16 msec; // [0,999]
  u16 sec;  // [0,60]
  u16 min;  // [0,59]
  u16 hour; // [0,24]
  u16 day;  // [0,30]
  union
  {
    WeekDay week_day;
    u32 wday;
  };
  union
  {
    Month month;
    u32 mon;
  };
  u32 year; // 1 = 1 CE, 0 = 1 BC
};

typedef u64 Dense_Time;

/////////////////
// File Types

typedef u32 FilePropertyFlags;
enum {
    FilePropertyFlag_IsFolder = (1 << 0)
};

typedef struct File_Properties File_Properties;
struct File_Properties {
    u64 size;
    Dense_Time modified;
    Dense_Time created;
    FilePropertyFlags flags;
};

/////////////////
// Safe casts

internal u16 safe_cast_u16(u32 x);
internal u32 safe_cast_u32(u64 x);
internal s32 safe_cast_s32(s64 x);

////////////////////////////////
// Large base type functions

internal u128   u128_zero(void);
internal u128   u128_make(u64 v0, u64 v1);
internal bool32 u128_match(u128 a, u128 b);

///////////////////
// bit patterns

internal u32 u32_from_u64_saturate(u64 x);
internal u64 u64_up_to_pow2(u64 x);
internal s32 extend_sign32(u32 x, u32 size);
internal s64 extend_sign64(u64 x, u64 size);

internal f32 inf32(void);
internal f32 neg_inf32(void);

internal u16 bswap_u16(u16 x);
internal u32 bswap_u32(u32 x);
internal u64 bswap_u64(u64 x);

#if ARCH_LITTLE_ENDIAN
# define from_be_u16(x) bswap_u16(x)
# define from_be_u32(x) bswap_u32(x)
# define from_be_u64(x) bswap_u64(x)
#else
# define from_be_u16(x) (x)
# define from_be_u32(x) (x)
# define from_be_u64(x) (x)
#endif

internal u64 count_bits_set32(u32 val);
internal u64 count_bits_set64(u64 val);

internal u64 ctz32(u32 val);
internal u64 ctz64(u64 val);
internal u64 clz32(u32 val);
internal u64 clz64(u64 val);

///////////////////
// Enum -> Sign

internal s32 sign_from_side_s32(Side side);
internal f32 sign_from_side_f32(Side side);

///////////////////////
// Memory functions

internal bool32 memory_is_zero(void *ptr, u64 size);
internal void memory_write16(void *ptr, u16 v);
internal void memory_write32(void *ptr, u32 v);
internal void memory_write64(void *ptr, u64 v);

internal u8  memory_read8(void *ptr);
internal u16 memory_read16(void *ptr);
internal u32 memory_read32(void *ptr);
internal u64 memory_read64(void *ptr);

/////////////////////////////////////////
// Text 2D Coordinate/Range Functions

internal TxtPt txt_pt(s64 line, s64 column);
internal bool32 txt_pt_match(TxtPt a, TxtPt b);
internal bool32 txt_pt_less_than(TxtPt a, TxtPt b);
internal TxtPt txt_pt_min(TxtPt a, TxtPt b);
internal TxtPt txt_pt_max(TxtPt a, TxtPt b);
internal TxtRng txt_rng(TxtPt min, TxtPt max);
internal TxtRng txt_rng_intersect(TxtRng a, TxtRng b);
internal TxtRng txt_rng_union(TxtRng a, TxtRng b);
internal bool32 txt_rng_contains(TxtRng r, TxtPt pt);

//////////////////////////////////////////
// toolchain/environent enum functions

internal u64 bit_size_from_arch(Arch arch);
internal u64 byte_size_from_arch(Arch arch);
internal u64 max_ops_per_instruction_from_arch(Arch arch);
internal u64 min_instruction_size_from_arch(Arch arch);
internal u64 max_instruction_size_from_arch(Arch arch);

/////////////////////
// time functions

internal Dense_Time dense_time_from_date_time(Date_Time date_time);
internal Date_Time date_time_from_dense_time(Dense_Time dense_time);
internal Date_Time date_time_from_micro_seconds(u64 micro_seconds);
internal Date_Time date_time_from_unix_time(u64 unix_time);

////////////////////////////////////////////////
// @per_os_impl Debugger Attachment Checking

internal bool32 debugger_is_attached(void);

// @per_os_impl Time Functions

internal u64 now_time_us(void);
internal u32 now_time_unix(void);
internal Date_Time now_time_universal(void);
internal Date_Time universal_from_local_time(Date_Time *dt);
internal Date_Time local_from_universal_time(Date_Time *dt);
internal void sleep_ms(u32 ms);

///////////////////////////////////////////
// @per_os_impl Platform GUID Functions

internal Guid make_guid(void);

///////////////////////////////////////
// Wrapped Ring Buffer Reads/Writes

internal u64 wrapped_write(u8 *ring_base, u64 ring_size, u64 ring_pos, void *src_data, u64 src_data_size);
internal u64 wrapped_read(u8 *ring_base, u64 ring_size, u64 ring_pos, void *dst_data, u64 read_size);
#define wrapped_write_struct(ring_base, ring_size, ring_pos, ptr) wrapped_write((ring_base), (ring_size), (ring_pos), (ptr), sizeof(*(ptr)))
#define wrapped_read_struct(ring_base, ring_size, ring_pos, ptr) wrapped_read((ring_base), (ring_size), (ring_pos), (ptr), sizeof(*(ptr)))

////////////
// sorts

#define quick_sort(ptr, count, element_size, cmp_function) qsort((ptr), (count), (element_size), (int (*)(const void *, const void *))(cmp_function))

////////////////////////////////

internal u64 array_bsearch(u64 *arr, u64 count, u64 value);

////////////////////////////////

internal u64 index_of_zero_byte64(u8 *ptr, u64 size); // size must be exactly 8 bytes
internal u64 index_of_zero_u32(u32 *ptr, u64 count);
internal u64 index_of_zero_u64(u64 *ptr, u64 count);
internal u64 count_digits_u64(u64 v, u64 radix);

#endif /* BASE_CORE_H */
