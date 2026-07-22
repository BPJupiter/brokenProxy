
#ifndef BASE_HASH_H
#define BASE_HASH_H

////////////////////////
// Hash Result Types

typedef union MD5 MD5;
union MD5 {
    u8   u8[16];
    u16  u16[8];
    u32  u32[4];
    u64  u64[2];
    u128 u128;
};

typedef union SHA1 SHA1;
union SHA1 {
    u8 u8[20];
};

typedef union SHA256 SHA256;
union SHA256 {
    u8   u8[32];
    u16  u16[16];
    u32  u32[8];
    u64  u64[4];
    u128 u128[2];
    u256 u256;
};

//////////
// MD5

internal MD5 md5_from_data(String8 data);

/////////
// SHA

internal SHA1 sha1_from_data(String8 data);
internal SHA256 sha256_from_data(String8 data);

#endif // BASE_HASH_H
