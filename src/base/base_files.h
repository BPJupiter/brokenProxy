#ifndef BASE_FILES_H
#define BASE_FILES_H

typedef u32 FileIterFlags;
enum {
    FileIterFlag_SkipFolders     = (1 << 0),
    FileIterFlag_SkipFiles       = (1 << 1),
    FileIterFlag_SkipHiddenFiles = (1 << 2),
    FileIterFlag_Done            = (1 << 31)
};

typedef struct File_Iter File_Iter;
struct File_Iter {
    FileIterFlags flags;
    u8 memory[800];
};

typedef struct File_Info File_Info;
struct File_Info {
    String8 name;
    File_Properties props;
};

typedef struct File_ID File_ID;
struct File_ID {
    u64 v[3];
};

typedef struct File File;
struct File {
    u64 u64[1];
};

typedef struct File_Map File_Map;
struct File_Map {
    u64 u64[1];
};

////////////////////////////
// Handle Type Functions

internal File file_zero(void);
internal bool32 file_match(File a, File b);

/////////////////////////
// Filesystem Helpers

internal String8    data_from_file_path(Arena *arena, String8 path);
internal bool32     write_data_to_file_path(String8 path, String8 data);
internal bool32     write_data_list_to_file_path(String8 path, String8_List list);
internal bool32     append_data_to_file_path(String8 path, String8 data);
internal File_ID    id_from_file_path(String8 path);
internal s64        file_id_compare(File_ID a, File_ID b);
internal String8    string_from_file_range(Arena *arena, File file, Rng1u64 range);
internal String8    file_read_cstring(Arena *arena, File file, u64 off);

///////////////////////////////
// @per_os_impl File System 

//- rjf: files
internal File            file_open(AccessFlags flags, String8 path);
internal void            file_close(File file);
internal u64             file_read(File file, Rng1u64 rng, void *out_data);
#define file_read_struct(f, off, ptr) file_read((f), r1u64((off), (off)+sizeof(*(ptr))), (ptr))
internal u64             file_write(File file, Rng1u64 rng, void *data);
internal bool32          file_set_times(File file, Date_Time time);
internal File_Properties properties_from_file(File file);
internal File_ID         id_from_file(File file);
internal bool32          file_reserve_size(File file, u64 size);
internal bool32          delete_file_at_path(String8 path);
internal bool32          copy_file_path(String8 dst, String8 src);
internal bool32          move_file_path(String8 dst, String8 src);
internal String8         full_path_from_path(Arena *arena, String8 path);
internal bool32          file_path_exists(String8 path);
internal bool32          folder_path_exists(String8 path);
internal File_Properties properties_from_file_path(String8 path);

//- rjf: file maps
internal File_Map file_map_open(AccessFlags flags, File file);
internal void     file_map_close(File_Map map);
internal void    *file_map_view_open(File_Map map, AccessFlags flags, Rng1u64 range);
internal void     file_map_view_close(File_Map map, void *ptr, Rng1u64 range);

//- rjf: directory iteration
internal File_Iter *file_iter_begin(Arena *arena, String8 path, FileIterFlags flags);
internal bool32 file_iter_next(Arena *arena, File_Iter *iter, File_Info *info_out);
internal void file_iter_end(File_Iter *iter);

//- rjf: directory creation
internal bool32 make_directory(String8 path);


#endif // BASE_FILES_H
