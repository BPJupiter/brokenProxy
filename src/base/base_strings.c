// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////

#ifndef MEM_STATIC
# define MEM_STATIC
# define MEM_API ASAN_NO_ADDR static inline
# include "third_party/martins_memfun/memfun.h"
#endif

////////////////////////////////
//~ rjf: Character Classification & Conversion Functions

internal bool32
char_is_space(u8 c)
{
  return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v');
}

internal bool32
char_is_upper(u8 c)
{
  return ('A' <= c && c <= 'Z');
}

internal bool32
char_is_lower(u8 c)
{
  return ('a' <= c && c <= 'z');
}

internal bool32
char_is_alpha(u8 c)
{
  return (char_is_upper(c) || char_is_lower(c));
}

internal bool32
char_is_slash(u8 c)
{
  return (c == '/' || c == '\\');
}

internal bool32
char_is_digit(u8 c, u32 base)
{
  bool32 result = 0;
  if(0 < base && base <= 16)
  {
    u8 val = integer_symbol_reverse[c];
    if(val < base)
    {
      result = 1;
    }
  }
  return result;
}

internal u8
lower_from_char(u8 c)
{
  if(char_is_upper(c))
  {
    c += ('a' - 'A');
  }
  return c;
}

internal u8
upper_from_char(u8 c)
{
  if(char_is_lower(c))
  {
    c += ('A' - 'a');
  }
  return c;
}

internal u8
correct_slash_from_char(u8 c)
{
  if(char_is_slash(c))
  {
    c = '/';
  }
  return c;
}

////////////////////////////////
//~ rjf: C-String Measurement

internal u64
cstring8_length(u8 *c)
{
  u64 length = 0;
  if(c)
  {
    u8 *p = c;
    for (;*p != 0; p += 1);
    length = (u64)(p - c);
  }
  return length;
}

internal u64
cstring16_length(u16 *c)
{
  u64 length = 0;
  if(c)
  {
    u16 *p = c;
    for (;*p != 0; p += 1);
    length = (u64)(p - c);
  }
  return length;
}

internal u64
cstring32_length(u32 *c)
{
  u64 length = 0;
  if(c)
  {
    u32 *p = c;
    for (;*p != 0; p += 1);
    length = (u64)(p - c);
  }
  return length;
}

////////////////////////////////
//~ rjf: String Constructors

internal String8
str8(u8 *str, u64 size)
{
  String8 result = {str, size};
  return result;
}

internal String8
str8_range(u8 *first, u8 *one_past_last)
{
  String8 result = {first, (u64)(one_past_last - first)};
  return result;
}

internal String8
str8_zero(void)
{
  String8 result = {0};
  return result;
}

internal String16
str16(u16 *str, u64 size)
{
  String16 result = {str, size};
  return result;
}

internal String16
str16_range(u16 *first, u16 *one_past_last)
{
  String16 result = {first, (u64)(one_past_last - first)};
  return result;
}

internal String16
str16_zero(void)
{
  String16 result = {0};
  return result;
}

internal String32
str32(u32 *str, u64 size)
{
  String32 result = {str, size};
  return result;
}

internal String32
str32_range(u32 *first, u32 *one_past_last)
{
  String32 result = {first, (u64)(one_past_last - first)};
  return result;
}

internal String32
str32_zero(void)
{
  String32 result = {0};
  return result;
}

internal String8
str8_cstring(char *c)
{
  String8 result = {(u8*)c, cstring8_length((u8*)c)};
  return result;
}

internal String16
str16_cstring(u16 *c)
{
  String16 result = {(u16*)c, cstring16_length((u16*)c)};
  return result;
}

internal String32
str32_cstring(u32 *c)
{
  String32 result = {(u32*)c, cstring32_length((u32*)c)};
  return result;
}

internal String8
str8_cstring_capped(void *cstr, void *cap)
{
  u64 cap_size = (u64)((u8 *)cap - (u8 *)cstr);
  u64 size     = MemFind(cstr, cap_size, 0);
  return str8(cstr, size);
}

internal String16
str16_cstring_capped(void *cstr, void *cap)
{
  u16 *ptr = (u16 *)cstr;
  u16 *opl = (u16 *)cap;
  for (;ptr < opl && *ptr != 0; ptr += 1);
  u64 size = (u64)(ptr - (u16 *)cstr);
  String16 result = str16(cstr, size);
  return result;
}

////////////////////////////////
//~ rjf: String Stylization

internal String8
upper_from_str8(Arena *arena, String8 string)
{
  string = push_str8_copy(arena, string);
  for(u64 idx = 0; idx < string.size; idx += 1)
  {
    string.str[idx] = upper_from_char(string.str[idx]);
  }
  return string;
}

internal String8
lower_from_str8(Arena *arena, String8 string)
{
  string = push_str8_copy(arena, string);
  for(u64 idx = 0; idx < string.size; idx += 1)
  {
    string.str[idx] = lower_from_char(string.str[idx]);
  }
  return string;
}

internal String8
backslashed_from_str8(Arena *arena, String8 string)
{
  string = push_str8_copy(arena, string);
  for(u64 idx = 0; idx < string.size; idx += 1)
  {
    string.str[idx] = char_is_slash(string.str[idx]) ? '\\' : string.str[idx];
  }
  return string;
}

////////////////////////////////
//~ rjf: String Matching

internal bool32
str8_match(String8 a, String8 b, StringMatchFlags flags)
{
  bool32 result = 0;
  if(a.size == b.size && flags == 0)
  {
    result = MemIsEqual(a.str, b.str, b.size);
  }
  else if (a.size == b.size && flags == StringMatchFlag_CaseInsensitive)
  {
    result = MemCompareI(a.str, b.str, a.size) == 0;
  }
  else if(a.size == b.size || (flags & StringMatchFlag_RightSideSloppy))
  {
    bool32 case_insensitive  = (flags & StringMatchFlag_CaseInsensitive);
    bool32 slash_insensitive = (flags & StringMatchFlag_SlashInsensitive);
    u64 size              = Min(a.size, b.size);
    result = 1;
    for(u64 i = 0; i < size; i += 1)
    {
      u8 at = a.str[i];
      u8 bt = b.str[i];
      if(case_insensitive)
      {
        at = upper_from_char(at);
        bt = upper_from_char(bt);
      }
      if(slash_insensitive)
      {
        at = correct_slash_from_char(at);
        bt = correct_slash_from_char(bt);
      }
      if(at != bt)
      {
        result = 0;
        break;
      }
    }
  }
  return result;
}

internal bool32
str8_char_match(u8 a, u8 b, StringMatchFlags flags)
{
  u8 at = a;
  u8 bt = b;
  if (flags & StringMatchFlag_CaseInsensitive) {
    at = upper_from_char(at);
    bt = upper_from_char(bt);
  }
  if (flags & StringMatchFlag_SlashInsensitive) {
    if (at == '\\') { at = '/'; }
    if (bt == '\\') { bt = '/'; }
  }
  return (at == bt);
}

internal bool32
str8_match_wildcard(String8 string, String8 pattern, StringMatchFlags flags)
{
  bool32 matched = 0;
  
  u64 pattern_cursor = 0;
  u64 string_cursor  = 0;
  
  u64 pattern_start = max_u64;
  u64 string_start  = 0;
  
  for (;;) {
    if (pattern_cursor == pattern.size) {
      if (string_cursor == string.size || (flags & StringMatchFlag_RightSideSloppy)) {
        matched = 1;
        break;
      }
    }
    
    if (string_cursor == string.size) {
      while (pattern_cursor < pattern.size && pattern.str[pattern_cursor] == '*') {
        pattern_cursor += 1;
      }
      matched = (pattern_cursor == pattern.size);
      break;
    }
    
    if (pattern_cursor < pattern.size && pattern.str[pattern_cursor] == '*') {
      pattern_start = pattern_cursor;
      string_start = string_cursor;
      pattern_cursor += 1;
      continue;
    }
    
    
    if (pattern_cursor < pattern.size && (pattern.str[pattern_cursor] == '?' || str8_char_match(string.str[string_cursor], pattern.str[pattern_cursor], flags))) {
      string_cursor  += 1;
      pattern_cursor += 1;
      continue;
    }
    
    if (pattern_start != max_u64) {
      pattern_cursor = pattern_start + 1;
      string_start += 1;
      string_cursor = string_start;
      continue;
    }
    
    break;
  }
  
  return matched;
}

internal bool32
str8_starts_with(String8 string, String8 expected_prefix)
{
  return str8_match(str8_prefix(string, expected_prefix.size), expected_prefix, 0);
}

internal bool32
str8_starts_withi(String8 string, String8 expected_prefix)
{
  return str8_match(str8_prefix(string, expected_prefix.size), expected_prefix, StringMatchFlag_CaseInsensitive);
}

internal u64
str8_find_needle(String8 string, u64 start_pos, String8 needle, StringMatchFlags flags)
{
  u8 *p = string.str + start_pos;
  u64 stop_offset = Max(string.size + 1, needle.size) - needle.size;
  u8 *stop_p = string.str + stop_offset;
  if(needle.size > 0)
  {
    u8 *string_opl = string.str + string.size;
    String8 needle_tail = str8_skip(needle, 1);
    StringMatchFlags adjusted_flags = flags | StringMatchFlag_RightSideSloppy;
    u8 needle_first_char_adjusted = needle.str[0];
    if(adjusted_flags & StringMatchFlag_CaseInsensitive)
    {
      needle_first_char_adjusted = upper_from_char(needle_first_char_adjusted);
    }
    if(adjusted_flags & StringMatchFlag_SlashInsensitive)
    {
      needle_first_char_adjusted = correct_slash_from_char(needle_first_char_adjusted);
    }
    for(;p < stop_p; p += 1)
    {
      u8 haystack_char_adjusted = *p;
      if(adjusted_flags & StringMatchFlag_CaseInsensitive)
      {
        haystack_char_adjusted = upper_from_char(haystack_char_adjusted);
      }
      if(adjusted_flags & StringMatchFlag_SlashInsensitive)
      {
        haystack_char_adjusted = correct_slash_from_char(haystack_char_adjusted);
      }
      if(haystack_char_adjusted == needle_first_char_adjusted)
      {
        if(str8_match(str8_range(p + 1, string_opl), needle_tail, adjusted_flags))
        {
          break;
        }
      }
    }
  }
  u64 result = string.size;
  if(p < stop_p)
  {
    result = (u64)(p - string.str);
  }
  return result;
}

internal u64
str8_find_needle_reverse(String8 string, u64 start_pos, String8 needle, StringMatchFlags flags)
{
  u64 result = 0;
  for(s64 i = string.size - start_pos - needle.size; i >= 0; --i)
  {
    String8 haystack = str8_substr(string, rng_1u64(i, i + needle.size));
    if(str8_match(haystack, needle, flags))
    {
      result = (u64)i + needle.size;
      break;
    }
  }
  return result;
}

internal bool32
str8_is_before(String8 a, String8 b)
{
  bool32 result = 0;
  {
    u64 common_size = Min(a.size, b.size);
    for(u64 off = 0; off < common_size; off += 1)
    {
      if(a.str[off] < b.str[off])
      {
        result = 1;
        break;
      }
      else if(a.str[off] > b.str[off])
      {
        result = 0;
        break;
      }
      else if(off+1 == common_size)
      {
        result = (a.size < b.size);
      }
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: String Slicing

internal String8
str8_substr(String8 str, Rng1u64 range)
{
  range.min = ClampTop(range.min, str.size);
  range.max = ClampTop(range.max, str.size);
  str.str += range.min;
  str.size = dim_1u64(range);
  return str;
}

internal String8
str8_prefix(String8 str, u64 size)
{
  str.size = ClampTop(size, str.size);
  return str;
}

internal String8
str8_skip(String8 str, u64 amt)
{
  amt = ClampTop(amt, str.size);
  str.str += amt;
  str.size -= amt;
  return str;
}

internal String8
str8_postfix(String8 str, u64 size)
{
  size = ClampTop(size, str.size);
  str.str = (str.str + str.size) - size;
  str.size = size;
  return str;
}

internal String8
str8_chop(String8 str, u64 amt)
{
  amt = ClampTop(amt, str.size);
  str.size -= amt;
  return str;
}

internal String8
str8_chop_line(String8 *str)
{
  u64     new_line_pos = str8_find_needle(*str, 0, str8_lit("\n"), 0);
  String8 line         = str8_prefix(*str, new_line_pos);
  if (str8_ends_with(line, str8_lit("\r"), 0)) {
    line = str8_chop(line, 1);
  }
  *str = str8_skip(*str, new_line_pos + 1);
  return line;
}

internal String8
str8_skip_chop_whitespace(String8 string)
{
  u8 *first = string.str;
  u8 *opl = first + string.size;
  for(;first < opl; first += 1)
  {
    if(!char_is_space(*first))
    {
      break;
    }
  }
  for(;opl > first;)
  {
    opl -= 1;
    if(!char_is_space(*opl))
    {
      opl += 1;
      break;
    }
  }
  String8 result = str8_range(first, opl);
  return result;
}

internal String8
str8_skip_chop_slashes(String8 string)
{
  u8 *first = string.str;
  u8 *opl = first + string.size;
  for(;first < opl; first += 1)
  {
    if(!char_is_slash(*first))
    {
      break;
    }
  }
  for(;opl > first;)
  {
    opl -= 1;
    if(!char_is_slash(*opl))
    {
      opl += 1;
      break;
    }
  }
  String8 result = str8_range(first, opl);
  return result;
}

////////////////////////////////
//~ rjf: String Formatting & Copying

internal String8
str8_cat(Arena *arena, String8 s1, String8 s2)
{
  String8 str;
  str.size = s1.size + s2.size;
  str.str = push_array_no_zero(arena, u8, str.size + 1);
  MemoryCopy(str.str, s1.str, s1.size);
  MemoryCopy(str.str + s1.size, s2.str, s2.size);
  str.str[str.size] = 0;
  return str;
}

internal String8
str8_copy(Arena *arena, String8 s)
{
  String8 str;
  str.size = s.size;
  str.str = push_array_no_zero(arena, u8, str.size + 1);
  MemoryCopy(str.str, s.str, s.size);
  str.str[str.size] = 0;
  return str;
}

internal String8
str8fv(Arena *arena, char *fmt, va_list args){
  va_list args2;
  va_copy(args2, args);
  u32 needed_bytes = bplib_vsnprintf(0, 0, fmt, args) + 1;
  String8 result = {0};
  result.str = push_array_no_zero(arena, u8, needed_bytes);
  result.size = bplib_vsnprintf((char*)result.str, needed_bytes, fmt, args2);
  result.str[result.size] = 0;
  va_end(args2);
  return result;
}

internal String8
str8f(Arena *arena, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 result = push_str8fv(arena, fmt, args);
  va_end(args);
  return result;
}

internal String8
push_cstr(Arena *arena, String8 str)
{
  u64 buffer_size = str.size + 1;
  u8 *buffer = push_array_no_zero(arena, u8, buffer_size);
  MemoryCopy(buffer, str.str, str.size);
  buffer[str.size] = 0;
  String8 result = str8(buffer, buffer_size);
  return result;
}

////////////////////////////////
//~ rjf: String <=> Integer Conversions

//- rjf: string -> integer

internal s64
sign_from_str8(String8 string, String8 *string_tail)
{
  // count negative signs
  u64 neg_count = 0;
  u64 i = 0;
  for(; i < string.size; i += 1)
  {
    if (string.str[i] == '-'){
      neg_count += 1;
    }
    else if (string.str[i] != '+'){
      break;
    }
  }
  
  // output part of string after signs
  *string_tail = str8_skip(string, i);
  
  // output integer sign
  s64 sign = (neg_count & 1)?-1:+1;
  return sign;
}

internal bool32
str8_is_integer(String8 string, u32 radix)
{
  bool32 result = 0;
  if(string.size > 0)
  {
    if(1 < radix && radix <= 16)
    {
      result = 1;
      for(u64 i = 0; i < string.size; i += 1)
      {
        u8 c = string.str[i];
        if(!(c < 0x80) || integer_symbol_reverse[c] >= radix)
        {
          result = 0;
          break;
        }
      }
    }
  }
  return result;
}

internal bool32
str8_is_integer_signed(String8 string, u32 radix)
{
  bool32 result = 0;
  String8 sign = str8_prefix(string, 1);
  if(str8_match(sign, str8_lit("-"), 0))
  {
    result = str8_is_integer(str8_skip(string, 1), radix);
  }
  else
  {
    result = str8_is_integer(string, radix);
  }
  return result;
}

internal u64
u64_from_str8(String8 string, u32 radix)
{
  u64 x = 0;
  if(1 < radix && radix <= 16)
  {
    for(u64 i = 0; i < string.size; i += 1)
    {
      x *= radix;
      x += integer_symbol_reverse[string.str[i]&0x7F];
    }
  }
  return x;
}

internal s64
s64_from_str8(String8 string, u32 radix)
{
  s64 sign = sign_from_str8(string, &string);
  s64 x = (s64)u64_from_str8(string, radix) * sign;
  return x;
}

internal u32
u32_from_str8(String8 string, u32 radix)
{
  u64 x64 = u64_from_str8(string, radix);
  u32 x32 = safe_cast_u32(x64);
  return x32;
}

internal s32
s32_from_str8(String8 string, u32 radix)
{
  s64 x64 = s64_from_str8(string, radix);
  s32 x32 = safe_cast_s32(x64);
  return x32;
}

internal bool32
try_u64_from_str8_c_rules(String8 string, u64 *x)
{
  // rjf: unpack radix / prefix size based on string prefix
  u64 radix = 0;
  u64 prefix_size = 0;
  {
    // hex
    if(str8_match(str8_prefix(string, 2), str8_lit("0x"), StringMatchFlag_CaseInsensitive))
    {
      radix = 0x10, prefix_size = 2;
    }
    // binary
    else if(str8_match(str8_prefix(string, 2), str8_lit("0b"), StringMatchFlag_CaseInsensitive))
    {
      radix = 2, prefix_size = 2;
    }
    // octal
    else if(str8_match(str8_prefix(string, 1), str8_lit("0"), StringMatchFlag_CaseInsensitive) && string.size > 1)
    {
      radix = 010, prefix_size = 1;
    }
    // decimal
    else
    {
      radix = 10, prefix_size = 0;
    }
  }
  
  // rjf: convert if we can
  String8 integer    = str8_skip(string, prefix_size);
  bool32     is_integer = str8_is_integer(integer, radix);
  if(is_integer)
  {
    *x = u64_from_str8(integer, radix);
  }
  
  return is_integer;
}

internal bool32
try_s64_from_str8_c_rules(String8 string, s64 *x)
{
  String8 string_tail = {0};
  s64 sign = sign_from_str8(string, &string_tail);
  u64 x_u64 = 0;
  bool32 is_integer = try_u64_from_str8_c_rules(string_tail, &x_u64);
  *x = x_u64*sign;
  return is_integer;
}

//- rjf: integer -> string

internal String8
str8_from_memory_size(Arena *arena, u64 size)
{
  String8 result = {0};
  {
    if(size < Kilobytes(1))
    {
      result = push_str8f(arena, "%llu byte%s", size, size == 1 ? "" : "s");
    }
    else if(size < Megabytes(1))
    {
      result = push_str8f(arena, "%llu.%02llu KiB", size / Kilobytes(1), ((size * 100) / Kilobytes(1)) % 100);
    }
    else if(size < Gigabytes(1))
    {
      result = push_str8f(arena, "%llu.%02llu MiB", size / Megabytes(1), ((size * 100) / Megabytes(1)) % 100);
    }
    else if(size < Terabytes(1))
    {
      result = push_str8f(arena, "%llu.%02llu GiB", size / Gigabytes(1), ((size * 100) / Gigabytes(1)) % 100);
    }
    else
    {
      result = push_str8f(arena, "%llu.%02llu TiB", size / Terabytes(1), ((size * 100) / Terabytes(1)) % 100);
    }
  }
  return result;
}

internal String8
str8_from_count(Arena *arena, u64 count)
{
  String8 result = {0};
  {
    if(count < 1 * 1000)
    {
      result = push_str8f(arena, "%llu", count);
    }
    else if(count < 1000000)
    {
      u64 frac = ((count * 100) / 1000) % 100;
      if(frac > 0)
      {
        result = push_str8f(arena, "%llu.%02lluK", count / 1000, frac);
      }
      else
      {
        result = push_str8f(arena, "%lluK", count / 1000);
      }
    }
    else if(count < 1000000000)
    {
      u64 frac = ((count * 100) / 1000000) % 100;
      if(frac > 0)
      {
        result = push_str8f(arena, "%llu.%02lluM", count / 1000000, frac);
      }
      else
      {
        result = push_str8f(arena, "%lluM", count / 1000000);
      }
    }
    else
    {
      u64 frac = ((count * 100) * 1000000000) % 100;
      if(frac > 0)
      {
        result = push_str8f(arena, "%llu.%02lluB", count / 1000000000, frac);
      }
      else
      {
        result = push_str8f(arena, "%lluB", count / 1000000000, frac);
      }
    }
  }
  return result;
}

internal String8
str8_from_bits_u32(Arena *arena, u32 x)
{
  u8 c0 = 'a' + ((x >> 28) & 0xf);
  u8 c1 = 'a' + ((x >> 24) & 0xf);
  u8 c2 = 'a' + ((x >> 20) & 0xf);
  u8 c3 = 'a' + ((x >> 16) & 0xf);
  u8 c4 = 'a' + ((x >> 12) & 0xf);
  u8 c5 = 'a' + ((x >>  8) & 0xf);
  u8 c6 = 'a' + ((x >>  4) & 0xf);
  u8 c7 = 'a' + ((x >>  0) & 0xf);
  String8 result = push_str8f(arena, "%c%c%c%c%c%c%c%c", c0, c1, c2, c3, c4, c5, c6, c7);
  return result;
}

internal String8
str8_from_bits_u64(Arena *arena, u64 x)
{
  u8 c0 = 'a' + ((x >> 60) & 0xf);
  u8 c1 = 'a' + ((x >> 56) & 0xf);
  u8 c2 = 'a' + ((x >> 52) & 0xf);
  u8 c3 = 'a' + ((x >> 48) & 0xf);
  u8 c4 = 'a' + ((x >> 44) & 0xf);
  u8 c5 = 'a' + ((x >> 40) & 0xf);
  u8 c6 = 'a' + ((x >> 36) & 0xf);
  u8 c7 = 'a' + ((x >> 32) & 0xf);
  u8 c8 = 'a' + ((x >> 28) & 0xf);
  u8 c9 = 'a' + ((x >> 24) & 0xf);
  u8 ca = 'a' + ((x >> 20) & 0xf);
  u8 cb = 'a' + ((x >> 16) & 0xf);
  u8 cc = 'a' + ((x >> 12) & 0xf);
  u8 cd = 'a' + ((x >>  8) & 0xf);
  u8 ce = 'a' + ((x >>  4) & 0xf);
  u8 cf = 'a' + ((x >>  0) & 0xf);
  String8 result = push_str8f(arena,
                              "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                              c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf);
  return result;
}

internal String8 str8_from_u64(Arena *arena, u64 number, u32 radix, u8 min_digits, u8 digit_group_separator)
{
    String8 result = {0};
    {
        // rjf: prefix
        String8 prefix = {0};
        switch(radix) {
        case 16:{prefix = str8_lit("0x");}break;
        case 8: {prefix = str8_lit("0o");}break;
        case 2: {prefix = str8_lit("0b");}break;
        }
        
        // rjf: determine # of chars between separators
        u8 digit_group_size = 3;
        switch(radix) {
        default:break;
        case 2:
        case 8:
        case 16:
            {digit_group_size = 4;}break;
        }
        
        // rjf: prep
        u64 needed_leading_0s = 0;
        {
            u64 needed_digits = 1;
            {
                u64 u64_reduce = number;
                for(;;) {
                    u64_reduce /= radix;
                    if(u64_reduce == 0) {
                        break;
                    }
                    needed_digits += 1;
                }
            }
            needed_leading_0s = (min_digits > needed_digits) ? min_digits - needed_digits : 0;
            u64 needed_separators = 0;
            if(digit_group_separator != 0) {
                needed_separators = (needed_digits+needed_leading_0s)/digit_group_size;
                if(needed_separators > 0 && (needed_digits+needed_leading_0s)%digit_group_size == 0) {
                    needed_separators -= 1;
                }
            }
            result.size = prefix.size + needed_leading_0s + needed_separators + needed_digits;
            result.str = push_array_no_zero(arena, u8, result.size + 1);
            result.str[result.size] = 0;
        }
        
        // rjf: fill contents
        {
            u64 u64_reduce = number;
            u64 digits_until_separator = digit_group_size;
            for(u64 idx = 0; idx < result.size; idx += 1) {
                if(digits_until_separator == 0 && digit_group_separator != 0) {
                    result.str[result.size - idx - 1] = digit_group_separator;
                    digits_until_separator = digit_group_size+1;
                }
                else {
                    result.str[result.size - idx - 1] = lower_from_char(integer_symbols[u64_reduce%radix]);
                    u64_reduce /= radix;
                }
                digits_until_separator -= 1;
                if(u64_reduce == 0) {
                    break;
                }
            }
            for(u64 leading_0_idx = 0; leading_0_idx < needed_leading_0s; leading_0_idx += 1) {
                result.str[prefix.size + leading_0_idx] = '0';
            }
        }
        
        // rjf: fill prefix
        if(prefix.size != 0) {
            MemoryCopy(result.str, prefix.str, prefix.size);
        }
    }
    return result;
}

internal String8
str8_from_s64(Arena *arena, s64 number, u32 radix, u8 min_digits, u8 digit_group_separator)
{
  String8 result = {0};
  // TODO(rjf): preeeeetty sloppy...
  if(number < 0) {
      Temp scratch = scratch_begin(&arena, 1);
      String8 numeric_part = str8_from_u64(scratch.arena, (u64)(-number), radix, min_digits, digit_group_separator);
      result = push_str8f(arena, "-%S", numeric_part);
      scratch_end(scratch);
  }
  else {
      result = str8_from_u64(arena, (u64)number, radix, min_digits, digit_group_separator);
  }
  return result;
}

////////////////////////////////
//~ rjf: String <=> Float Conversions

internal f64
f64_from_str8(String8 string)
{
  // TODO(rjf): crappy implementation for now that just uses atof.
  f64 result = 0;
  if(string.size > 0)
  {
    // rjf: find starting pos of numeric string, as well as sign
    f64 sign = +1.0;
    if(string.str[0] == '-')
    {
      sign = -1.0;
    }
    else if(string.str[0] == '+')
    {
      sign = 1.0;
    }
    
    // rjf: gather numerics
    u64 num_valid_chars = 0;
    char buffer[64];
    bool32 exp = 0;
    for(u64 idx = 0; idx < string.size && num_valid_chars < sizeof(buffer)-1; idx += 1)
    {
      if(char_is_digit(string.str[idx], 10) || string.str[idx] == '.' || string.str[idx] == 'e' ||
         (exp && (string.str[idx] == '+' || string.str[idx] == '-')))
      {
        buffer[num_valid_chars] = string.str[idx];
        num_valid_chars += 1;
        exp = 0;
        exp = (string.str[idx] == 'e');
      }
    }
    
    // rjf: null-terminate (the reason for all of this!!!!!!)
    buffer[num_valid_chars] = 0;
    
    // rjf: do final conversion
    result = sign * atof(buffer);
  }
  return result;
}

////////////////////////////////
//~ rjf: String List Construction Functions

internal String8_Node *str8_list_push_node(String8_List *list, String8_Node *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->node_count += 1;
  list->total_size += node->string.size;
  return node;
}

internal String8_Node *
str8_list_push_node_set_string(String8_List *list, String8_Node *node, String8 string)
{
  SLLQueuePush(list->first, list->last, node);
  list->node_count += 1;
  list->total_size += string.size;
  node->string = string;
  return node;
}

internal String8_Node *
str8_list_push_node_front(String8_List *list, String8_Node *node)
{
  SLLQueuePushFront(list->first, list->last, node);
  list->node_count += 1;
  list->total_size += node->string.size;
  return node;
}

internal String8_Node *
str8_list_push_node_front_set_string(String8_List *list, String8_Node *node, String8 string)
{
  SLLQueuePushFront(list->first, list->last, node);
  list->node_count += 1;
  list->total_size += string.size;
  node->string = string;
  return node;
}

internal String8_Node *
str8_list_push(Arena *arena, String8_List *list, String8 string)
{
  String8_Node *node = push_array_no_zero(arena, String8_Node, 1);
  str8_list_push_node_set_string(list, node, string);
  return node;
}

internal String8_Node *
str8_list_push_front(Arena *arena, String8_List *list, String8 string)
{
  String8_Node *node = push_array_no_zero(arena, String8_Node, 1);
  str8_list_push_node_front_set_string(list, node, string);
  return node;
}

internal void
str8_list_concat_in_place(String8_List *list, String8_List *to_push)
{
  if(to_push->node_count != 0)
  {
    if(list->last)
    {
      list->node_count += to_push->node_count;
      list->total_size += to_push->total_size;
      list->last->next = to_push->first;
      list->last = to_push->last;
    }
    else
    {
      *list = *to_push;
    }
    MemoryZeroStruct(to_push);
  }
}

internal String8_Node*
str8_list_push_aligner(Arena *arena, String8_List *list, u64 min, u64 align)
{
  read_only local_persist u8 zeroes[64] = {0};
  Assert(IsPow2OrZero(align));
  u64 pad = Max(min, AlignPadPow2(list->total_size, align));
  if(pad < sizeof(zeroes))
  {
    return str8_list_push(arena, list, str8(zeroes, pad));
  }
  else
  {
    return str8_list_push(arena, list, str8(push_array(arena, u8, pad), pad));
  }
}

internal String8_Node*
str8_list_pushf(Arena *arena, String8_List *list, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(arena, fmt, args);
  String8_Node *result = str8_list_push(arena, list, string);
  va_end(args);
  return result;
}

internal String8_Node*
str8_list_push_frontf(Arena *arena, String8_List *list, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(arena, fmt, args);
  String8_Node *result = str8_list_push_front(arena, list, string);
  va_end(args);
  return result;
}

internal String8_Node*
str8_list_pop_front(String8_List *list)
{
  String8_Node *node = 0;
  if(list->node_count)
  {
    node = list->first;
    Assert(list->total_size >= list->first->string.size);
    list->node_count -= 1;
    list->total_size -= list->first->string.size;
    SLLQueuePop(list->first, list->last);
  }
  return node;
}

internal String8_List
str8_list_copy(Arena *arena, String8_List *list)
{
  String8_List result = {0};
  for(String8_Node *node = list->first; node != 0; node = node->next)
  {
    String8_Node *new_node = push_array_no_zero(arena, String8_Node, 1);
    String8 new_string = push_str8_copy(arena, node->string);
    str8_list_push_node_set_string(&result, new_node, new_string);
  }
  return result;
}

internal String8_List
str8_list_substr(Arena *arena, String8_List list, Rng1u64 range)
{
  String8_List result = {0};
  
  String8_Node *n = list.first;
  
  u64 front_min = 0;
  {
    u64 cursor = 0;
    for (; n != 0; cursor += n->string.size, n = n->next) {
      if (cursor + n->string.size > range.min) {
        front_min = range.min - cursor;
        break;
      }
    }
  }
  
  if (front_min > 0) {
    u64 front_max = front_min + Min(dim_1u64(range), n->string.size);
    str8_list_push(arena, &result, str8_substr(n->string, r1u64(front_min, front_max)));
    n = n->next;
  }
  
  for (; n != 0; n = n->next) {
    if (result.total_size >= dim_1u64(range)) {
      break;
    }
    u64 copy_max  = dim_1u64(range) - result.total_size;
    u64 copy_size = Min(copy_max, n->string.size);
    str8_list_push(arena, &result, str8_substr(n->string, r1u64(0, copy_size)));
  }
  
  return result;
}

////////////////////////////////
//~ rjf: String Splitting & Joining

internal String8_List
str8_split(Arena *arena, String8 string, u8 *split_chars, u64 split_char_count, StringSplitFlags flags)
{
  String8_List list = {0};
  bool32 keep_empties = (flags & StringSplitFlag_KeepEmpties);
  u8 *ptr = string.str;
  u8 *opl = string.str + string.size;
  for(;ptr < opl;)
  {
    u8 *first = ptr;
    for(;ptr < opl; ptr += 1)
    {
      u8 c = *ptr;
      bool32 is_split = 0;
      for(u64 i = 0; i < split_char_count; i += 1)
      {
        if(split_chars[i] == c)
        {
          is_split = 1;
          break;
        }
      }
      if(is_split)
      {
        break;
      }
    }
    String8 string = str8_range(first, ptr);
    if(keep_empties || string.size > 0)
    {
      str8_list_push(arena, &list, string);
    }
    ptr += 1;
  }
  return list;
}

internal String8_List
str8_split_by_string_chars(Arena *arena, String8 string, String8 split_chars, StringSplitFlags flags)
{
  String8_List list = str8_split(arena, string, split_chars.str, split_chars.size, flags);
  return list;
}

internal String8
str8_list_join(Arena *arena, String8_List *list, String_Join *optional_params)
{
  String_Join join = {0};
  if(optional_params != 0)
  {
    MemoryCopyStruct(&join, optional_params);
  }
  u64 sep_count = 0;
  if(list->node_count > 0)
  {
    sep_count = list->node_count - 1;
  }
  String8 result;
  result.size = join.pre.size + join.post.size + sep_count*join.sep.size + list->total_size;
  u8 *ptr = result.str = push_array_no_zero(arena, u8, result.size + 1);
  MemoryCopy(ptr, join.pre.str, join.pre.size);
  ptr += join.pre.size;
  for(String8_Node *node = list->first;
      node != 0;
      node = node->next)
  {
    MemoryCopy(ptr, node->string.str, node->string.size);
    ptr += node->string.size;
    if(node->next != 0)
    {
      MemoryCopy(ptr, join.sep.str, join.sep.size);
      ptr += join.sep.size;
    }
  }
  MemoryCopy(ptr, join.post.str, join.post.size);
  ptr += join.post.size;
  *ptr = 0;
  return result;
}

////////////////////////////////
//~ rjf: Basic Data Structure Stringification Helpers

internal String8_List
numeric_str8_list_from_data(Arena *arena, u32 radix, String8 data, u64 stride)
{
  String8_List strs = {0};
  u64 count = data.size/stride;
  for EachIndex(idx, count)
  {
    u64 val = 0;
    MemoryCopy(&val, data.str + idx*stride, stride);
    str8_list_push(arena, &strs, str8_from_u64(arena, val, radix, 0, 0));
  }
  return strs;
}

////////////////////////////////
//~ rjf; String Arrays

internal String8_Array
str8_array_zero(void)
{
  String8_Array result = {0};
  return result;
}

internal String8_Array
str8_array_from_list(Arena *arena, String8_List *list)
{
  String8_Array array;
  array.count   = list->node_count;
  array.v = push_array_no_zero(arena, String8, array.count);
  u64 idx = 0;
  for(String8_Node *n = list->first; n != 0; n = n->next, idx += 1)
  {
    array.v[idx] = n->string;
  }
  return array;
}

internal String8_Array *
str8_array_from_list_arr(Arena *arena, String8_List *lists, u64 count)
{
  String8_Array *result = push_array(arena, String8_Array, count);
  for (u64 idx = 0; idx < count; idx += 1) {
    result[idx] = str8_array_from_list(arena, &lists[idx]);
  }
  return result;
}

internal String8_Array
str8_array_reserve(Arena *arena, u64 count)
{
  String8_Array arr;
  arr.count = 0;
  arr.v = push_array(arena, String8, count);
  return arr;
}

internal String8_Array
str8_array_copy(Arena *arena, String8_Array array)
{
  String8_Array result = {0};
  result.count = array.count;
  result.v = push_array(arena, String8, result.count);
  for EachIndex(idx, result.count)
  {
    result.v[idx] = push_str8_copy(arena, array.v[idx]);
  }
  return result;
}

////////////////////////////////
//~ rjf: String Version Helpers

internal u64
version_from_str8(String8 string)
{
  u64 result = 0;
  Temp scratch = scratch_begin(0, 0);
  u64 version_major = 0;
  u64 version_minor = 0;
  u64 version_patch = 0;
  String8_List version_parts = str8_split(scratch.arena, string, (u8 *)".", 1, 0);
  if(version_parts.first &&
     version_parts.first->next &&
     version_parts.first->next->next)
  {
    try_u64_from_str8_c_rules(version_parts.first->string, &version_major);
    try_u64_from_str8_c_rules(version_parts.first->next->string, &version_minor);
    try_u64_from_str8_c_rules(version_parts.first->next->next->string, &version_patch);
    result = Version(version_major, version_minor, version_patch);
  }
  scratch_end(scratch);
  return result;
}

internal String8
str8_from_version(Arena *arena, u64 version)
{
  u64 version_major = MajorFromVersion(version);
  u64 version_minor = MinorFromVersion(version);
  u64 version_patch = PatchFromVersion(version);
  String8 result = str8f(arena, "%I64d.%I64d.%I64d", version_major, version_minor, version_patch);
  return result;
}

////////////////////////////////
//~ rjf: String Path Helpers

internal String8
program_ext_postfix_from_os(OperatingSystem os, bool32 require_ext)
{
  String8 result = {0};
  switch(os)
  {
    default:{}break;
    case OperatingSystem_Windows:
    {
      result = s(".exe");
    }break;
    case OperatingSystem_Linux:
    if(require_ext)
    {
      result = s(".elf");
    }break;
    case OperatingSystem_Mac:
    if(require_ext)
    {
      result = s(".macho");
    }break;
  }
  return result;
}

internal String8
str8_chop_last_slash(String8 string)
{
  if(string.size > 0)
  {
    u8 *ptr = string.str + string.size - 1;
    for(;ptr >= string.str; ptr -= 1)
    {
      if(*ptr == '/' || *ptr == '\\')
      {
        break;
      }
    }
    if(ptr >= string.str)
    {
      string.size = (u64)(ptr - string.str);
    }
    else
    {
      string.size = 0;
    }
  }
  return string;
}

internal String8
str8_skip_last_slash(String8 string)
{
  if(string.size > 0)
  {
    u8 *ptr = string.str + string.size - 1;
    for(;ptr >= string.str; ptr -= 1)
    {
      if(*ptr == '/' || *ptr == '\\')
      {
        break;
      }
    }
    if(ptr >= string.str)
    {
      ptr += 1;
      string.size = (u64)(string.str + string.size - ptr);
      string.str = ptr;
    }
  }
  return string;
}

internal String8
str8_chop_last_dot(String8 string)
{
  String8 result = string;
  u64 p = string.size;
  for(;p > 0;)
  {
    p -= 1;
    if(string.str[p] == '.')
    {
      result = str8_prefix(string, p);
      break;
    }
  }
  return result;
}

internal String8
str8_skip_last_dot(String8 string)
{
  String8 result = string;
  u64 p = string.size;
  for(;p > 0;)
  {
    p -= 1;
    if(string.str[p] == '.')
    {
      result = str8_skip(string, p + 1);
      break;
    }
  }
  return result;
}

internal PathStyle
path_style_from_str8(String8 string)
{
  PathStyle result = PathStyle_Relative;
  if(string.size >= 1 && string.str[0] == '/')
  {
    result = PathStyle_UnixAbsolute;
  }
  else if(string.size >= 2 &&
          char_is_alpha(string.str[0]) &&
          string.str[1] == ':')
  {
    if(string.size == 2 || char_is_slash(string.str[2]))
    {
      result = PathStyle_WindowsAbsolute;
    }
  }
  return result;
}

internal String8_List
str8_split_path(Arena *arena, String8 string)
{
  String8_List result = str8_split(arena, string, (u8*)"/\\", 2, 0);
  return result;
}

internal void
str8_path_list_resolve_dots_in_place(String8_List *path, PathStyle style)
{
  Temp scratch = scratch_begin(0, 0);
  typedef struct String8MetaNode String8MetaNode;
  struct String8MetaNode
  {
    String8MetaNode *next;
    String8_Node *node;
  };
  String8MetaNode *stack = 0;
  String8MetaNode *free_meta_node = 0;
  String8_Node *first = path->first;
  MemoryZeroStruct(path);
  for(String8_Node *node = first, *next = 0;
      node != 0;
      node = next)
  {
    // save next now
    next = node->next;
    
    // cases:
    if(node == first && style == PathStyle_WindowsAbsolute)
    {
      goto save_without_stack;
    }
    if(node->string.size == 1 && node->string.str[0] == '.')
    {
      goto do_nothing;
    }
    if(node->string.size == 2 && node->string.str[0] == '.' && node->string.str[1] == '.')
    {
      if(stack != 0)
      {
        goto eliminate_stack_top;
      }
      else
      {
        goto save_without_stack;
      }
    }
    goto save_with_stack;
    
    
    // handlers:
    save_with_stack:
    {
      str8_list_push_node(path, node);
      String8MetaNode *stack_node = free_meta_node;
      if(stack_node != 0)
      {
        SLLStackPop(free_meta_node);
      }
      else
      {
        stack_node = push_array_no_zero(scratch.arena, String8MetaNode, 1);
      }
      SLLStackPush(stack, stack_node);
      stack_node->node = node;
      continue;
    }
    
    save_without_stack:
    {
      str8_list_push_node(path, node);
      continue;
    }
    
    eliminate_stack_top:
    {
      path->node_count -= 1;
      path->total_size -= stack->node->string.size;
      SLLStackPop(stack);
      if(stack == 0)
      {
        path->last = path->first;
      }
      else
      {
        path->last = stack->node;
      }
      continue;
    }
    
    do_nothing: continue;
  }
  scratch_end(scratch);
}

internal String8
str8_path_list_join_by_style(Arena *arena, String8_List *path, PathStyle style)
{
  String_Join params = {0};
  switch(style)
  {
    case PathStyle_Null:{}break;
    case PathStyle_Relative:
    case PathStyle_WindowsAbsolute:
    {
      params.sep = str8_lit("/");
    }break;
    
    case PathStyle_UnixAbsolute:
    {
      params.pre = str8_lit("/");
      params.sep = str8_lit("/");
    }break;
  }
  String8 result = str8_list_join(arena, path, &params);
  return result;
}

internal String8_TxtPt_Pair
str8_txt_pt_pair_from_string(String8 string)
{
  String8_TxtPt_Pair pair = {0};
  {
    String8 file_part = {0};
    String8 line_part = {0};
    String8 col_part = {0};
    
    // rjf: grab file part
    for(u64 idx = 0; idx <= string.size; idx += 1)
    {
      u8 byte = (idx < string.size) ? (string.str[idx]) : 0;
      u8 next_byte = ((idx+1 < string.size) ? (string.str[idx+1]) : 0);
      if(byte == ':' && next_byte != '/' && next_byte != '\\')
      {
        file_part = str8_prefix(string, idx);
        line_part = str8_skip(string, idx+1);
        break;
      }
      else if(byte == 0)
      {
        file_part = string;
        break;
      }
    }
    
    // rjf: grab line/column
    {
      u64 colon_pos = str8_find_needle(line_part, 0, str8_lit(":"), 0);
      if(colon_pos < line_part.size)
      {
        col_part = str8_skip(line_part, colon_pos+1);
        line_part = str8_prefix(line_part, colon_pos);
      }
    }
    
    // rjf: convert line/column strings to numerics
    u64 line = 0;
    u64 column = 0;
    try_u64_from_str8_c_rules(line_part, &line);
    try_u64_from_str8_c_rules(col_part, &column);
    
    // rjf: fill
    pair.string = file_part;
    pair.pt = txt_pt((s64)line, (s64)column);
    if(pair.pt.line == 0) { pair.pt.line = 1; }
    if(pair.pt.column == 0) { pair.pt.column = 1; }
  }
  return pair;
}

////////////////////////////////
//~ rjf: Relative <-> Absolute Path

internal String8
path_relative_dst_from_absolute_dst_src(Arena *arena, String8 dst, String8 src)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // rjf: gather path parts
  String8 dst_name = str8_skip_last_slash(dst);
  String8 src_folder = src;
  String8 dst_folder = str8_chop_last_slash(dst);
  String8_List src_folders = str8_split_path(scratch.arena, src_folder);
  String8_List dst_folders = str8_split_path(scratch.arena, dst_folder);
  
  // rjf: count # of backtracks to get from src -> dest
  u64 num_backtracks = src_folders.node_count;
  for(String8_Node *src_n = src_folders.first, *bp_n = dst_folders.first;
      src_n != 0 && bp_n != 0;
      src_n = src_n->next, bp_n = bp_n->next)
  {
    if(str8_match(src_n->string, bp_n->string, path_match_flags_from_os(OperatingSystem_CURRENT)))
    {
      num_backtracks -= 1;
    }
    else
    {
      break;
    }
  }
  
  // rjf: only build relative string if # of backtracks is not the entire `src`.
  // if getting to `dst` from `src` requires erasing the entire `src`, then the
  // only possible way to get to `dst` from `src` is via absolute path.
  String8 dst_path = {0};
  if(num_backtracks >= src_folders.node_count)
  {
    dst_path = dst;
  }
  else
  {
    // rjf: build backtrack parts
    String8_List dst_path_strs = {0};
    for(u64 idx = 0; idx < num_backtracks; idx += 1)
    {
      str8_list_push(scratch.arena, &dst_path_strs, str8_lit(".."));
    }
    
    // rjf: build parts of dst which are unique from src
    {
      bool32 unique_from_src = 0;
      for(String8_Node *src_n = src_folders.first, *bp_n = dst_folders.first;
          bp_n != 0;
          bp_n = bp_n->next)
      {
        if(!unique_from_src && (src_n == 0 || !str8_match(src_n->string, bp_n->string, path_match_flags_from_os(OperatingSystem_CURRENT))))
        {
          unique_from_src = 1;
        }
        if(unique_from_src)
        {
          str8_list_push(scratch.arena, &dst_path_strs, bp_n->string);
        }
        if(src_n != 0)
        {
          src_n = src_n->next;
        }
      }
    }
    
    // rjf: build file name
    str8_list_push(scratch.arena, &dst_path_strs, dst_name);
    
    // rjf: join
    String_Join join = {0};
    {
      join.sep = str8_lit("/");
    }
    dst_path = str8_list_join(arena, &dst_path_strs, &join);
  }
  scratch_end(scratch);
  return dst_path;
}

internal String8
path_absolute_dst_from_relative_dst_src(Arena *arena, String8 dst, String8 src)
{
  String8 result = dst;
  PathStyle dst_style = path_style_from_str8(dst);
  if(dst.size != 0 && dst_style == PathStyle_Relative)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 dst_from_src_absolute = push_str8f(scratch.arena, "%S/%S", src, dst);
    String8_List dst_from_src_absolute_parts = str8_split_path(scratch.arena, dst_from_src_absolute);
    PathStyle dst_from_src_absolute_style = path_style_from_str8(src);
    str8_path_list_resolve_dots_in_place(&dst_from_src_absolute_parts, dst_from_src_absolute_style);
    result = str8_path_list_join_by_style(arena, &dst_from_src_absolute_parts, dst_from_src_absolute_style);
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: Path Normalization

internal String8_List
path_normalized_list_from_string(Arena *arena, String8 path_string, PathStyle *style_out)
{
  // rjf: analyze path
  PathStyle path_style = path_style_from_str8(path_string);
  String8_List path = str8_split_path(arena, path_string);
  
  // rjf: resolve dots
  str8_path_list_resolve_dots_in_place(&path, path_style);
  
  // rjf: return
  if(style_out != 0)
  {
    *style_out = path_style;
  }
  return path;
}

internal String8
path_normalized_from_string(Arena *arena, String8 path_string)
{
  Temp scratch = scratch_begin(&arena, 1);
  PathStyle style = PathStyle_Relative;
  String8_List path = path_normalized_list_from_string(scratch.arena, path_string, &style);
  String8 result = str8_path_list_join_by_style(arena, &path, style);
  scratch_end(scratch);
  return result;
}

internal bool32
path_match_normalized(String8 left, String8 right)
{
  Temp scratch = scratch_begin(0, 0);
  String8 left_normalized = path_normalized_from_string(scratch.arena, left);
  String8 right_normalized = path_normalized_from_string(scratch.arena, right);
  bool32 result = str8_match(left_normalized, right_normalized, StringMatchFlag_CaseInsensitive);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Misc. Path Helpers

internal PathStyle
path_style_from_string(String8 string)
{
  for (u64 i = 0; i < ArrayCount(g_path_style_map); ++i)
  {
    if(str8_match(g_path_style_map[i].string, string, StringMatchFlag_CaseInsensitive))
    {
      return g_path_style_map[i].path_style;
    }
  }
  return PathStyle_Null;
}

internal String8
string_from_path_style(PathStyle style)
{
  Assert(style < ArrayCount(g_path_style_map));
  return g_path_style_map[style].string;
}

internal String8
path_separator_string_from_style(PathStyle style)
{
  String8 result = str8_zero();
  switch (style)
  {
    case PathStyle_Null:     break;
    case PathStyle_Relative: break;
    case PathStyle_WindowsAbsolute: result = str8_lit("\\"); break;
    case PathStyle_UnixAbsolute:    result = str8_lit("/");  break;
  }
  return result;
}

internal StringMatchFlags
path_match_flags_from_os(OperatingSystem os)
{
  StringMatchFlags flags = StringMatchFlag_SlashInsensitive;
  switch(os)
  {
    default:{}break;
    case OperatingSystem_Windows:
    {
      flags |= StringMatchFlag_CaseInsensitive;
    }break;
    case OperatingSystem_Linux:
    case OperatingSystem_Mac:
    {
      // NOTE(rjf): no-op
    }break;
  }
  return flags;
}

internal String8
path_convert_slashes(Arena *arena, String8 path, PathStyle path_style)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8_List list = str8_split_path(scratch.arena, path);
  String_Join join = {0};
  join.sep = path_separator_string_from_style(path_style);
  String8 result = str8_list_join(arena, &list, &join);
  scratch_end(scratch);
  return result;
}

internal String8
path_replace_file_extension(Arena *arena, String8 file_name, String8 ext)
{
  String8 file_name_no_ext = str8_chop_last_dot(file_name);
  String8 result           = str8f(arena, "%S.%S", file_name_no_ext, ext);
  return result;
}

////////////////////////////////
//~ rjf: UTF-8 & UTF-16 Decoding/Encoding

read_only global u8 utf8_class[32] =
{
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

internal Unicode_Decode
utf8_decode(u8 *str, u64 max)
{
  Unicode_Decode result = {1, max_u32};
  u8 byte = str[0];
  u8 byte_class = utf8_class[byte >> 3];
  switch(byte_class)
  {
    case 1:
    {
      result.codepoint = byte;
    }break;
    case 2:
    {
      if(1 < max)
      {
        u8 cont_byte = str[1];
        if(utf8_class[cont_byte >> 3] == 0)
        {
          result.codepoint = (byte & bitmask5) << 6;
          result.codepoint |=  (cont_byte & bitmask6);
          result.inc = 2;
        }
      }
    }break;
    case 3:
    {
      if(2 < max)
      {
        u8 cont_byte[2] = {str[1], str[2]};
        if(utf8_class[cont_byte[0] >> 3] == 0 &&
           utf8_class[cont_byte[1] >> 3] == 0)
        {
          result.codepoint = (byte & bitmask4) << 12;
          result.codepoint |= ((cont_byte[0] & bitmask6) << 6);
          result.codepoint |=  (cont_byte[1] & bitmask6);
          result.inc = 3;
        }
      }
    }break;
    case 4:
    {
      if(3 < max)
      {
        u8 cont_byte[3] = {str[1], str[2], str[3]};
        if(utf8_class[cont_byte[0] >> 3] == 0 &&
           utf8_class[cont_byte[1] >> 3] == 0 &&
           utf8_class[cont_byte[2] >> 3] == 0)
        {
          result.codepoint = (byte & bitmask3) << 18;
          result.codepoint |= ((cont_byte[0] & bitmask6) << 12);
          result.codepoint |= ((cont_byte[1] & bitmask6) <<  6);
          result.codepoint |=  (cont_byte[2] & bitmask6);
          result.inc = 4;
        }
      }
    }
  }
  return result;
}

internal Unicode_Decode
utf16_decode(u16 *str, u64 max)
{
  Unicode_Decode result = {1, max_u32};
  result.codepoint = str[0];
  result.inc = 1;
  if(max > 1 && 0xD800 <= str[0] && str[0] < 0xDC00 && 0xDC00 <= str[1] && str[1] < 0xE000)
  {
    result.codepoint = ((str[0] - 0xD800) << 10) | ((str[1] - 0xDC00) + 0x10000);
    result.inc = 2;
  }
  return result;
}

internal u32
utf8_encode(u8 *str, u32 codepoint)
{
  u32 inc = 0;
  if(codepoint <= 0x7F)
  {
    str[0] = (u8)codepoint;
    inc = 1;
  }
  else if(codepoint <= 0x7FF)
  {
    str[0] = (bitmask2 << 6) | ((codepoint >> 6) & bitmask5);
    str[1] = bit8 | (codepoint & bitmask6);
    inc = 2;
  }
  else if(codepoint <= 0xFFFF)
  {
    str[0] = (bitmask3 << 5) | ((codepoint >> 12) & bitmask4);
    str[1] = bit8 | ((codepoint >> 6) & bitmask6);
    str[2] = bit8 | ( codepoint       & bitmask6);
    inc = 3;
  }
  else if(codepoint <= 0x10FFFF)
  {
    str[0] = (bitmask4 << 4) | ((codepoint >> 18) & bitmask3);
    str[1] = bit8 | ((codepoint >> 12) & bitmask6);
    str[2] = bit8 | ((codepoint >>  6) & bitmask6);
    str[3] = bit8 | ( codepoint        & bitmask6);
    inc = 4;
  }
  else
  {
    str[0] = '?';
    inc = 1;
  }
  return inc;
}

internal u32
utf16_encode(u16 *str, u32 codepoint)
{
  u32 inc = 1;
  if(codepoint == max_u32)
  {
    str[0] = (u16)'?';
  }
  else if(codepoint < 0x10000)
  {
    str[0] = (u16)codepoint;
  }
  else
  {
    u32 v = codepoint - 0x10000;
    str[0] = safe_cast_u16(0xD800 + (v >> 10));
    str[1] = safe_cast_u16(0xDC00 + (v & bitmask10));
    inc = 2;
  }
  return inc;
}

////////////////////////////////
//~ rjf: Unicode String Conversions

internal String8
str8_from_16(Arena *arena, String16 in)
{
  String8 result = str8_zero();
  if(in.size)
  {
    u64 cap = in.size*3;
    u8 *str = push_array_no_zero(arena, u8, cap + 1);
    u16 *ptr = in.str;
    u16 *opl = ptr + in.size;
    u64 size = 0;
    Unicode_Decode consume;
    for(;ptr < opl; ptr += consume.inc)
    {
      consume = utf16_decode(ptr, opl - ptr);
      size += utf8_encode(str + size, consume.codepoint);
    }
    str[size] = 0;
    arena_pop(arena, (cap - size));
    result = str8(str, size);
  }
  return result;
}

internal String16
str16_from_8(Arena *arena, String8 in)
{
  String16 result = str16_zero();
  if(in.size)
  {
    u64 cap = in.size*2;
    u16 *str = push_array_no_zero(arena, u16, cap + 1);
    u8 *ptr = in.str;
    u8 *opl = ptr + in.size;
    u64 size = 0;
    Unicode_Decode consume;
    for(;ptr < opl; ptr += consume.inc)
    {
      consume = utf8_decode(ptr, opl - ptr);
      size += utf16_encode(str + size, consume.codepoint);
    }
    str[size] = 0;
    arena_pop(arena, (cap - size)*2);
    result = str16(str, size);
  }
  return result;
}

internal String8
str8_from_32(Arena *arena, String32 in)
{
  String8 result = str8_zero();
  if(in.size)
  {
    u64 cap = in.size*4;
    u8 *str = push_array_no_zero(arena, u8, cap + 1);
    u32 *ptr = in.str;
    u32 *opl = ptr + in.size;
    u64 size = 0;
    for(;ptr < opl; ptr += 1)
    {
      size += utf8_encode(str + size, *ptr);
    }
    str[size] = 0;
    arena_pop(arena, (cap - size));
    result = str8(str, size);
  }
  return result;
}

internal String32
str32_from_8(Arena *arena, String8 in)
{
  String32 result = str32_zero(); 
  if(in.size)
  {
    u64 cap = in.size;
    u32 *str = push_array_no_zero(arena, u32, cap + 1);
    u8 *ptr = in.str;
    u8 *opl = ptr + in.size;
    u64 size = 0;
    Unicode_Decode consume;
    for(;ptr < opl; ptr += consume.inc)
    {
      consume = utf8_decode(ptr, opl - ptr);
      str[size] = consume.codepoint;
      size += 1;
    }
    str[size] = 0;
    arena_pop(arena, (cap - size)*4);
    result = str32(str, size);
  }
  return result;
}

////////////////////////////////
//~ rjf: Basic Types & Space Enum -> String Conversions

read_only global struct
{
  String8         string;
  OperatingSystem os;
} g_os_enum_map[] =
{
  { str8_lit_comp(""),        OperatingSystem_Null     },
  { str8_lit_comp("Windows"), OperatingSystem_Windows, },
  { str8_lit_comp("Linux"),   OperatingSystem_Linux,   },
  { str8_lit_comp("Mac"),     OperatingSystem_Mac,     },
};
StaticAssert(ArrayCount(g_os_enum_map) == OperatingSystem_COUNT, g_os_enum_map_count_check);

internal OperatingSystem
operating_system_from_string(String8 string)
{
  for EachElement(idx, g_os_enum_map)
  {
    if(str8_match(g_os_enum_map[idx].string, string, StringMatchFlag_CaseInsensitive))
    {
      return g_os_enum_map[idx].os;
    }
  }
  return OperatingSystem_Null;
}

internal String8
string_from_dimension(Dimension dimension)
{
  read_only local_persist String8 strings[] =
  {
    str8_lit_comp("X"),
    str8_lit_comp("Y"),
    str8_lit_comp("Z"),
    str8_lit_comp("W"),
  };
  String8 result = str8_lit("error");
  if((u32)dimension < 4)
  {
    result = strings[dimension];
  }
  return result;
}

internal String8
string_from_side(Side side)
{
  local_persist String8 strings[] =
  {
    str8_lit_comp("Min"),
    str8_lit_comp("Max"),
  };
  String8 result = str8_lit("error");
  if((u32)side < 2)
  {
    result = strings[side];
  }
  return result;
}

internal String8
string_from_operating_system(OperatingSystem os)
{
  String8 result = g_os_enum_map[OperatingSystem_Null].string;
  if(os < ArrayCount(g_os_enum_map))
  {
    result = g_os_enum_map[os].string;
  }
  return result;
}

internal String8
string_from_arch(Arch arch)
{
  String8 result = {0};
  switch(arch)
  {
    case Arch_Null:  {result = str8_lit("Null");}break;
    case Arch_x64:   {result = str8_lit("x64");}break;
    case Arch_x86:   {result = str8_lit("x86");}break;
    case Arch_arm64: {result = str8_lit("arm64");}break;
    case Arch_arm32: {result = str8_lit("arm32");}break;
    case Arch_COUNT:
    {result = str8_lit("Invalid");}break;
  }
  return result;
}

internal String8
string_from_week_day(WeekDay week_day)
{
  read_only local_persist String8 strings[] =
  {
    str8_lit_comp("Sun"),
    str8_lit_comp("Mon"),
    str8_lit_comp("Tue"),
    str8_lit_comp("Wed"),
    str8_lit_comp("Thu"),
    str8_lit_comp("Fri"),
    str8_lit_comp("Sat"),
  };
  String8 result = str8_lit("Err");
  if((u32)week_day < WeekDay_COUNT)
  {
    result = strings[week_day];
  }
  return result;
}

internal String8
string_from_month(Month month)
{
  read_only local_persist String8 strings[] =
  {
    str8_lit_comp("Jan"),
    str8_lit_comp("Feb"),
    str8_lit_comp("Mar"),
    str8_lit_comp("Apr"),
    str8_lit_comp("May"),
    str8_lit_comp("Jun"),
    str8_lit_comp("Jul"),
    str8_lit_comp("Aug"),
    str8_lit_comp("Sep"),
    str8_lit_comp("Oct"),
    str8_lit_comp("Nov"),
    str8_lit_comp("Dec"),
  };
  String8 result = str8_lit("Err");
  if((u32)month < Month_COUNT)
  {
    result = strings[month];
  }
  return result;
}

internal String8
string_from_date_time(Arena *arena, Date_Time *date_time)
{
  char *mon_str = (char*)string_from_month(date_time->month).str;
  u32 adjusted_hour = date_time->hour%12;
  if(adjusted_hour == 0)
  {
    adjusted_hour = 12;
  }
  char *ampm = "am";
  if(date_time->hour >= 12)
  {
    ampm = "pm";
  }
  String8 result = push_str8f(arena, "%d %s %d, %02d:%02d:%02d %s",
                              date_time->day, mon_str, date_time->year,
                              adjusted_hour, date_time->min, date_time->sec, ampm);
  return result;
}

internal String8
string_from_date_time__file_name(Arena *arena, Date_Time *date_time)
{
  char *mon_str = (char*)string_from_month(date_time->month).str;
  String8 result = str8f(arena, "%d-%s-%0d--%02d-%02d-%02d",
                         date_time->year, mon_str, date_time->day,
                         date_time->hour, date_time->min, date_time->sec);
  return result;
}

internal String8
string_from_elapsed_time(Arena *arena, Date_Time dt)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8_List list = {0};
  if (dt.year) {
    str8_list_pushf(scratch.arena, &list, "%dy", dt.year);
    str8_list_pushf(scratch.arena, &list, "%um", dt.mon);
    str8_list_pushf(scratch.arena, &list, "%ud", dt.day);
  } else if (dt.mon) {
    str8_list_pushf(scratch.arena, &list, "%um", dt.mon);
    str8_list_pushf(scratch.arena, &list, "%ud", dt.day);
  } else if (dt.day) {
    str8_list_pushf(scratch.arena, &list, "%ud", dt.day);
  }
  
  if (dt.hour) {
    str8_list_pushf(scratch.arena, &list, "%uh %um %u.%us", dt.hour, dt.min, dt.sec, dt.msec);
  } else if (dt.min) {
    str8_list_pushf(scratch.arena, &list, "%um %u.%us", dt.min, dt.sec, dt.msec);
  } else if (dt.sec) {
    str8_list_pushf(scratch.arena, &list, "%u.%us", dt.sec, dt.msec);
  } else if (dt.msec) {
    str8_list_pushf(scratch.arena, &list, "%ums", dt.msec);
  } else if (dt.micro_sec) {
    str8_list_pushf(scratch.arena, &list, "%uus", dt.micro_sec);
  }
  
  if (list.node_count == 0) {
    str8_list_pushf(scratch.arena, &list, "0");
  }
  
  String8 result = str8_list_join(arena, &list, &(String_Join){ str8_lit_comp(""), str8_lit_comp(" "), str8_lit_comp("") });
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: String <-> Globally Unique IDs

internal String8
string_from_guid(Arena *arena, Guid guid)
{
  String8 result = str8f(arena, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                         guid.data1,
                         guid.data2,
                         guid.data3,
                         guid.data4[0],
                         guid.data4[1],
                         guid.data4[2],
                         guid.data4[3],
                         guid.data4[4],
                         guid.data4[5],
                         guid.data4[6],
                         guid.data4[7]);
  return result;
}

internal bool32
try_guid_from_string(String8 string, Guid *guid_out)
{
  Temp scratch = scratch_begin(0,0);
  bool32 is_parsed = 0;
  String8_List list = str8_split_by_string_chars(scratch.arena, string, str8_lit("-"), StringSplitFlag_KeepEmpties);
  if(list.node_count == 5)
  {
    String8 data1_str    = list.first->string;
    String8 data2_str    = list.first->next->string;
    String8 data3_str    = list.first->next->next->string;
    String8 data4_hi_str = list.first->next->next->next->string;
    String8 data4_lo_str = list.first->next->next->next->next->string;
    if(str8_is_integer(data1_str, 16) && 
       str8_is_integer(data2_str, 16) &&
       str8_is_integer(data3_str, 16) &&
       str8_is_integer(data4_hi_str, 16) &&
       str8_is_integer(data4_lo_str, 16))
    {
      u64 data1    = u64_from_str8(data1_str, 16);
      u64 data2    = u64_from_str8(data2_str, 16);
      u64 data3    = u64_from_str8(data3_str, 16);
      u64 data4_hi = u64_from_str8(data4_hi_str, 16);
      u64 data4_lo = u64_from_str8(data4_lo_str, 16);
      if(data1 <= max_u32 &&
         data2 <= max_u16 &&
         data3 <= max_u16 &&
         data4_hi <= max_u16 &&
         data4_lo <= 0xffffffffffff)
      {
        guid_out->data1 = (u32)data1;
        guid_out->data2 = (u16)data2;
        guid_out->data3 = (u16)data3;
        u64 data4 = (data4_hi << 48) | data4_lo;
        MemoryCopy(&guid_out->data4[0], &data4, sizeof(data4));
        is_parsed = 1;
      }
    }
  }
  scratch_end(scratch);
  return is_parsed;
}

internal Guid
guid_from_string(String8 string)
{
  Guid guid = {0};
  try_guid_from_string(string, &guid);
  return guid;
}

////////////////////////////////
//~ rjf: Basic Text Indentation

internal String8
indented_from_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  read_only local_persist u8 indentation_bytes[] = "                                                                                                                                ";
  String8_List indented_strings = {0};
  s64 depth = 0;
  s64 next_depth = 0;
  u64 line_begin_off = 0;
  for(u64 off = 0; off <= string.size; off += 1)
  {
    u8 byte = off<string.size ? string.str[off] : 0;
    switch(byte)
    {
      default:{}break;
      case '{':case '[':case '(':{next_depth += 1; next_depth = Max(0, next_depth);}break;
      case '}':case ']':case ')':{next_depth -= 1; next_depth = Max(0, next_depth); depth = next_depth;}break;
      case '\n':
      case 0:
      {
        String8 line = str8_skip_chop_whitespace(str8_substr(string, r1u64(line_begin_off, off)));
        if(line.size != 0)
        {
          str8_list_pushf(scratch.arena, &indented_strings, "%.*s%S\n", (int)depth*2, indentation_bytes, line);
        }
        if(line.size == 0 && indented_strings.node_count != 0 && off < string.size)
        {
          str8_list_pushf(scratch.arena, &indented_strings, "\n");
        }
        line_begin_off = off+1;
        depth = next_depth;
      }break;
    }
  }
  String8 result = str8_list_join(arena, &indented_strings, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Text Escaping

internal String8
escaped_from_raw_str8(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8_List parts = {0};
  u64 start_split_idx = 0;
  for(u64 idx = 0; idx <= string.size; idx += 1)
  {
    u8 byte = (idx < string.size) ? string.str[idx] : 0;
    bool32 split = 1;
    String8 separator_replace = {0};
    switch(byte)
    {
      default:{split = 0;}break;
      case 0:    {}break;
      case '\a': {separator_replace = str8_lit("\\a");}break;
      case '\b': {separator_replace = str8_lit("\\b");}break;
      case '\f': {separator_replace = str8_lit("\\f");}break;
      case '\n': {separator_replace = str8_lit("\\n");}break;
      case '\r': {separator_replace = str8_lit("\\r");}break;
      case '\t': {separator_replace = str8_lit("\\t");}break;
      case '\v': {separator_replace = str8_lit("\\v");}break;
      case '\\': {separator_replace = str8_lit("\\\\");}break;
      case '"':  {separator_replace = str8_lit("\\\"");}break;
    }
    if(split)
    {
      String8 substr = str8_substr(string, r1u64(start_split_idx, idx));
      start_split_idx = idx+1;
      str8_list_push(scratch.arena, &parts, substr);
      if(separator_replace.size != 0)
      {
        str8_list_push(scratch.arena, &parts, separator_replace);
      }
    }
  }
  String_Join join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}

internal String8
raw_from_escaped_str8(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8_List strs = {0};
  u64 start = 0;
  for(u64 idx = 0; idx <= string.size; idx += 1)
  {
    if(idx == string.size || string.str[idx] == '\\' || string.str[idx] == '\r')
    {
      String8 str = str8_substr(string, r1u64(start, idx));
      if(str.size != 0)
      {
        str8_list_push(scratch.arena, &strs, str);
      }
      start = idx+1;
    }
    if(idx < string.size && string.str[idx] == '\\')
    {
      u8 next_char = string.str[idx+1];
      u8 replace_byte = 0;
      switch(next_char)
      {
        default:{}break;
        case 'a': replace_byte = 0x07; break;
        case 'b': replace_byte = 0x08; break;
        case 'e': replace_byte = 0x1b; break;
        case 'f': replace_byte = 0x0c; break;
        case 'n': replace_byte = 0x0a; break;
        case 'r': replace_byte = 0x0d; break;
        case 't': replace_byte = 0x09; break;
        case 'v': replace_byte = 0x0b; break;
        case '\\':replace_byte = '\\'; break;
        case '\'':replace_byte = '\''; break;
        case '"': replace_byte = '"';  break;
        case '?': replace_byte = '?';  break;
      }
      String8 replace_string = push_str8_copy(scratch.arena, str8(&replace_byte, 1));
      str8_list_push(scratch.arena, &strs, replace_string);
      idx += 1;
      start += 1;
    }
  }
  String8 result = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Text Wrapping

internal String8_List
wrapped_lines_from_string(Arena *arena, String8 string, u64 first_line_max_width, u64 max_width, u64 wrap_indent)
{
  String8_List list = {0};
  Rng1u64 line_range = r1u64(0, 0);
  u64 wrapped_indent_level = 0;
  static char *spaces = "                                                                ";
  for (u64 idx = 0; idx <= string.size; idx += 1){
    u8 chr = idx < string.size ? string.str[idx] : 0;
    if (chr == '\n'){
      Rng1u64 candidate_line_range = line_range;
      candidate_line_range.max = idx;
      // NOTE(nick): when wrapping is interrupted with \n we emit a string without including \n
      // because later tool_fprint_list inserts separator after each node
      // except for last node, so don't strip last \n.
      if (idx + 1 == string.size){
        candidate_line_range.max += 1;
      }
      String8 substr = str8_substr(string, candidate_line_range);
      str8_list_push(arena, &list, substr);
      line_range = r1u64(idx+1,idx+1);
    }
    else
      if (char_is_space(chr) || chr == 0){
      Rng1u64 candidate_line_range = line_range;
      candidate_line_range.max = idx;
      String8 substr = str8_substr(string, candidate_line_range);
      u64 width_this_line = max_width-wrapped_indent_level;
      if (list.node_count == 0){
        width_this_line = first_line_max_width;
      }
      if (substr.size > width_this_line){
        String8 line = str8_substr(string, line_range);
        if (wrapped_indent_level > 0){
          line = push_str8f(arena, "%.*s%S", wrapped_indent_level, spaces, line);
        }
        str8_list_push(arena, &list, line);
        line_range = r1u64(line_range.max+1, candidate_line_range.max);
        wrapped_indent_level = ClampTop(64, wrap_indent);
      }
      else{
        line_range = candidate_line_range;
      }
    }
  }
  if (line_range.min < string.size && line_range.max > line_range.min){
    String8 line = str8_substr(string, line_range);
    if (wrapped_indent_level > 0){
      line = push_str8f(arena, "%.*s%S", wrapped_indent_level, spaces, line);
    }
    str8_list_push(arena, &list, line);
  }
  return list;
}

////////////////////////////////
//~ rjf: String <-> Color

internal String8
hex_string_from_rgba_4f32(Arena *arena, Vec4f32 rgba)
{
  String8 hex_string = str8f(arena, "%02x%02x%02x%02x", (u8)(rgba.x*255.f), (u8)(rgba.y*255.f), (u8)(rgba.z*255.f), (u8)(rgba.w*255.f));
  return hex_string;
}

internal Vec4f32
rgba_from_hex_string_4f32(String8 hex_string)
{
  u8 byte_text[8] = {0};
  u64 byte_text_idx = 0;
  for(u64 idx = 0; idx < hex_string.size && byte_text_idx < ArrayCount(byte_text); idx += 1)
  {
    if(char_is_digit(hex_string.str[idx], 16))
    {
      byte_text[byte_text_idx] = lower_from_char(hex_string.str[idx]);
      byte_text_idx += 1;
    }
  }
  u8 byte_vals[4] = {0};
  for(u64 idx = 0; idx < 4; idx += 1)
  {
    byte_vals[idx] = (u8)u64_from_str8(str8(&byte_text[idx*2], 2), 16);
  }
  Vec4f32 rgba = v4f32(byte_vals[0]/255.f, byte_vals[1]/255.f, byte_vals[2]/255.f, byte_vals[3]/255.f);
  return rgba;
}

////////////////////////////////
//~ rjf: String Fuzzy Matching

internal Fuzzy_Match_Range_List
fuzzy_match_find(Arena *arena, String8 needle, String8 haystack)
{
  Fuzzy_Match_Range_List result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  String8_List needles = str8_split(scratch.arena, needle, (u8*)" ", 1, 0);
  result.needle_part_count = needles.node_count;
  for(String8_Node *needle_n = needles.first; needle_n != 0; needle_n = needle_n->next)
  {
    u64 find_pos = 0;
    for(;find_pos < haystack.size;)
    {
      find_pos = str8_find_needle(haystack, find_pos, needle_n->string, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive);
      bool32 is_in_gathered_ranges = 0;
      for(Fuzzy_Match_Range_Node *n = result.first; n != 0; n = n->next)
      {
        if(n->range.min <= find_pos && find_pos < n->range.max)
        {
          is_in_gathered_ranges = 1;
          find_pos = n->range.max;
          break;
        }
      }
      if(!is_in_gathered_ranges)
      {
        break;
      }
    }
    if(find_pos < haystack.size)
    {
      Rng1u64 range = r1u64(find_pos, find_pos+needle_n->string.size);
      Fuzzy_Match_Range_Node *n = push_array(arena, Fuzzy_Match_Range_Node, 1);
      n->range = range;
      SLLQueuePush(result.first, result.last, n);
      result.count += 1;
      result.total_dim += dim_1u64(range);
    }
  }
  scratch_end(scratch);
  return result;
}

internal Fuzzy_Match_Range_List
fuzzy_match_range_list_copy(Arena *arena, Fuzzy_Match_Range_List *src)
{
  Fuzzy_Match_Range_List dst = {0};
  for(Fuzzy_Match_Range_Node *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    Fuzzy_Match_Range_Node *dst_n = push_array(arena, Fuzzy_Match_Range_Node, 1);
    SLLQueuePush(dst.first, dst.last, dst_n);
    dst_n->range = src_n->range;
  }
  dst.count = src->count;
  dst.needle_part_count = src->needle_part_count;
  dst.total_dim = src->total_dim;
  return dst;
}

////////////////////////////////
//~ NOTE(allen): Serialization Helpers

internal void
str8_serial_begin(Arena *arena, String8_List *srl)
{
  String8_Node *node = push_array(arena, String8_Node, 1);
  node->string.str = push_array_no_zero(arena, u8, 0);
  srl->first      = srl->last = node;
  srl->node_count = 1;
  srl->total_size = 0;
}

internal String8
str8_serial_end(Arena *arena, String8_List *srl)
{
  u64 size = srl->total_size;
  u8 *out = push_array_no_zero(arena, u8, size);
  str8_serial_write_to_dst(srl, out);
  String8 result = str8(out, size);
  return result;
}

internal void
str8_serial_write_to_dst(String8_List *srl, void *out)
{
  u8 *ptr = (u8*)out;
  for EachNode(n, String8_Node, srl->first)
  {
    u64 size = n->string.size;
    MemoryCopy(ptr, n->string.str, size);
    ptr += size;
  }
}

internal u64
str8_serial_push_align(Arena *arena, String8_List *srl, u64 align)
{
  Assert(IsPow2(align));
  
  u64 pos = srl->total_size;
  u64 new_pos = AlignPow2(pos, align);
  u64 size = (new_pos - pos);
  
  if(size != 0)
  {
    u8 *buf = push_array(arena, u8, size);
    
    String8 *str = &srl->last->string;
    if(str->str + str->size == buf)
    {
      srl->last->string.size += size;
      srl->total_size += size;
    }
    else
    {
      str8_list_push(arena, srl, str8(buf, size));
    }
  }
  return size;
}

internal void *
str8_serial_push_size(Arena *arena, String8_List *srl, u64 size)
{
  void *result = 0;
  if(size != 0)
  {
    u8 *buf = push_array(arena, u8, size);
    String8 *str = &srl->last->string;
    if(str->str + str->size == buf)
    {
      srl->last->string.size += size;
      srl->total_size += size;
    }
    else
    {
      str8_list_push(arena, srl, str8(buf, size));
    }
    result = buf;
  }
  return result;
}

internal void *
str8_serial_push_data(Arena *arena, String8_List *srl, void *data, u64 size)
{
  void *result = str8_serial_push_size(arena, srl, size);
  if(result != 0)
  {
    MemoryCopy(result, data, size);
  }
  return result;
}

internal void
str8_serial_push_data_list(Arena *arena, String8_List *srl, String8_Node *first)
{
  for EachNode(n, String8_Node, first)
  {
    str8_serial_push_data(arena, srl, n->string.str, n->string.size);
  }
}

internal void *
str8_serial_push_u64(Arena *arena, String8_List *srl, u64 x)
{
  return str8_serial_push_data(arena, srl, &x, sizeof(x));
}

internal void *
str8_serial_push_u32(Arena *arena, String8_List *srl, u32 x)
{
  return str8_serial_push_data(arena, srl, &x, sizeof(x));
}

internal void *
str8_serial_push_u16(Arena *arena, String8_List *srl, u16 x)
{
  return str8_serial_push_data(arena, srl, &x, sizeof(x));
}

internal void *
str8_serial_push_u8(Arena *arena, String8_List *srl, u8 x)
{
  return str8_serial_push_data(arena, srl, &x, sizeof(x));
}

internal void *
str8_serial_push_cstr(Arena *arena, String8_List *srl, String8 str)
{
  void *ptr = str8_serial_push_data(arena, srl, str.str, str.size);
  str8_serial_push_u8(arena, srl, 0);
  return ptr;
}

internal void *
str8_serial_push_string(Arena *arena, String8_List *srl, String8 str)
{
  return str8_serial_push_data(arena, srl, str.str, str.size);
}

////////////////////////////////
//~ rjf: Deserialization Helpers

internal u64
str8_deserial_read(String8 string, u64 off, void *read_dst, u64 read_size, u64 granularity)
{
  u64 bytes_left = string.size-Min(off, string.size);
  u64 actually_readable_size = Min(bytes_left, read_size);
  u64 legally_readable_size = actually_readable_size - actually_readable_size%granularity;
  if(legally_readable_size > 0)
  {
    MemoryCopy(read_dst, string.str+off, legally_readable_size);
  }
  return legally_readable_size;
}

internal u64
str8_deserial_find_first_match(String8 string, u64 off, u16 scan_val)
{
  u64 cursor = off;
  for (;;) {
    u16 val = 0;
    str8_deserial_read_struct(string, cursor, &val);
    if (val == scan_val) {
      break;
    }
    cursor += sizeof(val);
  }
  return cursor;
}

internal void *
str8_deserial_get_raw_ptr(String8 string, u64 off, u64 size)
{
  void *raw_ptr = 0;
  if (off + size <= string.size) {
    raw_ptr = string.str + off;
  }
  return raw_ptr;
}

internal u64
str8_deserial_read_cstr(String8 string, u64 off, String8 *cstr_out)
{
  u64 cstr_size = 0;
  if (off < string.size) {
    u8 *ptr = string.str + off;
    u8 *cap = string.str + string.size;
    *cstr_out = str8_cstring_capped(ptr, cap);
    cstr_size = (cstr_out->size + 1);
  }
  return cstr_size;
}

internal u64
str8_deserial_read_windows_utf16_string16(String8 string, u64 off, String16 *str_out)
{
  u64 null_off = str8_deserial_find_first_match(string, off, 0);
  u64 size = null_off - off;
  u16 *str = (u16 *)str8_deserial_get_raw_ptr(string, off, size);
  u64 count = size / sizeof(*str);
  *str_out = str16(str, count);
  
  u64 read_size_with_null = size + sizeof(*str);
  return read_size_with_null;
}

internal u64
str8_deserial_read_block(String8 string, u64 off, u64 size, String8 *block_out)
{
  Rng1u64 range = rng_1u64(off, off + size);
  *block_out = str8_substr(string, range);
  return block_out->size;
}

internal u64
str8_deserial_read_uleb128(String8 string, u64 off, u64 *value_out)
{
  u64 value  = 0;
  u64 shift  = 0;
  u64 cursor = off;
  for(;;)
  {
    u8  byte       = 0;
    u64 bytes_read = str8_deserial_read_struct(string, cursor, &byte);
    if(bytes_read != sizeof(byte))
    {
      break;
    }
    u8 val = byte & 0x7fu;
    value |= ((u64)val) << shift;
    cursor += bytes_read;
    shift += 7u;
    if((byte & 0x80u) == 0)
    {
      break;
    }
  }
  if(value_out != 0)
  {
    *value_out = value;
  }
  u64 bytes_read = cursor - off;
  return bytes_read;
}

internal u64
str8_deserial_read_sleb128(String8 string, u64 off, s64 *value_out)
{
  u64 value  = 0;
  u64 shift  = 0;
  u64 cursor = off;
  for(;;)
  {
    u8 byte;
    u64 bytes_read = str8_deserial_read_struct(string, cursor, &byte);
    if(bytes_read != sizeof(byte))
    {
      break;
    }
    u8 val = byte & 0x7fu;
    value |= ((u64)val) << shift;
    cursor += bytes_read;
    shift += 7u;
    if((byte & 0x80u) == 0)
    {
      if(shift < sizeof(value) * 8 && (byte & 0x40u) != 0)
      {
        value |= -(s64)(1ull << shift);
      }
      break;
    }
  }
  if(value_out != 0)
  {
    *value_out = value;
  }
  u64 bytes_read = cursor - off;
  return bytes_read;
}

////////////////////////////////

force_inline int
str8_compar(String8 a, String8 b, bool32 ignore_case)
{
  u64 size = Min(a.size, b.size);
  int cmp = ignore_case ? MemCompareI(a.str, b.str, size) : MemCompare(a.str, b.str, size);

  // normalize compar result
  cmp = cmp > 0 ? 1 : cmp < 0 ? -1 : 0;

  // shorter prefix must precede longer prefixes
  if (cmp == 0)
  {
    cmp = a.size < b.size ? -1 :
          a.size > b.size ? +1 : 0;
  }
  
  return cmp;
}

force_inline int
str8_compar_ignore_case(const void *a, const void *b)
{
  return str8_compar(*(String8*)a, *(String8*)b, 1);
}

force_inline int
str8_compar_case_sensitive(const void *a, const void *b)
{
  return str8_compar(*(String8*)a, *(String8*)b, 0);
}

force_inline int
str8_is_before_case_sensitive(const void *a, const void *b)
{
  int cmp = str8_compar_case_sensitive(a, b);
  return cmp < 0;
}

////////////////////////////////
// string buffer

internal bool32
str8_buffer_skip(String8_Node *buf, u64 *pos, u64 skip)
{
  s64 to_skip;
  for (to_skip = skip; to_skip > 0;) {
    if (buf == 0) { break; }
    
    u64 left = Min(to_skip, buf->string.size -  *pos);
    *pos += left;
    
    if (*pos == buf->string.size) {
      if (buf->next) { *buf = *buf->next;                }
      else           { *buf = (String8_Node){0}; buf = 0; }
      *pos = 0;
    }
    
    to_skip -= (s64)left;
  }
  Assert(to_skip == 0);
  return (to_skip == 0);
}

internal u64
str8_buffer_read(String8_Node *buf, u64 *pos, u64 read_size, void *out)
{
  u64 cursor = 0;
  for (; cursor < read_size ;) {
    if (buf == 0) { break; }
    
    u64   copy_size = Min(read_size - cursor, (buf->string.size - *pos));
    void *dst       = (u8 *)out + cursor;
    void *src       = buf->string.str + *pos;
    MemoryCopy(dst, src, copy_size);
    
    *pos   += copy_size;
    cursor += copy_size;
    
    if (*pos >= buf->string.size) {
      if (buf->next) { *buf = *buf->next;                }
      else           { *buf = (String8_Node){0}; buf = 0; }
      *pos = 0;
    }
  }
  return cursor;
}

internal u64
str8_buffer_peek(String8_Node *buf, u64 *pos, u64 read_size, void *out)
{
  String8_Node buf_copy = *buf;
  u64         pos_copy = *pos;
  return str8_buffer_read(&buf_copy, &pos_copy, read_size, out);
}

internal u64
str8_buffer_write(String8_Node *buf, u64 *pos, String8 data)
{
  u64 copy_size = 0;
  if (buf) {
    for (copy_size = 0; copy_size < data.size; ) {
      u64 data_size = data.size - copy_size;
      
      u64 available_size = buf->string.size - *pos;
      u64 to_copy        = Min(available_size, data_size);
      if (data.str == 0) {
        MemorySet(buf->string.str + *pos, 0, to_copy);
      } else {
        u8 *data_ptr = data.str + copy_size;
        MemoryCopy(buf->string.str + *pos, data_ptr, to_copy);
      }
      *pos      += to_copy;
      copy_size += to_copy;
      
      if (*pos >= buf->string.size) {
        if (buf->next) {
          *buf = *buf->next;
          *pos = 0;
        } else {
          break;
        }
      }
    }
    Assert(copy_size == data.size);
  } else {
    copy_size = data.size;
  }
  return copy_size;
}

internal u64
str8_buffer_write_u16(String8_Node *buf, u64 *pos, u16 v)
{
  return str8_buffer_write(buf, pos, str8_struct(&v));
}

internal u64
str8_buffer_write_u32(String8_Node *buf, u64 *pos, u32 v)
{
  return str8_buffer_write(buf, pos, str8_struct(&v));
}

internal u64
str8_buffer_write_zeroes(String8_Node *buf, u64 *pos, u64 size)
{
  return str8_buffer_write(buf, pos, str8(0, size));
}

internal u64
str8_buffer_write_string_list(String8_Node *buf, u64 *pos, String8_List list)
{
  u64 copy_size = 0;
  for EachNode(n, String8_Node, list.first) { copy_size += str8_buffer_write(buf, pos, n->string); }
  return copy_size;
}

////////////////////////////////
//~ rjf: Basic String Hashes

#pragma push_macro("__cpuid")
#undef __cpuid
#if !defined(XXH_IMPLEMENTATION)
# define XXH_INLINE_ALL
# define XXH_IMPLEMENTATION
# define XXH_STATIC_LINKING_ONLY
# include "third_party/xxHash/xxhash.h"
#endif
#pragma pop_macro("__cpuid")

internal u64
u64_hash_from_seed_str8(u64 seed, String8 string)
{
  u64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

internal u64
u64_hash_from_str8(String8 string)
{
  u64 result = u64_hash_from_seed_str8(5381, string);
  return result;
}

internal u128
u128_hash_from_seed_str8(u64 seed, String8 string)
{
  u128 result = {0};
  XXH128_hash_t hash = XXH3_128bits_withSeed(string.str, string.size, seed);
  MemoryCopy(&result, &hash, sizeof(result));
  return result;
}

internal u128
u128_hash_from_str8(String8 string)
{
  u128 result = u128_hash_from_seed_str8(5381, string);
  return result;
}
