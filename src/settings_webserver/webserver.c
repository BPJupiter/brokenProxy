#include "Clay/clay.h"
#include "Europa/europa.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "cjson/cJSON.h"

#include "webserver.h"
#include "response_codes.h"

#include <string.h>
#include <errno.h>

#define BUFFER_SIZE (1 << 12)
#define FIRST_LINE_MAX_LEN (1 << 10)
#define PROJECT_ROOT_FOLDER_NAME "brokenProxy"

static int handle_http_request(SHandle *client_handle);
static int send_resource(SHandle *client_handle, char resource[512], char *buffer, uint buffer_len);
static int update_settings(SHandle *client_handle, char resource[512], const char *buffer, uint buffer_len);
static int initiate_shutdown(SHandle *client_handle, char resource[512], char *buffer, uint buffer_len);
static int delete_datastore(SHandle *client_handle, char resource[512], char *buffer, uint buffer_len);

int g_proxy_pid = -1;

static void restart_proxy(void);

int webserver_start(uint16 port, int proxy_pid)
{
    SHandle *server_handle, *client_handle;
    StyxNetworkAddress client_addr;

    g_proxy_pid = proxy_pid;
    printf("proxy pid: %d\n", g_proxy_pid);

    server_handle = styx_network_stream_address_create(NULL, port, FALSE);
    if (!server_handle) {
        fprintf(stderr, "Could not create server handle\n");
        exit(1);
    }

    while (1) {
        client_handle = styx_network_stream_wait_for_connection(server_handle, &client_addr);
        if (client_handle) {
            handle_http_request(client_handle);
        }
    }
    initiate_shutdown(client_handle, NULL, NULL, 0);
    printf("OH NO\n");
    return 0;
}

static int handle_http_request(SHandle *client_handle)
{
    char request[BUFFER_SIZE] = { 0 };
    int bytes_recvd = 0;

    char first_line[FIRST_LINE_MAX_LEN] = { 0 };
    int first_line_len = 0;
    char *first_line_end = NULL;

    char method[16], resource[512], version[16];

    boolean ready = FALSE;
    const int timeout_us = 50000;

    while (!ready) {
        ready = TRUE;
        styx_network_wait(&client_handle, &ready, NULL, 1, timeout_us);
    }

    bytes_recvd = styx_unpack_raw(client_handle, (uint8 *)request, BUFFER_SIZE, NULL);
    if (bytes_recvd <= 0) {
        return 1;
    }

    first_line_end = strchr(request, '\n');
    if (!first_line_end)
        return 1; /* Malformed request */

    first_line_len = first_line_end - request;
    if (first_line_len > FIRST_LINE_MAX_LEN)
        return 1; /* Buffer overflow */

    c_text_copy(first_line_len-1, first_line, request);
    first_line[first_line_len-1] = '\0';

    if (sscanf(first_line, "%15s %511s %15s", method, resource, version) != 3)
        return 1; /* Malformed request */

    c_check_null_termination(method, sizeof method);
    c_check_null_termination(resource, sizeof resource);
    c_check_null_termination(version, sizeof version);

    if (strcmp(method, "GET") == 0)
    {
        send_resource(client_handle, resource, request, sizeof(request));
    }
    else if (strcmp(method, "PUT") == 0)
    {
        update_settings(client_handle, resource, request, sizeof(request));
    }
    else if (strcmp(method, "POST") == 0)
    {
        initiate_shutdown(client_handle, resource, request, sizeof(request));
    }
    else if (strcmp(method, "DELETE") == 0)
    {
        delete_datastore(client_handle, resource, request, sizeof(request));
    }
    /* These should not be requested. */
    else if (strcmp(method, "HEAD") == 0)
    {

    }
    else if (strcmp(method, "CONNECT") == 0)
    {

    }
    else if (strcmp(method, "OPTIONS") == 0)
    {

    }
    else if (strcmp(method, "TRACE") == 0)
    {

    }
    else if (strcmp(method, "PATCH") == 0)
    {

    }
    else
    {

    }

    if (0) {
        printf("REQUEST:\n%s...\n", request);
        printf("\nFIRSTLINE:\n%s...\n", first_line);
        printf("\nMETHOD:\n%s...\n", method);
        printf("\nRESOURCE:\n%s...\n", resource);
        printf("\nVERSION:\n%s...\n", version);
    }

    return 0;
}

static int send_http_404(SHandle *handle)
{
    styx_pack_string(handle, HTTP_404_HEADER, NULL);
    styx_pack_string(handle, HTTP_404_HTML, NULL);
    return styx_network_stream_send_force(handle);
}

static int send_http_500(SHandle *handle)
{
    styx_pack_string(handle, HTTP_500_HEADER, NULL);
    styx_pack_string(handle, HTTP_500_HTML, NULL);
    return styx_network_stream_send_force(handle);
}

static int send_resource(SHandle *client_handle, char resource[512], char *buffer, uint buffer_len)
{
    FILE *fp = NULL;

    char path[256] = { 0 };
    char extension[64] = { 0 };
    boolean r = FALSE, w = FALSE;

    long content_length;
    size_t bytes_read;
    size_t header_len;

    if (strcmp(resource, "/settings") == 0
        || strcmp(resource, "/settings.json") == 0)
        strcpy(path, "settings/");
    else strcpy(path, "static/");

    strcat(path, resource + 1);
    if (strcmp(resource, "/") == 0)
        strcat(path, "index.html");

    strcpy(extension, "text/plain"); /* Default */
    if (strstr(path, ".html") != NULL)            strcpy(extension, "text/html");
    else if (strstr(path, ".css") != NULL)        strcpy(extension, "text/css");
    else if (strstr(path, ".json") != NULL)        strcpy(extension, "application/json");
    else if (strstr(path, ".js.map") != NULL)    strcpy(extension, "application/json");
    else if (strstr(path, ".js") != NULL)        strcpy(extension, "text/javascript");
    else if (strstr(path, ".ico") != NULL)        strcpy(extension, "image/x-icon");

    fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, path, "rb");
    if (!fp) {
        if (send_http_404(client_handle) == 0) {
            printf("Socket fd: %p\n", (void *)client_handle);
            talos_print_error("Error sending 404!\n");
        }
        return 1;
    }

    fseek(fp, 0L, SEEK_END);
    content_length = ftell(fp);
    rewind(fp);

    header_len = sprintf(buffer,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        extension, content_length);

    w = TRUE;
    while (!w) {
        w = TRUE;
        styx_network_wait(&client_handle, NULL, &w, 1, 50000);
    }

    if (styx_pack_raw(client_handle, (uint8 *)buffer, header_len, NULL) == 0) {
        printf("Socket fd: %p\n", (void *)client_handle);
        talos_print_error("Error sending header!\n");
        fclose(fp);
        return 1;
    }
    while ((bytes_read = fread(buffer, 1, buffer_len, fp)) > 0) {
        size_t chunk_sent = 0;

        while (chunk_sent < bytes_read) {
            size_t sent = styx_pack_raw(client_handle, (uint8 *)buffer, bytes_read, NULL);
            chunk_sent += sent;

            if (chunk_sent < bytes_read) {
                w = FALSE;
                while (!w) {
                    w = TRUE;
                    styx_network_wait(&client_handle, NULL, &w, 1, 50000);
                }
            }
        }
    }
    
    fclose(fp);
    return 0;
}

static int update_settings(SHandle *client_handle, char resource[512], const char *request, uint request_len)
{
    FILE *fp;
    char *buffer;
    char *body_start;
    char *new_json;

    cJSON *file_json;
    cJSON *payload_json;

    cJSON *settings;
    cJSON *pingObj;
    cJSON *tracertObj;
    cJSON *localDNSObj;

    cJSON *incomingCables;

    size_t bytes_read;

    UNUSED(request_len);

    if (strcmp(resource, "/settings") != 0 && strcmp(resource, "/settings.json") != 0)
    {
        printf("PUT but no settings requested.\n");
        return 1;
    }

    body_start = strstr(request, "\r\n\r\n");
    if (!body_start) {
        (void)send_http_500(client_handle);    
        return 1;
    }
    body_start += 4;


    payload_json = cJSON_Parse(body_start);
    if (payload_json == NULL)
    {
        printf("Error: Failed to parse incoming POST JSON body.\n");
        (void)send_http_500(client_handle);
        return 1;
    }

    fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, "settings/settings.json", "rb");
    if (fp == NULL)
    {
        talos_print_error("Error opening settings.json file!");
        cJSON_Delete(payload_json);
        (void)send_http_500(client_handle);
        return 1;
    }

    buffer = malloc((sizeof * buffer) * BUFFER_SIZE);
    talos_malloc_assert(buffer);

    bytes_read = fread(buffer, 1, BUFFER_SIZE, fp);
    buffer[bytes_read] = '\0';
    fclose(fp);
    fp = NULL;

    file_json = cJSON_Parse(buffer);
    if (file_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("CJSON Error: %s: line %d file %s\n", error_ptr, __LINE__, __FILE__);
        }
        cJSON_Delete(payload_json);
        (void)send_http_500(client_handle);
        return 1;
    }

    settings = cJSON_GetObjectItemCaseSensitive(file_json, "settings");
    pingObj = cJSON_GetObjectItemCaseSensitive(settings, "ping");
    tracertObj = cJSON_GetObjectItemCaseSensitive(settings, "traceroute");
    localDNSObj = cJSON_GetObjectItemCaseSensitive(settings, "localDNS");

    if (!cJSON_IsObject(settings)
        || !cJSON_IsObject(pingObj)
        || !cJSON_IsObject(tracertObj)
        || !cJSON_IsObject(localDNSObj)
        )
    {
        printf("Error: JSON structure invalid!\n");
        cJSON_Delete(file_json);
        cJSON_Delete(payload_json);
        (void)send_http_500(client_handle);
        return 1;
    }

    cJSON_ReplaceItemInObject(pingObj, "maxLatency",
        cJSON_CreateNumber(cJSON_GetObjectItem(payload_json, "maxLatency")->valuedouble));
    cJSON_ReplaceItemInObject(pingObj, "pingEnabled",
        cJSON_CreateBool(cJSON_GetObjectItem(payload_json, "pingEnabled")->valueint));
    cJSON_ReplaceItemInObject(tracertObj, "trEnabled",
        cJSON_CreateBool(cJSON_GetObjectItem(payload_json, "trEnabled")->valueint));
    cJSON_ReplaceItemInObject(localDNSObj, "locDNSEnabled",
        cJSON_CreateBool(cJSON_GetObjectItem(payload_json, "locDNSEnabled")->valueint));

    incomingCables = cJSON_GetObjectItem(payload_json, "disabledCables");

    if (incomingCables != NULL && cJSON_IsArray(incomingCables))
    {
        if (cJSON_HasObjectItem(settings, "disabledCables"))
        {
            cJSON_DeleteItemFromObject(settings, "disabledCables");
        }
        cJSON_AddItemToObject(settings, "disabledCables", cJSON_Duplicate(incomingCables, 1));
    }

    new_json = cJSON_Print(file_json);
    fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, "settings/settings.json.tmp", "w");
    if (NULL != fp) {
        fputs(new_json, fp);
        fclose(fp);
        europa_path_rename("settings/settings.json.tmp", "settings/settings.json");
        restart_proxy();
        styx_pack_string(client_handle, HTTP_200_OK, NULL);
        styx_network_stream_send_force(client_handle);
    }
    else {
        (void)send_http_500(client_handle);
    }

    if (new_json)
    {
#ifdef C_MEMORY_DEBUG
        c_no_debug_free(new_json);
#else
        free(new_json);
#endif
    }
    cJSON_Delete(file_json);
    cJSON_Delete(payload_json);
    free(buffer);
    return 0;
}

static int initiate_shutdown(SHandle *client_handle, char resource[512], char *buffer, uint buffer_len)
{
    UNUSED(resource);
    UNUSED(buffer);
    UNUSED(buffer_len);
    styx_pack_string(client_handle, "HTTP/1.1 200 OK\r\n\r\nShutdown initiated.", NULL);
    styx_network_stream_send_force(client_handle);

    printf("\nTerminating proxy.\n");
    europa_process_terminate(g_proxy_pid);

#ifdef C_MEMORY_DEBUG
    c_debug_mem_print(0);
#endif
    
    printf("\nWebserver exiting.\n");
    exit(0);

    return 0;
}
static int delete_datastore(SHandle *client_handle, char resource[512], char *buffer, uint buffer_len)
{
    UNUSED(resource);
    UNUSED(buffer);
    UNUSED(buffer_len);
    UNUSED(client_handle);
    return 1;
    /* IMPLEMENT ME */
}

static void restart_proxy(void)
{
    europa_process_reload(g_proxy_pid);
}
