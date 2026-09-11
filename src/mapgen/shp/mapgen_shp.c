
#include "generated/mapgen_shp.meta.c"

internal u16 map_shp_read_le_u16(u8 *base, u64 *off)
{
    u16 *data16 = (u16 *)(base + *off);
    u16 le = *data16;
    *off += sizeof(le);
    return le;
}

internal u32 map_shp_read_le_u32(u8 *base, u64 *off)
{
    u32 *data32 = (u32 *)(base + *off);
    u32 le = *data32;
    *off += sizeof(le);
    return le;
}

internal s32 map_shp_read_le_s32(u8 *base, u64 *off)
{
    return (s32)map_shp_read_le_u32(base, off);
}

internal s32 map_shp_read_be_s32(u8 *base, u64 *off)
{
    u32 *data32 = (u32 *)(base + *off);
    s32 be = *data32;
    *off += sizeof(be);
    return (s32)from_be_u32(be);
}

internal f64 map_shp_read_le_f64(u8 *base, u64 *off)
{
    f64 le;
    MemoryCopy(&le, (base + *off), sizeof(le));
    *off += sizeof(le);
    return le;
}


