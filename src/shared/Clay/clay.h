#ifndef CLAY_H
#define CLAY_H

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#pragma warning(disable:4703)
#pragma warning(disable:4996)
//#pragma warning(disable:4664) 
#pragma warning(disable:4305)
#pragma warning(disable:4244)
//#pragma warning(disable:4101)
#pragma warning(error:4013)
//#pragma warning(error:4020)
#pragma warning(error:4716)
#pragma warning(error:4554)
#pragma warning(error:4553)
#pragma warning(error:4334)
#pragma warning(error:4431) /* implicit int type */
//#pragma warning(error:4047) // FIX ME
//#pragma warning(error:4774) // Forces printf functions to have a string literal as first argument.
// C4255
//#pragma warning(push, 4)
#endif

//#pragma warning( disable : 4507 34; once : 4385; error : 164 )

#if !defined(TYPES_H)
#define TYPES_H

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif
#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
typedef unsigned int uint;
#else
#include <sys/types.h>
#endif
#if !defined(VERSE_TYPES)
typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef float real32;
typedef double real64;
typedef unsigned char boolean;
#endif
#if defined _WIN32
typedef __int64 int64;
typedef unsigned __int64 int64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif

/* #define F_DOUBLE_PRECISION */

#ifdef F_DOUBLE_PRECISION
typedef double freal;
#else
typedef float freal;
#endif

#define UNUSED(x) (void)(x)
#define PI 3.141592653
#define CLAY_IS_BIG_ENDIAN (*(short *)"\0\xff" < 0x100)

#if defined(DEBUG) || defined (_DEBUG)
#define CLAY_DEBUG_BUILD
#endif

#ifndef CLAY_DEBUG_BUILD
#define CLAY_RELEASE_BUILD
#endif

#if !defined(C_NO_MEMORY_DEBUG)
/* turns on the memory debugging system */
#define C_MEMORY_DEBUG
/*#define C_MEMORY_PRINT */
#endif
#if !defined(C_EXIT_CRASH)
/* turns on  the crash on exit */
/*#define C_EXIT_CRASH*/ 
#endif

#include <stdlib.h>
#include <stdio.h>

/* ----- Debugging -----
 * If C_MEMORY_DEBUG is enabled, the memory debugging system will create macros that replace malloc, free, and realloc, and allows the system to keep track of and report where memory is being allocated, how much and if the memory is being freed. This is very useful for finding memory leaks in large applications. The system can also over allocate memory and fill it with a magic number and can therefore detect if the application writes outside of the allocated memory.
 */

extern void     c_debug_memory_init(void (*lock)(void *mutex), void (*unlock)(void *mutex), void *mutex); /* Required for memory debugger to be thread safe */
extern void 	*c_debug_mem_malloc(size_t size, char *file, uint line);     /* Replaces malloc and records the c file and line where it was called*/
extern void 	*c_debug_mem_realloc(void *pointer, size_t size, char *file, uint line);     /* Replaces realloc and records the c file and line where it was called*/
extern void     c_debug_mem_free(void *buf, char *file, uint line); /* Replaces free and records the c file and line where it was called*/
extern boolean  c_debug_mem_comment(void *buf, char *comment); /* Adds comment to an allocation to help identify its use */
extern void     c_debug_mem_print(uint min_allocs); /* Prints out a list of all allocations made, their location, how much memorey each has allocated, freed, and how many allocations have been made. The min_allocs parameter can be set to avoid printing any allocations that have been made fewer times then min_allocs */
extern void     c_debug_mem_reset(void); /* f_debug_mem_reset allows you to clear all memory stored in the debugging system if you only want to record allocations after a specific point in your code*/
extern size_t   c_debug_mem_consumption(void); /* sum all memory consumed by mallocs and reallocs covered by the memory debugger */
extern boolean  c_debug_mem_query(void *pointer, uint *line, char **file, size_t *size); /* query the size and place of allocation of a pointer */
extern boolean  c_debug_mem_test(void *pointer, size_t size, boolean ignore_not_found); /* query if a bit of memory is safe to access */
extern boolean  c_debug_memory(void); /*f_debug_memory checks if any of the bounds of any allocation has been over written and reports where to standard out. The function returns TRUE if any error was found*/

extern void 	*c_debug_mem_fopen(const char *file_name, const char *mode, char *file, uint line);
extern void     c_debug_mem_fclose(void *f, char *file, uint line);

extern void     c_no_debug_free(void *buf); /* Introduced as some libraries require use of stdlib free on certain lib objects. malloc and realloc calls within these libs are not tracked by the debugging program and as such calls to free normally crash. */

#ifdef C_MEMORY_DEBUG

#define malloc(n) c_debug_mem_malloc(n, __FILE__, __LINE__)
#define realloc(n, m) c_debug_mem_realloc(n, m, __FILE__, __LINE__)
#define free(n) c_debug_mem_free(n, __FILE__, __LINE__)

#define fopen(n, m) c_debug_mem_fopen(n, m, __FILE__, __LINE__)
#define fclose(n) c_debug_mem_fclose(n, __FILE__, __LINE__)

#else
#ifndef C_MEMORY_INTERNAL

#define c_debug_memory_init(n, m, k)
#define c_debug_mem_comment(n, m)
#define c_debug_mem_print(n)
#define c_debug_mem_reset()
#define c_debug_mem_consumption() 0
#define c_debug_mem_query(n, m, k, l)
#define c_debug_memory()

#endif
#endif

#ifdef C_EXIT_CRASH

extern void exit_crash(uint i);
#define exit(n) exit_crash(n)

#endif

/* -------- RNG --------
 * Fast pseudo random number generators useful for things like graphics
 */

extern float 	c_rand_float_no_crypto(uint32 index); /* [0, 1] */
extern double 	c_rand_double_no_crypto(uint32 index); /* [0, 1] */
extern float 	c_rand_neg_float_no_crypto(uint32 index); /* [-1, 1] */
extern double 	c_rand_neg_double_no_crypto(uint32 index); /* [-1, 1] */
extern uint 	c_rand_uint_no_crypto(uint32 index); 

/* -------- UTF8 support -------- */

extern uint32 c_utf8_to_uint32(char *c, uint *pos);
extern uint   c_uint32_to_utf8(uint32 character, char *out);
extern uint   c_uint32_to_utf8_string(uint32 *string, char *out, size_t buffer_size);
extern uint   c_uint16_to_utf8_string(uint16 *string, char *out, size_t buffer_size);


/* -------- Text -------- */

extern boolean  c_check_null_termination(char *text, uint len);

extern uint     c_find_next_word(char *text);
extern boolean  c_find_word_compare(char *text_a, char *text_b);
extern uint     c_text_copy(uint length, char *dest, const char *source);
extern boolean  c_text_compare(char *text_a, char *text_b);
extern void     c_text_replaceall(char *text, char find, char replace);
extern char 	*c_text_copy_allocate(char *source);
extern uint     c_word_copy(uint length, char *dest, char *source);
extern uint     c_text_copy_until(uint length, char *dest, char *source, char *until);
extern boolean  c_text_filter(char *text, char *filter);
extern boolean  c_text_filter_case_insensitive(char *text, char *filter);
extern uint     c_text_sort(char *text_a, char *text_b);
extern boolean  c_text_unique(char *text, uint buffer_length, char *compare);
extern char 	*c_text_load(char *file_name, size_t *size);

extern uint     c_text_parse_hex(char *text, uint64 *output);
extern uint     c_text_parse_decimal(char *text, uint64 *output);
extern uint     c_text_parse_real(char *text, int64 *integer_output, double *real_output, boolean *decimal);
extern uint     c_text_parse_double(char *text, double *real_output);

extern void     c_print_raw(uint8 *data, uint size);
extern void     c_fprint_raw(FILE *file, uint8 *data, uint size);

extern boolean  c_text_obfuscate(char *out_buffer, uint buffer_size, char *in_buffer);
extern void     c_text_obfuscate_print(char *in_buffer);

extern void     c_bits_to_text(uint64 bits, char *text); /* Generates a 13 byte string that is easy for a human to repeat. Terminated, needs 14 characters. */
extern uint     ctext_to_bits(uint64 *bits, char *text); /* Converts 14 byte string into a 64 bit value. Returns the number of bytes read, returns 0 if failure */

#endif

#endif /* CLAY_H */
