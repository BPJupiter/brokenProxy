#include <string.h>

#include "styx.h"

extern void styx_socket_destroy(VSocket socket);

void styx_free(SHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->type == S_HT_FILE_WRITE ||
        handle->type == S_HT_STREAMING_SERVER ||
        handle->type == S_HT_STREAMING_CONNECTION) {
        styx_pack_buffer_clear(handle);
    }

    if (handle->text_copy != NULL) {
        fprintf((FILE *)handle->text_copy, "styxFree\n");
        fclose((FILE *)handle->text_copy);
    }

    if (handle->read_buffer != NULL) {
        free(handle->read_buffer);
    }
    if (handle->type != S_HT_BUFFER && handle->write_buffer != NULL) {
        free(handle->write_buffer);
    }
    if (handle->file != NULL) {
        fclose((FILE *)handle->file);
    }

    if (handle->socket != INVALID_VSOCKET) {
        styx_socket_destroy(handle->socket);
    }

    if (handle->type == S_HT_FILE_WRITE && handle->file_name != NULL) {
        size_t len = strlen(handle->file_name);
        char *tmp = (char *)malloc(len + 5);

        if (tmp != NULL) {
            sprintf(tmp, "%s.tmp", handle->file_name);
            remove(handle->file_name);
            if (0 != rename(tmp, handle->file_name)) {
                char *alt = (char *)malloc(len + 32);
                if (alt != NULL) {
                    uint i;
                    sprintf(alt, "%s.emergency", handle->file_name);
                    for (i = 0; 0 != rename(tmp, alt) && i < 1024; i++) {
                        sprintf(alt, "%s.%uemergency", handle->file_name, i);
                    }
                    free(alt);
                }
            }
            free(tmp);
        }
    }
    free(handle);
}

SHandleType styx_type(SHandle *handle)
{
    return handle->type;
}
