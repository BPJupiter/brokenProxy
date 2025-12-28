#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "Callisto/callisto.h"
#include "cjson/cJSON.h"
#include "memory_usage.h"

#define PROXY_HOST "127.0.0.1"
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 8

static int proxy_printf(const char *format, ...);
#define printf(...) proxy_printf(__VA_ARGS__)

typedef struct
{
  int client_fd;
  struct sockaddr_in client_addr;
} client_info_t;

typedef struct
{
  int source_fd;
  int dest_fd;
  struct timespec start_time;
  int measure_latency;
} tunnel_args_t;

static pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_thread_count = 0;

short (*proxy_get_ip_addresses)(unsigned char *hostname, unsigned char ***answer_index) = NULL;
double (*proxy_traceroute)(const char *ip, char *out_buf,
                           size_t out_size) = NULL;
extern double rtt_cutoff;

void proxy_init(
  short (*hostname_resolver)(unsigned char *hostname, unsigned char ***answer_index),
  double (*tracert)(const char *ip, char *out_buf, size_t out_size))
{
  proxy_get_ip_addresses = hostname_resolver;
  proxy_traceroute = tracert;
}

void free_answers(unsigned char **answers, int n_ans)
{
  for (int i = 0; i < n_ans; i++)
    free(answers[i]);
  free(answers);
}

int host_port_from_url(const char *url, char *host, int *port)
{
  const char *http_pos = strstr(url, "http://");
  const char *https_pos = strstr(url, "https://");

  if (http_pos != NULL)
  {
    const char *temp = http_pos + 7;
    const char *slash = strchr(temp, '/');
    int host_len = slash ? (slash - temp) : strlen(temp);
    strncpy(host, temp, host_len);
    host[host_len] = '\0';
    *port = 80;
    return 0;
  }
  else if (https_pos != NULL)
  {
    const char *temp = https_pos + 8;
    const char *slash = strchr(temp, '/');
    int host_len = slash ? (slash - temp) : strlen(temp);
    strncpy(host, temp, host_len);
    host[host_len] = '\0';
    *port = 443;
    return 0;
  }
  else
  {
    // No protocol, parse as host:port
    const char *colon = strchr(url, ':');
    if (colon)
    {
      int host_len = colon - url;
      strncpy(host, url, host_len);
      host[host_len] = '\0';
      *port = atoi(colon + 1);
    }
    else
    {
      strcpy(host, url);
      *port = 80;
    }
    return 0;
  }
}

void get_text_file(char *filename, char *buffer, int client_fd)
{
  char path[64];
  strcpy(path, "./settings-page/");
  strcat(path, filename);
  FILE *fp = fopen(path, "rb");
  if (!fp)
  {
    printf("404 File not found");
    return;
  }
  fseek(fp, 0L, SEEK_END);
  long content_length = ftell(fp);
  rewind(fp);

  char *text_content = (char *)malloc(content_length);
  fread(text_content, 1, content_length, fp);
  fclose(fp);

  char extension[32];
  if (strstr(filename, ".html") != NULL)
  {
    strcpy(extension, "text/html");
  }
  else if (strstr(filename, ".css") != NULL)
  {
    strcpy(extension, "text/css");
  }
  else if (strstr(filename, ".json") != NULL)
  {
    strcpy(extension, "application/json");
  }
  else if (strstr(filename, ".js") != NULL)
  {
    strcpy(extension, "text/javascript");
  }

  snprintf(buffer, BUFFER_SIZE,
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %ld\r\n"
           "Connection: close\r\n"
           "\r\n"
           "%s",
           extension, content_length, text_content);
  send(client_fd, buffer, strlen(buffer), 0);
  free(text_content);
}

void get_favicon(char *buffer, int client_fd)
{
  FILE *fp = fopen("./settings-page/favicon.ico", "rb");
  if (!fp)
  {
    printf("404 File not found\n");
    return;
  }
  fseek(fp, 0L, SEEK_END);
  long content_length = ftell(fp);
  rewind(fp);

  char headers[256];
  int header_len = snprintf(headers, sizeof(headers),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: image/x-icon\r\n"
                            "Content-Length: %ld\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            content_length);
  send(client_fd, headers, header_len, 0);
  char file_buffer[4096];
  size_t bytes_read;
  while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0)
  {
    send(client_fd, file_buffer, bytes_read, 0);
  }

  fclose(fp);
}

void parse_settings(char *host, double *rtt)
{
  char *rtt_p = strstr(host, "rtt=");
  int rtt_i = atoi(rtt_p + 4);
  *rtt = (double)rtt_i;
}

void update_json_settings(char *host)
{
  double rtt;
  parse_settings(host, &rtt);
  FILE *fp = fopen("./settings-page/settings.json", "r");
  if (fp == NULL)
  {
    printf("Error opening settings.json file!\n");
    return;
  }

  char buffer[1024];
  int len = fread(buffer, 1, sizeof(buffer), fp);
  fclose(fp);

  cJSON *json = cJSON_Parse(buffer);
  if (json == NULL)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
      printf("Error: %s\n", error_ptr);
    }
    cJSON_Delete(json);
    return;
  }

  cJSON *settings = cJSON_GetObjectItemCaseSensitive(json, "settings");
  if (!cJSON_IsObject(settings))
  {
    printf("Error: settings is not an object!\n");
    cJSON_Delete(json);
    return;
  }
  cJSON *maxLatency = cJSON_GetObjectItemCaseSensitive(settings, "maxLatency");
  if (!cJSON_IsNumber(maxLatency))
  {
    printf("Error: maxLatency is not a number!\n");
    cJSON_Delete(json);
    return;
  }
  cJSON_SetNumberValue(maxLatency, rtt);
  char *new_json = cJSON_Print(json);
  fp = fopen("./settings-page/settings.json", "w");
  if (fp)
  {
    fputs(new_json, fp);
    fclose(fp);
  }
  free(new_json);
  cJSON_Delete(json);
  return;
}

// Tunnel data from source to destination
void *tunnel_data(void *arg)
{
  tunnel_args_t *args = (tunnel_args_t *)arg;
  char buffer[BUFFER_SIZE];
  int first_packet = 1;

  struct timeval timeout;
  timeout.tv_sec = 60;
  timeout.tv_usec = 0;
  setsockopt(args->source_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
             sizeof(timeout));

  while (1)
  {
    int n = recv(args->source_fd, buffer, BUFFER_SIZE, 0);
    if (n <= 0)
    {
      break;
    }

    if (first_packet && args->measure_latency)
    {
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);

      double elapsed_ms = (now.tv_sec - args->start_time.tv_sec) * 1000.0;
      elapsed_ms += (now.tv_nsec - args->start_time.tv_nsec) / 1000000.0;

      /*printf("\x1b[32m[Latency]\x1b[0m First data packet arrived after : %.2f "
             "ms\n",
             elapsed_ms);*/

      first_packet = 0;
    }

    int sent = 0;
    while (sent < n)
    {
      int s = send(args->dest_fd, buffer + sent, n - sent, 0);
      if (s <= 0)
      {
        return NULL;
      }
      sent += s;
    }
  }

  return NULL;
}

// Handle individual client connection
void *handle_client(void *arg)
{
  client_info_t *client_info = (client_info_t *)arg;
  int client_fd = client_info->client_fd;
  char buffer[BUFFER_SIZE];
  char host[512] = {0};
  int port = 80;
  int remote_fd = -1;
  char *url = NULL;

  pthread_mutex_lock(&thread_count_mutex);
  active_thread_count++;
  int current_count = active_thread_count;
  pthread_mutex_unlock(&thread_count_mutex);

  printf("\x1b[33m[Info]\x1b[0m ThreadCount: %d\n", current_count);
  printf("Thread %ld: %s KB\n", (long)arg, get_memory_usage_str_kb());

  // Receive initial request
  int n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
  if (n <= 0)
  {
    goto cleanup;
  }
  buffer[n] = '\0';

  // Parse first line - split by newline
  char *first_line_end = strchr(buffer, '\n');
  if (!first_line_end)
  {
    goto cleanup;
  }

  char first_line[1024];
  int first_line_len = first_line_end - buffer;
  strncpy(first_line, buffer, first_line_len);
  first_line[first_line_len] = '\0';

  // Parse: METHOD URL VERSION
  char method[16], url_buf[512], version[16];
  if (sscanf(first_line, "%s %s %s", method, url_buf, version) != 3)
  {
    goto cleanup;
  }

  url = url_buf;

  // Extract host and port from URL
  if (host_port_from_url(url, host, &port) != 0)
  {
    goto cleanup;
  }

  // Resolve hostname using custom DNS resolver
  unsigned char **answers;
  int n_ans = proxy_get_ip_addresses((unsigned char *)host, &answers);

  if (n_ans == (int)((unsigned short)-1))
  {
    printf("DNS RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
    goto cleanup;
  }

  if (n_ans == 0)
  {
    printf("\x1b[31m[Error]\x1b[0m Failed to resolve hostname: %s\n", host);
    goto cleanup;
  }

  char *destination_ip = (char *)answers[0];
  printf("%s resolved to %s\n", host, destination_ip);

  // Perform traceroute
  if (proxy_traceroute != NULL)
  {
    char tr_output[1024];
    double latency =
      proxy_traceroute(destination_ip, tr_output, sizeof(tr_output));
    if (latency > rtt_cutoff)
    {
      printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
      free_answers(answers, n_ans);
      goto cleanup;
    }
  }

  // Load proxy settings page if requested
  // This code sucks. But its short enough that i dont care.
  if (strstr(first_line, "GET") != NULL)
  {
    if (strcmp(host, "/") == 0)
    {
      get_text_file("index.html", buffer, client_fd);
    }
    else if (strcmp(host, "/style.css") == 0)
    {
      get_text_file("style.css", buffer, client_fd);
    }
    else if (strcmp(host, "/script.js") == 0)
    {
      get_text_file("script.js", buffer, client_fd);
    }
    else if (strcmp(host, "/settings.json") == 0)
    {
      get_text_file("settings.json", buffer, client_fd);
    }
    else if (strcmp(host, "/favicon.ico") == 0)
    {
      get_favicon(buffer, client_fd);
    }
    else if (strstr(host, "/settings") != NULL)
    {
      update_json_settings(host);
    }
    free_answers(answers, n_ans);
    goto cleanup;
  }

  // Connect to remote server
  remote_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (remote_fd < 0)
  {
    free_answers(answers, n_ans);
    goto cleanup;
  }

  struct sockaddr_in remote_addr;
  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(port);
  remote_addr.sin_addr.s_addr = inet_addr(destination_ip);

  if (connect(remote_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) <
      0)
  {
    printf("\x1b[31m[Error]\x1b[0m Failed to connect to %s:%d - %s\n",
           destination_ip, port, strerror(errno));
    free_answers(answers, n_ans);
    goto cleanup;
  }

  printf("Accepted request for: %s:%d\n", host, port);

  // Handle CONNECT method (HTTPS tunnel)
  if (strstr(first_line, "CONNECT") != NULL)
  {
    const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_fd, response, strlen(response), 0);
    printf("Client established connection!\n");
  }
  else
  {
    // Regular HTTP - forward the initial request
    send(remote_fd, buffer, n, 0);
    printf("Remote socket sent request!\n");
  }

  free_answers(answers, n_ans);

  // Create bidirectional tunnel
  pthread_t client_to_remote_thread, remote_to_client_thread;

  tunnel_args_t *c2r_args = malloc(sizeof(tunnel_args_t));
  c2r_args->source_fd = client_fd;
  c2r_args->dest_fd = remote_fd;
  c2r_args->measure_latency = 0;

  tunnel_args_t *r2c_args = malloc(sizeof(tunnel_args_t));
  r2c_args->source_fd = remote_fd;
  r2c_args->dest_fd = client_fd;
  r2c_args->measure_latency = 1;

  clock_gettime(CLOCK_MONOTONIC, &r2c_args->start_time);

  pthread_create(&client_to_remote_thread, NULL, tunnel_data, c2r_args);
  pthread_create(&remote_to_client_thread, NULL, tunnel_data, r2c_args);

  // Wait for both threads to complete
  pthread_join(client_to_remote_thread, NULL);
  pthread_join(remote_to_client_thread, NULL);

  free(c2r_args);
  free(r2c_args);

cleanup:
  if (client_fd >= 0)
    close(client_fd);
  if (remote_fd >= 0)
    close(remote_fd);
  free(client_info);

  pthread_mutex_lock(&thread_count_mutex);
  active_thread_count--;
  pthread_mutex_unlock(&thread_count_mutex);

  return NULL;
}

void update_proxy_settings()
{
  FILE *fp = fopen("./settings-page/settings.json", "r");
  if (fp == NULL)
  {
    printf("Error opening settings.json file!\n");
    return;
  }

  char buffer[1024];
  int len = fread(buffer, 1, sizeof(buffer), fp);
  fclose(fp);

  cJSON *json = cJSON_Parse(buffer);
  if (json == NULL)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
      printf("Error: %s\n", error_ptr);
    }
    cJSON_Delete(json);
    return;
  }

  cJSON *settings = cJSON_GetObjectItemCaseSensitive(json, "settings");
  if (!cJSON_IsObject(settings))
  {
    printf("Error: settings is not an object!\n");
    cJSON_Delete(json);
    return;
  }
  cJSON *maxLatency = cJSON_GetObjectItemCaseSensitive(settings, "maxLatency");
  if (!cJSON_IsNumber(maxLatency))
  {
    printf("Error: maxLatency is not a number!\n");
    cJSON_Delete(json);
    return;
  }
  rtt_cutoff = maxLatency->valuedouble;
  cJSON_Delete(json);
  return;
}

int proxy_start(int proxy_port)
{
  /*
   #ifdef C_MEMORY_DEBUG
     c_debug_memory_init(c_mem_mutex_lock, c_mem_mutex_unlock, &c_mem_mutex);
     signal(SIGINT, signal_handler);
   #endif
   */

  // Ignore SIGPIPE - prevents crash when writing to closed socket
  signal(SIGPIPE, SIG_IGN);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    perror("socket");
    exit(1);
  }

  // Set socket options
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(PROXY_HOST);
  addr.sin_port = htons(proxy_port);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    exit(1);
  }

  if (listen(server_fd, MAX_CLIENTS) < 0)
  {
    perror("listen");
    exit(1);
  }

  printf("Proxy server listening on %s:%d\n", PROXY_HOST, proxy_port);

  // Accept connections and spawn threads
  while (1)
  {
    printf("Memory (KB): %s\n", get_memory_usage_str_kb());
    client_info_t *client_info = malloc(sizeof(client_info_t));
    socklen_t client_len = sizeof(client_info->client_addr);

    update_proxy_settings();

    client_info->client_fd = accept(
      server_fd, (struct sockaddr *)&client_info->client_addr, &client_len);

    if (client_info->client_fd < 0)
    {
      perror("accept");
      free(client_info);
      continue;
    }

    printf("Accepted connection from %s:%d\n",
           inet_ntoa(client_info->client_addr.sin_addr),
           ntohs(client_info->client_addr.sin_port));

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attr, handle_client, client_info) != 0)
    {
      perror("pthread_create");
      close(client_info->client_fd);
      free(client_info);
    }

    pthread_attr_destroy(&attr);
  }

  close(server_fd);
  return 0;
}

int proxy_printf(const char *format, ...)
{
  va_list args;
  int ret;
  char *msg = "\x1b[35m[Proxy]\x1b[0m ";
  fprintf(stdout, "%s", msg);

  va_start(args, format);
  ret = vprintf(format, args);
  va_end(args);
  fflush(stdout);

  return ret;
}
