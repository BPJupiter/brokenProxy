#ifndef MAPGEN_SHP_H
#define MAPGEN_SHP_H

#include "generated/mapgen_shp.meta.h"

internal u16 map_shp_read_le_u16(u8 *base, u64 *off);
internal u32 map_shp_read_le_u32(u8 *base, u64 *off);
internal s32 map_shp_read_le_s32(u8 *base, u64 *off);
internal s32 map_shp_read_be_s32(u8 *base, u64 *off);
internal f64 map_shp_read_le_f64(u8 *base, u64 *off);



#endif // MAPGEN_SHP_H
