#ifndef MAPGEN_SHP_H
#define MAPGEN_SHP_H

typedef enum MAP_SHP_ShapeKind {
    MAP_SHP_ShapeKind_Null        = 0,
    MAP_SHP_ShapeKind_Point       = 1,
    MAP_SHP_ShapeKind_PolyLine    = 3,
    MAP_SHP_ShapeKind_Polygon     = 5,
    MAP_SHP_ShapeKind_MultiPoint  = 8,
    MAP_SHP_ShapeKind_PointZ      = 11,
    MAP_SHP_ShapeKind_PolyLineZ   = 13,
    MAP_SHP_ShapeKind_PolygonZ    = 15,
    MAP_SHP_ShapeKind_MultiPointZ = 18,
    MAP_SHP_ShapeKind_PointM      = 21,
    MAP_SHP_ShapeKind_PolyLineM   = 23,
    MAP_SHP_ShapeKind_PolygonM    = 25,
    MAP_SHP_ShapeKind_MultiPointM = 28,
    MAP_SHP_ShapeKind_MultiPatch  = 31,
} MAP_SHP_ShapeKind;

internal u16 map_shp_read_le_u16(u8 *data);
internal u32 map_shp_read_le_u32(u8 *data);
internal s32 map_shp_read_le_s32(u8 *data);
internal s32 map_shp_read_be_s32(u8 *data);
internal f64 map_shp_read_le_f64(u8 *data);

#endif // MAPGEN_SHP_H
