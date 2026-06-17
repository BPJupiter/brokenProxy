#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "styx.h"

char *styx_debug_magic_number = "StYxLiBdEbUg";

SHandle *styx_buffer_create(void)
{
    SHandle *handle;
    handle = malloc(sizeof *handle);
    if (handle == NULL) {
        return NULL;
    }

    styx_handle_clear(handle, S_HT_BUFFER);
    styx_unpack_buffer_get(handle);

    return handle;
}

void *styx_buffer_get(SHandle *handle, uint32 *size)
{
    if (handle == NULL || size == NULL) {
        return NULL;
    }

    if (handle->type != S_HT_BUFFER) {
        fprintf(stderr, "Styx: Cna't styxBufferGet on non buffer handles!\n");
        return NULL;
    }

    if (handle->read_buffer_pos > handle->write_buffer_pos) {
        *size = 0;
        return NULL;
    }

    *size = handle->write_buffer_pos - handle->read_buffer_pos;

    if (handle->read_buffer == NULL) {
        return NULL;
    }

    return &handle->read_buffer[handle->read_buffer_pos];
}

void styx_buffer_set(SHandle *handle, void *data, uint32 size)
{
    if (handle == NULL || data == NULL || size == 0) {
        return;
    }

    if (handle->type != S_HT_BUFFER) {
        fprintf(stderr, "Styx: can't styxBufferSet on non buffer handles!\n");
        return;
    }

    if (handle->write_buffer_pos + size > handle->write_buffer_size) {
        uint8 *temp;
        uint32 new_size = handle->write_buffer_pos + size;

        temp = realloc(handle->write_buffer, new_size);
        if (temp == NULL) {
            fprintf(stderr, "Styx: Out of memory in styxBufferSet!\n");
            return;
        }

        handle->write_buffer = temp;
        handle->read_buffer = temp;
        handle->write_buffer_size = new_size;
        handle->read_buffer_size = new_size;
    }

    memcpy(&handle->read_buffer[handle->write_buffer_pos], data, size);
    handle->write_buffer_pos += size;
}

void styx_handle_clear(SHandle *handle, uint type)
{
    uint styx_buffer_size[] = {4096, /* S_HT_STREAMING_SERVER */
                                4096, /* S_HT_STREAMING_CONNECTION */
                                1500, /* S_HT_PACKET_PEER (MTU) */
                                8192, /* S_HT_FILE_READ */
                                8192, /* S_HT_FILE_WRITE */
                                8192}; /* S_HT_BUFFER */
    memset(&handle->ip.v6, 0, 16);
    handle->port = 0;
    handle->socket = -1;
    handle->type = type;
    handle->read_buffer_size = styx_buffer_size[handle->type];
    if (type != S_HT_FILE_WRITE) {
        handle->read_buffer = malloc((sizeof *handle->read_buffer) * handle->read_buffer_size);
    }
    else {
        handle->read_buffer = NULL;
    }
    handle->read_buffer_used = 0;
    handle->read_buffer_pos = 0;
    handle->read_marker = -1;
    handle->read_raw_progress = 0;
    if (type == S_HT_BUFFER) {
        handle->write_buffer = handle->read_buffer;
        handle->write_buffer_size = handle->read_buffer_size;
    }
    else if (type != S_HT_FILE_READ) {
        handle->write_buffer_size = styx_buffer_size[handle->type];
        handle->write_buffer = malloc((sizeof *handle->write_buffer) * handle->write_buffer_size);
    }
    else {
        handle->write_buffer = NULL;
    }
    handle->write_buffer_pos = 0;
    handle->write_raw_progress = 0;
    handle->file = NULL;
    handle->text_copy = NULL;
    handle->debug_descriptor = FALSE;
    handle->connected = TRUE;
    handle->debug_header = FALSE;
    handle->file_name = NULL;
}

uint styx_unpack_buffer_get(SHandle *handle)
{
    size_t size, i, start;

    if (handle == NULL) {
        return FALSE;
    }

    if (handle->read_marker != (size_t)-1) {
        start = handle->read_marker;
    }
    else {
        start = handle->read_buffer_pos;
    }

    if (start > 0) {
        if (handle->read_buffer_used < start) {
            fprintf(stderr, "Styx: Buffer corruption (used: %lu, start: %lu\n", handle->read_buffer_used, start);
            return FALSE;
        }

        size = handle->read_buffer_used - start;

        if (handle->read_buffer_pos < start) {
            fprintf(stderr, "Styx: Invalid state: (read_buffer_pos: %lu, start: %lu\n", handle->read_buffer_pos, start);
            return FALSE;
        }

        if (size > 0) {
            memmove(handle->read_buffer, handle->read_buffer + start, size);
        }

        if (handle->read_marker != (size_t)-1) {
            handle->read_marker -= start;
        }

        handle->read_buffer_pos -= start;
        handle->read_buffer_used = size;
        if (handle->type == S_HT_BUFFER) {
            handle->write_buffer_pos -= start;
        }
    }

    if (handle->type != S_HT_BUFFER && handle->read_buffer_used + 1024 > handle->read_buffer_size) {
        size_t new_size = handle->read_buffer_size == 0 ? 1024 : handle->read_buffer_size * 2;
        uint8 *temp = realloc(handle->read_buffer, sizeof(*handle->read_buffer) * new_size);
        if (temp == NULL) {
            fprintf(stderr, "Styx: Out of memory in styx_unpack_buffer_get\n");
            return FALSE;
        }
        handle->read_buffer = temp;
        memset(&handle->read_buffer[handle->read_buffer_used], 69, new_size - handle->read_buffer_used);
        handle->read_buffer_size = new_size;
    }
    if (handle->read_buffer_used < handle->read_buffer_size) {
        size = 0;
        if (handle->type == S_HT_FILE_READ) {
            size = fread(&handle->read_buffer[handle->read_buffer_used], (sizeof *handle->read_buffer), handle->read_buffer_size - handle->read_buffer_used, handle->file);
        }
        if (handle->type == S_HT_STREAMING_SERVER || handle->type == S_HT_STREAMING_CONNECTION) {
            size = styx_network_stream_receive(handle, &handle->read_buffer[handle->read_buffer_used], handle->read_buffer_size - handle->read_buffer_used);
        }
        if (size > 0) {
            handle->read_buffer_used += size;
            if (!handle->debug_header) {
                for (i = 0; styx_debug_magic_number[i] != 0 && i < handle->read_buffer_used; i++) {
                    if (handle->read_buffer[i] != styx_debug_magic_number[i]) {
                        break;
                    }
                }
                if (styx_debug_magic_number[i] == 0) {
                    handle->debug_descriptor = TRUE;
                    handle->read_buffer_pos += i;
                }
            }
            return TRUE;
        }
    }
    if (handle->type == S_HT_BUFFER) {
        handle->read_buffer_used = handle->read_buffer_pos;
    }
    return FALSE;
}

boolean styx_pack_buffer_clear(SHandle *handle)
{
    if (handle == NULL) {
        return FALSE;
    }

    if (handle->type == S_HT_FILE_WRITE) {
        if (handle->write_buffer_pos > 0 && handle->file != NULL) {
            size_t written;

            written = fwrite(handle->write_buffer, 1, handle->write_buffer_pos, handle->file);

            if (written != handle->write_buffer_pos) {
                fprintf(stderr, "Styx: fwrite failed or partial write occurred!\n");
                return FALSE;
            }
            handle->write_buffer_pos = 0;
        }
    }

    if (handle->type == S_HT_STREAMING_SERVER || handle->type == S_HT_STREAMING_CONNECTION) {
        int out    = styx_network_stream_send_force(handle);

        if (out == -1) {
            return FALSE;
        }
    }

    if (handle->write_buffer_pos + STYX_MINIMUM_WRITE_SPACE > handle->write_buffer_size) {
        size_t new_size = handle->write_buffer_size + STYX_MINIMUM_WRITE_SPACE;
        uint8 *temp;

        temp = realloc(handle->write_buffer, new_size);
        if (temp == NULL) {
            fprintf(stderr, "Styx: Out of memory during buffer expansion!\n");
            return FALSE;
        }

        handle->write_buffer = temp;
        handle->write_buffer_size = new_size;

        if (handle->type == S_HT_BUFFER) {
            handle->read_buffer = handle->write_buffer;
            handle->read_buffer_size = handle->write_buffer_size;
        }
    }
    return TRUE;
}

boolean styx_pack_buffer_add(SHandle *handle, const uint8 *data, uint length)
{
    uint pos = 0;

    if (handle == NULL || data == NULL) {
        return FALSE;
    }

    while (pos < length) {
        size_t left;
        size_t size;

        left = handle->write_buffer_size - handle->write_buffer_pos;
        size = (length - pos < left) ? (length - pos) : left;

        if (size > 0) {
            memcpy(&handle->write_buffer[handle->write_buffer_pos], &data[pos], size);
            handle->write_buffer_pos += size;
            pos += size;
        }

        if (handle->write_buffer_pos == handle->write_buffer_size) {
            if (!styx_pack_buffer_clear(handle)) {
                return FALSE; /* output no longer available */
            }
        }
    }

    return TRUE;
}
