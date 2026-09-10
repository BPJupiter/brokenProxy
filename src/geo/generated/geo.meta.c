// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

internal String8
gj_string_from_type(GEO_Type v)
{
String8 result = str8_lit("<Unknown GEO_Type>");
switch(v)
{
default:{}break;
case GEO_Type_Feature:{result = str8_lit("Feature");}break;
case GEO_Type_FeatureCollection:{result = str8_lit("FeatureCollection");}break;
case GEO_Type_Point:{result = str8_lit("Point");}break;
case GEO_Type_MultiPoint:{result = str8_lit("MultiPoint");}break;
case GEO_Type_LineString:{result = str8_lit("LineString");}break;
case GEO_Type_MultiLineString:{result = str8_lit("MultiLineString");}break;
case GEO_Type_Polygon:{result = str8_lit("Polygon");}break;
case GEO_Type_MultiPolygon:{result = str8_lit("MultiPolygon");}break;
case GEO_Type_GeometryCollection:{result = str8_lit("GeometryCollection");}break;
}
return result;
}

internal bool32
gj_is_type_geometry(GEO_Type v)
{
bool32 result = 0;
switch(v)
{
default:{}break;
case GEO_Type_Feature:{result = 0;}break;
case GEO_Type_FeatureCollection:{result = 0;}break;
case GEO_Type_Point:{result = 1;}break;
case GEO_Type_MultiPoint:{result = 1;}break;
case GEO_Type_LineString:{result = 1;}break;
case GEO_Type_MultiLineString:{result = 1;}break;
case GEO_Type_Polygon:{result = 1;}break;
case GEO_Type_MultiPolygon:{result = 1;}break;
case GEO_Type_GeometryCollection:{result = 1;}break;
}
return result;
}
