
#ifndef BASE_HASH_TABLE_H
#define BASE_HASH_TABLE_H

typedef struct Key_Value_Pair Key_Value_Pair;
struct Key_Value_Pair {
    union
    {
        String8 key_string;
        void *key_raw;
        u32 key_u32;
        u64 key_u64;
    };
    union
    {
        String8 value_string;
        void *value_raw;
        u32 value_u32;
        u64 value_u64;
    };
};

typedef struct Bucket_Node Bucket_Node;
struct Bucket_Node {
    Key_Value_Pair v;
    struct Bucket_Node *next;
    struct Bucket_Node *prev;
};

typedef struct Bucket_List Bucket_List;
struct Bucket_List {
    Bucket_Node *first;
    Bucket_Node *last;
};

typedef struct Hash_Table Hash_Table;
struct Hash_Table {
    u64 count;
    u64 cap;
    Bucket_List *buckets;
    Bucket_List free_buckets;
};

// Bucket List

internal void         bucket_list_concat_in_place(Bucket_List *list, Bucket_List *to_concat);
internal Bucket_Node *bucket_list_pop(Bucket_List *list);

// Hash Table

internal u64 hash_table_hasher(String8 string);

internal Hash_Table *hash_table_init(Arena *arena, u64 cap);
internal void        hash_table_purge(Hash_Table *ht);

internal Bucket_Node *hash_table_push(Arena *arena, Hash_Table *ht, u64 hash, Key_Value_Pair kv);

internal Bucket_Node *hash_table_push_u32_string   (Arena *arena, Hash_Table *ht, u32     key, String8 value);
internal Bucket_Node *hash_table_push_u64_string   (Arena *arena, Hash_Table *ht, u64     key, String8 value);
internal Bucket_Node *hash_table_push_string_string(Arena *arena, Hash_Table *ht, String8 key, String8 value);
internal Bucket_Node *hash_table_push_string_raw   (Arena *arena, Hash_Table *ht, String8 key, void   *value);
internal Bucket_Node *hash_table_push_string_u64   (Arena *arena, Hash_Table *ht, String8 key, u64     value);
internal Bucket_Node *hash_table_push_path_string  (Arena *arena, Hash_Table *ht, String8 key, String8 value);
internal Bucket_Node *hash_table_push_u32_raw      (Arena *arena, Hash_Table *ht, u32     key, void   *value);
internal Bucket_Node *hash_table_push_u64_raw      (Arena *arena, Hash_Table *ht, u64     key, void   *value);
internal Bucket_Node *hash_table_push_path_raw     (Arena *arena, Hash_Table *ht, String8 key, void   *value);
internal Bucket_Node *hash_table_push_path_u64     (Arena *arena, Hash_Table *ht, String8 key, u64     value);
internal Bucket_Node *hash_table_push_u64_u64      (Arena *arena, Hash_Table *ht, u64     key, u64     value);
internal Bucket_Node *hash_table_push_u32_u32      (Arena *arena, Hash_Table *ht, u32     key, u32     value);
internal Bucket_Node *hash_table_push_raw_raw      (Arena *arena, Hash_Table *ht, void   *key, void   *value);

internal Bucket_Node *hash_table_search_string (Hash_Table *ht, String8 key);
internal Bucket_Node *hash_table_search_u32    (Hash_Table *ht, u32     key);
internal Bucket_Node *hash_table_search_u64    (Hash_Table *ht, u64     key);
internal Bucket_Node *hash_table_search_path   (Hash_Table *ht, String8 key);
internal Bucket_Node *hash_table_search_raw    (Hash_Table *ht, void   *key);

internal bool32 hash_table_search_path_u64     (Hash_Table *ht, String8 key, u64     *value_out);
internal bool32 hash_table_search_string_u64   (Hash_Table *ht, String8 key, u64     *value_out);
internal bool32 hash_table_search_string_string(Hash_Table *ht, String8 key, String8 *value_out);
internal bool32 hash_table_search_u32_u32      (Hash_Table *ht, u32     key, u32     *value_out);

internal void *hash_table_search_string_raw(Hash_Table *ht, String8 key);
internal void *hash_table_search_u64_raw   (Hash_Table *ht, u64     key);
internal void *hash_table_search_path_raw  (Hash_Table *ht, String8 key);
internal void *hash_table_search_raw_raw   (Hash_Table *ht, void   *key);

internal void   hash_table_purge_item  (Hash_Table *ht, u64 hash, Bucket_Node *node);
internal bool32 hash_table_purge_u64   (Hash_Table *ht, u64 key);
internal bool32 hash_table_purge_string(Hash_Table *ht, String8 key);

// Key-Value helpers

internal u32     *keys_from_hash_table_u32   (Arena *arena, Hash_Table *ht);
internal u64     *keys_from_hash_table_u64   (Arena *arena, Hash_Table *ht);
internal void    *keys_from_hash_table_raw   (Arena *arena, Hash_Table *ht);
internal String8 *keys_from_hash_table_string(Arena *arena, Hash_Table *ht);

internal Key_Value_Pair *key_value_pairs_from_hash_table(Arena *arena, Hash_Table *ht);

internal void sort_key_value_pairs_as_u32             (Key_Value_Pair *pairs, u64 count);
internal void sort_key_value_pairs_as_u64             (Key_Value_Pair *pairs, u64 count);
internal void sort_key_value_pairs_as_string_sensitive(Key_Value_Pair *pairs, u64 count);

internal String8 **string_from_key_value_pairs(Arena *arena, Key_Value_Pair *v, u64 count);

// Misc

internal u64_Array    remove_duplicates_u64_array(Arena *arena, u64_Array arr);
internal String8_List remove_duplicates_string_list(Arena *arena, String8_List list);

// Hash Map

typedef union Hash_Map_Key Hash_Map_Key;
union Hash_Map_Key {
    String8 key_string;
    void   *key_raw;
    u32     key_u32;
    u64     key_u64;
};

typedef union Hash_Map_Value Hash_Map_Value;
union Hash_Map_Value {
    String8 value_string;
    void   *value_raw;
    u32     value_u32;
    u64     value_u64;
};

typedef struct Hash_Map_Key_Value Hash_Map_Key_Value;
struct Hash_Map_Key_Value {
    Hash_Map_Key   key;
    Hash_Map_Value value;
};

typedef union Hash_Map_Node Hash_Map_Node;
union Hash_Map_Node {
    struct {
        Hash_Map_Node     *children[4];
        Hash_Map_Key_Value v;
        bool32             is_live;
    };
    Hash_Map_Node *next;
};

typedef struct Hash_Map Hash_Map;
struct Hash_Map {
    Hash_Map_Node *root;
    u64            count;
    u64            tombstone_count;
    Hash_Map_Node *free_list;
};

#define HASH_MAP_KEY_MATCH(name) bool32 name(Hash_Map_Key *a, Hash_Map_Key *b)
typedef HASH_MAP_KEY_MATCH(Hash_Map_Key_Match_Func);

internal u64 hash_map_hasher(String8 string);

internal Hash_Map_Node *hash_map_search             (Hash_Map *hm, u64 hash, Hash_Map_Key             key, Hash_Map_Key_Match_Func *match_func);
internal Hash_Map_Node *hash_map_push (Arena *arena, Hash_Map *hm, u64 hash, Hash_Map_Key_Value key_value, Hash_Map_Key_Match_Func *match_func);
internal bool32         hash_map_purge_item         (Hash_Map *hm, u64 hash, Hash_Map_Key             key, Hash_Map_Key_Match_Func *match_func);
internal void           hash_map_purge              (Hash_Map *hm);

internal Hash_Map_Node *hash_map_push_string_string(Arena *arena, Hash_Map *hm, String8 key, String8  value);
internal Hash_Map_Node *hash_map_push_string_raw   (Arena *arena, Hash_Map *hm, String8 key, void    *value);
internal Hash_Map_Node *hash_map_push_string_u64   (Arena *arena, Hash_Map *hm, String8 key, u64      value);
internal Hash_Map_Node *hash_map_push_u32_raw      (Arena *arena, Hash_Map *hm, u32     key, void    *value);
internal Hash_Map_Node *hash_map_push_u32_string   (Arena *arena, Hash_Map *hm, u32     key, String8  value);
internal Hash_Map_Node *hash_map_push_u64_raw      (Arena *arena, Hash_Map *hm, u64     key, void    *value);
internal Hash_Map_Node *hash_map_push_u64_string   (Arena *arena, Hash_Map *hm, u64     key, String8  value);
internal Hash_Map_Node *hash_map_push_u64_u64      (Arena *arena, Hash_Map *hm, u64     key, u64      value);
internal Hash_Map_Node *hash_map_push_raw_raw      (Arena *arena, Hash_Map *hm, void   *key, void    *value);
internal Hash_Map_Node *hash_map_push_raw_u64      (Arena *arena, Hash_Map *hm, void   *key, u64      value);
internal Hash_Map_Node *hash_map_push_path_u64     (Arena *arena, Hash_Map *hm, String8 key, u64      value);
internal Hash_Map_Node *hash_map_push_path_string  (Arena *arena, Hash_Map *hm, String8 key, String8  value);
internal Hash_Map_Node *hash_map_push_path_raw     (Arena *arena, Hash_Map *hm, String8 key, void    *value);

internal void    *hash_map_search_stringf_raw  (Hash_Map *hm, char *fmt, ...);
internal void    *hash_map_search_string_raw   (Hash_Map *hm, String8 key);
internal String8 *hash_map_search_string_string(Hash_Map *hm, String8 key);
internal u32     *hash_map_search_string_u32   (Hash_Map *hm, String8 key);
internal u64     *hash_map_search_string_u64   (Hash_Map *hm, String8 key);
internal u64     *hash_map_search_path_u64     (Hash_Map *hm, String8 key);
internal String8 *hash_map_search_path_string  (Hash_Map *hm, String8 key);
internal void    *hash_map_search_path_raw     (Hash_Map *hm, String8 key);
internal void    *hash_map_search_u64_raw      (Hash_Map *hm, u64     key);
internal u64     *hash_map_search_u64_u64      (Hash_Map *hm, u64     key);
internal void    *hash_map_search_raw_raw      (Hash_Map *hm, void   *key);
internal u64     *hash_map_search_raw_u64      (Hash_Map *hm, void   *key);

internal Hash_Map_Key_Value *key_value_from_hash_map  (Arena *arena, Hash_Map *hm);
internal u32                *keys_from_hash_map_u32   (Arena *arena, Hash_Map *hm);
internal u64                *keys_from_hash_map_u64   (Arena *arena, Hash_Map *hm);
internal String8            *keys_from_hash_map_string(Arena *arena, Hash_Map *hm);
internal void               *keys_from_hash_map_raw   (Arena *arena, Hash_Map *hm);
internal u64                *values_from_hash_map_u64 (Arena *arena, Hash_Map *hm);

internal void sort_hash_map_key_value_u32   (Hash_Map_Key_Value *pairs, u64 count);
internal void sort_hash_map_key_value_u64   (Hash_Map_Key_Value *pairs, u64 count);
internal void sort_hash_map_key_value_string(Hash_Map_Key_Value *pairs, u64 count);

#endif // BASE_HASH_TABLE_H
