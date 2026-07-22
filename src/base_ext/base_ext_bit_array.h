
#ifndef BASE_EXT_BIT_ARRAY_H
#define BASE_EXT_BIT_ARRAY_H

internal u32_Array bit_array_init32              (Arena *arena, u64 word_count);
internal u64       bit_array_scan_left_to_right32(u32_Array bit_array, u64 lo, u64 hi, bool32 state);
internal u64       bit_array_scan_right_to_left32(u32_Array bit_array, u64 lo, u64 hi, bool32 state);
internal void      bit_array_set_bit32           (u32_Array bit_array, u64 idx, bool32 state);
internal u32       bit_array_get_bit32           (u32_Array bit_array, u64 idx);


#endif // BASE_EXT_BIT_ARRAY_H
