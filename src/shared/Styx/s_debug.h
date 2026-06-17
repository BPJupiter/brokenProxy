#ifndef STYX_INTERNAL

extern void styx_restart_marker_reset_internal(SHandle *handle, char *file, uint line);
#define styx_restart_marker_reset(n) styx_restart_marker_reset_internal(n, __FILE__, __LINE__)

#ifdef STYX_DEBUG

extern void      styx_pack_uint8_internal(SHandle *handle, uint8 value, char *name, char *file, uint line);
extern void     styx_pack_int8_internal(SHandle *handle, int8 value, char *name, char *file, uint line);
extern void     styx_pack_uint16_internal(SHandle *handle, uint16 value, char *name, char *file, uint line);
extern void     styx_pack_int16_internal(SHandle *handle, int16 value, char *name, char *file, uint line);
extern void     styx_pack_uint32_internal(SHandle *handle, uint32 value, char *name, char *file, uint line);
extern void     styx_pack_int32_internal(SHandle *handle, int32 value, char *name, char *file, uint line);
extern void     styx_pack_uint64_internal(SHandle *handle, uint64 value, char *name, char *file, uint line);
extern void     styx_pack_int64_internal(SHandle *handle, int64 value, char *name, char *file, uint line);
extern void     styx_pack_float_internal(SHandle *handle, float value, char *name, char *file, uint line);
extern void     styx_pack_double_internal(SHandle *handle, double value, char *name, char *file, uint line);
extern void     styx_pack_string_internal(SHandle *handle, char *value, char *name, char *file, uint line);
extern uint64   styx_pack_raw_internal(SHandle *handle, uint8 *data, uint64 length, char *name, char *file, uint line);
extern uint8    styx_unpack_uint8_internal(SHandle *handle, char *name, char *file, uint line);
extern int8     styx_unpack_int8_internal(SHandle *handle, char *name, char *file, uint line);
extern uint16   styx_unpack_uint16_internal(SHandle *handle, char *name, char *file, uint line);
extern int16    styx_unpack_int16_internal(SHandle *handle, char *name, char *file, uint line);
extern uint32   styx_unpack_uint32_internal(SHandle *handle, char *name, char *file, uint line);
extern int32    styx_unpack_int32_internal(SHandle *handle, char *name, char *file, uint line);
extern uint64   styx_unpack_uint64_internal(SHandle *handle, char *name, char *file, uint line);
extern int64    styx_unpack_int64_internal(SHandle *handle, char *name, char *file, uint line);
extern float    styx_unpack_float_internal(SHandle *handle, char *name, char *file, uint line);
extern double   styx_unpack_double_internal(SHandle *handle, char *name, char *file, uint line);
extern uint     styx_unpack_string_internal(SHandle *handle, char *value, uint buffer_size, char *name, char *file, uint line);
extern char    *styx_unpack_string_with_ownership_internal(SHandle *handle, char *name, char *file, uint line);
extern uint64   styx_unpack_raw_internal(SHandle *handle, uint8 *buffer, uint64 buffer_length, char *name, char *file, uint line);

#define styx_pack_uint8(n, m, l) styx_pack_uint8_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_uint8(n, m) styx_unpack_uint8_internal(n, m, __FILE__, __LINE__)
#define styx_pack_int8(n, m, l) styx_pack_int8_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_int8(n, m) styx_unpack_int8_internal(n, m, __FILE__, __LINE__)

#define styx_pack_uint16(n, m, l) styx_pack_uint16_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_uint16(n, m) styx_unpack_uint16_internal(n, m, __FILE__, __LINE__)
#define styx_pack_int16(n, m, l) styx_pack_int16_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_int16(n, m) styx_unpack_int16_internal(n, m, __FILE__, __LINE__)

#define styx_pack_uint32(n, m, l) styx_pack_uint32_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_uint32(n, m) styx_unpack_uint32_internal(n, m, __FILE__, __LINE__)
#define styx_pack_int32(n, m, l) styx_pack_int32_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_int32(n, m) styx_unpack_int32_internal(n, m, __FILE__, __LINE__)

#define styx_pack_uint64(n, m, l) styx_pack_uint64_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_uint64(n, m) styx_unpack_uint64_internal(n, m, __FILE__, __LINE__)
#define styx_pack_int64(n, m, l) styx_pack_int64_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_int64(n, m) styx_unpack_int64_internal(n, m, __FILE__, __LINE__)

#define styx_pack_float(n, m, l) styx_pack_float_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_float(n, m) styx_unpack_float_internal(n, m, __FILE__, __LINE__)
#define styx_pack_double(n, m, l) styx_pack_double_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_double(n, m) styx_unpack_double_internal(n, m, __FILE__, __LINE__)

#define styx_pack_string(n, m, l) styx_pack_string_internal(n, m, l, __FILE__, __LINE__)
#define styx_unpack_string(n, m, l, k) styx_unpack_string_internal(n, m, l, k, __FILE__, __LINE__)
#define styx_unpack_string_with_ownership(n, m) styx_unpack_string_with_ownership_internal(n, m, __FILE__, __LINE__)

#define styx_pack_raw(n, m, l, k) styx_pack_raw_internal(n, m, l, k, __FILE__, __LINE__)
#define styx_unpack_raw(n, m, l, k) styx_unpack_raw_internal(n, m, l, k, __FILE__, __LINE__)

#else

extern void      styx_pack_uint8_internal(SHandle *handle, uint8 value);
extern void     styx_pack_int8_internal(SHandle *handle, int8 value);
extern void     styx_pack_uint16_internal(SHandle *handle, uint16 value);
extern void     styx_pack_int16_internal(SHandle *handle, int16 value);
extern void     styx_pack_uint32_internal(SHandle *handle, uint32 value);
extern void     styx_pack_int32_internal(SHandle *handle, int32 value);
extern void     styx_pack_uint64_internal(SHandle *handle, uint64 value);
extern void     styx_pack_int64_internal(SHandle *handle, int64 value);
extern void     styx_pack_float_internal(SHandle *handle, float value);
extern void     styx_pack_double_internal(SHandle *handle, double value);
extern boolean  styx_pack_string_internal(SHandle *handle, const char *value);
extern uint64   styx_pack_raw_internal(SHandle *handle, uint8 *data, uint64 length);
extern uint8    styx_unpack_uint8_internal(SHandle *handle);
extern int8     styx_unpack_int8_internal(SHandle *handle);
extern uint16   styx_unpack_uint16_internal(SHandle *handle);
extern int16    styx_unpack_int16_internal(SHandle *handle);
extern uint32   styx_unpack_uint32_internal(SHandle *handle);
extern int32    styx_unpack_int32_internal(SHandle *handle);
extern uint64   styx_unpack_uint64_internal(SHandle *handle);
extern int64    styx_unpack_int64_internal(SHandle *handle);
extern float    styx_unpack_float_internal(SHandle *handle);
extern double   styx_unpack_double_internal(SHandle *handle);
extern boolean  styx_unpack_string_internal(SHandle *handle, char *value, uint buffer_size);
extern char    *styx_unpack_string_with_ownership_internal(SHandle *handle);
extern uint64   styx_unpack_raw_internal(SHandle *handle, uint8 *buffer, uint64 buffer_length);


#define styx_pack_uint8(n, m, l) styx_pack_uint8_internal(n, m)
#define styx_unpack_uint8(n, m) styx_unpack_uint8_internal(n)
#define styx_pack_int8(n, m, l) styx_pack_int8_internal(n, m)
#define styx_unpack_int8(n, m) styx_unpack_int8_internal(n)

#define styx_pack_uint16(n, m, l) styx_pack_uint16_internal(n, m)
#define styx_unpack_uint16(n, m) styx_unpack_uint16_internal(n)
#define styx_pack_int16(n, m, l) styx_pack_int16_internal(n, m)
#define styx_unpack_int16(n, m) styx_unpack_int16_internal(n)

#define styx_pack_uint32(n, m, l) styx_pack_uint32_internal(n, m)
#define styx_unpack_uint32(n, m) styx_unpack_uint32_internal(n)
#define styx_pack_int32(n, m, l) styx_pack_int32_internal(n, m)
#define styx_unpack_int32(n, m) styx_unpack_int32_internal(n)

#define styx_pack_uint64(n, m, l) styx_pack_uint64_internal(n, m)
#define styx_unpack_uint64(n, m) styx_unpack_uint64_internal(n)
#define styx_pack_int64(n, m, l) styx_pack_int64_internal(n, m)
#define styx_unpack_int64(n, m) styx_unpack_int64_internal(n)

#define styx_pack_float(n, m, l) styx_pack_float_internal(n, m)
#define styx_unpack_float(n, m) styx_unpack_float_internal(n)
#define styx_pack_double(n, m, l) styx_pack_double_internal(n, m)
#define styx_unpack_double(n, m) styx_unpack_double_internal(n)

#define styx_pack_string(n, m, l) styx_pack_string_internal(n, m)
#define styx_unpack_string(n, m, l, k) styx_unpack_string_internal(n, m, l)
#define styx_unpack_string_with_ownership(n, m) styx_unpack_string_with_ownership_internal(n)

#define styx_pack_raw(n, m, l, k) styx_pack_raw_internal(n, m, l)
#define styx_unpack_raw(n, m, l, k) styx_unpack_raw_internal(n, m, l)

#endif
#endif