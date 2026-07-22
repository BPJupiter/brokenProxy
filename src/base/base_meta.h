// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_META_H
#define BASE_META_H

////////////////////////////////
//~ rjf: Meta Markup Features

#define EmbedFile(name, path)
#define TweakB32(name, default)           (TWEAK_##name)
#define TweakF32(name, default, min, max) (TWEAK_##name)

////////////////////////////////
//~ rjf: Tweak Info Tables

typedef struct Tweak_bool32_Info Tweak_bool32_Info;
struct Tweak_bool32_Info {
    String8 name;
    bool32 default_value;
    bool32 *value_ptr;
};

typedef struct Tweak_f32_Info Tweak_f32_Info;
struct Tweak_f32_Info {
    String8 name;
    f32 default_value;
    Rng1f32 value_range;
    f32 *value_ptr;
};

typedef struct Tweak_bool32_Info_Table Tweak_bool32_Info_Table;
struct Tweak_bool32_Info_Table {
    Tweak_bool32_Info *v;
    u64 count;
};

typedef struct Tweak_f32_Info_Table Tweak_f32_Info_Table;
struct Tweak_f32_Info_Table {
    Tweak_f32_Info *v;
    u64 count;
};

typedef struct Embed_Info Embed_Info;
struct Embed_Info {
    String8 name;
    String8 *data;
    u128 *hash;
};

typedef struct Embed_Info_Table Embed_Info_Table;
struct Embed_Info_Table {
    Embed_Info *v;
    u64 count;
};

////////////////////////////////
//~ rjf: Type Info Types

typedef enum TypeKind {
    TypeKind_Null,
    
    // rjf: leaves
    TypeKind_Void, TypeKind_FirstLeaf = TypeKind_Void,
    TypeKind_u8,
    TypeKind_u16,
    TypeKind_u32,
    TypeKind_u64,
    TypeKind_s8,
    TypeKind_s16,
    TypeKind_s32,
    TypeKind_s64,
    TypeKind_bool8,
    TypeKind_bool16,
    TypeKind_bool32,
    TypeKind_bool64,
    TypeKind_f32,
    TypeKind_f64, TypeKind_LastLeaf = TypeKind_f64,
    
    // rjf: operators
    TypeKind_Ptr,
    TypeKind_Array,
    
    // rjf: user-defined-types
    TypeKind_Struct,
    TypeKind_Union,
    TypeKind_Enum,
    
    TypeKind_COUNT
} TypeKind;

typedef u32 TypeFlags;
enum {
    TypeFlag_IsPlainText = (1<<0),
    TypeFlag_IsCodeText  = (1<<1),
    TypeFlag_IsPathText  = (1<<2),
};

typedef u32 MemberFlags;
enum {
    MemberFlag_DoNotSerialize  = (1<<0),
};

typedef struct Type Type;
typedef struct Member Member;
struct Member {
    String8 name;
    String8 pretty_name;
    Type *type;
    u64 value;
    MemberFlags flags;
};

typedef struct Type Type;
struct Type {
    TypeKind kind;
    TypeFlags flags;
    u64 size;
    Type *direct;
    String8 name;
    String8 count_delimiter_name; // gathered from surrounding members, turns *->[1] into *->[N]
    u64 count;
    Member *members;
};

////////////////////////////////
//~ rjf: Type Serialization Parameters

typedef struct Type_Serialize_Ptr_Ref_Info Type_Serialize_Ptr_Ref_Info;
struct Type_Serialize_Ptr_Ref_Info {
    Type *type;           // pointers to this
    void *indexify_base;  // can be indexified using this
    void *offsetify_base; // can be offsetified using this
    void *nil_ptr;        // is terminal if matching 0 or this
};

typedef struct Type_Serialize_Params Type_Serialize_Params;
struct Type_Serialize_Params {
    u64 *advance_out;
    Type_Serialize_Ptr_Ref_Info *ptr_ref_infos;
    u64 ptr_ref_infos_count;
};

////////////////////////////////
//~ rjf: Type Name -> Type Info

#define type(T) (&T##__type)

////////////////////////////////
//~ rjf: Type Info Table Initializer Helpers

#define member_lit_comp(S, ti, m, ...) {str8_lit_comp(#m), {0}, (ti), OffsetOf(S, m), __VA_ARGS__}
#define struct_members(S) read_only global Member S##__members[] =
#define struct_type(S, ...) read_only global Type S##__type = {TypeKind_Struct, 0, sizeof(S), &type_nil, str8_lit_comp(#S), {0}, ArrayCount(S##__members), S##__members, __VA_ARGS__}
#define named_struct_type(name, S, ...) read_only global Type name##__type = {TypeKind_Struct, 0, sizeof(S), &type_nil, str8_lit_comp(#name), {0}, ArrayCount(name##__members), name##__members, __VA_ARGS__}
#define ptr_type(name, ti, ...) read_only global Type name = {TypeKind_Ptr, 0, sizeof(void *), (ti), __VA_ARGS__}

////////////////////////////////
//~ rjf: Globals

read_only global Type type_nil   = {TypeKind_Null, 0, 0, &type_nil};
read_only global Member member_nil = {{0}, {0}, &type_nil};

////////////////////////////////
//~ rjf: Built-In Types

//- rjf: leaves
read_only global Type void__type    = {TypeKind_Void, 0, 0,           &type_nil, str8_lit_comp("void")};
read_only global Type u8__type      = {TypeKind_u8,   0, sizeof(u8),  &type_nil, str8_lit_comp("u8")};
read_only global Type u16__type     = {TypeKind_u16,  0, sizeof(u16), &type_nil, str8_lit_comp("u16")};
read_only global Type u32__type     = {TypeKind_u32,  0, sizeof(u32), &type_nil, str8_lit_comp("u32")};
read_only global Type u64__type     = {TypeKind_u64,  0, sizeof(u64), &type_nil, str8_lit_comp("u64")};
read_only global Type s8__type      = {TypeKind_s8,   0, sizeof(s8),  &type_nil, str8_lit_comp("s8")};
read_only global Type s16__type     = {TypeKind_s16,  0, sizeof(s16), &type_nil, str8_lit_comp("s16")};
read_only global Type s32__type     = {TypeKind_s32,  0, sizeof(s32), &type_nil, str8_lit_comp("s32")};
read_only global Type s64__type     = {TypeKind_s64,  0, sizeof(s64), &type_nil, str8_lit_comp("s64")};
read_only global Type bool8__type   = {TypeKind_bool8,   0, sizeof(bool8),  &type_nil, str8_lit_comp("bool8")};
read_only global Type bool16__type  = {TypeKind_bool16,  0, sizeof(bool16), &type_nil, str8_lit_comp("bool16")};
read_only global Type bool32__type  = {TypeKind_bool32,  0, sizeof(bool32), &type_nil, str8_lit_comp("bool32")};
read_only global Type bool64__type  = {TypeKind_bool64,  0, sizeof(bool64), &type_nil, str8_lit_comp("bool64")};
read_only global Type f32__type     = {TypeKind_f32,  0, sizeof(f32), &type_nil, str8_lit_comp("f32")};
read_only global Type f64__type     = {TypeKind_f64,  0, sizeof(f64), &type_nil, str8_lit_comp("f64")};

read_only global Type *type_kind_type_table[] = {
    &type_nil,
    type(void),
    type(u8),
    type(u16),
    type(u32),
    type(u64),
    type(s8),
    type(s16),
    type(s32),
    type(s64),
    type(bool8),
    type(bool16),
    type(bool32),
    type(bool64),
    type(f32),
    type(f64),
    &type_nil,
    &type_nil,
    &type_nil,
    &type_nil,
    &type_nil,
};

//- rjf: Rng1u64
struct_members(Rng1u64)
{
    member_lit_comp(Rng1u64, type(u64), min),
    member_lit_comp(Rng1u64, type(u64), max),
};
struct_type(Rng1u64);

//- rjf: String8
ptr_type(String8__str_ptr_type, type(u8), str8_lit_comp("size"));
struct_members(String8)
{
    member_lit_comp(String8, &String8__str_ptr_type, str),
    member_lit_comp(String8, type(u64),              size),
};
struct_type(String8);

//- rjf: String8_Node
extern Type String8_Node__type;
Type String8_Node__ptr_type = {TypeKind_Ptr, 0, sizeof(void *), &String8_Node__type};
Member String8_Node__members[] =
{
    {str8_lit_comp("next"),   {0}, &String8_Node__ptr_type,    OffsetOf(String8_Node, next)},
    {str8_lit_comp("string"), {0}, type(String8),              OffsetOf(String8_Node, string)},
};
Type String8_Node__type =
{
    TypeKind_Struct,
    0,
    sizeof(String8_Node),
    &type_nil,
    str8_lit_comp("String8_Node"),
    {0},
    ArrayCount(String8_Node__members),
    String8_Node__members,
};

//- rjf: String8_List
Member String8_List__members[] =
{
    {str8_lit_comp("first"),      {0}, &String8_Node__ptr_type,     OffsetOf(String8_List, first)},
    {str8_lit_comp("last"),       {0}, &String8_Node__ptr_type,     OffsetOf(String8_List, last), MemberFlag_DoNotSerialize},
    {str8_lit_comp("node_count"), {0}, type(u64), OffsetOf(String8_List, node_count)},
    {str8_lit_comp("total_size"), {0}, type(u64), OffsetOf(String8_List, total_size)},
};
Type String8_List__type = {
    TypeKind_Struct,
    0,
    sizeof(String8_List),
    &type_nil,
    str8_lit_comp("String8_List"),
    {0},
    ArrayCount(String8_List__members),
    String8_List__members,
};

////////////////////////////////
//~ rjf: Type Info Lookups

internal Member *member_from_name(Type *type, String8 name);
#define EachMember(T, it) (Member *it = (type(T))->members; it != 0 && it < (type(T))->members + (type(T))->count; it += 1)

////////////////////////////////
//~ rjf: Type Info * Instance Operations

internal void typed_data_rebase_ptrs(Type *type, String8 data, void *base_ptr);
internal String8 serialized_from_typed_data(Arena *arena, Type *type, String8 data, Type_Serialize_Params *params);
internal String8 deserialized_from_typed_data(Arena *arena, Type *type, String8 data, Type_Serialize_Params *params);
internal String8 deep_copy_from_typed_data(Arena *arena, Type *type, String8 data, Type_Serialize_Params *params);
#define struct_rebase_ptrs(T, ptr, base)                   typed_data_rebase_ptrs(type(T), str8_struct(ptr), (base))
#define serialized_from_struct(arena, T, ptr, ...)         serialized_from_typed_data((arena), type(T), str8_struct(ptr), &(Type_Serialize_Params){.ptr_ref_infos = 0, __VA_ARGS__})
#define struct_from_serialized(arena, T, string, ...) (T *)deserialized_from_typed_data((arena), type(T), (string), &(Type_Serialize_Params){.ptr_ref_infos = 0, __VA_ARGS__}).str
#define deep_copy_from_struct(arena, T, ptr, ...)     (T *)deep_copy_from_typed_data((arena), type(T), str8_struct(ptr), &(Type_Serialize_Params){.ptr_ref_infos = 0, __VA_ARGS__}).str

#endif // BASE_META_H
