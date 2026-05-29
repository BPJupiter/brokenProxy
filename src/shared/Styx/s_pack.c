#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include "Clay/clay.h"
#include "styx.h"

#ifdef STYX_DEBUG
#define STYX_HEADER_SIZE_MAX 64
#else
#define STYX_HEADER_SIZE_MAX 0
#endif

const char *styx_type_strings[] = {"S_TYPE_UINT8",
									"S_TYPE_INT8",
									"S_TYPE_UINT16",
									"S_TYPE_INT16",
									"S_TYPE_UINT32",
									"S_TYPE_INT32",
									"S_TYPE_UINT64",
									"S_TYPE_INT64",
									"S_TYPE_FLOAT",
									"S_TYPE_DOUBLE",
									"S_TYPE_RAW",
									"S_TYPE_STRING"};

const size_t styx_type_size[] = {sizeof(uint8),
								sizeof(int8),
								sizeof(uint16),
								sizeof(int16),
								sizeof(uint32),
								sizeof(int32),
								sizeof(uint64),
								sizeof(int64),
								sizeof(float),
								sizeof(double),
								sizeof(uint8),
								sizeof(char)};

extern boolean styx_pack_buffer_clear(SHandle *handle);
extern boolean styx_pack_buffer_add(SHandle *handle, const uint8 *data, uint length);

static boolean styx_pack_debug_header(SHandle *handle, STypes type, uint vector_length, char *name, char *file, uint line, int value_int, double value_double, char *value_string)
{
	if (handle == NULL || name == NULL || file == NULL) {
		return FALSE;
	}

	if (type >= S_TYPE_COUNT) {
		return FALSE;
	}

	if (handle->write_raw_progress != 0) {
		if (type == S_TYPE_RAW) {
			return FALSE;
		}
		fprintf(stderr, "Styx: Error on line %u in file: %s\n", line, file);
		fprintf(stderr, "-Loading non raw data into a raw stream that has not been completed.\n");
		return FALSE;
	}

	if (handle->write_buffer_pos + styx_type_size[type] > handle->write_buffer_size && handle->type == S_HT_PACKET_PEER) {
		fprintf(stderr, "Styx: Error on line %u in file: %s\n", line, file);
		fprintf(stderr, "-Adding more data then can fit in a single UDP packet without calling send\n");
		return FALSE;
	}

	if (handle->write_buffer_pos + styx_type_size[type] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return FALSE;
		}
	}

	if (handle->debug_descriptor) {
		uint i;
		handle->write_buffer[handle->write_buffer_pos++] = type;
		handle->write_buffer[handle->write_buffer_pos++] = (vector_length >> 24) & 0xFF;
		handle->write_buffer[handle->write_buffer_pos++] = (vector_length >> 16) & 0xFF;
		handle->write_buffer[handle->write_buffer_pos++] = (vector_length >> 8)  & 0xFF;
		handle->write_buffer[handle->write_buffer_pos++] = vector_length & 0xFF;
		for (i = 0; name[i] != 0; i++) {
			handle->write_buffer[handle->write_buffer_pos++] = name[i];
		}
		handle->write_buffer[handle->write_buffer_pos++] = 0;
	}

	if (handle->text_copy != NULL) {
		if (vector_length > 1 || type == S_TYPE_RAW) {
			fprintf(handle->text_copy, "%s %s length %u\n", styx_type_strings[type], name, value_int);
		}
		else if (type == S_TYPE_STRING) {
			fprintf(handle->text_copy, "%s %s = %s\n", styx_type_strings[type], name, value_string);
		}
		else if (type >= S_TYPE_FLOAT) {
			fprintf(handle->text_copy, "%s %s = %f\n", styx_type_strings[type], name, value_double);
		}
		else {
			fprintf(handle->text_copy, "%s %s = %i\n", styx_type_strings[type], name, value_int);
		}
	}
	return TRUE;
}

static boolean styx_unpack_debug_header(SHandle *handle, STypes type, uint vector_length, char *name, char *file, uint line)
{
	if (handle == NULL || name == NULL || file == NULL) {
		return FALSE;
	}

	if (type >= S_TYPE_COUNT) {
		return FALSE;
	}

	if (handle->debug_descriptor) {
		char read[32];
		uint i, j, read_type, length, start;

		if (handle->read_raw_progress != 0) {
			if (type == S_TYPE_RAW) {
				return FALSE;
			}
			fprintf(stderr, "Styx: Error on line %u in file: %s\n", line, file);
			fprintf(stderr, "-Reading non raw data from a raw stream that has not been completed.\n");
			return FALSE;
		}

		start = handle->read_buffer_pos;

		if (handle->read_buffer_pos + 5 > handle->read_buffer_used) {
			fprintf(stderr, "Styx: Error trying to read data that isnt there in file %s on line %u %u %u\n", file, line, (uint)handle->read_buffer_pos + 5, (uint)handle->read_buffer_used);
			return FALSE;
		}
		read_type = handle->read_buffer[handle->read_buffer_pos++];
		length  = ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 24;
		length |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 16;
		length |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 8;
		length |= handle->read_buffer[handle->read_buffer_pos++];

		for (i = 0; handle->read_buffer[handle->read_buffer_pos + i] != 0 && i < 31; i++) {
			read[i] = handle->read_buffer[handle->read_buffer_pos + i];
		}
		read[i] = 0;

		for (j = 0; name[j] == read[j] && name[j] != 0; j++)
			;

		if (name[j] != read[j]/* || read_type != type */) {
			fprintf(stderr, "Styx: Read error on line %u in file: %s (%u)\n", line, file, (uint)handle->read_buffer_pos);
			fprintf(stderr, "-Reading %s -> -%s-\n", styx_type_strings[read_type], read);
			fprintf(stderr, "-Expecting %s -> -%s-\n", styx_type_strings[type], name);
			if (handle->file_name != NULL) {
				fprintf(stderr, "File name: %s\n", handle->file_name);
			}
			for (i = 0; i < handle->read_buffer_used && i < handle->read_buffer_pos + 100; i++) {
				char character[2] = {0, 0};
				character[0] = handle->read_buffer[i];
				if (i == handle->read_buffer_pos) {
					fprintf(stderr, "Buffer [%u] = %s (%u) <----------------\n", i, character, (uint)handle->read_buffer[i]);
				}
				else {
					fprintf(stderr, "Buffer [%u] = %s (%u)\n", i, character, (uint)handle->read_buffer[i]);
				}
			}
			return FALSE;
		}

		if (length != vector_length) {
			uint j;
			fprintf(stderr, "Styx: Read error on line %u in file: %s\n", line, file);
			for (j = 0; j < 22; j++) {
				fprintf(stderr, " %u ", (uint)handle->read_buffer[start + j]);
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "-Reading %s with a vector length of %u %u\n", name, length, (uint)handle->read_buffer_pos);
			fprintf(stderr, "-Expecting a length of %u\n", vector_length);
			return FALSE;
		}
		handle->read_buffer_pos += i + 1;
	}
	return TRUE;
}

/* ----------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_uint8_internal(SHandle *handle uint8 value, char *name, char *file, uint line)
#else
void styx_pack_uint8_internal(SHandle *handle, uint8 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_UINT8, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_UINT8] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = value;
}

#ifdef STYX_DEBUG
uint8 styx_unpack_uint8_internal(SHandle *handle, char *name, char *file, uint line)
#else
uint8 styx_unpack_uint8_internal(SHandle *handle)
#endif
{
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_UINT8] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_UINT8, 1, name, file, line);
	}
#endif
	return handle->read_buffer[handle->read_buffer_pos++];
}

/* extern void styx_pack_vector_uint8_internal(SHandle *handle, uint8 value, char *name, uint length, char *file, uint line); */

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_int8_internal(SHandle *handle int8 value, char *name, char *file, uint line)
#else
void styx_pack_int8_internal(SHandle *handle, int8 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_INT8, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_INT8] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = value;
}

#ifdef STYX_DEBUG
int8 styx_unpack_int8_internal(SHandle *handle, char *name, char *file, uint line)
#else
int8 styx_unpack_int8_internal(SHandle *handle)
#endif
{
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_INT8] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_INT8, 1, name, file, line);
	}
#endif
	return (int8)handle->read_buffer[handle->read_buffer_pos++];
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_uint16_internal(SHandle *handle uint16 value, char *name, char *file, uint line)
#else
void styx_pack_uint16_internal(SHandle *handle, uint16 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_UINT16, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_UINT16] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = (value & 0xFF00) >> 8;
	handle->write_buffer[handle->write_buffer_pos++] = value & 0xFF;
}

#ifdef STYX_DEBUG
uint16 styx_unpack_uint16_internal(SHandle *handle, char *name, char *file, uint line)
#else
uint16 styx_unpack_uint16_internal(SHandle *handle)
#endif
{
	uint16 data;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_UINT16] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_UINT16, 1, name, file, line);
	}
#endif
	data = ((uint16) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	data |= (uint16) handle->read_buffer[handle->read_buffer_pos++];
	return data;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_int16_internal(SHandle *handle int16 value, char *name, char *file, uint line)
#else
void styx_pack_int16_internal(SHandle *handle, int16 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_INT16, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_INT16] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = (value & 0xFF00) >> 8;
	handle->write_buffer[handle->write_buffer_pos++] = value & 0xFF;
}

#ifdef STYX_DEBUG
int16 styx_unpack_int16_internal(SHandle *handle, char *name, char *file, uint line)
#else
int16 styx_unpack_int16_internal(SHandle *handle)
#endif
{
	int16 data;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_INT16] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_INT16, 1, name, file, line);
	}
#endif
	data = ((int16) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	data |= (int16) handle->read_buffer[handle->read_buffer_pos++];
	return data;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_uint32_internal(SHandle *handle uint32 value, char *name, char *file, uint line)
#else
void styx_pack_uint32_internal(SHandle *handle, uint32 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_UINT32, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_UINT32] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 8)  & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = value & 0xFF;
}

#ifdef STYX_DEBUG
uint32 styx_unpack_uint32_internal(SHandle *handle, char *name, char *file, uint line)
#else
uint32 styx_unpack_uint32_internal(SHandle *handle)
#endif
{
	uint32 data;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_UINT32] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_UINT32, 1, name, file, line);
	}
#endif
	data = ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	data |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	data |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	data |= (uint32) handle->read_buffer[handle->read_buffer_pos++];
	return data;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_int32_internal(SHandle *handle int32 value, char *name, char *file, uint line)
#else
void styx_pack_int32_internal(SHandle *handle, int32 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_INT32, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_INT32] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 8)  & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = value & 0xFF;
}

#ifdef STYX_DEBUG
int32 styx_unpack_int32_internal(SHandle *handle, char *name, char *file, uint line)
#else
int32 styx_unpack_int32_internal(SHandle *handle)
#endif
{
	int32 data;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_INT32] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_INT32, 1, name, file, line);
	}
#endif
	data = ((int32) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	data |= ((int32) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	data |= ((int32) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	data |= (int32) handle->read_buffer[handle->read_buffer_pos++];
	return data;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_uint64_internal(SHandle *handle uint64 value, char *name, char *file, uint line)
#else
void styx_pack_uint64_internal(SHandle *handle, uint64 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_UINT64, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_UINT64] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 56) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 48) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 40) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 32) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 8) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = value & 0xFF;
}

#ifdef STYX_DEBUG
uint64 styx_unpack_uint64_internal(SHandle *handle, char *name, char *file, uint line)
#else
uint64 styx_unpack_uint64_internal(SHandle *handle)
#endif
{
	uint64 data;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_UINT64] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_UINT64, 1, name, file, line);
	}
#endif
	data = ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 56;
	data |= ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 48;
	data |= ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 40;
	data |= ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 32;
	data |= ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	data |= ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	data |= ((uint64) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	data |= (uint64) handle->read_buffer[handle->read_buffer_pos++];
	return data;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_int64_internal(SHandle *handle int64 value, char *name, char *file, uint line)
#else
void styx_pack_int64_internal(SHandle *handle, int64 value)
#endif
{
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_INT64, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_INT64] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 56) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 48) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 40) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 32) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (value >> 8) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = value & 0xFF;
}

#ifdef STYX_DEBUG
int64 styx_unpack_int64_internal(SHandle *handle, char *name, char *file, uint line)
#else
int64 styx_unpack_int64_internal(SHandle *handle)
#endif
{
	int64 data;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_INT64] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_INT64, 1, name, file, line);
	}
#endif
	data = ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 56;
	data |= ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 48;
	data |= ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 40;
	data |= ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 32;
	data |= ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	data |= ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	data |= ((int64) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	data |= (int64) handle->read_buffer[handle->read_buffer_pos++];
	return data;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_float_internal(SHandle *handle float value, char *name, char *file, uint line)
#else
void styx_pack_float_internal(SHandle *handle, float value)
#endif
{
	union { uint32 integer; float real; } punt;
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_FLOAT, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_FLOAT] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	punt.real = value;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer >> 8)  & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = punt.integer & 0xFF;
}

#ifdef STYX_DEBUG
float styx_unpack_float_internal(SHandle *handle, char *name, char *file, uint line)
#else
float styx_unpack_float_internal(SHandle *handle)
#endif
{
	union { uint32 integer; float real; } punt;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_FLOAT] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_FLOAT, 1, name, file, line);
	}
#endif
	punt.integer = ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	punt.integer |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	punt.integer |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	punt.integer |= (uint32) handle->read_buffer[handle->read_buffer_pos++];
	return punt.real;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
void styx_pack_double_internal(SHandle *handle double value, char *name, char *file, uint line)
#else
void styx_pack_double_internal(SHandle *handle, double value)
#endif
{
	union { uint32 integer[2]; double real; } punt;
#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_DOUBLE, 1, name, file, line, value, value, "")) {
		return; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + styx_type_size[S_TYPE_DOUBLE] > handle->write_buffer_size - STYX_HEADER_SIZE_MAX) {
		if (!styx_pack_buffer_clear(handle)) {
			return; /* dest is gone */
		}
	}
#endif
	punt.real = value;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer[0] >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer[0] >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer[0] >> 9) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = punt.integer[0] & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer[1] >> 24) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer[1] >> 16) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = (punt.integer[1] >> 8) & 0xFF;
	handle->write_buffer[handle->write_buffer_pos++] = punt.integer[1] & 0xFF;
}

#ifdef STYX_DEBUG
double styx_unpack_double_internal(SHandle *handle, char *name, char *file, uint line)
#else
double styx_unpack_double_internal(SHandle *handle)
#endif
{
	union { uint32 integer[2]; double real; } punt;
	/* send or save data if buffer is filling up */
	if (handle->read_buffer_pos + styx_type_size[S_TYPE_DOUBLE] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	/* include the data desc header */
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_DOUBLE, 1, name, file, line);
	}
#endif
	punt.integer[0] = ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	punt.integer[0] |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	punt.integer[0] |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	punt.integer[0] |= (uint32) handle->read_buffer[handle->read_buffer_pos++];
	punt.integer[1] = ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 24;
	punt.integer[1] |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 16;
	punt.integer[1] |= ((uint32) handle->read_buffer[handle->read_buffer_pos++]) << 8;
	punt.integer[1] |= (uint32) handle->read_buffer[handle->read_buffer_pos++];
	return punt.real;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
uint64 styx_pack_raw_internal(SHandle *handle, uint8 *data, uint64 length, char *name, char *file, uint line)
#else
uint64 styx_pack_raw_internal(SHandle *handle, uint8 *data, uint64 length)
#endif
{
	if (handle == NULL || data == NULL) {
		return 0;
	}

#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_RAW, (uint)length, name, file, line, (int)length, (double)length, "")) {
		return 0; /* dest is gone */
	}
#else
	if (handle->write_buffer_pos + length + STYX_HEADER_SIZE_MAX > handle->write_buffer_size) {
		if (!styx_pack_buffer_clear(handle)) {
			return 0; /* dest is gone */
		}
	}
#endif

	if (handle->type == S_HT_STREAMING_SERVER || handle->type == S_HT_STREAMING_CONNECTION) {
		size_t size;
		if (handle->write_buffer_pos) {
			size = styx_network_stream_send(handle, handle->write_buffer, handle->write_buffer_pos);
			if (size == handle->write_buffer_pos) {
				handle->write_buffer_pos = 0;
			}
			else if (size != 0) {
				memmove(handle->write_buffer, handle->write_buffer + size, handle->write_buffer_pos - size);
				handle->write_buffer_pos -= size;
				return 0;
			}
		}

		size = styx_network_stream_send(handle, &data[handle->write_raw_progress], length - handle->write_raw_progress);
		handle->write_raw_progress += size;
		if (handle->write_raw_progress == length) {
			handle->write_raw_progress = 0;
		}
		return size;
	}
	else if (handle->type == S_HT_PACKET_PEER) {
		memcpy(&handle->write_buffer[handle->write_buffer_pos], data, length);
		handle->write_buffer_pos += length;
		return length;
	}
	else if (handle->type == S_HT_FILE_WRITE) {
		styx_pack_buffer_clear(handle);
		fwrite(data, 1, length, handle->file);
		return length;
	}
	return 0;
}

#ifdef STYX_DEBUG
uint64 styx_unpack_raw_internal(SHandle *handle, uint8 *buffer, uint64 buffer_length, char *name, char *file, uint line)
#else
uint64 styx_unpack_raw_internal(SHandle *handle, uint8 *buffer, uint64 buffer_length)
#endif
{
	if (handle == NULL || buffer == NULL || buffer_length == 0) {
		return 0;
	}

#ifdef STYX_DEBUG
	if (handle->debug_descriptor && handle->read_raw_progress == 0) {
		if (!styx_unpack_buffer_get(handle) && handle->read_buffer_pos >= handle->read_buffer_used) {
			return 0;
		}
		if (handle->read_buffer_pos + STYX_HEADER_SIZE_MAX <= handle->read_buffer_used) {
			styx_unpack_debug_header(handle, S_TYPE_RAW, 1, name, file, line);
		}
		else {
			return 0;
		}
	}
#endif
	if (handle->type == S_HT_STREAMING_SERVER || handle->type == S_HT_STREAMING_CONNECTION) {
		size_t progress, size;
		progress = handle->read_buffer_used - handle->read_buffer_pos;
		if (progress >= buffer_length) {
			memcpy(buffer, &handle->read_buffer[handle->read_buffer_pos], buffer_length);
			handle->read_buffer_pos += buffer_length;
			return buffer_length;
		}
		memcpy(buffer, &handle->read_buffer[handle->read_buffer_pos], progress);
		handle->read_buffer_pos = handle->read_buffer_used;
		size = styx_network_stream_receive(handle, &buffer[progress], buffer_length - progress);

		progress += size;
		if (progress == buffer_length) {
			handle->read_raw_progress = 0;
		}
		else {
			handle->read_raw_progress = progress;
		}
		return progress;
	}
	else if (handle->type == S_HT_PACKET_PEER) {
		memcpy(buffer, &handle->read_buffer[handle->read_buffer_pos], buffer_length);
		handle->read_buffer_pos += buffer_length;
		return buffer_length;
	}
	else if (handle->type == S_HT_FILE_READ) {
		if (handle->read_buffer_used - handle->read_buffer_pos >= buffer_length) {
			memcpy(buffer, &handle->read_buffer[handle->read_buffer_pos], buffer_length);
			handle->read_buffer_pos += buffer_length;
			handle->read_raw_progress = 0;
			return buffer_length;
		}
		else {
			uint64 progress;
			progress = handle->read_buffer_used - handle->read_buffer_pos;
			memcpy(buffer, &handle->read_buffer[handle->read_buffer_pos], progress);
			handle->read_buffer_pos = handle->read_buffer_used;
			return fread(&buffer[progress], 1, buffer_length - progress, handle->file) + progress;
		}
	}
	return 0;
}

/* ---------------------------------------------------------------- */

#ifdef STYX_DEBUG
boolean styx_pack_string_internal(SHandle *handle, const char *string, char *name, char *file, uint line)
#else
boolean styx_pack_string_internal(SHandle *handle, const char *string)
#endif
{
	uint length = 0, pos = 0, size;

	if (handle == NULL) {
		return FALSE;
	}

#ifdef STYX_DEBUG
	if (!styx_pack_debug_header(handle, S_TYPE_STRING, 1, name, file, line, 0, 0, string)) {
		return FALSE;
	}
#endif
	if (string == NULL) {
		if (handle->write_buffer_pos == handle->write_buffer_size) {
			if (!styx_pack_buffer_clear(handle)) {
				return FALSE;
			}
		}
		if (handle->write_buffer_pos < handle->write_buffer_size) {
			handle->write_buffer[handle->write_buffer_pos++] = 0;
		}
	}
	else {
		length = strlen(string) + 1;
		while (pos < length) {
			size_t left = handle->write_buffer_size - handle->write_buffer_pos;
			size = (length - pos < left) ? (length - pos) : left;
			if (size > 0) {
				memcpy(&handle->write_buffer[handle->write_buffer_pos], &string[pos], size);
				handle->write_buffer_pos += size;
				pos += size;
			}
			if (handle->write_buffer_pos == handle->write_buffer_size) {
				if (!styx_pack_buffer_clear(handle)) {
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

#ifdef STYX_DEBUG
boolean styx_unpack_string_internal(SHandle *handle, char *string, uint buffer_size, char *name, char *file, uint line)
#else
boolean styx_unpack_string_internal(SHandle *handle, char *string, uint buffer_size)
#endif
{
	boolean marker;
	uint i, size;

	if (handle == NULL || string == NULL || buffer_size == 0) {
		return FALSE;
	}

	if (handle->read_buffer_pos + styx_type_size[S_TYPE_STRING] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_STRING, 1, name, file, line);
	}
#endif

	marker = handle->read_marker != -1;
	if (!marker) {
		handle->read_marker = handle->read_buffer_pos;
	}

	while (TRUE) {
		for (size = 0; handle->read_buffer_pos + size < handle->read_buffer_used && handle->read_buffer[handle->read_buffer_pos + size] != 0; size++)
			;

		if (handle->read_buffer_pos + size < handle->read_buffer_used) {
			--buffer_size;
			for (i = 0; i < buffer_size && handle->read_buffer[i + handle->read_buffer_pos] != 0; i++) {
				string[i] = handle->read_buffer[i + handle->read_buffer_pos];
			}
			string[i] = 0;
			handle->read_buffer_pos += size + 1;
			if (!marker) {
				handle->read_marker = -1;
			}
			return TRUE;
		}
		if (!styx_unpack_buffer_get(handle)) {
			if (!marker) {
				handle->read_marker = -1;
			}
			return FALSE;
		}
	}
}

#ifdef STYX_DEBUG
char *styx_unpack_string_with_ownership_internal(SHandle *handle, char *name, char *file, uint line)
#else
char *styx_unpack_string_with_ownership_internal(SHandle *handle)
#endif
{
	char *string = NULL;
	boolean marker;
	uint i, size;

	if (handle == NULL) {
		return NULL;
	}

	if (handle->read_buffer_pos + styx_type_size[S_TYPE_STRING] + STYX_HEADER_SIZE_MAX >= handle->read_buffer_used) {
		styx_unpack_buffer_get(handle);
	}
#ifdef STYX_DEBUG
	if (handle->debug_descriptor) {
		styx_unpack_debug_header(handle, S_TYPE_STRING, 1, name, file, line);
	}
#endif
	marker = handle->read_marker != -1;
	if (!marker) {
		handle->read_marker = handle->read_buffer_pos;
	}

	while (TRUE) {
		for (size = 0; handle->read_buffer_pos + size < handle->read_buffer_used && handle->read_buffer[handle->read_buffer_pos + size] != 0; size++)
			;

		if (handle->read_buffer_pos + size < handle->read_buffer_used) {
			if (size == 0) {
				handle->read_buffer_pos++;
				return NULL;
			}
			string = malloc((sizeof *string) * ++size);
			for (i = 0; handle->read_buffer[i + handle->read_buffer_pos] != 0; i++) {
				string[i] = handle->read_buffer[i + handle->read_buffer_pos];
			}
			string[i] = 0;
			handle->read_buffer_pos += size;
			if (!marker) {
				handle->read_marker = -1;
			}
			return string;
		}
		if (!styx_unpack_buffer_get(handle)) {
			if (!marker) {
				handle->read_marker = -1;
			}
			return NULL;
		}
	}
}

void styx_restart_marker_set(SHandle *handle)
{
	handle->read_marker = handle->read_buffer_pos;
}

void styx_restart_marker_release(SHandle *handle)
{
	handle->read_marker = -1;
}

void styx_restart_marker_reset_internal(SHandle *handle, char *file, uint line)
{
	if (handle->read_marker == -1) {
		fprintf(stderr, "Styx: styxRestartMarkerReset error on line %u in file %s. Marker not set.\n", line, file);
		return;
	}
	handle->read_buffer_pos = handle->read_marker;
	handle->read_marker = -1;
}

boolean styx_retrievable_internal(SHandle *handle, uint size)
{
	if (handle == NULL) {
		return FALSE;
	}

	if (handle->type == S_HT_BUFFER) {
		handle->read_buffer_used = handle->write_buffer_pos;
	}

	if (handle->debug_descriptor) {
		uint i, pos, collected = 0, length, add;
		uint8 read_type;
		pos = handle->read_buffer_pos;

		while (pos + 5 < handle->read_buffer_used && size > collected) {
			read_type = handle->read_buffer[pos++];
			length = ((uint32) handle->read_buffer[pos++]) << 24;
			length |= ((uint32) handle->read_buffer[pos++]) << 16;
			length |= ((uint32) handle->read_buffer[pos++]) << 8;
			length |= handle->read_buffer[pos++];

			for (i = 0; pos < handle->read_buffer_used && handle->read_buffer[pos] != 0 && i < 31; i++) {
				pos++; /* name */
			}
			if (i == 31) {
				for (i = 0; i < handle->read_buffer_used && i < pos + 10000; i++) {
					char character[2] = {0, 0};
					character[0] = handle->read_buffer[i];
					if (i == pos) {
						fprintf(stderr, "Buffer [%u] = %s (%u) <---------------- used %zu pos %zu\n", i, character, (uint)handle->read_buffer[i], handle->read_buffer_used, handle->read_buffer_pos);
					}
					else {
						fprintf(stderr, "Buffer [%u] = %s (%u)\n", i, character, (uint)handle->read_buffer[i]);
					}
				}
				return FALSE;
			}
			pos++;
			if (pos == handle->read_buffer_used) {
				return FALSE;
			}

			if (read_type >= S_TYPE_COUNT) {
				return FALSE;
			}

			if (length <= 1) {
				add = styx_type_size[read_type];
			}
			else {
				add = styx_type_size[read_type] * length;
			}
			pos += add;
			if (pos > handle->read_buffer_used) {
				return FALSE;
			}
			collected += add;
		}
		return size <= collected;
	}
	return handle->read_buffer_pos + size <= handle->read_buffer_used;
}

boolean styx_retrievable(SHandle *handle, uint size)
{
	if (handle == NULL) {
		return FALSE;
	}

	if (size > handle->read_buffer_size) {
		return FALSE;
	}

	while (TRUE) {
		if (styx_retrievable_internal(handle, size)) {
			return TRUE;
		}
		if (!styx_unpack_buffer_get(handle)) {
			return FALSE;
		}
	}
}

boolean styx_retrievable_terminated_internal(SHandle *handle)
{
	uint i;

	if (handle == NULL) {
		return FALSE;
	}

	if (handle->debug_descriptor) {
		uint pos, length;
		uint8 read_type;
		pos = handle->read_buffer_pos;

		if (pos + 6 >= handle->read_buffer_used) {
			return FALSE;
		}

		read_type = handle->read_buffer[pos++];
		if (read_type != S_TYPE_STRING) {
			uint8 read[2048];
			char string[2048];
			uint *X = NULL, j;
			j = 0;
			if (pos > 50) {
				j = pos - 50;
			}
			for (i = 0; i < 2048 && i < handle->read_buffer_used; i++) {
				read[i] = string[i] = handle->read_buffer[j++];
			}
			fprintf(stderr, "Styx: Error trying to styxRetrievableTerminated on non string\n");
			X[0] = 0;
			return FALSE;
		}
		length  = ((uint32) handle->read_buffer[pos++]) << 24;
		length |= ((uint32) handle->read_buffer[pos++]) << 16;
		length |= ((uint32) handle->read_buffer[pos++]) << 8;
		length |= handle->read_buffer[pos++];

		for (i = 0; pos < handle->read_buffer_used && handle->read_buffer[pos] != 0 && i < 31; i++) {
			pos++; /* name */
		}
		if (pos == handle->read_buffer_used) {
			return FALSE;
		}

		for (pos++; pos < handle->read_buffer_used && handle->read_buffer[pos] != 0; pos++)
			;
		if (pos < handle->read_buffer_used) {
			return TRUE;
		}
		return FALSE;
	}
	for (i = handle->read_buffer_pos; i < handle->read_buffer_used && handle->read_buffer[i] != 0; i++)
		;
	return handle->read_buffer[i] == 0;
}

boolean styx_retrievable_terminated(SHandle *handle)
{
	if (handle == NULL) {
		return FALSE;
	}

	while (TRUE) {
		if (styx_retrievable_terminated_internal(handle)) {
			return TRUE;
		}

		if (handle->read_buffer_used >= handle->read_buffer_size) {
			return FALSE;
		}
		
		if (!styx_unpack_buffer_get(handle)) {
			return FALSE;
		}
	}
}