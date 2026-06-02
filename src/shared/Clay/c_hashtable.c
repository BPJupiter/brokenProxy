#include "Clay/clay.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#define GROUP_SIZE 16
#define BIT_EMPTY 0x80
#define BIT_DELETED 0xFE
#define CAPACITY (1 << 16)

struct cHashTable {
    uint32 size;
    uint32 capacity;

    size_t key_size;
    size_t data_size;

    uint8 *ctrl;
    uint8 *slots;

    uint64 seed;

    uint64 (*hash_func)(const void *key);
    boolean (*eq_func)(const void *k1, const void *k2);
};

static void set_ctrl(cHashTable *table, size_t index, uint8 val)
{
    table->ctrl[index] = val;
    if (index < GROUP_SIZE) {
        table->ctrl[table->capacity + index] = val;
    }
}

static uint64 get_secure_hash(const cHashTable *table, const void *key)
{
    uint64 hash = table->hash_func(key);

    hash ^= table->seed;
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;

    return hash;
}

static boolean c_hashtable_resize(cHashTable *table)
{
	uint8 *new_ctrl, *new_slots;
    uint32 new_capacity = table->capacity * 2;
    size_t stride = table->key_size + table->data_size;
    size_t i;

    new_ctrl = malloc(new_capacity + GROUP_SIZE);
    if (!new_ctrl) {
        return FALSE;
    }

    new_slots = calloc(new_capacity, stride);
    if (!new_slots) {
        free(new_ctrl);
        return FALSE;
    }

    memset(new_ctrl, BIT_EMPTY, new_capacity + GROUP_SIZE);

    for (i = 0; i < table->capacity; i++) {
        uint8 ctrl_val = table->ctrl[i];

        if (ctrl_val != BIT_EMPTY && ctrl_val != BIT_DELETED) {
            uint8 *old_slot_ptr = &table->slots[i * stride];

            uint64 hash = get_secure_hash(table, old_slot_ptr);
            size_t h1 = hash >> 7;
            uint8 h2 = hash & 0x7F;

            size_t index = h1 & (new_capacity - 1);
            size_t probe_step = 0;

            while (1) {
                uint32 mask_empty = 0;

#if defined(__SSE2__)
                __m128i ctrl_group = _mm_loadu_si128((__m128i *)&new_ctrl[index]);
                __m128i match_empty = _mm_set1_epi8(BIT_EMPTY);
                mask_empty = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_empty));
#else
                for (int j = 0; j < GROUP_SIZE; j++) {
                    if (new_ctrl[index + j] == BIT_EMPTY) {
                        mask_empty |= (1 << j);
                    }
                }
#endif
                if (mask_empty > 0) {
                    int offset = __builtin_ctz(mask_empty);
                    size_t insert_idx = (index + offset) & (new_capacity - 1);

                    memcpy(&new_slots[insert_idx * stride], old_slot_ptr, stride);
                    new_ctrl[insert_idx] = h2;
                    if (insert_idx < GROUP_SIZE) {
                        new_ctrl[new_capacity + insert_idx] = h2;
                    }
                    break;
                }

                probe_step++;
                index = (index + probe_step * GROUP_SIZE) & (new_capacity - 1);
            }
        }
    }

    free(table->ctrl);
    free(table->slots);

    table->ctrl = new_ctrl;
    table->slots = new_slots;
    table->capacity = new_capacity;

    return TRUE;
}

cHashTable *c_hashtable_init(size_t key_size, size_t data_size, uint64 (*hash_func)(const void *), boolean (*eq_func)(const void *, const void *))
{
    cHashTable *table = malloc(sizeof(*table));
    if (!table) {
        return NULL;
    }

    table->size = 0;
    table->capacity = CAPACITY;
    table->key_size = key_size;
    table->data_size = data_size;

    table->ctrl = malloc(CAPACITY + GROUP_SIZE); /* padding for SIMD bounds safety */
    if (!table->ctrl) {
        free(table);
        return NULL;
    }

    table->slots = calloc(CAPACITY, key_size + data_size);
    if (!table->slots) {
        free(table->ctrl);
        free(table);
        return NULL;
    }

    table->seed = ((uint64)time(NULL) << 32) ^ (uint64)table;

    table->hash_func = hash_func;
    table->eq_func = eq_func;

    memset(table->ctrl, BIT_EMPTY, CAPACITY + GROUP_SIZE);
    return table;
}

void c_hashtable_free(cHashTable *table)
{
    if (!table) {
        return;
    }
    free(table->ctrl);
    free(table->slots);
    free(table);
}

boolean c_hashtable_insert(cHashTable *table, const void *key, void *value)
{
    uint64 hash = get_secure_hash(table, key);
    size_t h1 = hash >> 7;
    uint8 h2 = hash & 0x7F;

    size_t index = h1 & (table->capacity - 1);
    size_t stride = table->key_size + table->data_size;
    int first_deleted_idx = -1;
    size_t probe_step = 0;

    if (table->size >= (table->capacity * 0.8)) {
        if (!c_hashtable_resize(table)) {
            return FALSE;
        }
    }

    while (1) {
        uint32 mask_matches = 0;
        uint32 mask_empty = 0;
        uint32 mask_deleted = 0;
        uint32 candidates = 0;

#if defined(__SSE2__)
        __m128i ctrl_group   = _mm_loadu_si128((__m128i *)&table->ctrl[index]);
        __m128i match_h2     = _mm_set1_epi8(h2);
        __m128i match_empty  = _mm_set1_epi8(BIT_EMPTY);
        __m128i match_deleted = _mm_set1_epi8(BIT_DELETED);

        mask_matches = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_h2));
        mask_empty   = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_empty));
        mask_deleted = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_deleted));
#else
        for (int i = 0; i < GROUP_SIZE; i++) {
            uint8 c = table->ctrl[index + i];
            if (c == h2)          mask_matches |= (1 << i);
            if (c == BIT_EMPTY)   mask_empty   |= (1 << i);
            if (c == BIT_DELETED) mask_deleted |= (1 << i);
        }
#endif

        candidates = mask_matches;
        while (candidates > 0) {
            int offset = __builtin_ctz(candidates);
            size_t slot_idx = (index + offset) & (table->capacity - 1);
            uint8 *slot_ptr = &table->slots[slot_idx * stride];

            if (table->eq_func(slot_ptr, key)) {
                memcpy(slot_ptr + table->key_size, value, table->data_size);
                return TRUE;
            }
            candidates &= ~(1 << offset);
        }

        if (first_deleted_idx == -1 && mask_deleted > 0) {
            int offset = __builtin_ctz(mask_deleted);
            first_deleted_idx = (index + offset) & (table->capacity - 1);
        }

        if (mask_empty > 0) {
            size_t insert_idx;
            uint8 *slot_ptr;
            if (first_deleted_idx != -1) {
                insert_idx = first_deleted_idx;
            } else {
                int offset = __builtin_ctz(mask_empty);
                insert_idx = (index + offset) & (table->capacity - 1);
            }

            slot_ptr = &table->slots[insert_idx * stride];
            memcpy(slot_ptr, key, table->key_size);
            memcpy(slot_ptr + table->key_size, value, table->data_size);

            set_ctrl(table, insert_idx, h2);
            table->size++;
            return TRUE;
        }

        probe_step++;
        index = (index + probe_step * GROUP_SIZE) & (table->capacity - 1);
    }
}

boolean c_hashtable_delete(cHashTable *table, const void *key)
{
    uint64 hash = get_secure_hash(table, key);
    size_t h1 = hash >> 7;
    uint8 h2 = hash & 0x7F;

    size_t index = h1 & (table->capacity - 1);
    size_t stride = table->key_size + table->data_size;
    size_t probe_step = 0;

    while (1) {
        uint32 mask_matches = 0;
        uint32 mask_empty = 0;
        uint32 candidates = 0;

#if defined(__SSE2__)
        __m128i ctrl_group  = _mm_loadu_si128((__m128i *)&table->ctrl[index]);
        __m128i match_h2    = _mm_set1_epi8(h2);
        __m128i match_empty = _mm_set1_epi8(BIT_EMPTY);

        mask_matches = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_h2));
        mask_empty   = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_empty));
#else
        for (int i = 0; i < GROUP_SIZE; i++) {
            uint8 c = table->ctrl[index + i];
            if (c == h2)        mask_matches |= (1 << i);
            if (c == BIT_EMPTY) mask_empty   |= (1 << i);
        }
#endif

        candidates = mask_matches;
        while (candidates > 0) {
            int offset = __builtin_ctz(candidates);
            size_t slot_idx = (index + offset) & (table->capacity - 1);
            uint8 *slot_ptr = &table->slots[slot_idx * stride];

            if (table->eq_func(slot_ptr, key)) {
                set_ctrl(table, slot_idx, BIT_DELETED);
                table->size--;
                return TRUE;
            }
            candidates &= ~(1 << offset);
        }

        if (mask_empty > 0) {
            return FALSE;
        }

        probe_step++;
        index = (index + probe_step * GROUP_SIZE) & (table->capacity - 1);
    }
}

boolean c_hashtable_get(cHashTable *table, const void *key, void *out_value)
{
    uint64 hash = get_secure_hash(table, key);
    size_t h1 = hash >> 7;
    uint8 h2 = hash & 0x7F;

    size_t index = h1 & (table->capacity - 1);
    size_t stride = table->key_size + table->data_size;
    size_t probe_step = 0;

    while (1) {
        uint32 mask_matches = 0;
        uint32 mask_empty = 0;
        uint32 candidates = 0;

#if defined(__SSE2__)
        __m128i ctrl_group  = _mm_loadu_si128((__m128i *)&table->ctrl[index]);
        __m128i match_h2    = _mm_set1_epi8(h2);
        __m128i match_empty = _mm_set1_epi8(BIT_EMPTY);

        mask_matches = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_h2));
        mask_empty   = _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_group, match_empty));
#else
        for (int i = 0; i < GROUP_SIZE; i++) {
            uint8 c = table->ctrl[index + i];
            if (c == h2)        mask_matches |= (1 << i);
            if (c == BIT_EMPTY) mask_empty   |= (1 << i);
        }
#endif

        candidates = mask_matches;
        while (candidates > 0) {
            int offset = __builtin_ctz(candidates);
            size_t slot_idx = (index + offset) & (table->capacity - 1);
            uint8 *slot_ptr = &table->slots[slot_idx * stride];

            if (table->eq_func(slot_ptr, key)) {
                if (out_value) {
                    memcpy(out_value, slot_ptr + table->key_size, table->data_size);
                }
                return TRUE;
            }
            candidates &= ~(1 << offset);
        }

        if (mask_empty > 0) {
            return FALSE;
        }

        probe_step++;
        index = (index + probe_step * GROUP_SIZE) & (table->capacity - 1);
    }
}