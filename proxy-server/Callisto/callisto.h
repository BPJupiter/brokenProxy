#ifndef CALYPSO_H
#define CALYPSO_H

typedef struct _DeferredList DeferredList;

extern DeferredList deferred_init();
extern void defer(DeferredList* deferredList, void (*func)(void*), void* arg);
extern void deferred_run(DeferredList* deferredList);

#ifdef C_MEMORY_DEBUG

/* ----- Debugging -----
 * If C_MEMORY_DEBUG is enabled, the memory debugging system will create macros that replace malloc, free, and realloc, and allows the system to keep track of and report where memory is being allocated, how much and if the memory is being freed. This is very useful for finding memory leaks in large applications. The system can also over allocate memory and fill it with a magic number and can therefore detect if the application writes outside of the allocated memory.
 */

extern void c_debug_memory_init(void (*lock)(void *mutex), void (*unlock)(void *mutex), void *mutex); /* Required for memory debugger to be thread safe */
extern void *c_debug_mem_malloc(unsigned int size, char *file, unsigned int line); /* Replaces malloc and records the c file and line where it was called*/
extern void *c_debug_mem_realloc(void *pointer, unsigned int size, char *file, unsigned int line); /* Replaces realloc and records the c file and line where it was called*/
extern void c_debug_mem_free(void *buf); /* Replaces free and records the c file and line where it was called*/
extern void c_debug_mem_print(unsigned int min_allocs); /* Prints out a list of all allocations made, their location, how much memorey each has allocated, freed, and how many allocations have been made. The min_allocs parameter can be set to avoid printing any allocations that have been made fewer times then min_allocs */
extern void c_debug_mem_reset(); /* f_debug_mem_reset allows you to clear all memory stored in the debugging system if you only want to record allocations after a specific point in your code*/
extern int c_debug_memory(); /*f_debug_memory checks if any of the bounds of any allocation has been over written and reports where to standard out. The function returns TRUE if any error was found*/

extern void c_debug_mem_check_defer(DeferredList* dl, void (*func)(void*), void* arg);

#define malloc(n) c_debug_mem_malloc(n, __FILE__, __LINE__)
#define realloc(n, m) c_debug_mem_realloc(n, m, __FILE__, __LINE__)
#define free(n) c_debug_mem_free(n)

#define defer(dl, f, n) c_debug_mem_check_defer(dl, f, n)

#endif
#endif
