#include "Styx/styx.h"
#include "Europa/europa.h"
#include "Talos/talos.h"

#include "cjson/cJSON.h"

#include "webserver.h"
#include "response_codes.h"

#include <string.h>

#define BUFFER_SIZE (1 << 16)
#define FIRST_LINE_MAX_LEN (1 << 10)
#define PROJECT_ROOT_FOLDER_NAME "brokenProxy"

static int send_all(VSocket socket, const char *buf, size_t len);

static int handle_http_request(VSocket client_handle);
static int send_resource(char resource[512], char *buffer, uint buffer_len, VSocket client_handle);
static int update_settings(char resource[512], const char *buffer, uint buffer_len, const VSocket client_handle);
static int initiate_shutdown(char resource[512], char *buffer, uint buffer_len, VSocket client_handle);
static int delete_datastore(char resource[512], char *buffer, uint buffer_len, VSocket client_handle);

int g_proxy_pid = -1;

static void restart_proxy(void);

int webserver_start(uint16 port, int proxy_pid)
{
	VSocket server_handle;
	int yes = 1;

	g_proxy_pid = proxy_pid;
	printf("proxy pid: %d\n", g_proxy_pid);

	server_handle = styx_socket_create(TRUE, port);
	if (!styx_socket_assert(server_handle, "Webserver socket creation: "))
		exit(1);

	setsockopt(server_handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

	while (1)
	{
		socklen_t client_len;
		VSocket client_handle;
		StyxSockaddrInet client_addr;

		client_len = sizeof client_addr;
		client_handle = accept(server_handle, (struct sockaddr*)&client_addr.Ipv4, &client_len);

		if (!styx_socket_assert(client_handle, "Client socket creation: "))
			exit(1);

		handle_http_request(client_handle);
	}
}

static int handle_http_request(VSocket client_handle)
{
	char request[BUFFER_SIZE] = { 0 };
	int bytes_recvd = 0;

	char first_line[FIRST_LINE_MAX_LEN] = { 0 };
	int first_line_len = 0;
	char *first_line_end = NULL;

	char method[16], resource[512], version[16];

	bytes_recvd = recv(client_handle, request, BUFFER_SIZE - 1, 0);
	if (bytes_recvd <= 0)
		return 1;

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
		send_resource(resource, request, sizeof(request), client_handle);
	}
	else if (strcmp(method, "PUT") == 0)
	{
		update_settings(resource, request, sizeof(request), client_handle);
	}
	else if (strcmp(method, "POST") == 0)
	{
		initiate_shutdown(resource, request, sizeof(request), client_handle);
	}
	else if (strcmp(method, "DELETE") == 0)
	{
		delete_datastore(resource, request, sizeof(request), client_handle);
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

static int send_resource(char resource[512], char *buffer, uint buffer_len, VSocket client_handle)
{
	FILE *fp = NULL;

	char path[256] = { 0 };
	char extension[64] = { 0 };

	long content_length;
	int header_length;
	size_t bytes_read;

	if (strcmp(resource, "/settings") == 0
		|| strcmp(resource, "/settings.json") == 0)
		strcpy(path, "settings/");
	else strcpy(path, "static/");

	strcat(path, resource + 1);
	if (strcmp(resource, "/") == 0)
		strcat(path, "index.html");

	strcpy(extension, "text/plain"); /* Default */
	if (strstr(path, ".html") != NULL)			strcpy(extension, "text/html");
	else if (strstr(path, ".css") != NULL)		strcpy(extension, "text/css");
	else if (strstr(path, ".json") != NULL)		strcpy(extension, "application/json");
	else if (strstr(path, ".js.map") != NULL)	strcpy(extension, "application/json");
	else if (strstr(path, ".js") != NULL)		strcpy(extension, "text/javascript");
	else if (strstr(path, ".ico") != NULL)		strcpy(extension, "image/x-icon");

	fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, path, "rb");
	if (!fp)
	{
		if (send_all(client_handle, HTTP_404_HEADER, strlen(HTTP_404_HEADER)) == -1)
		{
			printf("Socket fd: %d\n", client_handle);
			talos_print_error("Error sending header!\n");
		}
		if (send_all(client_handle, HTTP_404_HTML, strlen(HTTP_404_HTML)) == -1)
		{
			printf("Socket fd: %d\n", client_handle);
			talos_print_error("Error sending body!\n");
		}
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	content_length = ftell(fp);
	rewind(fp);

	header_length = sprintf(buffer,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %ld\r\n"
		"Connection: close\r\n"
		"\r\n",
		extension, content_length);

	if (send_all(client_handle, buffer, header_length) == -1)
	{
		printf("Socket fd: %d\n", client_handle);
		talos_print_error("Error sending header!\n");
		fclose(fp);
		return 1;
	}
	while ((bytes_read = fread(buffer, 1, buffer_len, fp)) > 0)
	{
		if (send_all(client_handle, buffer, bytes_read) == -1)
		{
			printf("Socket fd: %d\n", client_handle);
			talos_print_error("Error sending body!\n");
			fclose(fp);
			return 1;
		}
	}
	
	fclose(fp);
	return 0;
}

static int update_settings(char resource[512], const char *request, uint request_len, const VSocket client_handle)
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
	if (!body_start)
	{
		send_all(client_handle, HTTP_500_HEADER, strlen(HTTP_500_HEADER));
		send_all(client_handle, HTTP_500_HTML, strlen(HTTP_500_HTML));
		return 1;
	}
	body_start += 4;


	payload_json = cJSON_Parse(body_start);
	if (payload_json == NULL)
	{
		printf("Error: Failed to parse incoming POST JSON body.\n");
		send_all(client_handle, HTTP_500_HEADER, strlen(HTTP_500_HEADER));
		send_all(client_handle, HTTP_500_HTML, strlen(HTTP_500_HTML));
		return 1;
	}

	fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, "settings/settings.json", "rb");
	if (fp == NULL)
	{
		talos_print_error("Error opening settings.json file!");
		cJSON_Delete(payload_json);
		send_all(client_handle, HTTP_500_HEADER, strlen(HTTP_500_HEADER));
		send_all(client_handle, HTTP_500_HTML, strlen(HTTP_500_HTML));
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
		send_all(client_handle, HTTP_500_HEADER, strlen(HTTP_500_HEADER));
		send_all(client_handle, HTTP_500_HTML, strlen(HTTP_500_HTML));
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
		send_all(client_handle, HTTP_500_HEADER, strlen(HTTP_500_HEADER));
		send_all(client_handle, HTTP_500_HTML, strlen(HTTP_500_HTML));
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
	if (NULL != fp)
	{
		fputs(new_json, fp);
		fclose(fp);
		europa_path_rename("settings/settings.json.tmp", "settings/settings.json");
		restart_proxy();
		send_all(client_handle, HTTP_200_OK, strlen(HTTP_200_OK));
	}
	else
	{
		send_all(client_handle, HTTP_500_HEADER, strlen(HTTP_500_HEADER));
		send_all(client_handle, HTTP_500_HTML, strlen(HTTP_500_HTML));
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

static int initiate_shutdown(char resource[512], char *buffer, uint buffer_len, VSocket client_handle)
{
	UNUSED(resource);
	UNUSED(buffer);
	UNUSED(buffer_len);
	UNUSED(client_handle);
	send_all(client_handle, "HTTP/1.1 200 OK\r\n\r\nShutdown initiated.", 39);

	printf("\nTerminating proxy.\n");
	europa_process_terminate(g_proxy_pid);

	printf("\nWebserver exiting.\n");
	exit(0);

	return 0;
}
static int delete_datastore(char resource[512], char *buffer, uint buffer_len, VSocket client_handle)
{
	UNUSED(resource);
	UNUSED(buffer);
	UNUSED(buffer_len);
	UNUSED(client_handle);
	return 1;
	/* IMPLEMENT ME */
}

static int send_all(VSocket socket, const char *buf, size_t len)
{
	size_t total_sent = 0;
    size_t bytes_left = len;
    int n;

    while (total_sent < len)
    {
        n = send(socket, buf + total_sent, bytes_left, 0);
        if (n == -1) { return -1; }
        total_sent += n;
        bytes_left -= n;
    }
    return 0;
}

static void restart_proxy(void)
{
	europa_process_reload(g_proxy_pid);
}
