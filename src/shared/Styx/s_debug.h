#ifndef STYX_INTERNAL

extern void styxRestartMarkerResetInternal(SHandle *handle, char *file, uint line);
#define styxRestartMarkerReset(n) styxRestartMarkerResetInternal(n, __FILE__, __LINE__)

#ifdef STYX_DEBUG

extern void  	styxPackUint8Internal(SHandle *handle, uint8 value, char *name, char *file, uint line);
extern void     styxPackInt8Internal(SHandle *handle, int8 value, char *name, char *file, uint line);
extern void     styxPackUint16Internal(SHandle *handle, uint16 value, char *name, char *file, uint line);
extern void     styxPackInt16Internal(SHandle *handle, int16 value, char *name, char *file, uint line);
extern void     styxPackUint32Internal(SHandle *handle, uint32 value, char *name, char *file, uint line);
extern void     styxPackInt32Internal(SHandle *handle, int32 value, char *name, char *file, uint line);
extern void     styxPackUint64Internal(SHandle *handle, uint64 value, char *name, char *file, uint line);
extern void     styxPackInt64Internal(SHandle *handle, int64 value, char *name, char *file, uint line);
extern void     styxPackFloatInternal(SHandle *handle, float value, char *name, char *file, uint line);
extern void     styxPackDoubleInternal(SHandle *handle, double value, char *name, char *file, uint line);
extern void     styxPackStringInternal(SHandle *handle, char *value, char *name, char *file, uint line);
extern uint64   styxPackRawInternal(SHandle *handle, uint8 *data, uint64 length, char *name, char *file, uint line);
extern uint8    styxUnpackUint8Internal(SHandle *handle, char *name, char *file, uint line);
extern int8     styxUnpackInt8Internal(SHandle *handle, char *name, char *file, uint line);
extern uint16   styxUnpackUint16Internal(SHandle *handle, char *name, char *file, uint line);
extern int16    styxUnpackInt16Internal(SHandle *handle, char *name, char *file, uint line);
extern uint32   styxUnpackUint32Internal(SHandle *handle, char *name, char *file, uint line);
extern int32    styxUnpackInt32Internal(SHandle *handle, char *name, char *file, uint line);
extern uint64   styxUnpackUint64Internal(SHandle *handle, char *name, char *file, uint line);
extern int64    styxUnpackInt64Internal(SHandle *handle, char *name, char *file, uint line);
extern float    styxUnpackFloatInternal(SHandle *handle, char *name, char *file, uint line);
extern double   styxUnpackDoubleInternal(SHandle *handle, char *name, char *file, uint line);
extern uint     styxUnpackStringInternal(SHandle *handle, char *value, uint buffer_size, char *name, char *file, uint line);
extern char    *styxUnpackStringWithOwnershipInternal(SHandle *handle, char *name, char *file, uint line);
extern uint64   styxUnpackRawInternal(SHandle *handle, uint8 *buffer, uint64 buffer_length, char *name, char *file, uint line);

#define styxPackUint8(n, m, l) styxPackUint8Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackUint8(n, m) styxUnpackUint8Internal(n, m, __FILE__, __LINE__)
#define styxPackInt8(n, m, l) styxPackInt8Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackInt8(n, m) styxUnpackInt8Internal(n, m, __FILE__, __LINE__)

#define styxPackUint16(n, m, l) styxPackUint16Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackUint16(n, m) styxUnpackUint16Internal(n, m, __FILE__, __LINE__)
#define styxPackInt16(n, m, l) styxPackInt16Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackInt16(n, m) styxUnpackInt16Internal(n, m, __FILE__, __LINE__)

#define styxPackUint32(n, m, l) styxPackUint32Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackUint32(n, m) styxUnpackUint32Internal(n, m, __FILE__, __LINE__)
#define styxPackInt32(n, m, l) styxPackInt32Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackInt32(n, m) styxUnpackInt32Internal(n, m, __FILE__, __LINE__)

#define styxPackUint64(n, m, l) styxPackUint64Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackUint64(n, m) styxUnpackUint64Internal(n, m, __FILE__, __LINE__)
#define styxPackInt64(n, m, l) styxPackInt64Internal(n, m, l, __FILE__, __LINE__)
#define styxUnpackInt64(n, m) styxUnpackInt64Internal(n, m, __FILE__, __LINE__)

#define styxPackFloat(n, m, l) styxPackFloatInternal(n, m, l, __FILE__, __LINE__)
#define styxUnpackFloat(n, m) styxUnpackFloatInternal(n, m, __FILE__, __LINE__)
#define styxPackDouble(n, m, l) styxPackDoubleInternal(n, m, l, __FILE__, __LINE__)
#define styxUnpackDouble(n, m) styxUnpackDoubleInternal(n, m, __FILE__, __LINE__)

#define styxPackString(n, m, l) styxPackStringInternal(n, m, l, __FILE__, __LINE__)
#define styxUnpackString(n, m, l, k) styxUnpackStringInternal(n, m, l, k, __FILE__, __LINE__)
#define styxUnpackStringWithOwnership(n, m) styxUnpackStringWithOwnershipInternal(n, m, __FILE__, __LINE__)

#define styxPackRaw(n, m, l, k) styxPackRawInternal(n, m, l, k, __FILE__, __LINE__)
#define styxUnpackRaw(n, m, l, k) styxUnpackRawInternal(n, m, l, k, __FILE__, __LINE__)

#else

extern void  	styxPackUint8Internal(SHandle *handle, uint8 value);
extern void     styxPackInt8Internal(SHandle *handle, int8 value);
extern void     styxPackUint16Internal(SHandle *handle, uint16 value);
extern void     styxPackInt16Internal(SHandle *handle, int16 value);
extern void     styxPackUint32Internal(SHandle *handle, uint32 value);
extern void     styxPackInt32Internal(SHandle *handle, int32 value);
extern void     styxPackUint64Internal(SHandle *handle, uint64 value);
extern void     styxPackInt64Internal(SHandle *handle, int64 value);
extern void     styxPackFloatInternal(SHandle *handle, float value);
extern void     styxPackDoubleInternal(SHandle *handle, double value);
extern boolean  styxPackStringInternal(SHandle *handle, const char *value);
extern uint64   styxPackRawInternal(SHandle *handle, uint8 *data, uint64 length);
extern uint8    styxUnpackUint8Internal(SHandle *handle);
extern int8     styxUnpackInt8Internal(SHandle *handle);
extern uint16   styxUnpackUint16Internal(SHandle *handle);
extern int16    styxUnpackInt16Internal(SHandle *handle);
extern uint32   styxUnpackUint32Internal(SHandle *handle);
extern int32    styxUnpackInt32Internal(SHandle *handle);
extern uint64   styxUnpackUint64Internal(SHandle *handle);
extern int64    styxUnpackInt64Internal(SHandle *handle);
extern float    styxUnpackFloatInternal(SHandle *handle);
extern double   styxUnpackDoubleInternal(SHandle *handle);
extern boolean  styxUnpackStringInternal(SHandle *handle, char *value, uint buffer_size);
extern char    *styxUnpackStringWithOwnershipInternal(SHandle *handle);
extern uint64   styxUnpackRawInternal(SHandle *handle, uint8 *buffer, uint64 buffer_length);


#define styxPackUint8(n, m, l) styxPackUint8Internal(n, m)
#define styxUnpackUint8(n, m) styxUnpackUint8Internal(n)
#define styxPackInt8(n, m, l) styxPackInt8Internal(n, m)
#define styxUnpackInt8(n, m) styxUnpackInt8Internal(n)

#define styxPackUint16(n, m, l) styxPackUint16Internal(n, m)
#define styxUnpackUint16(n, m) styxUnpackUint16Internal(n)
#define styxPackInt16(n, m, l) styxPackInt16Internal(n, m)
#define styxUnpackInt16(n, m) styxUnpackInt16Internal(n)

#define styxPackUint32(n, m, l) styxPackUint32Internal(n, m)
#define styxUnpackUint32(n, m) styxUnpackUint32Internal(n)
#define styxPackInt32(n, m, l) styxPackInt32Internal(n, m)
#define styxUnpackInt32(n, m) styxUnpackInt32Internal(n)

#define styxPackUint64(n, m, l) styxPackUint64Internal(n, m)
#define styxUnpackUint64(n, m) styxUnpackUint64Internal(n)
#define styxPackInt64(n, m, l) styxPackInt64Internal(n, m)
#define styxUnpackInt64(n, m) styxUnpackInt64Internal(n)

#define styxPackFloat(n, m, l) styxPackFloatInternal(n, m)
#define styxUnpackFloat(n, m) styxUnpackFloatInternal(n)
#define styxPackDouble(n, m, l) styxPackDoubleInternal(n, m)
#define styxUnpackDouble(n, m) styxUnpackDoubleInternal(n)

#define styxPackString(n, m, l) styxPackStringInternal(n, m, l)
#define styxUnpackString(n, m, l, k) styxUnpackStringInternal(n, m, l)
#define styxUnpackStringWithOwnership(n, m) styxUnpackStringWithOwnershipInternal(n)

#define styxPackRaw(n, m, l, k) styxPackRawInternal(n, m, l)
#define styxUnpackRaw(n, m, l, k) styxUnpackRawInternal(n, m, l)

#endif
#endif