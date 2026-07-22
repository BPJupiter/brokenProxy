
////////////////////////////
// Handle Type Functions

internal File file_zero(void)
{
    File f = {0};
    return f;
}

internal bool32 file_match(File a, File b)
{
    bool32 result = MemoryMatchStruct(&a, &b);
    return result;
}

/////////////////////////
// Filesystem Helpers

internal String8 data_from_file_path(Arena *arena, String8 path)
{
    File file = file_open(AccessFlag_Read|AccessFlag_ShareRead, path);
    File_Properties properties = properties_from_file(file);
    String8 data = string_from_file_range(arena, file, r1u64(0, properties.size));
    file_close(file);
    return data;
}

internal bool32 write_data_to_file_path(String8 path, String8 data)
{
    bool32 good = false;
    File file = file_open(AccessFlag_Write, path);
    if (!file_match(file, file_zero())) {
        u64 bytes_written = file_write(file, r1u64(0, data.size), data.str);
        good = (bytes_written == data.size);
        file_close(file);
    }
    return good;
}

internal bool32 write_data_list_to_file_path(String8 path, String8_List list)
{
    bool32 good = 0;
    File file = file_open(AccessFlag_Write, path);
    if(!file_match(file, file_zero())) {
        Temp scratch = scratch_begin(0, 0);
        u64 write_buffer_size = Kilobytes(64);
        u8 *write_buffer = push_array_no_zero(scratch.arena, u8, write_buffer_size);
        u64 write_buffer_write_pos = 0;
        u64 write_buffer_read_pos = 0;
        u64 file_off = 0;
        {
            for(String8_Node *n = list.first; n != 0; n = n->next) {
                for(u64 n_off = 0; n_off < n->string.size;) {
                    u64 write_buffer_unconsumed_size = (write_buffer_write_pos - write_buffer_read_pos);
                    u64 write_buffer_available_size = (write_buffer_size - write_buffer_unconsumed_size);
                    if(write_buffer_available_size == 0) {
                        u64 file_write_size = file_write(file, r1u64(file_off, file_off+write_buffer_size), write_buffer);
                        if(file_write_size != write_buffer_size) {
                            goto dbl_break;
                        }
                        file_off += write_buffer_size;
                        write_buffer_read_pos += write_buffer_size;
                    }
                    else {
                        u64 bytes_to_copy = Min(write_buffer_available_size, n->string.size - n_off);
                        write_buffer_write_pos += wrapped_write(write_buffer, write_buffer_size, write_buffer_write_pos, n->string.str + n_off, bytes_to_copy);
                        n_off += bytes_to_copy;
                    }
                }
            }
            if(write_buffer_write_pos > write_buffer_read_pos) {
                u64 file_write_size = file_write(file, r1u64(file_off, file_off + (write_buffer_write_pos-write_buffer_read_pos)), write_buffer);
                file_off += file_write_size;
            }
        }
    dbl_break:;
        good = (file_off == list.total_size);
        file_close(file);
        scratch_end(scratch);
    }
    return good;
}

internal bool32 append_data_to_file_path(String8 path, String8 data)
{
    bool32 good = 0;
    if(data.size != 0) {
        File file = file_open(AccessFlag_Write|AccessFlag_Append, path);
        if(!file_match(file, file_zero())) {
            u64 pos = properties_from_file(file).size;
            u64 bytes_written = file_write(file, r1u64(pos, pos+data.size), data.str);
            good = (bytes_written == data.size);
            file_close(file);
        }
    }
    return good;
}

internal File_ID id_from_file_path(String8 path)
{
    File file = file_open(AccessFlag_Read|AccessFlag_ShareRead, path);
    File_ID id = id_from_file(file);
    file_close(file);
    return id;
}

internal s64 file_id_compare(File_ID a, File_ID b)
{
    s64 cmp = MemoryCompare((void*)&a.v[0], (void*)&b.v[0], sizeof(a.v));
    return cmp;
}

internal String8 string_from_file_range(Arena *arena, File file, Rng1u64 range)
{
    u64 pre_pos = arena_pos(arena);
    String8 result;
    result.size = dim_1u64(range);
    result.str = push_array_no_zero(arena, u8, result.size);
    u64 actual_read_size = file_read(file, range, result.str);
    if(actual_read_size < result.size) {
        arena_pop_to(arena, pre_pos + actual_read_size);
        result.size = actual_read_size;
    }
    return result;
}

internal String8 file_read_cstring(Arena *arena, File file, u64 off)
{
    Temp scratch = scratch_begin(&arena, 1);
    String8_List block_list = {0};
    for(u64 cursor = off, stride = 256;; cursor += stride) {
        u8      *raw_block = push_array_no_zero(scratch.arena, u8, stride);
        u64      read_size = file_read(file, r1u64(cursor, cursor + stride), raw_block);
        String8  block     = str8_cstring_capped(raw_block, raw_block+read_size);
        str8_list_push(scratch.arena, &block_list, block);
        if(read_size != stride || (block.size+1 <= read_size && block.str[block.size] == 0)) {
            break;
        }
    }
    String8 result = str8_list_join(arena, &block_list, 0);
    scratch_end(scratch);
    return result;
}
