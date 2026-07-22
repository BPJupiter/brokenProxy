#ifndef BASE_STRINGS_H
#define BASE_STRINGS_H

////////////////////////////////
//~ rjf: String Types

typedef struct String8 String8;
struct String8 {
    u8 *str;
    u64 size;
};

typedef struct String16 String16;
struct String16 {
    u16 *str;
    u64 size;
};

typedef struct String32 String32;
struct String32 {
    u32 *str;
    u64 size;
};

////////////////////////////////
//~ rjf: String List & Array Types

typedef struct String8_Node String8_Node;
struct String8_Node {
    String8_Node *next;
    String8 string;
};

typedef struct String8_List String8_List;
struct String8_List {
    String8_Node *first;
    String8_Node *last;
    u64 node_count;
    u64 total_size;
};

typedef struct String8_Array String8_Array;
struct String8_Array {
    String8 *v;
    u64 count;
    u64 total_size;
};

////////////////////////////////
//~ rjf: String Matching, Splitting, & Joining Types

typedef u32 StringMatchFlags;
enum {
    StringMatchFlag_CaseInsensitive  = (1 << 0),
    StringMatchFlag_RightSideSloppy  = (1 << 1),
    StringMatchFlag_SlashInsensitive = (1 << 2),
};

typedef u32 StringSplitFlags;
enum {
    StringSplitFlag_KeepEmpties = (1 << 0),
};

typedef enum PathStyle {
    PathStyle_Null,
    PathStyle_Relative,
    PathStyle_WindowsAbsolute,
    PathStyle_UnixAbsolute,
    
#if OS_WINDOWS
    PathStyle_SystemAbsolute = PathStyle_WindowsAbsolute
#elif OS_LINUX
    PathStyle_SystemAbsolute = PathStyle_UnixAbsolute
#else
# error Absolute path style is undefined for this OS.
#endif
}
PathStyle;

typedef struct String_Join String_Join;
struct String_Join {
    String8 pre;
    String8 sep;
    String8 post;
};

////////////////////////////////
//~ rjf: String Pair Types

typedef struct String8_TxtPt_Pair String8_TxtPt_Pair;
struct String8_TxtPt_Pair {
    String8 string;
    TxtPt pt;
};

////////////////////////////////
//~ rjf: UTF Decoding Types

typedef struct Unicode_Decode Unicode_Decode;
struct Unicode_Decode {
    u32 inc;
    u32 codepoint;
};

////////////////////////////////
//~ rjf: String Fuzzy Matching Types

typedef struct Fuzzy_Match_Range_Node Fuzzy_Match_Range_Node;
struct Fuzzy_Match_Range_Node {
    Fuzzy_Match_Range_Node *next;
    Rng1u64 range;
};

typedef struct Fuzzy_Match_Range_List Fuzzy_Match_Range_List;
struct Fuzzy_Match_Range_List {
    Fuzzy_Match_Range_Node *first;
    Fuzzy_Match_Range_Node *last;
    u64 count;
    u64 needle_part_count;
    u64 total_dim;
};

////////////////////////////////
//~ rjf: Character Classification & Conversion Functions

internal bool32 char_is_space(u8 c);
internal bool32 char_is_upper(u8 c);
internal bool32 char_is_lower(u8 c);
internal bool32 char_is_alpha(u8 c);
internal bool32 char_is_slash(u8 c);
internal bool32 char_is_digit(u8 c, u32 base);
internal u8 lower_from_char(u8 c);
internal u8 upper_from_char(u8 c);
internal u8 correct_slash_from_char(u8 c);

////////////////////////////////
//~ rjf: C-String Measurement

internal u64 cstring8_length(u8 *c);
internal u64 cstring16_length(u16 *c);
internal u64 cstring32_length(u32 *c);

////////////////////////////////
//~ rjf: String Constructors

#define s(S) str8_lit(S)

#define str8_lit(S)  str8((u8*)(S), sizeof(S) - 1)
#define str8_lit_comp(S) {(u8*)(S), sizeof(S) - 1,}
#define str8_lit_cstr(S) str8((u8*)(S), sizeof(S))
#define str8_varg(S) (int)((S).size), ((S).str)

#define str8_array(S,C) str8((u8*)(S), sizeof(*(S))*(C))
#define str8_array_fixed(S) str8((u8*)(S), sizeof(S))
#define str8_struct(S) str8((u8*)(S), sizeof(*(S)))

internal String8  str8(u8 *str, u64 size);
internal String8  str8_range(u8 *first, u8 *one_past_last);
internal String8  str8_zero(void);
internal String16 str16(u16 *str, u64 size);
internal String16 str16_range(u16 *first, u16 *one_past_last);
internal String16 str16_zero(void);
internal String32 str32(u32 *str, u64 size);
internal String32 str32_range(u32 *first, u32 *one_past_last);
internal String32 str32_zero(void);
internal String8  str8_cstring(char *c);
internal String16 str16_cstring(u16 *c);
internal String32 str32_cstring(u32 *c);
internal String8  str8_cstring_capped(void *cstr, void *cap);
internal String16 str16_cstring_capped(void *cstr, void *cap);

////////////////////////////////
//~ rjf: String Stylization

internal String8 upper_from_str8(Arena *arena, String8 string);
internal String8 lower_from_str8(Arena *arena, String8 string);
internal String8 backslashed_from_str8(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: String Matching

#define str8_match_lit(a_lit, b, flags)   str8_match(str8_lit(a_lit), (b), (flags))
#define str8_match_cstr(a_cstr, b, flags) str8_match(str8_cstring(a_cstr), (b), (flags))
internal bool32 str8_match(String8 a, String8 b, StringMatchFlags flags);
#define str8_matchi(a, b) str8_match(a, b, StringMatchFlag_CaseInsensitive)
internal bool32 str8_match_wildcard(String8 string, String8 pattern, StringMatchFlags flags);
internal bool32 str8_starts_with(String8 string, String8 expected_prefix);
internal bool32 str8_starts_withi(String8 string, String8 expected_prefix);
internal u64 str8_find_needle(String8 string, u64 start_pos, String8 needle, StringMatchFlags flags);
internal u64 str8_find_needle_reverse(String8 string, u64 start_pos, String8 needle, StringMatchFlags flags);
internal bool32 str8_is_before(String8 a, String8 b);
#define str8_ends_with(string, end, flags) str8_match(str8_postfix((string), (end).size), (end), (flags))

////////////////////////////////
//~ rjf: String Slicing

internal String8 str8_substr(String8 str, Rng1u64 range);
internal String8 str8_prefix(String8 str, u64 size);
internal String8 str8_skip(String8 str, u64 amt);
internal String8 str8_postfix(String8 str, u64 size);
internal String8 str8_chop(String8 str, u64 amt);
internal String8 str8_chop_line(String8 *str);
internal String8 str8_skip_chop_whitespace(String8 string);
internal String8 str8_skip_chop_slashes(String8 string);

////////////////////////////////
//~ rjf: String Formatting & Copying

internal String8 str8_cat(Arena *arena, String8 s1, String8 s2);
internal String8 str8_copy(Arena *arena, String8 s);
internal String8 str8fv(Arena *arena, char *fmt, va_list args);
internal String8 str8f(Arena *arena, char *fmt, ...);
// TODO(rjf): remove these once we're ready to convert all usages:
#define push_str8_cat(arena, s1, s2) str8_cat((arena), (s1), (s2))
#define push_str8_copy(arena, s) str8_copy((arena), (s))
#define push_str8fv(arena, fmt, args) str8fv((arena), (fmt), (args))
#define push_str8f(arena, ...) str8f((arena), __VA_ARGS__)
internal String8 push_cstr(Arena *arena, String8 str); // TODO(rjf): this is unnecessary - this is implied by `push_str8_copy`. need to remove.

////////////////////////////////
//~ rjf: String <=> Integer Conversions

//- rjf: string -> integer
internal s64 sign_from_str8(String8 string, String8 *string_tail);
internal bool32 str8_is_integer(String8 string, u32 radix);
internal bool32 str8_is_integer_signed(String8 string, u32 radix);
internal u64 u64_from_str8(String8 string, u32 radix);
internal s64 s64_from_str8(String8 string, u32 radix);
internal u32 u32_from_str8(String8 string, u32 radix);
internal s32 s32_from_str8(String8 string, u32 radix);
internal bool32 try_u64_from_str8_c_rules(String8 string, u64 *x);
internal bool32 try_s64_from_str8_c_rules(String8 string, s64 *x);

//- rjf: integer -> string
internal String8 str8_from_memory_size(Arena *arena, u64 size);
internal String8 str8_from_count(Arena *arena, u64 count);
internal String8 str8_from_bits_u32(Arena *arena, u32 x);
internal String8 str8_from_bits_u64(Arena *arena, u64 x);
internal String8 str8_from_u64(Arena *arena, u64 u64, u32 radix, u8 min_digits, u8 digit_group_separator);
internal String8 str8_from_s64(Arena *arena, s64 s64, u32 radix, u8 min_digits, u8 digit_group_separator);

////////////////////////////////
//~ rjf: String <=> Float Conversions

internal f64 f64_from_str8(String8 string);

////////////////////////////////
//~ rjf: String List Construction Functions

internal String8_Node *str8_list_push_node                 (String8_List *list, String8_Node *node);
internal String8_Node *str8_list_push_node_set_string      (String8_List *list, String8_Node *node, String8 string);
internal String8_Node *str8_list_push_node_front           (String8_List *list, String8_Node *node);
internal String8_Node *str8_list_push_node_front_set_string(String8_List *list, String8_Node *node, String8 string);
internal String8_Node *str8_list_push                      (Arena *arena, String8_List *list, String8 string);
internal String8_Node *str8_list_push_front                (Arena *arena, String8_List *list, String8 string);
internal void         str8_list_concat_in_place            (String8_List *list, String8_List *to_push);
internal String8_Node* str8_list_push_aligner              (Arena *arena, String8_List *list, u64 min, u64 align);
internal String8_Node* str8_list_pushf                     (Arena *arena, String8_List *list, char *fmt, ...);
internal String8_Node* str8_list_push_frontf               (Arena *arena, String8_List *list, char *fmt, ...);
internal String8_Node* str8_list_pop_front                 (String8_List *list);
internal String8_List  str8_list_copy                      (Arena *arena, String8_List *list);
internal String8_List  str8_list_substr                    (Arena *arena, String8_List list, Rng1u64 range);
#define str8_list_first(list) ((list)->first ? (list)->first->string : str8_zero())

////////////////////////////////
//~ rjf: String Splitting & Joining

internal String8_List  str8_split(Arena *arena, String8 string, u8 *split_chars, u64 split_char_count, StringSplitFlags flags);
internal String8_List  str8_split_by_string_chars(Arena *arena, String8 string, String8 split_chars, StringSplitFlags flags);
internal String8       str8_list_join(Arena *arena, String8_List *list, String_Join *optional_params);

////////////////////////////////
//~ rjf: Basic Data Stringification Helpers

internal String8_List numeric_str8_list_from_data(Arena *arena, u32 radix, String8 data, u64 stride);

////////////////////////////////
//~ rjf; String Arrays

internal String8_Array str8_array_zero     (void);
internal String8_Array str8_array_from_list(Arena *arena, String8_List *list);
internal String8_Array str8_array_reserve  (Arena *arena, u64 count);
internal String8_Array str8_array_copy     (Arena *arena, String8_Array array);

////////////////////////////////
//~ rjf: String <-> Version Helpers

internal u64 version_from_str8(String8 string);
internal String8 str8_from_version(Arena *arena, u64 version);

////////////////////////////////
//~ rjf: String Path Helpers

global read_only struct
{
  String8   string;
  PathStyle path_style;
}
g_path_style_map[] =
{
  { str8_lit_comp(""),         PathStyle_Null            },
  { str8_lit_comp("relative"), PathStyle_Relative        },
  { str8_lit_comp("windows"),  PathStyle_WindowsAbsolute },
  { str8_lit_comp("unix"),     PathStyle_UnixAbsolute    },
  { str8_lit_comp("system"),   PathStyle_SystemAbsolute  },
};

internal String8 program_ext_postfix_from_os(OperatingSystem os, bool32 require_ext);

internal String8 str8_chop_last_slash(String8 string);
internal String8 str8_skip_last_slash(String8 string);
internal String8 str8_chop_last_dot(String8 string);
internal String8 str8_skip_last_dot(String8 string);

internal PathStyle   path_style_from_str8(String8 string);
internal String8_List str8_split_path(Arena *arena, String8 string);
internal void        str8_path_list_resolve_dots_in_place(String8_List *path, PathStyle style);
internal String8     str8_path_list_join_by_style(Arena *arena, String8_List *path, PathStyle style);

internal String8_TxtPt_Pair str8_txt_pt_pair_from_string(String8 string);

////////////////////////////////
//~ rjf: Relative <-> Absolute Path

internal String8 path_relative_dst_from_absolute_dst_src(Arena *arena, String8 dst, String8 src);
internal String8 path_absolute_dst_from_relative_dst_src(Arena *arena, String8 dst, String8 src);

////////////////////////////////
//~ rjf: Path Normalization

internal String8_List path_normalized_list_from_string(Arena *arena, String8 path, PathStyle *style_out);
internal String8      path_normalized_from_string(Arena *arena, String8 path);
internal bool32       path_match_normalized(String8 left, String8 right);

////////////////////////////////
//~ rjf: Misc. Path Helpers

internal PathStyle        path_style_from_string(String8 string);
internal String8          string_from_path_style(PathStyle style);
internal String8          path_separator_string_from_style(PathStyle style);
internal StringMatchFlags path_match_flags_from_os(OperatingSystem os);
internal String8          path_convert_slashes(Arena *arena, String8 path, PathStyle path_style);
internal String8          path_replace_file_extension(Arena *arena, String8 file_name, String8 ext);

////////////////////////////////
//~ rjf: UTF-8 & UTF-16 Decoding/Encoding

internal Unicode_Decode utf8_decode (u8  *str, u64 max);
internal Unicode_Decode utf16_decode(u16 *str, u64 max);
internal u32            utf8_encode (u8  *str, u32 codepoint);
internal u32            utf16_encode(u16 *str, u32 codepoint);

////////////////////////////////
//~ rjf: Unicode String Conversions

internal String8  str8_from_16(Arena *arena, String16 in);
internal String16 str16_from_8(Arena *arena, String8  in);
internal String8  str8_from_32(Arena *arena, String32 in);
internal String32 str32_from_8(Arena *arena, String8  in);

////////////////////////////////
//~ rjf: Basic Types & Space Enum <-> String Conversions

internal OperatingSystem operating_system_from_string(String8 string);
internal String8 string_from_dimension               (Dimension dimension);
internal String8 string_from_side                    (Side side);
internal String8 string_from_operating_system        (OperatingSystem os);
internal String8 string_from_arch                    (Arch arch);
internal String8 string_from_week_day                (WeekDay week_day);
internal String8 string_from_month                   (Month month);
internal String8 string_from_date_time               (Arena *arena, Date_Time *date_time);
internal String8 string_from_date_time__file_name    (Arena *arena, Date_Time *date_time);
internal String8 string_from_elapsed_time            (Arena *arena, Date_Time dt);

////////////////////////////////
//~ rjf: String <-> Globally Unique IDs

internal String8 string_from_guid    (Arena *arena, Guid guid);
internal bool32  try_guid_from_string(String8 string, Guid *guid_out);
internal Guid    guid_from_string    (String8 string);

////////////////////////////////
//~ rjf: Basic Text Indentation

internal String8 indented_from_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Text Escaping

internal String8 escaped_from_raw_str8(Arena *arena, String8 string);
internal String8 raw_from_escaped_str8(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Text Wrapping

internal String8_List wrapped_lines_from_string(Arena *arena, String8 string, u64 first_line_max_width, u64 max_width, u64 wrap_indent);

////////////////////////////////
//~ rjf: String <-> Color

internal String8 hex_string_from_rgba_4f32(Arena *arena, Vec4f32 rgba);
internal Vec4f32 rgba_from_hex_string_4f32(String8 hex_string);

////////////////////////////////
//~ rjf: String Fuzzy Matching

internal Fuzzy_Match_Range_List fuzzy_match_find(Arena *arena, String8 needle, String8 haystack);
internal Fuzzy_Match_Range_List fuzzy_match_range_list_copy(Arena *arena, Fuzzy_Match_Range_List *src);

////////////////////////////////
//~ NOTE(allen): Serialization Helpers

internal void    str8_serial_begin(Arena *arena, String8_List *srl);
internal String8 str8_serial_end(Arena *arena, String8_List *srl);
internal void    str8_serial_write_to_dst(String8_List *srl, void *out);
internal u64     str8_serial_push_align(Arena *arena, String8_List *srl, u64 align);
internal void *  str8_serial_push_size(Arena *arena, String8_List *srl, u64 size);
internal void *  str8_serial_push_data(Arena *arena, String8_List *srl, void *data, u64 size);
internal void    str8_serial_push_data_list(Arena *arena, String8_List *srl, String8_Node *first);
internal void *  str8_serial_push_u64(Arena *arena, String8_List *srl, u64 x);
internal void *  str8_serial_push_u32(Arena *arena, String8_List *srl, u32 x);
internal void *  str8_serial_push_u16(Arena *arena, String8_List *srl, u16 x);
internal void *  str8_serial_push_u8(Arena *arena, String8_List *srl, u8 x);
internal void *  str8_serial_push_cstr(Arena *arena, String8_List *srl, String8 str);
internal void *  str8_serial_push_string(Arena *arena, String8_List *srl, String8 str);
#define str8_serial_push_array(arena, srl, ptr, count) str8_serial_push_data(arena, srl, ptr, sizeof(*(ptr)) * (count))
#define str8_serial_push_struct(arena, srl, ptr) str8_serial_push_array(arena, srl, ptr, 1)

////////////////////////////////
//~ rjf: Deserialization Helpers

internal u64    str8_deserial_read(String8 string, u64 off, void *read_dst, u64 read_size, u64 granularity);
internal u64    str8_deserial_find_first_match(String8 string, u64 off, u16 scan_val);
internal void * str8_deserial_get_raw_ptr(String8 string, u64 off, u64 size);
internal u64    str8_deserial_read_cstr(String8 string, u64 off, String8 *cstr_out);
internal u64    str8_deserial_read_windows_utf16_string16(String8 string, u64 off, String16 *str_out);
internal u64    str8_deserial_read_block(String8 string, u64 off, u64 size, String8 *block_out);
#define str8_deserial_read_array(string, off, ptr, count) str8_deserial_read((string), (off), (ptr), sizeof(*(ptr))*(count), sizeof(*(ptr)))
#define str8_deserial_read_struct(string, off, ptr)       str8_deserial_read_array(string, off, ptr, 1)
internal u64 str8_deserial_read_uleb128(String8 string, u64 off, u64 *value_out);
internal u64 str8_deserial_read_sleb128(String8 string, u64 off, s64 *value_out);

////////////////////////////////
//~ string buffer

internal bool32 str8_buffer_skip(String8_Node *buf, u64 *pos, u64 skip);
internal u64 str8_buffer_read(String8_Node *buf, u64 *pos, u64 read_size, void *out);
internal u64 str8_buffer_peek(String8_Node *buf, u64 *pos, u64 read_size, void *out);
internal u64 str8_buffer_write(String8_Node *buf, u64 *pos, String8 data);
internal u64 str8_buffer_write_u16(String8_Node *buf, u64 *pos, u16 v);
internal u64 str8_buffer_write_u32(String8_Node *buf, u64 *pos, u32 v);
internal u64 str8_buffer_write_zeroes(String8_Node *buf, u64 *pos, u64 size);
internal u64 str8_buffer_write_string_list(String8_Node *buf, u64 *pos, String8_List list);

////////////////////////////////
//~ rjf: Basic String Hashes

internal u64 u64_hash_from_seed_str8(u64 seed, String8 string);
internal u64 u64_hash_from_str8(String8 string);
internal u128 u128_hash_from_seed_str8(u64 seed, String8 string);
internal u128 u128_hash_from_str8(String8 string);

#endif // BASE_STRINGS_H
