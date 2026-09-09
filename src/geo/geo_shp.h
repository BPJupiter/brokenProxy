#ifndef GEO_SHP_H
#define GEO_SHP_H

typedef enum GEO_SHP_ShapeKind {
    GEO_SHP_ShapeKind_Null        = 0,
    GEO_SHP_ShapeKind_Point       = 1,
    GEO_SHP_ShapeKind_PolyLine    = 3,
    GEO_SHP_ShapeKind_Polygon     = 5,
    GEO_SHP_ShapeKind_MultiPoint  = 8,
    GEO_SHP_ShapeKind_PointZ      = 11,
    GEO_SHP_ShapeKind_PolyLineZ   = 13,
    GEO_SHP_ShapeKind_PolygonZ    = 15,
    GEO_SHP_ShapeKind_MultiPointZ = 18,
    GEO_SHP_ShapeKind_PointM      = 21,
    GEO_SHP_ShapeKind_PolyLineM   = 23,
    GEO_SHP_ShapeKind_PolygonM    = 25,
    GEO_SHP_ShapeKind_MultiPointM = 28,
    GEO_SHP_ShapeKind_MultiPatch  = 31,
} GEO_SHP_ShapeKind;

typedef struct GEO_SHP_Record GEO_SHP_Record;
struct GEO_SHP_Record
{
    GEO_SHP_ShapeKind shape_type;
    f64 box[4];
    s32_Array parts;
    Vec3f32_Array points;
};

typedef struct GEO_SHP_File GEO_SHP_File;
struct GEO_SHP_File
{
    bool32 valid;
    s32 shape_type;
    f64 box[4];
    u64 record_count;
    GEO_SHP_Record *records;
};

typedef struct GEO_DBF_Field GEO_DBF_Field;
struct GEO_DBF_Field
{
    u8 name[12];
    u8 type;
    u8 length;
    u8 decimal_count;
};

typedef struct GEO_DBF_Record GEO_DBF_Record;
struct GEO_DBF_Record
{
    bool32 deleted;
    String8 data;
};

typedef struct GEO_DBF_File GEO_DBF_File;
struct GEO_DBF_File
{
    bool32 valid;
    u64 field_count;
    GEO_DBF_Field *fields;
    u64 record_count;
    GEO_DBF_Record *records;
};

typedef struct GEO_Render_Feature GEO_Render_Feature;
struct GEO_Render_Feature
{
    Rng2f32 aabb;
    u64 vertex_base;
    u32_Array indices;
    R_Handle index_buffer;
};

typedef struct GEO_Render_Map GEO_Render_Map;
struct GEO_Render_Map
{
    Vec3f32_Array vertices;
    R_Handle vertex_buffer;
    GEO_Render_Feature *features;
    u64 feature_count;
};

internal u16 geo_read_le_u16(u8 *data);
internal u32 geo_read_le_u32(u8 *data);
internal s32 geo_read_le_s32(u8 *data);
internal s32 geo_read_be_s32(u8 *data);
internal f64 geo_read_le_f64(u8 *data);

internal GEO_SHP_File geo_shp_file_from_data(Arena *arena, String8 data);
internal GEO_DBF_File geo_dbf_file_from_data(Arena *arena, String8 data);

internal f32 geo_ring_signed_area2(Vec2f32 *pts, u64 start, u64 count);
internal bool32 geo_point_in_ring(Vec3f32 p, Vec2f32 *pts, u64 start, u64 count);

#endif // GEO_SHP_H
