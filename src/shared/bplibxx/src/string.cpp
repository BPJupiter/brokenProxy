#include "bplibxx/string.hpp"

#include <ostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void panic(const char *message)
{
    fprintf(stderr, "PANIC: %s\n", message);
    abort();
}

static u8 string_get(String s, u64 index)
{
    if (index < 0 || index >= s.count) panic("array_bounds_check_fail");
    return s.data[index];
}

static void string_set(String s, u64 index, u8 value)
{
    if (s.is_literal) panic("attempt to write through a read-only string literal");
    if (index < 0 || index >= s.count) panic("array bounds check fail");
    s.data[index] = value;
}

String::String()
{
    count = 0;
    is_literal = false;
    data = nullptr;
}

String::String(const char *string_literal)
{
    count = (u64)strlen(string_literal);
    is_literal = true;
    data = (u8 *)string_literal;
}

String::String(u8 *buffer, u64 length)
{
    count = length;
    data = buffer;
    is_literal = false;
}

String::Byte_Ref::Byte_Ref(String *owner_, u64 index_)
{
    owner = owner_;
    index = index_;
}

String::Byte_Ref::operator u8() const
{
    return string_get(*owner, index);
}

String::Byte_Ref &String::Byte_Ref::operator=(u8 value)
{
    string_set(*owner, index, value);
    return *this;
}

u8 String::operator[](u64 index) const
{
    return string_get(*this, index);
}

String::Byte_Ref String::operator[](u64 index)
{
    return Byte_Ref(this, index);
}

String::operator bool() const
{
    return count != 0;
}

bool operator==(String a, String b)
{
    return equal(a, b);
}

bool operator!=(String a, String b)
{
    return !equal(a, b);
}

String operator+(String a, String b)
{
    String result;
    result.count = a.count + b.count;
    result.is_literal = false;
    result.data = result.count > 0 ? (u8 *)malloc(result.count) : NULL;
    if (a.count > 0) memcpy(result.data, a.data, a.count);
    if (b.count > 0) memcpy(result.data + a.count, b.data, b.count);
    return result;
}

std::ostream &operator<<(std::ostream &os, String s)
{
    if (s.count > 0) os.write((const char *)s.data, (std::streamsize)s.count);
    return os;
}

String copy_string(String s)
{
    String result;
    result.count = s.count;
    result.is_literal = false;
    if (s.count == 0) {
        result.data = nullptr;
        return result;
    }
    result.data = (u8 *)malloc(s.count);
    memcpy(result.data, s.data, s.count);
    return result;
}

String to_string(u8 *c_string)
{
    return String(c_string, c_style_strlen(c_string));
}

String to_string(u8 *buffer, s64 length)
{
    return String(buffer, length);
}

void free_string(String s)
{
    if (s.is_literal) {
        fprintf(stderr, "WARNING: free() called on a string literal, ignored.\n");
        return;
    }
    if (s.data != nullptr) free(s.data);
}

u8 to_upper(u8 c)
{
    if (c >= 'a' && c <= 'z') return (u8)(c - 'a' + 'A');
    return c;
}

u8 to_lower(u8 c)
{
    if (c >= 'A' && c <= 'Z') return (u8)(c - 'A' + 'a');
    return c;
}

bool is_digit(u8 c)
{
    return c >= '0' && c <= '9';
}

bool is_alpha(u8 c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_alnum(u8 c)
{
    return is_alpha(c) || is_digit(c);
}

bool is_space(u8 c)
{
    bool is = false;
    if (c == ' ')  is = true;
    if (c == '\t') is = true;
    if (c == '\n') is = true;
    if (c == '\r') is = true;
    if (c == '\v') is = true;
    if (c == '\f') is = true;
    return is;
}

bool is_any(u8 c, String chars)
{
    for (u64 i = 0; i < chars.count; i++) {
        if (chars.data[i] == c) return true;
    }
    return false;
}

bool equal(String a, String b)
{
    if (a.count != b.count) return false;
    if (a.count == 0) return true;
    return memcmp(a.data, b.data, a.count) == 0;
}

bool equal_nocase(String a, String b)
{
    if (a.count != b.count) return false;
    for (u64 i = 0; i < a.count; i++) {
        if (to_lower(a.data[i]) != to_lower(b.data[i])) return false;
    }
    return true;
}

int compare(String a, String b)
{
    u64 n = a.count < b.count ? a.count : b.count;
    for (u64 i = 0; i < n; i++) {
        int diff = (int)a.data[i] - (int)b.data[i];
        if (diff != 0) return diff;
    }
    if (a.count < b.count) return -1;
    if (a.count > b.count) return -1;
    return 0;
}

int compare_nocase(String a, String b)
{
    u64 n = a.count < b.count ? a.count : b.count;
    for (u64 i = 0; i < n; i++) {
        int diff = (int)to_lower(a.data[i]) - (int)to_lower(b.data[i]);
        if (diff != 0) return diff;
    }
    if (a.count < b.count) return -1;
    if (a.count > b.count) return -1;
    return 0;
}

bool contains(String s, String substring)
{
    return find_index_from_left(s, substring) != -1;
}

bool contains(String s, u8 c)
{
    return find_index_from_left(s, c) != -1;
}

bool begins_with(String s, String prefix)
{
    if (prefix.count > s.count) return false;
    String head = slice(s, 0, prefix.count);
    return equal(head, prefix);
}

bool ends_with(String s, String suffix)
{
    if (suffix.count > s.count) return false;
    String tail = slice(s, s.count - suffix.count, suffix.count);
    return equal(tail, suffix);
}

s64 find_index_from_left(String s, u8 byte)
{
    for (u64 i = 0; i < s.count; i += 1) {
        if (s.data[i] == byte) return i;
    }
    return -1;
}

s64 find_index_from_left(String s, String substring)
{
    if (substring.count == 0) return 0;
    if (substring.count > s.count) return -1;
    for (s64 i = 0; i <= s.count - substring.count; i += 1) {
        if (memcmp(s.data + i, substring.data, (size_t)substring.count) == 0) return i;
    }
    return -1;
}

s64 find_index_from_right(String s, u8 byte)
{
    for (u64 i = s.count - 1; i >= 0; i -= 1) {
        if (s.data[i] == byte) return i;
    }
    return -1;
}

s64 find_index_from_right(String s, String substring)
{
    if (substring.count == 0) return s.count;
    if (substring.count > s.count) return -1;
    for (u64 i = s.count - substring.count; i >= 0; i -= 1) {
        if (memcmp(s.data + i, substring.data, (size_t)substring.count) == 0) return i;
    }
    return -1;
}

String trim_left(String s)
{
    s64 i = 0;
    while (i < s.count && is_space(s.data[i])) i += 1;
    return slice(s, i, s.count - i);
}

String trim_right(String s)
{
    s64 end = s.count;
    while (end > 0 && is_space(s.data[end - 1])) end -= 1;
    return slice(s, 0, end);
}

String trim(String s)
{
    return trim_right(trim_left(s));
}

void to_lower_in_place(String s)
{
    if (s.is_literal) panic("attempt to write through a read-only string literal");
    for (s64 i = 0; i < s.count; i += 1) s.data[i] = to_lower(s.data[i]);
}

void to_upper_in_place(String s)
{
    if (s.is_literal) panic("attempt to write through a read-only string literal");
    for (s64 i = 0; i < s.count; i += 1) s.data[i] = to_upper(s.data[i]);
}

void replace_chars(String s, String chars, u8 replacement)
{
    if (s.is_literal) panic("attempt to write through a read-only string literal");
    for (s64 i = 0; i < s.count; i += 1) {
        if (is_any(s.data[i], chars)) s.data[i] = replacement;
    }
}

String replace(String s, String old_substring, String new_substring, int *occurrences)
{
    int found = 0;
 
    if (old_substring.count == 0) {
        if (occurrences) *occurrences = 0;
        return copy_string(s);
    }
 
    /* First pass: count occurrences so we can size the result exactly. */
    s64 i = 0;
    while (i <= s.count - old_substring.count) {
        if (memcmp(s.data + i, old_substring.data, (size_t)old_substring.count) == 0) {
            found += 1;
            i += old_substring.count;
        } else {
            i += 1;
        }
    }
 
    s64 result_count = s.count + found * (new_substring.count - old_substring.count);
    String result;
    result.count      = result_count;
    result.is_literal = false;
    result.data       = result_count > 0 ? (u8 *)malloc((size_t)result_count) : NULL;
 
    /* Second pass: build the result. */
    s64 src = 0;
    s64 dst = 0;
    while (src < s.count) {
        bool matched = (src <= s.count - old_substring.count) &&
            memcmp(s.data + src, old_substring.data, (size_t)old_substring.count) == 0;
        if (matched) {
            memcpy(result.data + dst, new_substring.data, (size_t)new_substring.count);
            dst += new_substring.count;
            src += old_substring.count;
        } else {
            result.data[dst] = s.data[src];
            dst += 1;
            src += 1;
        }
    }
 
    if (occurrences) *occurrences = found;
    return result;
}

String slice(String s, s64 index, s64 count)
{
    if (index < 0 || count < 0 || index + count > s.count) panic("array_bounds_check_fail");
    String result;
    result.data       = s.data + index;
    result.count      = count;
    result.is_literal  = s.is_literal;
    return result;
}

static bool parse_int(String *s, s64 *out_value)
{
    s64 i = 0;
    bool negative = false;
 
    if (i < s->count && (s->data[i] == '-' || s->data[i] == '+')) {
        negative = (s->data[i] == '-');
        i += 1;
    }
 
    s64 start_of_digits = i;
    s64 value = 0;
    while (i < s->count && is_digit(s->data[i])) {
        value = value * 10 + (s->data[i] - '0');
        i += 1;
    }
 
    if (i == start_of_digits) return false; /* no digits found */
 
    *out_value = negative ? -value : value;
 
    /* advance *s past what we consumed */
    s->data  += i;
    s->count -= i;
    return true;
}

bool string_to_int(String s, s64 *out_value)
{
    String rest = s;
    return parse_int(&rest, out_value) && rest.count == 0;
}

bool string_to_float(String s, double *out_value)
{
    if (s.count == 0) return false;
 
    /* strtod needs a 0-terminated buffer; our strings aren't, so make a
       temporary C string on the stack (or heap if unusually long). */
    char stack_buffer[128];
    char *buffer = stack_buffer;
    bool  heap   = false;
    if (s.count >= (s64)sizeof(stack_buffer)) {
        buffer = (char *)malloc((size_t)s.count + 1);
        heap   = true;
    }
    memcpy(buffer, s.data, (size_t)s.count);
    buffer[s.count] = 0;
 
    char *end = NULL;
    double value = strtod(buffer, &end);
    bool ok = (end != buffer) && (*end == 0);
 
    if (heap) free(buffer);
    if (!ok) return false;
    *out_value = value;
    return true;
}

String sprint(const char *format, ...)
{
    char stack_buffer[256];
 
    va_list args;
    va_start(args, format);
    int needed = vsnprintf(stack_buffer, sizeof(stack_buffer), format, args);
    va_end(args);
 
    if (needed < 0) panic("sprint: formatting error");
 
    String result;
    result.is_literal = false;
 
    if ((size_t)needed < sizeof(stack_buffer)) {
        result.count = needed;
        result.data  = (u8 *)malloc((size_t)needed);
        memcpy(result.data, stack_buffer, (size_t)needed);
        return result;
    }
 
    /* Didn't fit: allocate exactly enough and format again. */
    char *big_buffer = (char *)malloc((size_t)needed + 1);
    va_start(args, format);
    vsnprintf(big_buffer, (size_t)needed + 1, format, args);
    va_end(args);
 
    result.count = needed;
    result.data  = (u8 *)big_buffer; /* reuse the allocation directly */
    return result;
}

String join(String *inputs, int input_count, String separator,
            bool before_first, bool after_last)
{
    s64 total = 0;
    if (before_first) total += separator.count;
    for (int i = 0; i < input_count; i += 1) {
        total += inputs[i].count;
        if (i + 1 < input_count) total += separator.count;
    }
    if (after_last) total += separator.count;
 
    String result;
    result.count      = total;
    result.is_literal  = false;
    result.data        = total > 0 ? (u8 *)malloc((size_t)total) : NULL;
 
    s64 pos = 0;
    if (before_first && separator.count > 0) {
        memcpy(result.data + pos, separator.data, (size_t)separator.count);
        pos += separator.count;
    }
    for (int i = 0; i < input_count; i += 1) {
        memcpy(result.data + pos, inputs[i].data, (size_t)inputs[i].count);
        pos += inputs[i].count;
        if (i + 1 < input_count && separator.count > 0) {
            memcpy(result.data + pos, separator.data, (size_t)separator.count);
            pos += separator.count;
        }
    }
    if (after_last && separator.count > 0) {
        memcpy(result.data + pos, separator.data, (size_t)separator.count);
        pos += separator.count;
    }
    return result;
}

String_Array split(String s, String separator)
{
    String_Array result;
 
    if (separator.count == 0) {
        /* Degenerate case: nothing to split on, return the whole string. */
        result.count = 1;
        result.data  = (String *)malloc(sizeof(String));
        result.data[0] = s;
        return result;
    }
 
    /* First pass: count pieces. */
    int pieces = 1;
    s64 i = 0;
    while (i <= s.count - separator.count) {
        if (memcmp(s.data + i, separator.data, (size_t)separator.count) == 0) {
            pieces += 1;
            i += separator.count;
        } else {
            i += 1;
        }
    }
 
    result.count = pieces;
    result.data  = (String *)malloc((size_t)pieces * sizeof(String));
 
    /* Second pass: fill in the (non-owning) slices. */
    int piece_index = 0;
    s64 piece_start = 0;
    i = 0;
    while (i <= s.count - separator.count) {
        if (memcmp(s.data + i, separator.data, (size_t)separator.count) == 0) {
            result.data[piece_index] = slice(s, piece_start, i - piece_start);
            piece_index += 1;
            i += separator.count;
            piece_start = i;
        } else {
            i += 1;
        }
    }
    result.data[piece_index] = slice(s, piece_start, s.count - piece_start);
 
    return result;
}

void free_string_array(String_Array arr)
{
    if (arr.data != NULL) free(arr.data);
}

u8 *to_c_string(String s)
{
    u8 *result = (u8 *)malloc((size_t)s.count + 1);
    if (s.count > 0) memcpy(result, s.data, (size_t)s.count);
    result[s.count] = 0;
    return result;
}
 
s64 c_style_strlen(u8 *ptr)
{
    s64 len = 0;
    while (ptr[len] != 0) len += 1;
    return len;
}

static void builder_ensure_capacity(String_Builder *builder, s64 extra)
{
    s64 needed = builder->count + extra;
    if (needed <= builder->capacity) return;
 
    s64 new_capacity = builder->capacity == 0 ? 64 : builder->capacity;
    while (new_capacity < needed) new_capacity *= 2;
 
    builder->data     = (u8 *)realloc(builder->data, (size_t)new_capacity);
    builder->capacity = new_capacity;
}
 
void init_string_builder(String_Builder *builder)
{
    builder->data     = NULL;
    builder->count    = 0;
    builder->capacity = 0;
}
 
void append(String_Builder *builder, u8 *s, s64 length)
{
    if (length <= 0) return;
    builder_ensure_capacity(builder, length);
    memcpy(builder->data + builder->count, s, (size_t)length);
    builder->count += length;
}
 
void append(String_Builder *builder, String s)
{
    append(builder, s.data, s.count);
}
 
void append(String_Builder *builder, const char *c_string)
{
    append(builder, (u8 *)c_string, (s64)strlen(c_string));
}
 
void append(String_Builder *builder, u8 byte)
{
    builder_ensure_capacity(builder, 1);
    builder->data[builder->count] = byte;
    builder->count += 1;
}
 
bool print_to_builder(String_Builder *builder, const char *format, ...)
{
    char stack_buffer[256];
 
    va_list args;
    va_start(args, format);
    int needed = vsnprintf(stack_buffer, sizeof(stack_buffer), format, args);
    va_end(args);
 
    if (needed < 0) return false;
 
    if ((size_t)needed < sizeof(stack_buffer)) {
        append(builder, (u8 *)stack_buffer, needed);
        return true;
    }
 
    char *big_buffer = (char *)malloc((size_t)needed + 1);
    va_start(args, format);
    vsnprintf(big_buffer, (size_t)needed + 1, format, args);
    va_end(args);
    append(builder, (u8 *)big_buffer, needed);
    free(big_buffer);
    return true;
}
 
s64 builder_string_length(String_Builder *builder)
{
    return builder->count;
}
 
String builder_to_string(String_Builder *builder)
{
    String result;
    result.count      = builder->count;
    result.is_literal = false;
    if (builder->count == 0) {
        result.data = NULL;
        return result;
    }
    result.data = (u8 *)malloc((size_t)builder->count);
    memcpy(result.data, builder->data, (size_t)builder->count);
    return result;
}
 
void free_buffers(String_Builder *builder)
{
    if (builder->data != NULL) free(builder->data);
    builder->data     = NULL;
    builder->count    = 0;
    builder->capacity = 0;
}
