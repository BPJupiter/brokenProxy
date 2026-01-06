#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "callisto.h"

#ifdef C_MEMORY_DEBUG
#undef malloc
#undef realloc
#undef free
#endif

extern void c_debug_mem_print(unsigned int min_allocs);

#define C_MEMORY_OVER_ALLOC 32
#define C_MEMORY_MAGIC_NUMBER 132
#define C_MEMORY_MAX_ALLOCS ((int)1 << 16)

typedef struct
{
    unsigned int size;
    void *buf;
} STMemAllocBuf;

typedef struct
{
    unsigned int line;
    char file[256];
    STMemAllocBuf *allocs;
    unsigned int alloc_count;
    unsigned int alloc_allocated;
    unsigned int size;
    unsigned int allocated;
    unsigned int freed;
} STMemAllocLine;

STMemAllocLine c_alloc_lines[C_MEMORY_MAX_ALLOCS];
unsigned int c_alloc_line_count = 0;
void *c_alloc_mutex = NULL;
void (*c_alloc_mutex_lock)(void *mutex) = NULL;
void (*c_alloc_mutex_unlock)(void *mutex) = NULL;

void c_debug_memory_init(void (*lock)(void *mutex), void (*unlock)(void *mutex), void *mutex)
{
    c_alloc_mutex = mutex;
    c_alloc_mutex_lock = lock;
    c_alloc_mutex_unlock = unlock;
}

int c_debug_memory(void)
{
    int output = 0;
    unsigned int i, j, k;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            unsigned char *buf;
            unsigned int size;
            buf = c_alloc_lines[i].allocs[j].buf;
            size = c_alloc_lines[i].allocs[j].size;
            for (k = 0; k < C_MEMORY_OVER_ALLOC; k++)
                if (buf[size + k] != C_MEMORY_MAGIC_NUMBER)
                    break;
            if (k < C_MEMORY_OVER_ALLOC)
            {
                printf("MEM ERROR: Overshoot at line %u in file %s\n",
                       c_alloc_lines[i].line, c_alloc_lines[i].file);
                {
                    unsigned int *X = NULL;
                    X[0] = 0;
                }
                output = 1;
            }
        }
    }
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return output;
}

void c_debug_mem_add(void *pointer, unsigned int size, char *file, unsigned int line)
{
    unsigned int i, j;
    for (i = 0; i < C_MEMORY_OVER_ALLOC; i++)
        ((unsigned char *)pointer)[size + i] = C_MEMORY_MAGIC_NUMBER;

    for (i = 0; i < c_alloc_line_count; i++)
    {
        if (line == c_alloc_lines[i].line)
        {
            for (j = 0; file[j] != 0 && file[j] == c_alloc_lines[i].file[j]; j++)
                ;
            if (file[j] == c_alloc_lines[i].file[j])
                break;
        }
    }
    if (i < c_alloc_line_count)
    {
        if (c_alloc_lines[i].alloc_allocated == c_alloc_lines[i].alloc_count)
        {
            c_alloc_lines[i].alloc_allocated += C_MEMORY_MAX_ALLOCS;
            c_alloc_lines[i].allocs = realloc(c_alloc_lines[i].allocs,
                                              (sizeof *c_alloc_lines[i].allocs) * c_alloc_lines[i].alloc_allocated);
        }
        c_alloc_lines[i].allocs[c_alloc_lines[i].alloc_count].size = size;
        c_alloc_lines[i].allocs[c_alloc_lines[i].alloc_count++].buf = pointer;
        c_alloc_lines[i].size += size;
        c_alloc_lines[i].allocated++;
    }
    else
    {
        if (i < C_MEMORY_MAX_ALLOCS)
        {
            c_alloc_lines[i].line = line;
            for (j = 0; j < 255 && file[j] != 0; j++)
                c_alloc_lines[i].file[j] = file[j];
            c_alloc_lines[i].file[j] = 0;
            c_alloc_lines[i].alloc_allocated = 256;
            c_alloc_lines[i].allocs = malloc((sizeof *c_alloc_lines[i].allocs) * c_alloc_lines[i].alloc_allocated);
            c_alloc_lines[i].allocs[0].size = size;
            c_alloc_lines[i].allocs[0].buf = pointer;
            c_alloc_lines[i].alloc_count = 1;
            c_alloc_lines[i].size = size;
            c_alloc_lines[i].freed = 0;
            c_alloc_lines[i].allocated++;
            c_alloc_line_count++;
        }
    }
}

void *c_debug_mem_malloc(unsigned int size, char *file, unsigned int line)
{
    void *pointer;
    unsigned int i;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    pointer = malloc(size + C_MEMORY_OVER_ALLOC);

    if (pointer == NULL)
    {
        printf("MEM ERROR: Malloc returns NULL when trying to allocated %u bytes at line %u in file %s\n",
               size, line, file);
        if (c_alloc_mutex != NULL)
            c_alloc_mutex_unlock(c_alloc_mutex);
        c_debug_mem_print(0);
        exit(0);
    }
    for (i = 0; i < size + C_MEMORY_OVER_ALLOC; i++)
        ((unsigned char *)pointer)[i] = C_MEMORY_MAGIC_NUMBER + 1;
    c_debug_mem_add(pointer, size, file, line);
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return pointer;
}

int c_debug_mem_remove(void *buf)
{
    unsigned int i, j, k;
    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            if (c_alloc_lines[i].allocs[j].buf == buf)
            {
                for (k = 0; k < C_MEMORY_OVER_ALLOC; k++)
                    if (((unsigned char *)buf)[c_alloc_lines[i].allocs[j].size + k] != C_MEMORY_MAGIC_NUMBER)
                        break;
                if (k < C_MEMORY_OVER_ALLOC)
                    printf("MEM ERROR: Overshoot at line %u in file %s\n",
                           c_alloc_lines[i].line, c_alloc_lines[i].file);
                c_alloc_lines[i].size -= c_alloc_lines[i].allocs[j].size;
                c_alloc_lines[i].allocs[j] =
                    c_alloc_lines[i].allocs[--c_alloc_lines[i].alloc_count];
                c_alloc_lines[i].freed++;
                return 1;
            }
        }
    }
    return 0;
}

void c_debug_mem_free(void *buf)
{
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    if (!c_debug_mem_remove(buf))
    {
        unsigned int *X = NULL;
        X[0] = 0;
    }
    free(buf);
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
}

void *c_debug_mem_realloc(void *pointer, unsigned int size, char *file, unsigned int line)
{
    unsigned int i, j, k, move;
    void *pointer2;
    if (pointer == NULL)
        return c_debug_mem_malloc(size, file, line);

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
            if (c_alloc_lines[i].allocs[j].buf == pointer)
                break;
        if (j < c_alloc_lines[i].alloc_count)
            break;
    }
    if (i == c_alloc_line_count)
    {
        printf("Callisto Mem debugger error. Trying to reallocate pointer %p in %s line %u. Pointer has never been allocated\n",
               pointer, file, line);
        for (i = 0; i < c_alloc_line_count; i++)
        {
            for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
            {
                unsigned int *buf;
                buf = c_alloc_lines[i].allocs[j].buf;
                for (k = 0; k < c_alloc_lines[i].allocs[j].size; k++)
                {
                    if (&buf[k] == pointer)
                    {
                        printf("Trying to reallocate pointer %u bytes (out of %u) in to allocate made in %s on line %u.\n",
                               k, c_alloc_lines[i].allocs[j].size, c_alloc_lines[i].file, c_alloc_lines[i].line);
                    }
                }
            }
        }
        exit(0);
    }
    move = c_alloc_lines[i].allocs[j].size;

    if (move > size)
        move = size;

    pointer2 = malloc(size + C_MEMORY_OVER_ALLOC);
    if (pointer2 == NULL)
    {
        printf("MEM ERROR: Realloc returns NULL when trying to allocate %u bytes at line %u in file %s\n",
               size, line, file);
        if (c_alloc_mutex != NULL)
            c_alloc_mutex_unlock(c_alloc_mutex);
        c_debug_mem_print(0);
        exit(0);
    }
    for (i = 0; i < size + C_MEMORY_OVER_ALLOC; i++)
        ((unsigned char *)pointer2)[i] = C_MEMORY_MAGIC_NUMBER + 1;
    memcpy(pointer2, pointer, move);

    c_debug_mem_add(pointer2, size, file, line);
    c_debug_mem_remove(pointer);
    free(pointer);

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return pointer2;
}

void c_debug_mem_print(unsigned int min_allocs)
{
    unsigned int i;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    printf("Memory report:\n----------------------------------------------\n");
    for (i = 0; i < c_alloc_line_count; i++)
    {
        if (min_allocs < c_alloc_lines[i].allocated)
        {
            printf("%s line: %u\n", c_alloc_lines[i].file, c_alloc_lines[i].line);
            printf(" - Bytes allocated: %u\n - Allocations: %u\n - Frees: %u\n\n",
                   c_alloc_lines[i].size, c_alloc_lines[i].allocated, c_alloc_lines[i].freed);
        }
    }
    printf("----------------------------------------------\n");
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
}

unsigned int c_debug_mem_consumption(void)
{
    unsigned int i, sum = 0;

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
        sum += c_alloc_lines[i].size;

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return sum;
}

void c_debug_mem_reset(void)
{
    unsigned int i;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
        free(c_alloc_lines[i].allocs);
    c_alloc_line_count = 0;

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
}

void c_debug_mem_check_defer(DeferredList *dl, void (*func)(void *), void *arg)
{
    if (func == free) defer(dl, c_debug_mem_free, arg);
}
