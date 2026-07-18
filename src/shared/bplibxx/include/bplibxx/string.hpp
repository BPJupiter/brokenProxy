#ifndef BPLIBXX_STRING_HPP
#define BPLIBXX_STRING_HPP

#include "types.hpp"
#include <iosfwd>

struct String {
    u64 count;
    bool is_literal;
    u8 *data;

    String();

    String(const char *string_literal);

    String(u8 *buffer, u64 length);

    class Byte_Ref {
    public:
        Byte_Ref(String *owner_, u64 index_);
        operator u8() const;
        Byte_Ref &operator=(u8 value);
    private:
        String *owner;
        s64 index;
    };

    u8       operator[](u64 index) const;
    Byte_Ref operator[](u64 index);

    operator bool() const;
};

bool operator==(String a, String b);
bool operator!=(String a, String b);

String operator+(String a, String b);

std::ostream &operator<<(std::ostream &os, String s);

struct String_Array {
    s64 count;
    String *data;
};

String copy_string(String s);

String to_string(u8 *c_string);
String to_string(u8 *buffer, s64 length);

void free_string(String s);

u8   to_upper(u8 c);
u8   to_lower(u8 c);
bool is_digit(u8 c);
bool is_alpha(u8 c);
bool is_alnum(u8 c);
bool is_space(u8 c);
bool is_any(u8 c, String chars);

bool equal(String a, String b);
bool equal_nocase(String a, String b);
int  compare(String a, String b); /* like strcmp */
int  compare_nocase(String a, String b);
bool contains(String s, String substring);
bool contains(String s, u8 c);
bool begins_with(String s, String prefix);
bool ends_with(String s, String suffix);

s64 find_index_from_left(String s, u8 byte);
s64 find_index_from_left(String s, String substring);
s64 find_index_from_right(String s, u8 byte);
s64 find_index_from_right(String s, String substring);

String trim_left(String s);
String trim_right(String s);
String trim(String s);
void   to_lower_in_place(String s);
void   to_upper_in_place(String s);
void   replace_chars(String s, String chars, u8 replacement);
String replace(String s, String old_substring, String new_substring, int *occurrences /* out, may be NULL */);
String slice(String s, s64 index, s64 count);

bool   string_to_int(String s, s64 *out_value);
bool   string_to_float(String s, double *out_value);

String sprint(const char *format, ...);

String join(String *inputs, int input_count, String separator,
            bool before_first, bool after_last);
String_Array split(String s, String separator);
void free_string_array(String_Array arr); /* frees the array itself, not the (non-owned) bytes it points at */

u8  *to_c_string(String s);      /* heap-allocates a 0-terminated copy */
s64  c_style_strlen(u8 *ptr);

struct String_Builder {
    u8  *data;
    s64  count;
    s64  capacity;
};
 
void   init_string_builder(String_Builder *builder);
void   append(String_Builder *builder, String s);
void   append(String_Builder *builder, const char *c_string);
void   append(String_Builder *builder, u8 byte);
void   append(String_Builder *builder, u8 *s, s64 length);
bool   print_to_builder(String_Builder *builder, const char *format, ...);
s64    builder_string_length(String_Builder *builder);
String builder_to_string(String_Builder *builder); /* owned result */
void   free_buffers(String_Builder *builder);

#endif // BPLIBXX_STRING_HPP
