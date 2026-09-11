// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

internal String8
map_shp_string_from_shape_kind(MAP_SHP_ShapeKind v)
{
String8 result = str8_lit("<Unknown MAP_SHP_ShapeKind>");
switch(v)
{
default:{}break;
case MAP_SHP_ShapeKind_Null: {result = str8_lit("Null");}break;
case MAP_SHP_ShapeKind_Point: {result = str8_lit("Point");}break;
case MAP_SHP_ShapeKind_PolyLine: {result = str8_lit("PolyLine");}break;
case MAP_SHP_ShapeKind_Polygon: {result = str8_lit("Polygon");}break;
case MAP_SHP_ShapeKind_MultiPoint: {result = str8_lit("MultiPoint");}break;
case MAP_SHP_ShapeKind_PointZ: {result = str8_lit("PointZ");}break;
case MAP_SHP_ShapeKind_PolyLineZ: {result = str8_lit("PolyLineZ");}break;
case MAP_SHP_ShapeKind_PolygonZ: {result = str8_lit("PolygonZ");}break;
case MAP_SHP_ShapeKind_MultiPointZ: {result = str8_lit("MultiPointZ");}break;
case MAP_SHP_ShapeKind_PointM: {result = str8_lit("PointM");}break;
case MAP_SHP_ShapeKind_PolyLineM: {result = str8_lit("PolyLineM");}break;
case MAP_SHP_ShapeKind_PolygonM: {result = str8_lit("PolygonM");}break;
case MAP_SHP_ShapeKind_MultiPointM: {result = str8_lit("MultiPointM");}break;
case MAP_SHP_ShapeKind_MultiPatch: {result = str8_lit("MultiPatch");}break;
}
return result;
}

