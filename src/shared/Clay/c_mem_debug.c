#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define C_MEMORY_INTERNAL
#define C_NO_MEMORY_DEBUG
#include "Clay/clay.h"

#pragma warning(disable:4477) /* size_t printf with %lu */

extern void c_debug_mem_print(unsigned int min_allocs);

#define C_MEMORY_OVER_ALLOC 256
#define C_MEMORY_MAGIC_NUMBER 132
#define C_MEMORY_MAX_ALLOCS ((int)1 << 12)

typedef struct
{
    uint size;
    void *buf;
    char *comment;
} STMemAllocBuf;

typedef struct
{
    uint line;
    char file[256];
    STMemAllocBuf *allocs;
    uint alloc_count;
    size_t alloc_allocated;
    size_t size;
    size_t allocated;
    size_t freed;
} STMemAllocLine;

STMemAllocLine c_alloc_lines[C_MEMORY_MAX_ALLOCS];
unsigned int c_alloc_line_count = 0;

typedef struct {
    uint alloc_line;
    char alloc_file[256];
    uint free_line;
    char free_file[256];
    size_t size;
    void *pointer;
    boolean realloc;
} STMemFreeBuf;

STMemFreeBuf c_freed_memory[C_MEMORY_MAX_ALLOCS];
uint c_freed_memory_current = 0;
uint c_freed_memory_count = 0;

void *c_alloc_mutex = NULL;
void (*c_alloc_mutex_lock)(void *mutex) = NULL;
void (*c_alloc_mutex_unlock)(void *mutex) = NULL;

void c_debug_memory_init(void (*lock)(void *mutex), void (*unlock)(void *mutex), void *mutex)
{
    c_alloc_mutex = mutex;
    c_alloc_mutex_lock = lock;
    c_alloc_mutex_unlock = unlock;
}

void *c_debug_mem_fopen(const char *file_name, const char *mode, char *file, uint line)
{
    FILE *f;
    UNUSED(file);
    UNUSED(line);
    f = fopen(file_name, mode);
    return f;
}

void c_debug_mem_fclose(void *f, char *file, uint line)
{
    UNUSED(file);
    UNUSED(line);
    fclose(f);
}

boolean c_debug_memory(void)
{
    boolean output = FALSE;
    uint i, j, k;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            uint8 *buf;
            size_t size;
            buf = c_alloc_lines[i].allocs[j].buf;
            size = c_alloc_lines[i].allocs[j].size;
            for (k = 0; k < C_MEMORY_OVER_ALLOC; k++)
                if (buf[size + k] != C_MEMORY_MAGIC_NUMBER)
                    break;
            if (k < C_MEMORY_OVER_ALLOC)
            {
                if (c_alloc_lines[i].allocs[j].comment == NULL)
                    printf("MEM ERROR: Overshoot at line %u in file %s\n", c_alloc_lines[i].line, c_alloc_lines[i].file);
                else
                    printf("MEM ERROR: Overshoot at line %u in file %s /* %s */\n", c_alloc_lines[i].line, c_alloc_lines[i].file, c_alloc_lines[i].allocs[j].comment);
                {
                    unsigned int *X = NULL;
                    X[0] = 0;
                }
                output = TRUE;
            }
        }
        /*
        if(output)
            exit(0);
        */
    }
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return output;
}

void c_debug_mem_add(void *pointer, size_t size, char *file, uint line)
{
    uint i, j;
    for (i = 0; i < C_MEMORY_OVER_ALLOC; i++)
        ((uint8 *)pointer)[size + i] = C_MEMORY_MAGIC_NUMBER;

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
            c_alloc_lines[i].allocs = realloc(c_alloc_lines[i].allocs, (sizeof *c_alloc_lines[i].allocs) * c_alloc_lines[i].alloc_allocated);
        }
        c_alloc_lines[i].allocs[c_alloc_lines[i].alloc_count].size = size;
        c_alloc_lines[i].allocs[c_alloc_lines[i].alloc_count].comment = NULL;
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
            c_alloc_lines[i].allocs[0].comment = NULL;
            c_alloc_lines[i].alloc_count = 1;
            c_alloc_lines[i].size = size;
            c_alloc_lines[i].freed = 0;
            c_alloc_lines[i].allocated++;
            c_alloc_line_count++;
        }
    }
    for (i = 0; i < c_freed_memory_count; i++)
    {
        if (pointer == c_freed_memory[i].pointer)
        {
            c_freed_memory[i] = c_freed_memory[--c_freed_memory_count];
            break;
        }
    }
}

void *c_debug_mem_malloc(size_t size, char *file, uint line)
{
    void *pointer;
    uint i;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    pointer = malloc(size + C_MEMORY_OVER_ALLOC);

#ifdef C_MEMORY_PRINT
    printf("Malloc %u bytes at pointer %p at %s line %u\n", size, pointer, file, line);
#endif

    if (pointer == NULL)
    {
        printf("MEM ERROR: Malloc returns NULL when trying to allocated %lu bytes at line %u in file %s\n", size, line, file);
        if (c_alloc_mutex != NULL)
            c_alloc_mutex_unlock(c_alloc_mutex);
        c_debug_mem_print(0);
        exit(0);
    }
    for (i = 0; i < size + C_MEMORY_OVER_ALLOC; i++)
        ((uint8 *)pointer)[i] = C_MEMORY_MAGIC_NUMBER + 1;
    c_debug_mem_add(pointer, size, file, line);


    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return pointer;
}

boolean c_debug_mem_remove(void *buf, char *file, uint line, boolean realloc, size_t *size)
{
    STMemFreeBuf *f;
    uint i, j, k;

    f = &c_freed_memory[c_freed_memory_current];
    c_freed_memory_current = (c_freed_memory_current + 1) % C_MEMORY_MAX_ALLOCS;
    if (c_freed_memory_current > c_freed_memory_count)
        c_freed_memory_count = c_freed_memory_current;
    for (i = 0; i < 255 && file[i] != 0; i++)
        f->free_file[i] = file[i];
    f->free_file[i] = 0;
    f->free_line = line;
    f->realloc = realloc;
    f->pointer = buf;

    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            if (c_alloc_lines[i].allocs[j].buf == buf)
            {
                for (k = 0; k < C_MEMORY_OVER_ALLOC; k++)
                    if (((uint8 *)buf)[c_alloc_lines[i].allocs[j].size + k] != C_MEMORY_MAGIC_NUMBER)
                        break;
                if (k < C_MEMORY_OVER_ALLOC)
                {
                    uint *a = NULL;
                    printf("MEM ERROR: Overshoot at line %u in file %s\n", c_alloc_lines[i].line, c_alloc_lines[i].file);
                    exit(0);
                    a[0] = 0;
                }
                for (k = 0; k < c_alloc_lines[i].allocs[j].size; k++)
                    ((uint8 *)buf)[k] = 255;
                f->alloc_line = c_alloc_lines[i].line;
                for (k = 0; k < 255 && c_alloc_lines[i].file[k] != 0; k++)
                    f->alloc_file[k] = c_alloc_lines[i].file[k];
                f->alloc_file[k] = 0;
                f->size = c_alloc_lines[i].allocs[j].size;
                *size = c_alloc_lines[i].allocs[j].size;
                c_alloc_lines[i].size -= c_alloc_lines[i].allocs[j].size;
                c_alloc_lines[i].allocs[j] = c_alloc_lines[i].allocs[--c_alloc_lines[i].alloc_count];
                c_alloc_lines[i].freed++;
                return TRUE;
            }
        }
    }
    for (i = 0; i < c_freed_memory_count; i++)
    {
        if (f != &c_freed_memory[i] && buf == c_freed_memory[i].pointer)
        {
            if (f->realloc)
                printf("MEM ERROR: Pointer %p in file is freed twice! It was freed on line %u in %s, was reallocated to %lu bytes long on line %u in file %s\n", f->pointer, f->free_line, f->free_file, f->size, f->alloc_line, f->alloc_file);
            else
                printf("MEM ERROR: Pointer %p in file is freed twice! It was freed on line %u in %s, was allocated to %lu bytes long on line %u in file %s\n", f->pointer, f->free_line, f->free_file, f->size, f->alloc_line, f->alloc_file);

            return FALSE;
        }
    }
    return TRUE;
}

void c_debug_mem_free(void *buf, char *file, uint line)
{
    size_t size = 0;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    if (!c_debug_mem_remove(buf, file, line, FALSE, &size))
    {
        uint *X = NULL;
        X[0] = 0;
    }

#ifdef C_MEMORY_PRINT
    printf("Free %u bytes at pointer %p at %s line %u\n", size, buf, file, line);
#endif

    free(buf);
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
}

boolean c_debug_mem_comment(void *buf, char *comment)
{
    uint i, j;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            if (c_alloc_lines[i].allocs[j].buf == buf)
            {
                c_alloc_lines[i].allocs[j].comment = comment;
                return TRUE;
            }
        }
    }
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    return FALSE;
}

void *c_debug_mem_realloc(void *pointer, size_t size, char *file, uint line)
{
    uint i, j = 0, k;
    size_t move;
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
        printf("CLAY Mem debugger error. Trying to reallocate pointer %p in %s line %u. Pointer has never been allocated\n", pointer, file, line);
        for (i = 0; i < c_alloc_line_count; i++)
        {
            for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
            {
                uint *buf;
                buf = c_alloc_lines[i].allocs[j].buf;
                for (k = 0; k < c_alloc_lines[i].allocs[j].size; k++)
                {
                    if (&buf[k] == pointer)
                    {
                        printf("Trying to reallocate pointer %u bytes (out of %u) in to allocation made in %s on line %u.\n", k, c_alloc_lines[i].allocs[j].size, c_alloc_lines[i].file, c_alloc_lines[i].line);
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
        printf("MEM ERROR: Realloc returns NULL when trying to allocate %lu bytes at line %u in file %s\n", size, line, file);
        if (c_alloc_mutex != NULL)
            c_alloc_mutex_unlock(c_alloc_mutex);
        c_debug_mem_print(0);
        exit(0);
    }
    for (i = 0; i < size + C_MEMORY_OVER_ALLOC; i++)
        ((uint8 *)pointer2)[i] = C_MEMORY_MAGIC_NUMBER + 1;
    memcpy(pointer2, pointer, move);

    c_debug_mem_add(pointer2, size, file, line);
    move = 0;
    c_debug_mem_remove(pointer, file, line, TRUE, &move);
#ifdef C_MEMORY_PRINT
    printf("Realloc %u bytes at pointer %p to %u bytes at pointer %p at %s line %u\n", size, pointer, move, pointer2, file, line);
#endif
    free(pointer);

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);

    return pointer2;
}

void c_debug_mem_print(uint min_allocs)
{
    uint i, j;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
    printf("Memory report:\n----------------------------------------------\n");
    for (i = 0; i < c_alloc_line_count; i++)
    {
        if (min_allocs < c_alloc_lines[i].allocated - c_alloc_lines[i].freed)
        {
            printf("%s line: %u\n", c_alloc_lines[i].file, c_alloc_lines[i].line);
            printf(" - Bytes allocated: %lu\n - Allocations: %lu\n - Frees: %lu\n\n", c_alloc_lines[i].size, c_alloc_lines[i].allocated, c_alloc_lines[i].freed);
            for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
                if (c_alloc_lines[i].allocs[j].comment != NULL)
                    printf("\t\t comment %p : %s\n", c_alloc_lines[i].allocs[j].buf, c_alloc_lines[i].allocs[j].comment);
        }
    }
    printf("----------------------------------------------\n");
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
}

size_t c_debug_mem_footprint(uint min_allocs)
{
    uint i;
    size_t size = 0;

    UNUSED(min_allocs);

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
        size += c_alloc_lines[i].size;

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);

    return size;
}

boolean c_debug_mem_query(void *pointer, uint *line, char **file, size_t *size)
{
    uint i, j;

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            if (c_alloc_lines[i].allocs[j].buf == pointer)
            {
                if (line != NULL)
                    *line = c_alloc_lines[i].line;
                if (file != NULL)
                    *file = c_alloc_lines[i].file;
                if (size != NULL)
                    *size = c_alloc_lines[i].allocs[j].size;
                if (c_alloc_mutex != NULL)
                    c_alloc_mutex_unlock(c_alloc_mutex);
                return TRUE;
            }
        }
    }
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);

    return FALSE;
}

boolean c_debug_mem_test(void *pointer, size_t size, boolean ignore_not_found)
{
    uint i, j;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);

    for (i = 0; i < c_alloc_line_count; i++)
    {
        for (j = 0; j < c_alloc_lines[i].alloc_count; j++)
        {
            if (c_alloc_lines[i].allocs[j].buf >= pointer && ((uint8 *)c_alloc_lines[i].allocs[j].buf) + c_alloc_lines[i].allocs[j].size <= (uint8 *)pointer) /* technically UB */
            {
                if (((uint8 *)c_alloc_lines[i].allocs[j].buf) + c_alloc_lines[i].allocs[j].size < ((uint8 *)pointer) + size)
                {
                    printf("CLAY Mem Debugger error: Not enough memory to access pointer %p, %u bytes missing\n", pointer, (uint)(((uint8 *)c_alloc_lines[i].allocs[j].buf) + c_alloc_lines[i].allocs[j].size) - (uint)(((uint8 *)pointer) + size));
                    if (c_alloc_mutex != NULL)
                        c_alloc_mutex_unlock(c_alloc_mutex);
                    return TRUE;
                }
                else
                {
                    if (c_alloc_mutex != NULL)
                        c_alloc_mutex_unlock(c_alloc_mutex);
                    return FALSE;
                }
            }
        }
    }

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
    if (ignore_not_found)
        return FALSE;

    for (i = 0; i < c_freed_memory_count; i++)
    {
        if (pointer >= c_freed_memory[i].pointer && ((uint8 *)c_freed_memory[i].pointer) + c_freed_memory[i].size >= ((uint8 *)pointer) + size)
        {
            printf("MEM ERROR: Pointer %p was freed on line %u in file %s\n", pointer, c_freed_memory[i].free_line, c_freed_memory[i].free_file);
        }
    }

    printf("MEM ERROR: No matching memory for %p found!\n", pointer);
    return TRUE;
}

size_t c_debug_mem_consumption(void)
{
    uint i;
    size_t sum = 0;

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
    uint i;
    if (c_alloc_mutex != NULL)
        c_alloc_mutex_lock(c_alloc_mutex);
#ifdef C_MEMORY_PRINT
    printf("Memory reset--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
#endif

    for (i = 0; i < c_alloc_line_count; i++)
        free(c_alloc_lines[i].allocs);
    c_alloc_line_count = 0;

    if (c_alloc_mutex != NULL)
        c_alloc_mutex_unlock(c_alloc_mutex);
}

void exit_crash(uint i)
{
    uint *a = NULL;
    UNUSED(i);
    a[0] = 0;
}

void c_no_debug_free(void *buf)
{
    free(buf);
}
