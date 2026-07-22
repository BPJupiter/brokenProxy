#ifndef BASE_MATH_H
#define BASE_MATH_H

typedef union Vec2f32 Vec2f32;
union Vec2f32 {
    struct {
        f32 x;
        f32 y;
    };
    f32 v[2];
};

typedef union Vec2s64 Vec2s64;
union Vec2s64 {
    struct {
        s64 x;
        s64 y;
    };
    s64 v[2];
};

typedef union Vec2s32 Vec2s32;
union Vec2s32 {
    struct {
        s32 x;
        s32 y;
    };
    s32 v[2];
};

typedef union Vec2s16 Vec2s16;
union Vec2s16 {
    struct {
        s16 x;
        s16 y;
    };
    s16 v[2];
};

// 3 vectors

typedef union Vec3f32 Vec3f32;
union Vec3f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        Vec2f32 xy;
        f32 _z0;
    };
    struct {
        f32 _x0;
        Vec2f32 yz;
    };
    f32 v[3];
};

typedef union Vec3s32 Vec3s32;
union Vec3s32 {
    struct {
        s32 x;
        s32 y;
        s32 z;
    };
    struct {
        Vec2s32 xy;
        s32 _z0;
    };
    struct {
        s32 _x0;
        Vec2s32 yz;
    };
    s32 v[3];
};

// 4 vectors

typedef union Vec4f32 Vec4f32;
union Vec4f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
        Vec2f32 xy;
        Vec2f32 zw;
    };
    struct {
        Vec3f32 xyz;
        f32 _z0;
    };
    struct {
        f32 _x0;
        Vec3f32 yzw;
    };
    f32 v[4];
};

typedef union Vec4s32 Vec4s32;
union Vec4s32 {
    struct {
        s32 x;
        s32 y;
        s32 z;
        s32 w;
    };
    struct {
        Vec2s32 xy;
        Vec2s32 zw;
    };
    struct {
        Vec3s32 xyz;
        s32 _z0;
    };
    struct {
        s32 _x0;
        Vec3s32 yzw;
    };
    s32 v[4];
};

////////////////////////////////
//~ rjf: Matrix Types

typedef struct Mat3x3f32 Mat3x3f32;
struct Mat3x3f32
{
  f32 v[3][3];
};

typedef struct Mat4x4f32 Mat4x4f32;
struct Mat4x4f32
{
  f32 v[4][4];
};

////////////////////////////////
//~ rjf: Range Types

//- rjf: 1-range

typedef union Rng1u8 Rng1u8;
union Rng1u8
{
  struct
  {
    u8 min;
    u8 max;
  };
  u8 v[2];
};

typedef union Rng1u16 Rng1u16;
union Rng1u16
{
  struct
  {
    u16 min;
    u16 max;
  };
  u16 v[2];
};

typedef union Rng1u32 Rng1u32;
union Rng1u32
{
  struct
  {
    u32 min;
    u32 max;
  };
  u32 v[2];
};

typedef union Rng1s32 Rng1s32;
union Rng1s32
{
  struct
  {
    s32 min;
    s32 max;
  };
  s32 v[2];
};

typedef union Rng1u64 Rng1u64;
union Rng1u64
{
  struct
  {
    u64 min;
    u64 max;
  };
  u64 v[2];
};

typedef union Rng1s64 Rng1s64;
union Rng1s64
{
  struct
  {
    s64 min;
    s64 max;
  };
  s64 v[2];
};

typedef union Rng1f32 Rng1f32;
union Rng1f32
{
  struct
  {
    f32 min;
    f32 max;
  };
  f32 v[2];
};

//- rjf: 2-range (rectangles)

typedef union Rng2s16 Rng2s16;
union Rng2s16
{
  struct
  {
    Vec2s16 min;
    Vec2s16 max;
  };
  struct
  {
    Vec2s16 p0;
    Vec2s16 p1;
  };
  struct
  {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
  };
  Vec2s16 v[2];
};

typedef union Rng2s32 Rng2s32;
union Rng2s32
{
  struct
  {
    Vec2s32 min;
    Vec2s32 max;
  };
  struct
  {
    Vec2s32 p0;
    Vec2s32 p1;
  };
  struct
  {
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
  };
  Vec2s32 v[2];
};

typedef union Rng2f32 Rng2f32;
union Rng2f32
{
  struct
  {
    Vec2f32 min;
    Vec2f32 max;
  };
  struct
  {
    Vec2f32 p0;
    Vec2f32 p1;
  };
  struct
  {
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
  };
  Vec2f32 v[2];
};

typedef union Rng2s64 Rng2s64;
union Rng2s64
{
  struct
  {
    Vec2s64 min;
    Vec2s64 max;
  };
  struct
  {
    Vec2s64 p0;
    Vec2s64 p1;
  };
  struct
  {
    s64 x0;
    s64 y0;
    s64 x1;
    s64 y1;
  };
  Vec2s64 v[2];
};

////////////////////////////////
//~ rjf: Range List Types

typedef struct Rng1u64Node Rng1u64Node;
struct Rng1u64Node
{
  Rng1u64Node *next;
  Rng1u64 v;
};

typedef struct Rng1u64List Rng1u64List;
struct Rng1u64List
{
  Rng1u64Node *first;
  Rng1u64Node *last;
  u64 count;
};

typedef struct Rng1u64Array Rng1u64Array;
struct Rng1u64Array
{
  Rng1u64 *v;
  u64 count;
};

typedef struct Rng1s64Node Rng1s64Node;
struct Rng1s64Node
{
  Rng1s64Node *next;
  Rng1s64 v;
};

typedef struct Rng1s64List Rng1s64List;
struct Rng1s64List
{
  Rng1s64Node *first;
  Rng1s64Node *last;
  u64 count;
};

typedef struct Rng1s64Array Rng1s64Array;
struct Rng1s64Array
{
  Rng1s64 *v;
  u64 count;
};

////////////////////////////////
//~ rjf: Scalar Math Ops

#define abs_s64(v) (s64)llabs(v)

#define sqrt_f32(v)   sqrtf(v)
#define cbrt_f32(v)   cbrtf(v)
#define mod_f32(a, b) fmodf((a), (b))
#define pow_f32(b, e) powf((b), (e))
#define ceil_f32(v)   ceilf(v)
#define floor_f32(v)  floorf(v)
#define round_f32(v)  roundf(v)
#define abs_f32(v)    fabsf(v)
#define radians_from_turns_f32(v) ((v)*(2*3.1415926535897f))
#define turns_from_radians_f32(v) ((v)/(2*3.1415926535897f))
#define degrees_from_turns_f32(v) ((v)*360.f)
#define turns_from_degrees_f32(v) ((v)/360.f)
#define degrees_from_radians_f32(v) (degrees_from_turns_f32(turns_from_radians_f32(v)))
#define radians_from_degrees_f32(v) (radians_from_turns_f32(turns_from_degrees_f32(v)))
#define sin_f32(v)    sinf(radians_from_turns_f32(v))
#define cos_f32(v)    cosf(radians_from_turns_f32(v))
#define tan_f32(v)    tanf(radians_from_turns_f32(v))

#define sqrt_f64(v)   sqrt(v)
#define cbrt_f64(v)   cbrt(v)
#define mod_f64(a, b) fmod((a), (b))
#define pow_f64(b, e) pow((b), (e))
#define ceil_f64(v)   ceil(v)
#define floor_f64(v)  floor(v)
#define round_f64(v)  round(v)
#define abs_f64(v)    fabs(v)
#define radians_from_turns_f64(v) ((v)*(2*3.1415926535897))
#define turns_from_radians_f64(v) ((v)/(2*3.1415926535897))
#define degrees_from_turns_f64(v) ((v)*360.0)
#define turns_from_degrees_f64(v) ((v)/360.0)
#define degrees_from_radians_f64(v) (degrees_from_turns_f64(turns_from_radians_f64(v)))
#define radians_from_degrees_f64(v) (radians_from_turns_f64(turns_from_degrees_f64(v)))
#define sin_f64(v)    sin(radians_from_turns_f64(v))
#define cos_f64(v)    cos(radians_from_turns_f64(v))
#define tan_f64(v)    tan(radians_from_turns_f64(v))

internal f32 mix_1f32(f32 a, f32 b, f32 t);
internal f64 mix_1f64(f64 a, f64 b, f64 t);

////////////////////////////////
//~ rjf: Vector Ops

#define v2f32(x, y) vec_2f32((x), (y))
internal Vec2f32 vec_2f32(f32 x, f32 y);
internal Vec2f32 add_2f32(Vec2f32 a, Vec2f32 b);
internal Vec2f32 sub_2f32(Vec2f32 a, Vec2f32 b);
internal Vec2f32 mul_2f32(Vec2f32 a, Vec2f32 b);
internal Vec2f32 div_2f32(Vec2f32 a, Vec2f32 b);
internal Vec2f32 scale_2f32(Vec2f32 v, f32 s);
internal f32 dot_2f32(Vec2f32 a, Vec2f32 b);
internal f32 length_squared_2f32(Vec2f32 v);
internal f32 length_2f32(Vec2f32 v);
internal Vec2f32 normalize_2f32(Vec2f32 v);
internal Vec2f32 mix_2f32(Vec2f32 a, Vec2f32 b, f32 t);

#define v2s64(x, y) vec_2s64((x), (y))
internal Vec2s64 vec_2s64(s64 x, s64 y);
internal Vec2s64 add_2s64(Vec2s64 a, Vec2s64 b);
internal Vec2s64 sub_2s64(Vec2s64 a, Vec2s64 b);
internal Vec2s64 mul_2s64(Vec2s64 a, Vec2s64 b);
internal Vec2s64 div_2s64(Vec2s64 a, Vec2s64 b);
internal Vec2s64 scale_2s64(Vec2s64 v, s64 s);
internal s64 dot_2s64(Vec2s64 a, Vec2s64 b);
internal s64 length_squared_2s64(Vec2s64 v);
internal s64 length_2s64(Vec2s64 v);
internal Vec2s64 mix_2s64(Vec2s64 a, Vec2s64 b, f32 t);

#define v2s32(x, y) vec_2s32((x), (y))
internal Vec2s32 vec_2s32(s32 x, s32 y);
internal Vec2s32 add_2s32(Vec2s32 a, Vec2s32 b);
internal Vec2s32 sub_2s32(Vec2s32 a, Vec2s32 b);
internal Vec2s32 mul_2s32(Vec2s32 a, Vec2s32 b);
internal Vec2s32 div_2s32(Vec2s32 a, Vec2s32 b);
internal Vec2s32 scale_2s32(Vec2s32 v, s32 s);
internal s32 dot_2s32(Vec2s32 a, Vec2s32 b);
internal s32 length_squared_2s32(Vec2s32 v);
internal s32 length_2s32(Vec2s32 v);
internal Vec2s32 mix_2s32(Vec2s32 a, Vec2s32 b, f32 t);

#define v2s16(x, y) vec_2s16((x), (y))
internal Vec2s16 vec_2s16(s16 x, s16 y);
internal Vec2s16 add_2s16(Vec2s16 a, Vec2s16 b);
internal Vec2s16 sub_2s16(Vec2s16 a, Vec2s16 b);
internal Vec2s16 mul_2s16(Vec2s16 a, Vec2s16 b);
internal Vec2s16 div_2s16(Vec2s16 a, Vec2s16 b);
internal Vec2s16 scale_2s16(Vec2s16 v, s16 s);
internal s16 dot_2s16(Vec2s16 a, Vec2s16 b);
internal s16 length_squared_2s16(Vec2s16 v);
internal s16 length_2s16(Vec2s16 v);
internal Vec2s16 mix_2s16(Vec2s16 a, Vec2s16 b, f32 t);

#define v3f32(x, y, z) vec_3f32((x), (y), (z))
internal Vec3f32 vec_3f32(f32 x, f32 y, f32 z);
internal Vec3f32 add_3f32(Vec3f32 a, Vec3f32 b);
internal Vec3f32 sub_3f32(Vec3f32 a, Vec3f32 b);
internal Vec3f32 mul_3f32(Vec3f32 a, Vec3f32 b);
internal Vec3f32 div_3f32(Vec3f32 a, Vec3f32 b);
internal Vec3f32 scale_3f32(Vec3f32 v, f32 s);
internal f32 dot_3f32(Vec3f32 a, Vec3f32 b);
internal f32 length_squared_3f32(Vec3f32 v);
internal f32 length_3f32(Vec3f32 v);
internal Vec3f32 normalize_3f32(Vec3f32 v);
internal Vec3f32 mix_3f32(Vec3f32 a, Vec3f32 b, f32 t);
internal Vec3f32 cross_3f32(Vec3f32 a, Vec3f32 b);
internal Vec3f32 xform_3f32(Vec3f32 v, Mat3x3f32 m);

#define v3s32(x, y, z) vec_3s32((x), (y), (z))
internal Vec3s32 vec_3s32(s32 x, s32 y, s32 z);
internal Vec3s32 add_3s32(Vec3s32 a, Vec3s32 b);
internal Vec3s32 sub_3s32(Vec3s32 a, Vec3s32 b);
internal Vec3s32 mul_3s32(Vec3s32 a, Vec3s32 b);
internal Vec3s32 div_3s32(Vec3s32 a, Vec3s32 b);
internal Vec3s32 scale_3s32(Vec3s32 v, s32 s);
internal s32 dot_3s32(Vec3s32 a, Vec3s32 b);
internal s32 length_squared_3s32(Vec3s32 v);
internal s32 length_3s32(Vec3s32 v);
internal Vec3s32 mix_3s32(Vec3s32 a, Vec3s32 b, f32 t);
internal Vec3s32 cross_3s32(Vec3s32 a, Vec3s32 b);

#define v4f32(x, y, z, w) vec_4f32((x), (y), (z), (w))
internal Vec4f32 vec_4f32(f32 x, f32 y, f32 z, f32 w);
internal Vec4f32 add_4f32(Vec4f32 a, Vec4f32 b);
internal Vec4f32 sub_4f32(Vec4f32 a, Vec4f32 b);
internal Vec4f32 mul_4f32(Vec4f32 a, Vec4f32 b);
internal Vec4f32 div_4f32(Vec4f32 a, Vec4f32 b);
internal Vec4f32 scale_4f32(Vec4f32 v, f32 s);
internal f32 dot_4f32(Vec4f32 a, Vec4f32 b);
internal f32 length_squared_4f32(Vec4f32 v);
internal f32 length_4f32(Vec4f32 v);
internal Vec4f32 normalize_4f32(Vec4f32 v);
internal Vec4f32 mix_4f32(Vec4f32 a, Vec4f32 b, f32 t);

#define v4s32(x, y, z, w) vec_4s32((x), (y), (z), (w))
internal Vec4s32 vec_4s32(s32 x, s32 y, s32 z, s32 w);
internal Vec4s32 add_4s32(Vec4s32 a, Vec4s32 b);
internal Vec4s32 sub_4s32(Vec4s32 a, Vec4s32 b);
internal Vec4s32 mul_4s32(Vec4s32 a, Vec4s32 b);
internal Vec4s32 div_4s32(Vec4s32 a, Vec4s32 b);
internal Vec4s32 scale_4s32(Vec4s32 v, s32 s);
internal s32 dot_4s32(Vec4s32 a, Vec4s32 b);
internal s32 length_squared_4s32(Vec4s32 v);
internal s32 length_4s32(Vec4s32 v);
internal Vec4s32 mix_4s32(Vec4s32 a, Vec4s32 b, f32 t);

////////////////////////////////
//~ rjf: Matrix Ops

internal Mat3x3f32 mat_3x3f32(f32 diagonal);
internal Mat3x3f32 make_translate_3x3f32(Vec2f32 delta);
internal Mat3x3f32 make_scale_3x3f32(Vec2f32 scale);
internal Mat3x3f32 mul_3x3f32(Mat3x3f32 a, Mat3x3f32 b);

internal Mat4x4f32 mat_4x4f32(f32 diagonal);
internal Mat4x4f32 make_translate_4x4f32(Vec3f32 delta);
internal Mat4x4f32 make_scale_4x4f32(Vec3f32 scale);
internal Mat4x4f32 make_perspective_4x4f32(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
internal Mat4x4f32 make_orthographic_4x4f32(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z);
internal Mat4x4f32 make_look_at_4x4f32(Vec3f32 eye, Vec3f32 center, Vec3f32 up);
internal Mat4x4f32 make_rotate_4x4f32(Vec3f32 axis, f32 turns);
internal Mat4x4f32 mul_4x4f32(Mat4x4f32 a, Mat4x4f32 b);
internal Mat4x4f32 scale_4x4f32(Mat4x4f32 m, f32 scale);
internal Mat4x4f32 inverse_4x4f32(Mat4x4f32 m);
internal Mat4x4f32 derotate_4x4f32(Mat4x4f32 mat);
internal Mat4x4f32 transpose_4x4f32(Mat4x4f32 mat);

////////////////////////////////
//~ rjf: Range Ops

#define r1u8(min, max) rng_1u8((min), (max))
#define r1u8s(min, size) rng_1u8((min), (min)+(size))
internal Rng1u8 rng_1u8(u8 min, u8 max);
internal Rng1u8 shift_1u8(Rng1u8 r, u8 x);
internal Rng1u8 pad_1u8(Rng1u8 r, u8 x);
internal u8 center_1u8(Rng1u8 r);
internal bool32 contains_1u8(Rng1u8 r, u8 x);
internal u8 dim_1u8(Rng1u8 r);
internal Rng1u8 union_1u8(Rng1u8 a, Rng1u8 b);
internal Rng1u8 intersect_1u8(Rng1u8 a, Rng1u8 b);
internal u8 clamp_1u8(Rng1u8 r, u8 v);

#define r1u16(min, max) rng_1u16((min), (max))
#define r1u16s(min, size) rng_1u16((min), (min)+(size))
internal Rng1u16 rng_1u16(u16 min, u16 max);
internal Rng1u16 shift_1u16(Rng1u16 r, u16 x);
internal Rng1u16 pad_1u16(Rng1u16 r, u16 x);
internal u16 center_1u16(Rng1u16 r);
internal bool32 contains_1u16(Rng1u16 r, u16 x);
internal u16 dim_1u16(Rng1u16 r);
internal Rng1u16 union_1u16(Rng1u16 a, Rng1u16 b);
internal Rng1u16 intersect_1u16(Rng1u16 a, Rng1u16 b);
internal u16 clamp_1u16(Rng1u16 r, u16 v);

#define r1u32(min, max) rng_1u32((min), (max))
#define r1u32s(min, size) rng_1u32((min), (min)+(size))
internal Rng1u32 rng_1u32(u32 min, u32 max);
internal Rng1u32 shift_1u32(Rng1u32 r, u32 x);
internal Rng1u32 pad_1u32(Rng1u32 r, u32 x);
internal u32 center_1u32(Rng1u32 r);
internal bool32 contains_1u32(Rng1u32 r, u32 x);
internal u32 dim_1u32(Rng1u32 r);
internal Rng1u32 union_1u32(Rng1u32 a, Rng1u32 b);
internal Rng1u32 intersect_1u32(Rng1u32 a, Rng1u32 b);
internal u32 clamp_1u32(Rng1u32 r, u32 v);

#define r1s32(min, max) rng_1s32((min), (max))
#define r1s32s(min,size) rng_1s32((min), (min)+(size))
internal Rng1s32 rng_1s32(s32 min, s32 max);
internal Rng1s32 shift_1s32(Rng1s32 r, s32 x);
internal Rng1s32 pad_1s32(Rng1s32 r, s32 x);
internal s32 center_1s32(Rng1s32 r);
internal bool32 contains_1s32(Rng1s32 r, s32 x);
internal s32 dim_1s32(Rng1s32 r);
internal Rng1s32 union_1s32(Rng1s32 a, Rng1s32 b);
internal Rng1s32 intersect_1s32(Rng1s32 a, Rng1s32 b);
internal s32 clamp_1s32(Rng1s32 r, s32 v);

#define r1u64(min, max) rng_1u64((min), (max))
#define r1u64s(min, size) rng_1u64((min), (min)+(size))
internal Rng1u64 rng_1u64(u64 min, u64 max);
internal Rng1u64 shift_1u64(Rng1u64 r, u64 x);
internal Rng1u64 pad_1u64(Rng1u64 r, u64 x);
internal u64 center_1u64(Rng1u64 r);
internal bool32 contains_1u64(Rng1u64 r, u64 x);
internal u64 dim_1u64(Rng1u64 r);
internal Rng1u64 union_1u64(Rng1u64 a, Rng1u64 b);
internal Rng1u64 intersect_1u64(Rng1u64 a, Rng1u64 b);
internal u64 clamp_1u64(Rng1u64 r, u64 v);

#define r1s64(min, max) rng_1s64((min), (max))
#define r1s64s(min, size) rng_1s64((min), (min)+(size))
internal Rng1s64 rng_1s64(s64 min, s64 max);
internal Rng1s64 shift_1s64(Rng1s64 r, s64 x);
internal Rng1s64 pad_1s64(Rng1s64 r, s64 x);
internal s64 center_1s64(Rng1s64 r);
internal bool32 contains_1s64(Rng1s64 r, s64 x);
internal s64 dim_1s64(Rng1s64 r);
internal Rng1s64 union_1s64(Rng1s64 a, Rng1s64 b);
internal Rng1s64 intersect_1s64(Rng1s64 a, Rng1s64 b);
internal s64 clamp_1s64(Rng1s64 r, s64 v);

#define r1f32(min, max) rng_1f32((min), (max))
#define r1f32s(min, size) rng_1f32((min), (min)+(size))
internal Rng1f32 rng_1f32(f32 min, f32 max);
internal Rng1f32 shift_1f32(Rng1f32 r, f32 x);
internal Rng1f32 pad_1f32(Rng1f32 r, f32 x);
internal f32 center_1f32(Rng1f32 r);
internal bool32 contains_1f32(Rng1f32 r, f32 x);
internal f32 dim_1f32(Rng1f32 r);
internal Rng1f32 union_1f32(Rng1f32 a, Rng1f32 b);
internal Rng1f32 intersect_1f32(Rng1f32 a, Rng1f32 b);
internal f32 clamp_1f32(Rng1f32 r, f32 v);

#define r2s16(min, max) rng_2s16((min), (max))
#define r2s16s(min, size) r2s16((min), (min)+(size))
#define r2s16p(x, y, z, w) r2s16(v2s16((x), (y)), v2s16((z), (w)))
internal Rng2s16 rng_2s16(Vec2s16 min, Vec2s16 max);
internal Rng2s16 shift_2s16(Rng2s16 r, Vec2s16 x);
internal Rng2s16 pad_2s16(Rng2s16 r, s16 x);
internal Vec2s16 center_2s16(Rng2s16 r);
internal bool32 contains_2s16(Rng2s16 r, Vec2s16 x);
internal Vec2s16 dim_2s16(Rng2s16 r);
internal Rng2s16 union_2s16(Rng2s16 a, Rng2s16 b);
internal Rng2s16 intersect_2s16(Rng2s16 a, Rng2s16 b);
internal Vec2s16 clamp_2s16(Rng2s16 r, Vec2s16 v);

#define r2s32(min, max) rng_2s32((min), (max))
#define r2s32s(min, size) r2s32((min), (min)+(size))
#define r2s32p(x, y, z, w) r2s32(v2s32((x), (y)), v2s32((z), (w)))
internal Rng2s32 rng_2s32(Vec2s32 min, Vec2s32 max);
internal Rng2s32 shift_2s32(Rng2s32 r, Vec2s32 x);
internal Rng2s32 pad_2s32(Rng2s32 r, s32 x);
internal Vec2s32 center_2s32(Rng2s32 r);
internal bool32 contains_2s32(Rng2s32 r, Vec2s32 x);
internal Vec2s32 dim_2s32(Rng2s32 r);
internal Rng2s32 union_2s32(Rng2s32 a, Rng2s32 b);
internal Rng2s32 intersect_2s32(Rng2s32 a, Rng2s32 b);
internal Vec2s32 clamp_2s32(Rng2s32 r, Vec2s32 v);

#define r2s64(min, max) rng_2s64((min), (max))
#define r2s64s(min, size) r2s64((min), (min)+(size))
#define r2s64p(x, y, z, w) r2s64(v2s64((x), (y)), v2s64((z), (w)))
internal Rng2s64 rng_2s64(Vec2s64 min, Vec2s64 max);
internal Rng2s64 shift_2s64(Rng2s64 r, Vec2s64 x);
internal Rng2s64 pad_2s64(Rng2s64 r, s64 x);
internal Vec2s64 center_2s64(Rng2s64 r);
internal bool32 contains_2s64(Rng2s64 r, Vec2s64 x);
internal Vec2s64 dim_2s64(Rng2s64 r);
internal Rng2s64 union_2s64(Rng2s64 a, Rng2s64 b);
internal Rng2s64 intersect_2s64(Rng2s64 a, Rng2s64 b);
internal Vec2s64 clamp_2s64(Rng2s64 r, Vec2s64 v);

#define r2f32(min, max) rng_2f32((min), (max))
#define r2f32s(min, size) r2f32((min), (min)+(size))
#define r2f32p(x, y, z, w) r2f32(v2f32((x), (y)), v2f32((z), (w)))
internal Rng2f32 rng_2f32(Vec2f32 min, Vec2f32 max);
internal Rng2f32 shift_2f32(Rng2f32 r, Vec2f32 x);
internal Rng2f32 pad_2f32(Rng2f32 r, f32 x);
internal Vec2f32 center_2f32(Rng2f32 r);
internal bool32 contains_2f32(Rng2f32 r, Vec2f32 x);
internal Vec2f32 dim_2f32(Rng2f32 r);
internal Rng2f32 union_2f32(Rng2f32 a, Rng2f32 b);
internal Rng2f32 intersect_2f32(Rng2f32 a, Rng2f32 b);
internal Vec2f32 clamp_2f32(Rng2f32 r, Vec2f32 v);

////////////////////////////////
//~ rjf: Color Operations

//- rjf: hsv <-> rgb
internal Vec3f32 hsv_from_rgb(Vec3f32 rgb);
internal Vec3f32 rgb_from_hsv(Vec3f32 hsv);
internal Vec4f32 hsva_from_rgba(Vec4f32 rgba);
internal Vec4f32 rgba_from_hsva(Vec4f32 hsva);

//- rjf: srgb <-> linear
internal Vec3f32 linear_from_srgb(Vec3f32 srgb);
internal Vec3f32 srgb_from_linear(Vec3f32 linear);
internal Vec4f32 linear_from_srgba(Vec4f32 srgba);
internal Vec4f32 srgba_from_linear(Vec4f32 linear);

//- rjf: oklab <-> linear
internal Vec3f32 oklab_from_linear(Vec3f32 linear);
internal Vec3f32 linear_from_oklab(Vec3f32 oklab);
internal Vec4f32 oklab_from_lineara(Vec4f32 lineara);
internal Vec4f32 lineara_from_oklab(Vec4f32 oklab);

//- rjf: rgba <-> u32
internal u32 u32_from_rgba(Vec4f32 rgba);
internal Vec4f32 rgba_from_u32(u32 hex);
#define rgba_from_u32_lit_comp(h) { (((h)&0xff000000)>>24)/255.f, (((h)&0x00ff0000)>>16)/255.f, (((h)&0x0000ff00)>> 8)/255.f, (((h)&0x000000ff)>> 0)/255.f }

////////////////////////////////
//~ rjf: List Type Functions

internal void rng1u64_list_push_node(Rng1u64List *list, Rng1u64Node *n);
internal Rng1u64Node * rng1u64_list_push(Arena *arena, Rng1u64List *list, Rng1u64 rng);
internal void rng1u64_list_concat(Rng1u64List *list, Rng1u64List *to_concat);
internal Rng1u64Array rng1u64_array_from_list(Arena *arena, Rng1u64List *list);
internal u64 rng1u64_array_num_from_value__binary_search(Rng1u64Array *array, u64 value);

internal void rng1s64_list_push(Arena *arena, Rng1s64List *list, Rng1s64 rng);
internal Rng1s64Array rng1s64_array_from_list(Arena *arena, Rng1s64List *list);

////////////////////////////////
//~ rjf: N -> M Element Subdivision

internal Rng1u64 m_range_from_n_idx_m_count(u64 n_idx, u64 n_count, u64 m_count);

#endif // BASE_MATH_H
