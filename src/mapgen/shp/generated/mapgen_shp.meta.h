// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef MAPGEN_SHP_META_H
#define MAPGEN_SHP_META_H

typedef enum MAP_SHP_ShapeKind
{
MAP_SHP_ShapeKind_Null                 = 0,
MAP_SHP_ShapeKind_Point                = 1,
MAP_SHP_ShapeKind_PolyLine             = 3,
MAP_SHP_ShapeKind_Polygon              = 5,
MAP_SHP_ShapeKind_MultiPoint           = 8,
MAP_SHP_ShapeKind_PointZ               = 11,
MAP_SHP_ShapeKind_PolyLineZ            = 13,
MAP_SHP_ShapeKind_PolygonZ             = 15,
MAP_SHP_ShapeKind_MultiPointZ          = 18,
MAP_SHP_ShapeKind_PointM               = 21,
MAP_SHP_ShapeKind_PolyLineM            = 23,
MAP_SHP_ShapeKind_PolygonM             = 25,
MAP_SHP_ShapeKind_MultiPointM          = 28,
MAP_SHP_ShapeKind_MultiPatch           = 31,
} MAP_SHP_ShapeKind;

internal String8 map_shp_string_from_shape_kind(MAP_SHP_ShapeKind v);
#endif // MAPGEN_SHP_META_H
