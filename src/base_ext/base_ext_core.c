
internal int u16_compar(const void *raw_a, const void *raw_b)
{
    u16 a = *(u16 *)raw_a;
    u16 b = *(u16 *)raw_b;
    int result = a < b ? -1 :
                 a > b ? +1 :
                 0;
    return result;
}

internal int u32_compar(const void *raw_a, const void *raw_b)
{
    u32 a = *(u32 *)raw_a;
    u32 b = *(u32 *)raw_b;
    int result = a < b ? -1 :
                 a > b ? +1 :
                 0;
    return result;
}

internal int u64_compar(const void *raw_a, const void *raw_b)
{
    u64 a = *(u64 *)raw_a;
    u64 b = *(u64 *)raw_b;
    int result = a < b ? -1 :
                 a > b ? +1 :
                 0;
    return result;
}

internal int u8_is_before(void *raw_a, void *raw_b)
{
    u8 *a = (u8 *)raw_a;
    u8 *b = (u8 *)raw_b;
    return *a < *b;
}

internal int u16_is_before(void *raw_a, void *raw_b)
{
    u16 *a = (u16 *)raw_a;
    u16 *b = (u16 *)raw_b;
    return *a < *b;
}

internal int u32_is_before(void *raw_a, void *raw_b)
{
    u32 *a = (u32 *)raw_a;
    u32 *b = (u32 *)raw_b;
    return *a < *b;
}

internal int u64_is_before(void *raw_a, void *raw_b)
{
    u64 *a = (u64 *)raw_a;
    u64 *b = (u64 *)raw_b;
    return *a < *b;
}
