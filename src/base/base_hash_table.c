
internal void bucket_list_concat_in_place(Bucket_List *list, Bucket_List *to_concat)
{
    if (to_concat->first) {
        if (list->first) {
            list->last->next = to_concat->first;
            list->last = to_concat->last;
        }
        else {
            list->first = to_concat->first;
            list->last = to_concat->last;
        }
        MemoryZeroStruct(to_concat);
    }
}

internal Bucket_Node *bucket_list_pop(Bucket_List *list)
{
    Bucket_Node *result = list->first;
    SLLQueuePop(list->first, list->last);
    return result;
}

internal u64 hash_table_hasher(String8 string)
{
    return u64_hash_from_str8(string);
}

internal Hash_Table *hash_table_init(Arena *arena, u64 cap)
{
    Hash_Table *ht = push_array(arena, Hash_Table, 1);
    ht->cap = cap;
    ht->buckets = push_array(arena, Bucket_List, cap);
    return ht;
}

internal void hash_table_purge(Hash_Table *ht)
{
    ht->count = 0;

    for (u64 ibucket = 0; ibucket < ht->cap; ++ibucket) {
        bucket_list_concat_in_place(&ht->free_buckets, &ht->buckets[ibucket]);
    }
}

internal Bucket_Node *hash_table_push(Arena *arena, Hash_Table *ht, u64 hash, Key_Value_Pair v)
{
    Bucket_Node *node;
    if (ht->free_buckets.first != 0) {
        node = bucket_list_pop(&ht->free_buckets);
    }
    else {
        node = push_array(arena, Bucket_Node, 1);
    }
    node->next = 0;
    node->v    = v;

    u64 ibucket = hash % ht->cap;
    DLLPushBack(ht->buckets[ibucket].first, ht->buckets[ibucket].last, node);
    ht->count += 1;

    return node;
}

internal Bucket_Node *hash_table_push_string_string(Arena *arena, Hash_Table *ht, String8 key, String8 value)
{
    u64 hash = hash_table_hasher(key);
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_string = key, .value_string = value });
}

internal Bucket_Node *hash_table_push_string_raw(Arena *arena, Hash_Table *ht, String8 key, void *value)
{
    u64 hash = hash_table_hasher(key);
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_string = key, .value_raw = value });
}

internal Bucket_Node *hash_table_push_string_u64(Arena *arena, Hash_Table *ht, String8 key, u64 value)
{
    u64 hash = hash_table_hasher(key);
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_string = key, .value_u64 = value });
}

internal Bucket_Node *hash_table_push_u32_raw(Arena *arena, Hash_Table *ht, u32 key, void *value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_u32 = key, .value_raw = value });
}

internal Bucket_Node *hash_table_push_u32_string(Arena *arena, Hash_Table *ht, u32 key, String8 value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_u32 = key, .value_string = value });
}

internal Bucket_Node *hash_table_push_u64_raw(Arena *arena, Hash_Table *ht, u64 key, void *value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_u64 = key, .value_raw = value });
}

internal Bucket_Node *hash_table_push_u64_string(Arena *arena, Hash_Table *ht, u64 key, String8 value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_u64 = key, .value_string = value });
}

internal Bucket_Node *hash_table_push_u64_u64(Arena *arena, Hash_Table *ht, u64 key, u64 value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_u64 = key, .value_u64 = value });
}

internal String8 hash_table_normalize_path_string(Arena *arena, String8 path)
{
    Temp scratch = scratch_begin(&arena, 1);
    String8 result;
    result = lower_from_str8(scratch.arena, path);
    result = path_convert_slashes(arena, result, PathStyle_UnixAbsolute);
    scratch_end(scratch);
    return result;
}

internal Bucket_Node *hash_table_push_path_string(Arena *arena, Hash_Table *ht, String8 path, String8 value)
{
    String8 path_canon = hash_table_normalize_path_string(arena, path); 
    return hash_table_push_string_string(arena, ht, path_canon, value);
}

internal Bucket_Node *hash_table_push_path_u64(Arena *arena, Hash_Table *ht, String8 path, u64 value)
{
    String8 path_canon = hash_table_normalize_path_string(arena, path);
    u64 hash = hash_table_hasher(path_canon);
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_string = path_canon, .value_u64 = value });
}

internal Bucket_Node *hash_table_push_path_raw(Arena *arena, Hash_Table *ht, String8 path, void *value)
{
    String8 path_canon = hash_table_normalize_path_string(arena, path);
    u64 hash = hash_table_hasher(path_canon);
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_string = path_canon, .value_raw = value });
}

////////////////////////////////

internal Bucket_Node *hash_table_bucket_from_hash(Hash_Table *ht, u64 hash)
{
    Bucket_Node *bucket = 0;
    if (ht->cap > 0) {
        bucket = ht->buckets[hash % ht->cap].first;
    }
    return bucket;
}

internal Bucket_Node *hash_table_search_string(Hash_Table *ht, String8 key_string)
{
    Bucket_Node *bucket = hash_table_bucket_from_hash(ht, hash_table_hasher(key_string));
    for EachNode(n, Bucket_Node, bucket) {
        if (str8_match(n->v.key_string, key_string, 0)) {
            return n;
        }
    }
    return 0;
}

internal Bucket_Node *hash_table_search_u32(Hash_Table *ht, u32 key_u32)
{
    Bucket_Node *bucket = hash_table_bucket_from_hash(ht, hash_table_hasher(str8_struct(&key_u32)));
    for EachNode(n, Bucket_Node, bucket) {
            if (n->v.key_u32 == key_u32) {
                return n;
            }
        }
    return 0;
}

internal Bucket_Node *hash_table_search_u64(Hash_Table *ht, u64 key_u64)
{
    Bucket_Node *bucket = hash_table_bucket_from_hash(ht, hash_table_hasher(str8_struct(&key_u64)));
    for EachNode(n, Bucket_Node, bucket) {
            if (n->v.key_u64 == key_u64) {
                return n;
            }
        }
    return 0;
}

internal Bucket_Node *hash_table_search_path(Hash_Table *ht, String8 path)
{
    Temp scratch = scratch_begin(0,0);
    String8 path_canon = path;
    path_canon = lower_from_str8(scratch.arena, path_canon);
    path_canon = path_convert_slashes(scratch.arena, path_canon, PathStyle_UnixAbsolute);
    Bucket_Node *result = hash_table_search_string(ht, path_canon);
    scratch_end(scratch);
    return result;
}

internal Bucket_Node *hash_table_search_raw(Hash_Table *ht, void *key)
{
    Bucket_Node *bucket = hash_table_bucket_from_hash(ht, hash_table_hasher(str8_struct(&key)));
    for EachNode(n, Bucket_Node, bucket) {
            if (n->v.key_raw == key) {
                return n;
            }
        }
    return 0;
}

internal bool32 hash_table_search_path_u64(Hash_Table *ht, String8 key, u64 *value_out)
{
    Bucket_Node *result = hash_table_search_path(ht, key);
    if (result != 0) {
        if (value_out != 0) {
            *value_out = result->v.value_u64;
        }
        return 1;
    }
    return 0;
}

internal Bucket_Node *hash_table_push_u32_u32(Arena *arena, Hash_Table *ht, u32 key, u32 value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_u32 = key, .value_u32 = value });
}

internal Bucket_Node *hash_table_push_raw_raw(Arena *arena, Hash_Table *ht, void *key, void *value)
{
    u64 hash = hash_table_hasher(str8_struct(&key));
    return hash_table_push(arena, ht, hash, (Key_Value_Pair){ .key_raw = key, .value_raw = value });
}

internal bool32 hash_table_search_string_u64(Hash_Table *ht, String8 key, u64 *value_out)
{
    Bucket_Node *result = hash_table_search_string(ht, key);
    if (result != 0) {
        if (value_out != 0) {
            *value_out = result->v.value_u64;
        }
        return 1;
    }
    return 0;
}

internal bool32 hash_table_search_string_string(Hash_Table *ht, String8 key, String8 *value_out)
{
    Bucket_Node *result = hash_table_search_string(ht, key);
    if (result) {
        if (value_out) {
            *value_out = result->v.value_string;
        }
        return 1;
    }
    return 0;
}

internal bool32 hash_table_search_u32_u32(Hash_Table *ht, u32 key, u32 *value_out)
{
    Bucket_Node *result = hash_table_search_u32(ht, key);
    if (result) {
        if (value_out) {
            *value_out = result->v.value_u32;
        }
        return 1;
    }
    return 0;
}

internal void *hash_table_search_string_raw(Hash_Table *ht, String8 key)
{
    Bucket_Node *result = hash_table_search_string(ht, key);
    if (result) {
        return result->v.value_raw;
    }
    return 0;
}

internal void *hash_table_search_u64_raw(Hash_Table *ht, u64 key_u64)
{
    Bucket_Node *n = hash_table_search_u64(ht, key_u64);
    return n ? n->v.value_raw : 0;
}

internal void *hash_table_search_path_raw(Hash_Table *ht, String8 path)
{
    Bucket_Node *n = hash_table_search_path(ht, path);
    return n ? n->v.value_raw : 0;
}

internal void *hash_table_search_raw_raw(Hash_Table *ht, void *key)
{
    Bucket_Node *n = hash_table_search_raw(ht, key);
    return n ? n->v.value_raw : 0;
}

internal void hash_table_purge_item(Hash_Table *ht, u64 hash, Bucket_Node *node)
{
    u64 bucket_idx = hash % ht->cap;
    DLLRemove(ht->buckets[bucket_idx].first, ht->buckets[bucket_idx].last, node);
    ht->count -= 1;
}

internal bool32 hash_table_purge_u64(Hash_Table *ht, u64 key)
{
    u64         hash = hash_table_hasher(str8_struct(&key));
    Bucket_Node *n    = hash_table_search_u64(ht, key);
    if (n) {
        hash_table_purge_item(ht, hash, n);
        return 1;
    }
    return 0;
}

internal bool32 hash_table_purge_string(Hash_Table *ht, String8 key)
{
    u64         hash = hash_table_hasher(key);
    Bucket_Node *n    = hash_table_search_string(ht, key);
    if (n) {
        hash_table_purge_item(ht, hash, n);
        return 1;
    }
    return 0;
}

internal int key_value_pair_is_before_u32(void *a, void *b)
{
    return ((Key_Value_Pair *)a)->key_u32 < ((Key_Value_Pair *)b)->key_u32;
}

internal int key_value_pair_is_before_u64(void *a, void *b)
{
    return ((Key_Value_Pair *)a)->key_u64 < ((Key_Value_Pair *)b)->key_u64;
}

internal int key_value_pair_is_before_string_sensitive(void *a, void *b)
{
    return str8_compar_case_sensitive(&((Key_Value_Pair*)a)->key_string, &((Key_Value_Pair*)b)->key_string) < 0;
}

internal u32 *keys_from_hash_table_u32(Arena *arena, Hash_Table *ht)
{
    u32 *result = push_array_no_zero(arena, u32, ht->count);
    for (u64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
        for (Bucket_Node *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
            Assert(cursor < ht->count);
            result[cursor++] = n->v.key_u32;
        }
    }
    return result;
}

internal u64 *keys_from_hash_table_u64(Arena *arena, Hash_Table *ht)
{
    u64 *result = push_array_no_zero(arena, u64, ht->count);
    for (u64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
        for (Bucket_Node *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
            Assert(cursor < ht->count);
            result[cursor++] = n->v.key_u64;
        }
    }
    return result;
}

internal String8 *keys_from_hash_table_string(Arena *arena, Hash_Table *ht)
{
    String8 *result = push_array_no_zero(arena, String8, ht->count);
    for (u64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
        for (Bucket_Node *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
            Assert(cursor < ht->count);
            result[cursor++] = n->v.key_string;
        }
    }
    return result;
}

internal Key_Value_Pair *key_value_pairs_from_hash_table(Arena *arena, Hash_Table *ht)
{
    Key_Value_Pair *pairs = push_array_no_zero(arena, Key_Value_Pair, ht->count);
    for (u64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
        for (Bucket_Node *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
            Assert(cursor < ht->count);
            pairs[cursor++] = n->v;
        }
    }
    return pairs;
}

internal void *keys_from_hash_table_raw(Arena *arena, Hash_Table *ht)
{
    void **result = push_array(arena, void *, ht->count);
    for (u64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
        for (Bucket_Node *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
            Assert(cursor < ht->count);
            result[cursor++] = n->v.key_raw;
        }
    }
    return result;
}

internal void *values_from_hash_table_raw(Arena *arena, Hash_Table *ht)
{
    void **result = push_array(arena, void *, ht->count);
    for (u64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
        for (Bucket_Node *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
            Assert(cursor < ht->count);
            result[cursor++] = n->v.value_raw;
        }
    }
    return result;
}

#include "third_party/radsort/radsort.h"

internal void sort_key_value_pairs_as_u32(Key_Value_Pair *pairs, u64 count)
{
    radsort(pairs, count, key_value_pair_is_before_u32);
}

internal void sort_key_value_pairs_as_u64(Key_Value_Pair *pairs, u64 count)
{
    radsort(pairs, count, key_value_pair_is_before_u64);
}

internal void sort_key_value_pairs_as_string_sensitive(Key_Value_Pair *pairs, u64 count)
{
    radsort(pairs, count, key_value_pair_is_before_string_sensitive);
}

internal u64_Array remove_duplicates_u64_array(Arena *arena, u64_Array arr)
{
    Temp scratch = scratch_begin(&arena, 1);
    
    Hash_Table *ht = hash_table_init(scratch.arena, ((u64)(f64)arr.count * 0.5));
  
    for (u64 i = 0; i < arr.count; ++i) {
        Bucket_Node *is_present = hash_table_search_u64(ht, arr.v[i]);
        if (!is_present) {
            hash_table_push_u64_raw(scratch.arena, ht, arr.v[i], 0);
        }
    }
  
    u64_Array result = {0};
    result.count    = ht->count;
    result.v        = keys_from_hash_table_u64(arena, ht);
    
    scratch_end(scratch);
    return result;
}

internal String8_List remove_duplicates_str8_list(Arena *arena, String8_List list)
{
    Temp scratch = scratch_begin(&arena, 1);
    
    String8_List  result = {0};
    Hash_Table   *ht     = hash_table_init(scratch.arena, list.node_count);
    
    for (String8_Node *node = list.first; node != 0; node = node->next) {
        Bucket_Node *is_present = hash_table_search_string(ht, node->string);
        if (!is_present) {
            hash_table_push_string_raw(scratch.arena, ht, node->string, 0);
            str8_list_push(arena, &result, node->string);
        }
    }
    
    scratch_end(scratch);
    return result;
}

internal String8 **str8_from_key_value_pairs(Arena *arena, Key_Value_Pair *v, u64 count)
{
    String8 **result = push_array(arena, String8 *, count);
    for EachIndex(i, count) {
        result[i] = &v[i].value_string;
    }
    return result;
}

////////////////////////////////

internal u64 hash_map_hasher(String8 string)
{
    return u64_hash_from_str8(string);
}

internal Hash_Map_Node **hash_map_search_(Hash_Map *hm, u64 hash, Hash_Map_Key key, Hash_Map_Key_Match_Func *match_func, bool32 *found_out)
{
    Hash_Map_Node **n = &hm->root;
    Hash_Map_Node **last_tombstone = 0;
    
    *found_out = 0;
    
    for (u64 h = hash; *n != 0; h <<= 2) {
        if ((*n)->is_live) {
            if (match_func(&(*n)->v.key, &key)) {
                *found_out = 1;
                return n;
            }
        } else {
            last_tombstone = n;
        }
        n = &(*n)->children[h >> 62];
    }
    
    if (last_tombstone != 0) {
        hm->tombstone_count -= 1;
        return last_tombstone;
    }
    
    return n;
}

internal Hash_Map_Node *hash_map_search(Hash_Map *hm, u64 hash, Hash_Map_Key key, Hash_Map_Key_Match_Func *match_func)
{
    bool32           found = 0;
    Hash_Map_Node **n     = hash_map_search_(hm, hash, key, match_func, &found);
    return found ? *n : 0;
}

internal Hash_Map_Node *hash_map_push(Arena *arena, Hash_Map *hm, u64 hash, Hash_Map_Key_Value key_value, Hash_Map_Key_Match_Func *match_func)
{
    bool32           found = 0;
    Hash_Map_Node **n     = hash_map_search_(hm, hash, key_value.key, match_func, &found);
    if (found) { return *n; }
    
    if (*n == 0) {
        *n = hm->free_list;
        if (hm->free_list) { SLLStackPop(hm->free_list);             }
        else               { *n = push_array(arena, Hash_Map_Node, 1); }
    }
    
    (*n)->v       = key_value;
    (*n)->is_live = 1;
    
    hm->count += 1;
    return *n;
}

internal bool32 hash_map_purge_item(Hash_Map *hm, u64 hash, Hash_Map_Key key, Hash_Map_Key_Match_Func *match_func)
{
    bool32           found = 0;
    Hash_Map_Node **n     = hash_map_search_(hm, hash, key, match_func, &found);
    if (found) {
        (*n)->is_live = 0;
        hm->count           -= 1;
        hm->tombstone_count += 1;
    }
    return found;
}

internal void hash_map_purge(Hash_Map *hm)
{
    Temp scratch = scratch_begin(0,0);
    
    typedef struct Stack {
        struct Stack *next;
        Hash_Map_Node *node;
        u32 next_child_idx;
    } Stack;
    
    Stack *free_list = 0;
    Stack *stack     = 0;
    
    if (hm->count > 0) {
        stack = push_array(scratch.arena, Stack, 1);
        stack->node = hm->root;
    }
    
    while (stack) {
        // descend
        while (stack->next_child_idx < ArrayCount(stack->node->children)) {
            Hash_Map_Node *child = stack->node->children[stack->next_child_idx];
            stack->next_child_idx += 1;
            
            if (child) {
                Stack *f = free_list;
                if (f) { SLLStackPop(free_list);                  }
                else   { f = push_array(scratch.arena, Stack, 1); }
                
                f->node = child;
                SLLStackPush(stack, f);
                break;
            }
        }
        
        // ascend
        if (stack->next_child_idx >= ArrayCount(stack->node->children)) {
            SLLStackPush(hm->free_list, stack->node);
            SLLStackPop(stack);
        }
    }
    
    scratch_end(scratch);
}

////////////////////////////////

force_inline HASH_MAP_KEY_MATCH(hash_map_match_u32)    { return a->key_u32 == b->key_u32;                    }
force_inline HASH_MAP_KEY_MATCH(hash_map_match_u64)    { return a->key_u64 == b->key_u64;                    }
force_inline HASH_MAP_KEY_MATCH(hash_map_match_raw)    { return a->key_raw == b->key_raw;                    }
force_inline HASH_MAP_KEY_MATCH(hash_map_match_string) { return str8_match(a->key_string, b->key_string, 0); }
force_inline HASH_MAP_KEY_MATCH(hash_map_match_path)   { return str8_match(a->key_string, b->key_string, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive); }

internal u64 hash_map_hash_from_path(String8 path)
{
    Temp scratch = scratch_begin(0, 0);
    
    String8 path_canon = path;
    path_canon = lower_from_str8(scratch.arena, path_canon);
    path_canon = path_convert_slashes(scratch.arena, path_canon, PathStyle_UnixAbsolute);
    
    u64 hash = hash_map_hasher(path_canon);
    
    scratch_end(scratch);
    return hash;
}

////////////////////////////////

internal Hash_Map_Node *hash_map_push_string_string(Arena *arena, Hash_Map *hm, String8 key, String8 value)
{
    key   = push_str8_copy(arena, key);
    value = push_str8_copy(arena, value);
    return hash_map_push(arena, hm, hash_map_hasher(key), (Hash_Map_Key_Value){ .key = { .key_string = key }, .value = { .value_string = value } }, hash_map_match_string);
}

internal Hash_Map_Node *hash_map_push_string_raw(Arena *arena, Hash_Map *hm, String8 key, void *value)
{
    key = push_str8_copy(arena, key);
    return hash_map_push(arena, hm, hash_map_hasher(key), (Hash_Map_Key_Value){ .key = { .key_string = key }, .value = { .value_raw = value } }, hash_map_match_string);
}

internal Hash_Map_Node *hash_map_push_string_u64(Arena *arena, Hash_Map *hm, String8 key, u64 value)
{
    key = push_str8_copy(arena, key);
    return hash_map_push(arena, hm, hash_map_hasher(key), (Hash_Map_Key_Value){ .key = { .key_string = key }, .value = { .value_u64 = value } }, hash_map_match_string);
}

internal Hash_Map_Node *hash_map_push_u32_raw(Arena *arena, Hash_Map *hm, u32 key, void *value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_u32 = key }, .value = { .value_raw = value } }, hash_map_match_u32);
}

internal Hash_Map_Node *hash_map_push_u32_string(Arena *arena, Hash_Map *hm, u32 key, String8 value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_u32 = key }, .value = { .value_string = value } }, hash_map_match_u32);
}

internal Hash_Map_Node *hash_map_push_u64_raw(Arena *arena, Hash_Map *hm, u64 key, void *value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_u64 = key }, .value = { .value_raw = value } }, hash_map_match_u64);
}

internal Hash_Map_Node *hash_map_push_u64_string(Arena *arena, Hash_Map *hm, u64 key, String8 value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_u64 = key }, .value = { .value_string = value } }, hash_map_match_u64);
}

internal Hash_Map_Node *hash_map_push_u64_u64(Arena *arena, Hash_Map *hm, u64 key, u64 value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_u64 = key }, .value = { .value_u64 = value } }, hash_map_match_u64);
}

internal Hash_Map_Node *hash_map_push_raw_raw(Arena *arena, Hash_Map *hm, void *key, void *value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_raw = key }, .value = { .value_raw = value } }, hash_map_match_raw);
}

internal Hash_Map_Node *hash_map_push_raw_u64(Arena *arena, Hash_Map *hm, void *key, u64 value)
{
    return hash_map_push(arena, hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key_Value){ .key = { .key_raw = key }, .value = { .value_u64 = value } }, hash_map_match_u64);
}

internal Hash_Map_Node *hash_map_push_path_u64(Arena *arena, Hash_Map *hm, String8 path, u64 value)
{
    path = push_str8_copy(arena, path);
    return hash_map_push(arena, hm, hash_map_hash_from_path(path), (Hash_Map_Key_Value){ .key = { .key_string = path }, .value = { .value_u64 = value } }, hash_map_match_path);
}

internal Hash_Map_Node *hash_map_push_path_string(Arena *arena, Hash_Map *hm, String8 path, String8 value)
{
    path  = push_str8_copy(arena, path);
    value = push_str8_copy(arena, value);
    return hash_map_push(arena, hm, hash_map_hash_from_path(path), (Hash_Map_Key_Value){ .key = { .key_string = path }, .value = { .value_string = value } }, hash_map_match_path);
}

internal Hash_Map_Node *hash_map_push_path_raw(Arena *arena, Hash_Map *hm, String8 path, void *value)
{
    path = push_str8_copy(arena, path);
    return hash_map_push(arena, hm, hash_map_hash_from_path(path), (Hash_Map_Key_Value){ .key = { .key_string = path }, .value = { .value_raw = value } }, hash_map_match_path);
}

////////////////////////////////

internal void *hash_map_search_stringf_raw(Hash_Map *hm, char *fmt, ...)
{
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    void *raw = hash_map_search_string_raw(hm, string); 
    va_end(args);
    scratch_end(scratch);
    return raw;
}

internal void *hash_map_search_string_raw(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_string);
    return n ? n->v.value.value_raw : 0;
}

internal String8 *hash_map_search_string_string(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_string);
    return n ? &n->v.value.value_string : 0;
}

internal u32 *hash_map_search_string_u32(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_string);
    return n ? &n->v.value.value_u32 : 0;
}

internal u64 *hash_map_search_string_u64(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_string);
    return n ? &n->v.value.value_u64 : 0;
}

internal u64 *hash_map_search_path_u64(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hash_from_path(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_path);
    return n ? &n->v.value.value_u64 : 0;
}

internal String8 *hash_map_search_path_string(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hash_from_path(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_path);
    return n ? &n->v.value.value_string : 0;
}

internal void *hash_map_search_path_raw(Hash_Map *hm, String8 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hash_from_path(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_path);
    return n ? n->v.value.value_raw : 0;
}

internal void *hash_map_search_u64_raw(Hash_Map *hm, u64 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key){ .key_u64 = key }, hash_map_match_u64);
    return n ? n->v.value.value_raw : 0;
}

internal u64 *hash_map_search_u64_u64(Hash_Map *hm, u64 key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key){ .key_u64 = key }, hash_map_match_u64);
    return n ? &n->v.value.value_u64 : 0;
}

internal void *hash_map_search_raw_raw(Hash_Map *hm, void *key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key){ .key_raw = key }, hash_map_match_raw);
    return n ? n->v.value.value_raw : 0;
}

internal u64 *hash_map_search_raw_u64(Hash_Map *hm, void *key)
{
    Hash_Map_Node *n = hash_map_search(hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key){ .key_raw = key }, hash_map_match_raw);
    return n ? &n->v.value.value_u64 : 0;
}

////////////////////////////////

internal bool32 hash_map_purge_u32(Hash_Map *hm, u32 key)        { return hash_map_purge_item(hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key){ .key_u32 = key }, hash_map_match_u32); }
internal bool32 hash_map_purge_u64(Hash_Map *hm, u64 key)        { return hash_map_purge_item(hm, hash_map_hasher(str8_struct(&key)), (Hash_Map_Key){ .key_u64 = key }, hash_map_match_u64); }
internal bool32 hash_map_purge_string(Hash_Map *hm, String8 key) { return hash_map_purge_item(hm, hash_map_hasher(key), (Hash_Map_Key){ .key_string = key }, hash_map_match_string);         }

////////////////////////////////

#define HASH_MAP_EXTRACT_FUNC(name) void name(Hash_Map_Key_Value *kv, void *buffer)
typedef HASH_MAP_EXTRACT_FUNC(Hash_Map_Extract_Func);

typedef struct
{
    void *keys;
    void *values;
} Hash_Map_Extract;

internal Hash_Map_Extract
hash_map_extract(Arena               *arena,
                 Hash_Map             *hm,
                 Hash_Map_Extract_Func  *key_func,   u64 key_size,
                 Hash_Map_Extract_Func  *value_func, u64 value_size)
{
    Temp scratch = scratch_begin(&arena, 1);
    
    u64  key_offset   = 0;
    u64  value_offset = 0;
    u8  *key_buffer   = 0;
    u8  *value_buffer = 0;
    if (key_func && key_size)     { key_buffer   = push_array_no_zero(arena, u8, key_size * hm->count);   }
    if (value_func && value_size) { value_buffer = push_array_no_zero(arena, u8, value_size * hm->count); }
    
    typedef struct Stack {
        struct Stack *next;
        Hash_Map_Node *node;
        u32 next_child_idx;
    } Stack;
    
    Stack *free_list = 0;
    Stack *stack     = 0;
    
    if (hm->count > 0) {
        stack = push_array(scratch.arena, Stack, 1);
        stack->node = hm->root;
    }
    
    while (stack) {
        // descend
        while (stack->next_child_idx < ArrayCount(stack->node->children)) {
            Hash_Map_Node *child = stack->node->children[stack->next_child_idx];
            stack->next_child_idx += 1;
            
            if (child) {
                Stack *f = free_list;
                if (f) { SLLStackPop(free_list);                  }
                else   { f = push_array(scratch.arena, Stack, 1); }
                
                f->node = child;
                SLLStackPush(stack, f);
                break;
            }
        }
        
        // ascend
        if (stack->next_child_idx >= ArrayCount(stack->node->children)) {
            
            if (stack->node->is_live) {
                if (key_func) {
                    key_func(&stack->node->v, key_buffer + key_offset);
                    key_offset += key_size;
                }
                if (value_func) {
                    value_func(&stack->node->v, value_buffer + value_offset);
                    value_offset += value_size;
                }
            }
            
            SLLStackPop(stack);
        }
    }
    
    scratch_end(scratch);
    return (Hash_Map_Extract){ .keys = key_buffer, .values = value_buffer };
}

// { keys, value }
force_inline HASH_MAP_EXTRACT_FUNC(hash_map_extract_key_value)  { MemoryCopy(buffer, kv, sizeof(*kv));                                 }
internal Hash_Map_Key_Value * key_value_from_hash_map  ( Arena *arena, Hash_Map *hm) { return hash_map_extract(arena, hm, hash_map_extract_key_value,  sizeof(Hash_Map_Key_Value), 0, 0).keys; }

// keys
force_inline HASH_MAP_EXTRACT_FUNC(hash_map_extract_key_u32)    { MemoryCopy(buffer, &kv->key.key_u32, sizeof(kv->key.key_u32));       }
force_inline HASH_MAP_EXTRACT_FUNC(hash_map_extract_key_u64)    { MemoryCopy(buffer, &kv->key.key_u64, sizeof(kv->key.key_u64));       }
force_inline HASH_MAP_EXTRACT_FUNC(hash_map_extract_key_string) { MemoryCopy(buffer, &kv->key.key_string, sizeof(kv->key.key_string)); }
internal u32 *     keys_from_hash_map_u32   ( Arena *arena, Hash_Map *hm) { return hash_map_extract(arena, hm, hash_map_extract_key_u32,    sizeof(u32),     0, 0).keys; }
internal u64 *     keys_from_hash_map_u64   ( Arena *arena, Hash_Map *hm) { return hash_map_extract(arena, hm, hash_map_extract_key_u64,    sizeof(u64),     0, 0).keys; }
internal String8 * keys_from_hash_map_string( Arena *arena, Hash_Map *hm) { return hash_map_extract(arena, hm, hash_map_extract_key_string, sizeof(String8), 0, 0).keys; }
internal void *    keys_from_hash_map_raw   ( Arena *arena, Hash_Map *hm) { return hash_map_extract(arena, hm, hash_map_extract_key_string, sizeof(void *),  0, 0).keys; }

// values
force_inline HASH_MAP_EXTRACT_FUNC(hash_map_extract_value_u64) { MemoryCopy(buffer, &kv->value.value_u64, sizeof(kv->value.value_u64)); }
force_inline HASH_MAP_EXTRACT_FUNC(hash_map_extract_value_raw) { MemoryCopy(buffer, &kv->value.value_raw, sizeof(kv->value.value_raw)); }
internal u64 * values_from_hash_map_u64(Arena *arena, Hash_Map *hm) { return hash_map_extract(arena, hm, 0, 0, hash_map_extract_value_u64, sizeof(u64)).values;    }
internal void * values_from_hash_map_raw(Arena *arena, Hash_Map *hm) {return hash_map_extract(arena, hm, 0, 0, hash_map_extract_value_raw, sizeof(void *)).values; }

////////////////////////////////

force_inline int hash_map_key_is_before_u32(void *a, void *b)    { return ((Hash_Map_Key_Value*)a)->key.key_u32 < ((Hash_Map_Key_Value*)b)->key.key_u32; }
force_inline int hash_map_key_is_before_u64(void *a, void *b)    { return ((Hash_Map_Key_Value*)a)->key.key_u64 < ((Hash_Map_Key_Value*)b)->key.key_u64; }
force_inline int hash_map_key_is_before_string(void *a, void *b) { return str8_is_before_case_sensitive(a, b);                                     }

internal void sort_hash_map_key_value_u32(Hash_Map_Key_Value *pairs, u64 count)    { radsort(pairs, count, hash_map_key_is_before_u32);    }
internal void sort_hash_map_key_value_u64(Hash_Map_Key_Value *pairs, u64 count)    { radsort(pairs, count, hash_map_key_is_before_u64);    }
internal void sort_hash_map_key_value_string(Hash_Map_Key_Value *pairs, u64 count) { radsort(pairs, count, hash_map_key_is_before_string); }

