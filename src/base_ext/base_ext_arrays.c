
internal void u64_list_push_node(u64_List *list, u64_Node *n)
{
    SLLQueuePush(list->first, list->last, n);
    list->count += 1;
}

internal u64_Node *u64_list_push(Arena *arena, u64_List *list, u64 data)
{
    u64_Node *n = push_array(arena, u64_Node, 1);
    n->next = 0;
    n->data = data;
    u64_list_push_node(list, n);
    return n;
}

internal void u64_list_concat_in_place(u64_List *list, u64_List *to_concat)
{
    SLLConcatInPlace(list, to_concat);
}

internal u64 *offsets_from_counts_array_u64(Arena *arena, u64 *v, u64 count)
{
    u64 *result = push_array_copy_u64(arena, v, count);
    u64_array_counts_to_offsets(count, result);
    return result;
}

internal void u64_array_counts_to_offsets(u64 count, u64 *arr)
{
    u64 next_offset = 0;
    for (u64 i = 0; i < count; i += 1) {
        u64 current_offset = next_offset;
        next_offset += arr[i];
        arr[i] = current_offset;
    }
}

internal void u64_array_sort(u64 count, u64 *v)
{
    radsort(v, count, u64_is_before);
}

internal u64 u64_array_max(u64 count, u64 *v)
{
    u64 result = 0;
    for (u64 i = 0; i < count; i++) {
        result = Max(v[i], result);
    }
    return result;
}

internal u64 u64_array_min(u64 count, u64 *v)
{
    u64 result = max_u64;
    for (u64 i = 0; i < count; i++) {
        result = Min(v[i], result);
    }
    return result;
}

internal u64 sum_array_u64(u64 count, u64 *v)
{
    u64 result = 0;
    for (u64 i = 0; i < count; i++) {
        result += v[i];
    }
    return result;
}

internal u64_Array u64_array_remove_duplicates(Arena *arena, u64_Array in)
{
    u64_Array result;
    result.count = 0;
    result.v = push_array(arena, u64, in.count);

    for (u64 i = 1; i < in.count; ++i) {
        bool32 is_unique = in.v[i - 1] != in.v[i];
        if (is_unique) {
            result.v[result.count++] = in.v[i - 1];
        }
    }

    if (in.count > 0 && result.count > 0) {
        bool32 is_unique = result.v[result.count - 1] != in.v[in.count - 1];
        if (is_unique) {
            result.v[result.count++] = in.v[in.count - 1];
        }
    }

    u64 slack_size = (in.count - result.count) * sizeof(result.v[0]);
    arena_pop(arena, slack_size);

    return result;
}

internal u64_Array u64_array_from_list(Arena *arena, u64_List *list)
{
    u64_Array result = {0};
    result.v = push_array(arena, u64, list->count);
    for EachNode(n, u64_Node, list->first) { result.v[result.count++] = n->data; }
    return result;
}


