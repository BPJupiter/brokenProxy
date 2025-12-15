#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "Callisto/callisto.h"
#include "memory_usage.h"

#define PROXY_HOST "127.0.0.1"
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 8

static int proxy_printf(const char *format, ...);
#define printf(...) proxy_printf(__VA_ARGS__)

typedef struct {
  int client_fd;
  struct sockaddr_in client_addr;
} client_info_t;

typedef struct {
  int source_fd;
  int dest_fd;
  struct timespec start_time;
  int measure_latency;
} tunnel_args_t;

#ifdef C_MEMORY_DEBUG
  void c_mem_mutex_lock(void* mutex) {
    pthread_mutex_lock((pthread_mutex_t*)mutex);
  }

  void c_mem_mutex_unlock(void* mutex) {
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
  }

  void signal_handler(int sig) {
    printf("Caught signal %d\n", sig);
    //c_debug_memory();
    c_debug_mem_print(0);
    exit(sig);
  }
#endif

static pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_thread_count = 0;

static pthread_mutex_t c_mem_mutex = PTHREAD_MUTEX_INITIALIZER;

unsigned short (*proxy_get_ip_addresses)(unsigned char* hostname, unsigned char*** answer_index) = NULL;
double (*proxy_traceroute)(const char* ip, char* out_buf, size_t out_size) = NULL;
double proxy_rtt_cutoff = 9999.0f;

void proxy_init(unsigned short (*hostname_resolver)(unsigned char* hostname, unsigned char*** answer_index), double (*tracert)(const char* ip, char* out_buf, size_t out_size), double rtt_cutoff)
{
  proxy_get_ip_addresses = hostname_resolver;
  proxy_traceroute = tracert;
  proxy_rtt_cutoff = rtt_cutoff;
}

int host_port_from_url(const char *url, char *host, int *port)
{
  const char *http_pos = strstr(url, "http://");
  const char *https_pos = strstr(url, "https://");

  if (http_pos != NULL) {
    const char *temp = http_pos + 7;
    const char *slash = strchr(temp, '/');
    int host_len = slash ? (slash - temp) : strlen(temp);
    strncpy(host, temp, host_len);
    host[host_len] = '\0';
    *port = 80;
    return 0;
  } else if (https_pos != NULL) {
    const char *temp = https_pos + 8;
    const char *slash = strchr(temp, '/');
    int host_len = slash ? (slash - temp) : strlen(temp);
    strncpy(host, temp, host_len);
    host[host_len] = '\0';
    *port = 443;
    return 0;
  } else {
    // No protocol, parse as host:port
    const char *colon = strchr(url, ':');
    if (colon) {
      int host_len = colon - url;
      strncpy(host, url, host_len);
      host[host_len] = '\0';
      *port = atoi(colon + 1);
    } else {
      strcpy(host, url);
      *port = 80;
    }
    return 0;
  }
}

// Tunnel data from source to destination
void *tunnel_data(void *arg) {
  tunnel_args_t *args = (tunnel_args_t *)arg;
  char buffer[BUFFER_SIZE];
  int first_packet = 1;

  struct timeval timeout;
  timeout.tv_sec = 15;
  timeout.tv_usec = 0;
  setsockopt(args->source_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  while (1) {
    int n = recv(args->source_fd, buffer, BUFFER_SIZE, 0);
    if (n <= 0) {
      break;
    }

    if (first_packet && args->measure_latency)
    {
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);

      double elapsed_ms = (now.tv_sec - args->start_time.tv_sec) * 1000.0;
      elapsed_ms += (now.tv_nsec - args->start_time.tv_nsec) / 1000000.0;

      printf("\x1b[32m[Latency]\x1b[0m First data packet arrived after : %.2f ms\n", elapsed_ms);

      first_packet = 0;
    }

    int sent = 0;
    while (sent < n) {
      int s = send(args->dest_fd, buffer + sent, n - sent, 0);
      if (s <= 0) {
        free(args);
        return NULL;
      }
      sent += s;
    }
  }

  free(args);
  return NULL;
}

// Handle individual client connection
void *handle_client(void *arg) {
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
  if (n <= 0) {
    goto cleanup;
  }
  buffer[n] = '\0';

  // Parse first line - split by newline
  char *first_line_end = strchr(buffer, '\n');
  if (!first_line_end) {
    goto cleanup;
  }

  char first_line[1024];
  int first_line_len = first_line_end - buffer;
  strncpy(first_line, buffer, first_line_len);
  first_line[first_line_len] = '\0';

  // Parse: METHOD URL VERSION
  char method[16], url_buf[512], version[16];
  if (sscanf(first_line, "%s %s %s", method, url_buf, version) != 3) {
    goto cleanup;
  }

  url = url_buf;

  // Extract host and port from URL
  if (host_port_from_url(url, host, &port) != 0) {
    goto cleanup;
  }

  // Resolve hostname using custom DNS resolver
  unsigned char **answers;
  int n_ans = proxy_get_ip_addresses((unsigned char *)host, &answers);

  if (n_ans == (int)((unsigned short)-1))
  {
    printf("DNS RTT Exceeded %.2lf ms! Packet dropped!\n", proxy_rtt_cutoff);
    goto cleanup;
  }

  if (n_ans == 0) {
    printf("\x1b[31m[Error]\x1b[0m Failed to resolve hostname: %s\n", host);
    goto cleanup;
  }

  char *destination_ip = (char *)answers[0];
  printf("%s resolved to %s\n", host, destination_ip);

  // Perform traceroute
  if (proxy_traceroute != NULL)
  {
    char tr_output[1024];
    double latency = proxy_traceroute(destination_ip, tr_output, sizeof(tr_output));
    if (latency > proxy_rtt_cutoff) {
      printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", proxy_rtt_cutoff);
      for (int i = 0; i < n_ans; i++) free(answers[i]);
      free(answers);
      goto cleanup;
    }
  }

  // Connect to remote server
  remote_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (remote_fd < 0) {
    for (int i = 0; i < n_ans; i++) free(answers[i]);
    free(answers);
    goto cleanup;
  }

  struct sockaddr_in remote_addr;
  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(port);
  remote_addr.sin_addr.s_addr = inet_addr(destination_ip);

  if (connect(remote_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
    printf("\x1b[31m[Error]\x1b[0m Failed to connect to %s:%d - %s\n", 
           destination_ip, port, strerror(errno));
    for (int i = 0; i < n_ans; i++) free(answers[i]);
    free(answers);
    goto cleanup;
  }

  printf("Accepted request for: %s:%d\n", host, port);

  // Handle CONNECT method (HTTPS tunnel)
  if (strstr(first_line, "CONNECT") != NULL) {
    const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_fd, response, strlen(response), 0);
    printf("Client established connection!\n");
  } else {
    // Regular HTTP - forward the initial request
    send(remote_fd, buffer, n, 0);
    printf("Remote socket sent request!\n");
  }

  // Free DNS answers
  for (int i = 0; i < n_ans; i++) free(answers[i]);
  free(answers);

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

cleanup:
  if (client_fd >= 0) close(client_fd);
  if (remote_fd >= 0) close(remote_fd);
  free(client_info);

  pthread_mutex_lock(&thread_count_mutex);
  active_thread_count--;
  pthread_mutex_unlock(&thread_count_mutex);

  return NULL;
}

int proxy_start(int proxy_port) {
  #ifdef C_MEMORY_DEBUG
  c_debug_memory_init(c_mem_mutex_lock, c_mem_mutex_unlock, &c_mem_mutex);
  signal(SIGINT, signal_handler);
  #endif

  // Ignore SIGPIPE - prevents crash when writing to closed socket
  signal(SIGPIPE, SIG_IGN);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
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

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(server_fd, MAX_CLIENTS) < 0) {
    perror("listen");
    exit(1);
  }

  printf("Proxy server listening on %s:%d\n", PROXY_HOST, proxy_port);

  // Accept connections and spawn threads
  while (1) {
    printf("Memory (KB): %s\n", get_memory_usage_str_kb());
    client_info_t *client_info = malloc(sizeof(client_info_t));
    socklen_t client_len = sizeof(client_info->client_addr);

    client_info->client_fd = accept(server_fd, 
                                    (struct sockaddr *)&client_info->client_addr, 
                                    &client_len);

    if (client_info->client_fd < 0) {
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

    if (pthread_create(&thread, &attr, handle_client, client_info) != 0) {
      perror("pthread_create");
      close(client_info->client_fd);
      free(client_info);
    }

    pthread_attr_destroy(&attr);
  }

  close(server_fd);
  return 0;
}

int proxy_printf(const char *format, ...) {
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
