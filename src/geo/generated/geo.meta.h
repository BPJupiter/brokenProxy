// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef GEO_META_H
#define GEO_META_H

typedef enum GEO_Type
{
GEO_Type_Null,
GEO_Type_Feature,
GEO_Type_FeatureCollection,
GEO_Type_Point,
GEO_Type_MultiPoint,
GEO_Type_LineString,
GEO_Type_MultiLineString,
GEO_Type_Polygon,
GEO_Type_MultiPolygon,
GEO_Type_GeometryCollection,
GEO_Type_COUNT,
} GEO_Type;

internal String8 gj_string_from_type(GEO_Type v);
internal GEO_Type gj_type_from_string(String8 v);
internal bool32 gj_is_type_geometry(GEO_Type v);
#endif // GEO_META_H
