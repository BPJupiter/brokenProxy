
#include "third_party/martins_bitscan/bitscan.h"

internal u32_Array bit_array_init32(Arena *arena, u64 bit_count)
{
    u64 count = CeilIntegerDiv(bit_count, 32);
    return (u32_Array){ .count = count, .v = push_array(arena, u32, count) };
}

internal u64 bit_array_scan_left_to_right32(u32_Array bit_array, u64 lo, u64 hi, bool32 state)
{
    return bitscan_lsb_index32(bit_array.v, lo, hi, state);
}

internal u64 bit_array_scan_right_to_left32(u32_Array bit_array, u64 lo, u64 hi, bool32 state)
{
    return bitscan_msb_index32(bit_array.v, lo, hi, state);
}

internal void bit_array_set_bit32(u32_Array bit_array, u64 idx, bool32 state)
{
    Assert(idx < bit_array.count * 32);
    u64 word_idx = idx / 32;
    u64 bit_idx = idx % 32;
    if (state) {
        bit_array.v[word_idx] |= (1u << bit_idx);
    }
    else {
        bit_array.v[word_idx] &= ~(1u << bit_idx);
    }
}

internal u32 bit_array_get_bit32(u32_Array bit_array, u64 idx)
{
    Assert(idx < bit_array.count * 32);
    u64 word_idx = idx / 32;
    u64 bit_idx = idx % 32;
    u32 bit = (bit_array.v[word_idx] & (1 << bit_idx)) >> bit_idx;
    return bit;
}

internal bool32 bit_array_is_bit_set(u32_Array bit_arr, u64 bit_pos)
{
    u64 word_idx = bit_pos / 32;
    Assert(word_idx < bit_arr.count);
    u32 word = bit_arr.v[word_idx];
    u64 bit_idx = bit_pos % 32;
    bool32 is_set = !!(word & (1u << bit_idx));
    return is_set;
}
