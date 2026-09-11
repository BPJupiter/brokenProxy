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

internal GEO_Type
gj_type_from_string(String8 v)
{
GEO_Type result = 0;
if (0) {}
else if (str8_match(v, str8_lit("Feature"), 0)) {result = GEO_Type_Feature;}
else if (str8_match(v, str8_lit("FeatureCollection"), 0)) {result = GEO_Type_FeatureCollection;}
else if (str8_match(v, str8_lit("Point"), 0)) {result = GEO_Type_Point;}
else if (str8_match(v, str8_lit("MultiPoint"), 0)) {result = GEO_Type_MultiPoint;}
else if (str8_match(v, str8_lit("LineString"), 0)) {result = GEO_Type_LineString;}
else if (str8_match(v, str8_lit("MultiLineString"), 0)) {result = GEO_Type_MultiLineString;}
else if (str8_match(v, str8_lit("Polygon"), 0)) {result = GEO_Type_Polygon;}
else if (str8_match(v, str8_lit("MultiPolygon"), 0)) {result = GEO_Type_MultiPolygon;}
else if (str8_match(v, str8_lit("GeometryCollection"), 0)) {result = GEO_Type_GeometryCollection;}
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
