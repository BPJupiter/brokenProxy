
internal u16 geo_read_le_u16(u8 *data)
{
    u16 *data16 = (u16 *)data;
    u16 le = *data16;
    return le;
}

internal u32 geo_read_le_u32(u8 *data)
{
    u32 *data32 = (u32 *)data;
    u32 le = *data32;
    return le;
}

internal s32 geo_read_le_s32(u8 *data)
{
    u32 *data32 = (u32 *)data;
    s32 le = *data32;
    return (s32)le;
}

internal s32 geo_read_be_s32(u8 *data)
{
    u32 *data32 = (u32 *)data;
    s32 le = *data32;
    return (s32)from_be_u32(le);
}

internal f64 geo_read_le_f64(u8 *data)
{
    f64 le;
    MemoryCopy(&le, data, sizeof(le));
    return le;
}

internal GEO_SHP_File geo_shp_file_from_data(Arena *arena, String8 data)
{
    GEO_SHP_File result = {0};

    if (data.size >= 100 && geo_read_be_s32(data.str + 0) == 9994)
    {
        u8 *base = data.str;
        result.shape_type = geo_read_le_s32(base + 32);
        result.box[0] = geo_read_le_f64(base + 36);
        result.box[1] = geo_read_le_f64(base + 44);
        result.box[2] = geo_read_le_f64(base + 52);
        result.box[3] = geo_read_le_f64(base + 60);

        u64 off = 100;
        u64 record_count = 0;
        for (; off + 8 <= data.size;)
        {
            s32 content_words = geo_read_be_s32(base + off + 4);
            if (content_words < 0) { break; }
            u64 content_size = (u64)content_words * 2;
            if (off + 8 + content_size > data.size) { break; }
            off += 8 + content_size;
            record_count += 1;
        }

        GEO_SHP_Record *records = push_array(arena, GEO_SHP_Record, record_count);

        off = 100;
        for (u64 idx = 0; idx < record_count; idx += 1)
        {
            GEO_SHP_Record *rec = &records[idx];
            s32 content_words = geo_read_be_s32(base + off + 4);
            u64 content_size = (u64)content_words * 2;
            u8 *body = base + off + 8;

            rec->shape_type = geo_read_le_s32(body + 0);
            switch(rec->shape_type)
            {
                default:{}break;
                case GEO_SHP_ShapeKind_Point:
                case GEO_SHP_ShapeKind_PointZ:
                case GEO_SHP_ShapeKind_PointM: {
                    {
                        rec->points.count = 1;
                        rec->points.v = push_array(arena, Vec3f32, 1);
                        rec->points.v[0].x = geo_read_le_f64(body + 4);
                        rec->points.v[0].y = geo_read_le_f64(body + 12);
                    }
                } break;
                case GEO_SHP_ShapeKind_PolyLine:
                case GEO_SHP_ShapeKind_Polygon:
                case GEO_SHP_ShapeKind_PolyLineZ:
                case GEO_SHP_ShapeKind_PolygonZ:
                case GEO_SHP_ShapeKind_PolyLineM:
                case GEO_SHP_ShapeKind_PolygonM: {
                    {
                        rec->box[0] = geo_read_le_f64(body + 4);
                        rec->box[1] = geo_read_le_f64(body + 12);
                        rec->box[2] = geo_read_le_f64(body + 20);
                        rec->box[3] = geo_read_le_f64(body + 28);
                        rec->parts.count  = (u64)geo_read_le_s32(body + 36);
                        rec->points.count = (u64)geo_read_le_s32(body + 40);
                        rec->parts.v  = push_array(arena, s32, rec->parts.count);
                        rec->points.v = push_array(arena, Vec3f32, rec->points.count);
                        for (u64 p = 0; p < rec->parts.count; p += 1)
                        {
                            rec->parts.v[p] = geo_read_le_s32(body + 44 + p * 4);
                        }
                        u8 *pts = body + 44 + rec->parts.count * 4;
                        for (u64 p = 0; p < rec->points.count; p += 1)
                        {
                            rec->points.v[p].x = geo_read_le_f64(pts + p * 16 + 0);
                            rec->points.v[p].y = geo_read_le_f64(pts + p * 16 + 8);
                        }
                    }
                } break;
                case GEO_SHP_ShapeKind_MultiPoint:
                case GEO_SHP_ShapeKind_MultiPointZ:
                case GEO_SHP_ShapeKind_MultiPointM: {
                    {
                        rec->box[0] = geo_read_le_f64(body + 4);
                        rec->box[1] = geo_read_le_f64(body + 12);
                        rec->box[2] = geo_read_le_f64(body + 20);
                        rec->box[3] = geo_read_le_f64(body + 28);
                        rec->points.count = (u64)geo_read_le_s32(body + 36);
                        rec->points.v = push_array(arena, Vec3f32, rec->points.count);
                        for (u64 p = 0; p < rec->points.count; p += 1)
                        {
                            rec->points.v[p].x = geo_read_le_f64(body + 40 + p * 16 + 0);
                            rec->points.v[p].y = geo_read_le_f64(body + 40 + p * 16 + 8);
                        }
                    }
                } break;
            }

            off += 8 + content_size;
        }

        result.record_count = record_count;
        result.records = records;
        result.valid = 1;
    }
    return result;
}

internal GEO_DBF_File geo_dbf_file_from_data(Arena *arena, String8 data)
{
    GEO_DBF_File result = {0};

    if (data.size >= 32)
    {
        u8 *base = data.str;
        u16 header_size = geo_read_le_u16(base + 8);
        u16 record_size = geo_read_le_u16(base + 10);
        u32 record_count = geo_read_le_u32(base + 4);

        if (header_size >= 32 && header_size <= data.size && record_size != 0)
        {
            u64 field_count = 0;
            for (u64 off = 32; off < header_size && base[off] != 0x0D; off += 32)
            {
                field_count += 1;
            }

            GEO_DBF_Field *fields = push_array(arena, GEO_DBF_Field, field_count);
            u64 field_idx = 0;
            for (u64 off = 32; off < header_size && base[off] != 0x0D; off += 32)
            {
                GEO_DBF_Field *f = &fields[field_idx];
                MemoryCopy(f->name, base + off, 11);
                f->name[11] = 0;
                f->type = base[off + 11];
                f->length = base[off + 16];
                f->decimal_count = base[off + 17];
                field_idx += 1;
            }

            u64 records_off = header_size;
            if (records_off + (u64)record_size * record_count <= data.size)
            {
                GEO_DBF_Record *records = push_array(arena, GEO_DBF_Record, record_count);
                for (u64 idx = 0; idx < record_count; idx += 1)
                {
                    u8 *rec_base = base + records_off + idx * record_size;
                    records[idx].deleted = (rec_base[0] == '*');
                    records[idx].data = str8(rec_base + 1, record_size - 1);
                }

                result.field_count  = field_count;
                result.fields       = fields;
                result.record_count = record_count;
                result.records      = records;
                result.valid = 1;
            }
        }
    }

    return result;
}

internal f32 geo_ring_signed_area2(Vec2f32 *pts, u64 start, u64 count)
{
    f64 sum = 0.f;
    for (u64 i = 0; i < count; i += 1)
    {
        Vec3f32 a = pts[start + i];
        Vec2f32 b = pts[start + (i  + 1) % count];
        sum += (f64)a.x * (f64)b.y - (f64)b.x * (f64)a.y;
    }
    return (f32)sum;
}


