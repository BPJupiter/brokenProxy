
internal u16 map_shp_read_le_u16(u8 *data)
{
    u16 *data16 = (u16 *)data;
    u16 le = *data16;
    return le;
}

internal u32 map_shp_read_le_u32(u8 *data)
{
    u32 *data32 = (u32 *)data;
    u32 le = *data32;
    return le;
}

internal s32 map_shp_read_le_s32(u8 *data)
{
    return (s32)map_shp_read_le_u32(data);
}

internal s32 map_shp_read_be_s32(u8 *data)
{
    u32 *data32 = (u32 *)data;
    s32 be = *data32;
    return (s32)from_be_u32(le);
}

internal f64 map_shp_read_le_f64(u8 *data)
{
    f64 le;
    MemoryCopy(&le, data, sizeof(le));
    return le;
}


