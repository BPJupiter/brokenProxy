
#ifndef BASE_EXT_ARRAYS_H
#define BASE_EXT_ARRAYS_H

typedef struct u64_Node u64_Node;
struct u64_Node {
    u64 data;
    struct u64_Node *next;
};

typedef struct u64_List u64_List;
struct u64_List {
    u64 count;
    u64_Node *first;
    u64_Node *last;
};

/////////////////////////

internal void u64_list_push_node(u64_List *list, u64_Node *n);
internal u64_Node *u64_list_push(Arena *arena, u64_List *list, u64 v);
internal void u64_list_concat_in_place(u64_List *list, u64_List *to_concat);

internal u64      *offsets_from_counts_array_u64(Arena *arena, u64 *v, u64 count);
internal void      u64_array_counts_to_offsets  (u64 count, u64 *arr);
internal void      u64_array_sort               (u64 count, u64 *v);
internal u64       u64_array_max                (u64 count, u64 *v);
internal u64       u64_array_min                (u64 count, u64 *v);
internal u64       sum_array_u64                (u64 count, u64 *v);
internal u64_Array u64_array_remove_duplicates  (Arena *arena, u64_Array in);
internal u64_Array u64_array_from_list          (Arena *arena, u64_List *list);

#endif // BASE_EXT_ARRAYS_H
