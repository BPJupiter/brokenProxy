#include "proxy.h"

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "cjson/cJSON.h"

#include "context/context.h"
#include "verify/verify.h"
#include "memory_usage.h"

#include <signal.h>
#include <stdarg.h>

#define BUFFER_SIZE (1 << 16)
#define UDP_HEADER_IPV4_LEN 10
#define IP_SIZE 4
#define MAX_CLIENTS 8
#define PROJECT_ROOT_FOLDER_NAME "brokenProxy"

uint16 proxy_port = 0;
extern volatile sig_atomic_t g_reload_settings;
void *g_settings_mutex = NULL;

enum socks {
	RESERVED = 0x00,
	VERSION4 = 0x04,
	VERSION5 = 0x05
};

enum socks_auth_methods {
	NOAUTH = 0x00,
	GSSAPI = 0x01,
	USERPASS = 0x02,
	NOMETHOD = 0xff
};

enum socks_auth_userpass {
	AUTH_OK = 0x00,
	AUTH_VERSION = 0x01,
	AUTH_FAIL = 0xff
};

enum socks_command {
	CONNECT = 0x01,
	BIND = 0x02,
	UDP_ASSOCIATE = 0x03
};

enum socks_command_type {
	IPV4 = 0x01,
	DOMAIN = 0x03,
	IPV6 = 0x04
};

enum socks_status {
	OK = 0x00,
	GENERAL_FAILURE = 0x01,
	NOT_ALLOWED = 0x02,
	NETWORK_UNREACHABLE = 0x03,
	HOST_UNREACHABLE = 0x04,
	CONNECTION_REFUSED = 0x05,
	TTL_EXPIRED = 0x06,
	UNSUPPORTED_CMD = 0x07,
	UNSUPPORTED_ADDRESS = 0x08
};

static int proxy_printf(const char *format, ...);
#define printf proxy_printf

static void setup_signals();
static void proxy_settings_init();
static void proxy_reload_settings();
static void thread_handler(void *arg);
static void handle_client(VSocket client_handle);
static int transfer_data(VSocket from_handle, VSocket to_handle);
static void handle_udp_associate(VSocket tcp_client_handle);

static void reload_config();

static void _restart(int signum);
static void _shutdown(int signum);

int proxy_start(int port)
{
	VSocket server_handle, client_handle;
	struct sockaddr_in server_addr;
	int opt = 1;

	setup_signals();
	proxy_port = port;

	server_handle = styx_socket_create(TRUE, proxy_port);
	styx_socket_assert(server_handle, "Server handle");
	setsockopt(server_handle, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	proxy_settings_init();

	printf("SOCKS5 Proxy listening on port: %d...\n", proxy_port);

	while (1) {
		if (g_reload_settings) {
			g_reload_settings = 0;
			proxy_reload_settings();
		}

		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(server_handle, &read_fds);

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int activity = select(server_handle + 1, &read_fds, NULL, NULL, &timeout);

		if (activity < 0) {
			continue;
		}

		if (activity == 0) {
			continue;
		}

		if (FD_ISSET(server_handle, &read_fds)) {
			client_handle = accept(server_handle, NULL, NULL);

			if (styx_socket_assert(client_handle, NULL)) {
				VSocket *temp_handle = malloc(sizeof(*temp_handle));
				*temp_handle = client_handle;

				EuropaThread thread;

				if ((thread = europa_thread_create(thread_handler, (void *)temp_handle, NULL)) < 0) {
					printf("Failed to create new thread.\n");
					free(temp_handle);
					styx_socket_destroy(client_handle);
				}
				else {
					europa_thread_detach(thread);
				}
			}
		}
	}
	styx_socket_destroy(server_handle);

	return 0;
}

void proxy_shutdown()
{

}

static void setup_signals()
{
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif
}

static void proxy_settings_init()
{
	if (!g_settings_mutex) {
		g_settings_mutex = europa_mutex_create();
	}
	proxy_reload_settings();
}

static void proxy_reload_settings()
{
	europa_mutex_lock(g_settings_mutex);
	reload_config();
	europa_mutex_unlock(g_settings_mutex);
	printf("Settings reloaded.\n");
}

static void thread_handler(void *arg)
{
	VSocket client_handle = *(VSocket *)arg;
	free(arg);
	handle_client(client_handle);
	return;
}

static void handle_client(VSocket client_handle)
{
	uint8 buf[256];

	if (recv(client_handle, buf, 256, 0) <= 0 || buf[0] != VERSION5) {
		styx_socket_destroy(client_handle); return;
	}

	uint8 greeting_reply[] = { 0x05, 0x00 };
	send(client_handle, greeting_reply, 2, 0);

	int bytes_read = recv(client_handle, buf, 256, 0);
	if (bytes_read < 4) {
		styx_socket_destroy(client_handle); return;
	}

	if (buf[1] == CONNECT) {
		struct sockaddr_in target_addr;
		memset(&target_addr, 0, sizeof(target_addr));
		target_addr.sin_family = AF_INET;
		char target_str[256];
		char target_ip[256];

		if (buf[3] == IPV4) {
			if (bytes_read < 10) { styx_socket_destroy(client_handle); return; }
			memcpy(&target_addr.sin_addr.s_addr, &buf[4], 4);
			memcpy(&target_addr.sin_port, &buf[8], 2);
			inet_ntop(AF_INET, &target_addr.sin_addr, target_str, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &target_addr.sin_addr, target_ip, INET_ADDRSTRLEN);
		}
		else if (buf[3] == DOMAIN) {
			int domain_len = buf[4];
			if (bytes_read < 5 + domain_len + 2) { styx_socket_destroy(client_handle); return; }

			char domain[256];
			memcpy(domain, &buf[5], domain_len);
			domain[domain_len] = '\0';
			c_text_copy(strlen(domain)+1, target_str, domain);

			memcpy(&target_addr.sin_port, &buf[5 + domain_len], 2);

			DnsResult dns_result;
			sharedContext_callback_execute_dnsResolve(&dns_result, target_str);
			if (dns_result.nAns == 0) {
				printf("DNS resolution failed for domain: %s\n", domain);
				styx_socket_destroy(client_handle); return;
			}
			/*
			inet_ntop(AF_INET, &dns_result.answers[0], target_ip, INET_ADDRSTRLEN);
			inet_pton(AF_INET, target_ip, &target_addr.sin_addr.s_addr);
			*/
			strncpy(target_ip, dns_result.answers[0], INET_ADDRSTRLEN - 1);
			target_ip[INET_ADDRSTRLEN - 1] = '\0';
			inet_pton(AF_INET, target_ip, &target_addr.sin_addr.s_addr);
			DnsResult_free(&dns_result);
		}
		else {
			printf("Unsupported ATYP: 0x%02x\n", buf[3]);
			styx_socket_destroy(client_handle); return;
		}

		double rtt_cutoff;
		sharedContext_var_get_maxRtt(&rtt_cutoff);

		printf("App requested connection to: %s:%d\n", target_str, ntohs(target_addr.sin_port));

		if (!verify_latency(target_ip)) {
			printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
			styx_socket_destroy(client_handle); return;
		}
		if (!verify_cable(target_ip)) {
			printf("Request uses disabled cable! Packet dropped!\n");
			styx_socket_destroy(client_handle); return;
		}

		VSocket target_handle = styx_socket_create(TRUE, 0);
		styx_socket_assert(target_handle, "Target handle");
		if (connect(target_handle, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
			printf("Failed to connect to target.\n");
			styx_socket_destroy(client_handle); styx_socket_destroy(target_handle); return;
		}

		uint8 connect_reply[10] = { 0x05, 0x00, 0x00, 0x01, 0,0,0,0, 0,0 };
		send(client_handle, connect_reply, 10, 0);
		printf("Connection established.\n");

		fd_set read_handles;
		VSocket max_handle = (client_handle > target_handle) ? client_handle : target_handle;

		while (1) {
			FD_ZERO(&read_handles);
			FD_SET(client_handle, &read_handles);
			FD_SET(target_handle, &read_handles);

			if (select(max_handle + 1, &read_handles, NULL, NULL, NULL) < 0) {
				if (errno == EINTR) continue;
				break;
			}

			if (FD_ISSET(client_handle, &read_handles)) {
				if (transfer_data(client_handle, target_handle) < 0) break;
			}
			if (FD_ISSET(target_handle, &read_handles)) {
				if (transfer_data(target_handle, client_handle) < 0) break;
			}
		}
		printf("Connection to %s is closed.\n", target_str);
		styx_socket_destroy(client_handle);
		styx_socket_destroy(target_handle);
	}
	else if (buf[1] == UDP_ASSOCIATE) {
		printf("App requested UDP Association.\n");
		handle_udp_associate(client_handle);
	}
	else {
		printf("Unsupported SOCKS command: 0x%02x\n", buf[1]);
		styx_socket_destroy(client_handle);
	}
}

static int transfer_data(VSocket from_handle, VSocket to_handle)
{
	char buffer[8192];
	int bytes_read = recv(from_handle, buffer, sizeof(buffer), 0);
	if (bytes_read <= 0) return -1;
	send(to_handle, buffer, bytes_read, 0);
	return bytes_read;
}

static void handle_udp_associate(VSocket tcp_client_handle)
{
	VSocket udp_handle = styx_socket_create(FALSE, 0);
	struct sockaddr_in proxy_udp_addr;
	socklen_t len = sizeof(proxy_udp_addr);

	getsockname(udp_handle, (struct sockaddr *)&proxy_udp_addr, &len);

	uint8 reply[10] = { 0x05, 0x00, 0x00, 0x01 };

	uint32 bind_ip = inet_addr("127.0.0.1");
	memcpy(&reply[4], &bind_ip, 4);
	memcpy(&reply[8], &proxy_udp_addr.sin_port, 2);

	send(tcp_client_handle, reply, 10, 0);
	printf("UDP ASSOCIATE active. Listening for datagrams on UDP port: %d\n", ntohs(proxy_udp_addr.sin_port));

	fd_set read_handles;
	VSocket max_handle = (tcp_client_handle > udp_handle) ? tcp_client_handle : udp_handle;
	uint8 buffer[65535];
	struct sockaddr_in client_udp_addr;
	int client_udp_known = 0;

	while (1) {
		FD_ZERO(&read_handles);
		FD_SET(tcp_client_handle, &read_handles);
		FD_SET(udp_handle, &read_handles);

		if (select(max_handle + 1, &read_handles, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) continue;
			break;
		}

		if (FD_ISSET(tcp_client_handle, &read_handles)) {
			int bytes = recv(tcp_client_handle, buffer, sizeof(buffer), 0);
			if (bytes <= 0) {
				printf("TCP control channel closed. Termination UDP association.\n");
				break;
			}
		}
		if (FD_ISSET(udp_handle, &read_handles)) {
			struct sockaddr_in sender_addr;
			socklen_t sender_len = sizeof(sender_addr);
			int bytes = recvfrom(udp_handle, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &sender_len);

			if (bytes <= 0) continue;

			if (!client_udp_known || (sender_addr.sin_addr.s_addr == client_udp_addr.sin_addr.s_addr)) {
				client_udp_addr = sender_addr;
				client_udp_known = 1;

				if (bytes < UDP_HEADER_IPV4_LEN || buffer[2] != 0x00 || buffer[3] != 0x01) {
					continue;
				}

				struct sockaddr_in target_addr;
				memset(&target_addr, 0, sizeof(target_addr));
				target_addr.sin_family = AF_INET;
				memcpy(&target_addr.sin_addr.s_addr, &buffer[4], 4);
				memcpy(&target_addr.sin_port, &buffer[8], 2);

				char target_ip[256];
				double rtt_cutoff;
				inet_ntop(AF_INET, &target_addr.sin_addr, target_ip, INET_ADDRSTRLEN);
				sharedContext_var_get_maxRtt(&rtt_cutoff);

				if (!verify_latency(target_ip)) {
					printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
					styx_socket_destroy(udp_handle); styx_socket_destroy(tcp_client_handle); return;
				}
				if (!verify_cable(target_ip)) {
					printf("Request uses disabled cable! Packet dropped!\n");
					styx_socket_destroy(udp_handle); styx_socket_destroy(tcp_client_handle); return;
				}

				sendto(udp_handle, buffer + UDP_HEADER_IPV4_LEN, bytes - UDP_HEADER_IPV4_LEN, 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
			}
			else {
				uint8 reply_packet[65535];
				reply_packet[0] = 0x00;
				reply_packet[1] = 0x00;
				reply_packet[2] = 0x00;
				reply_packet[3] = 0x01;

				memcpy(&reply_packet[4], &sender_addr.sin_addr.s_addr, 4);
				memcpy(&reply_packet[8], &sender_addr.sin_port, 2);

				memcpy(&reply_packet[10], buffer, bytes);

				sendto(udp_handle, reply_packet, bytes + UDP_HEADER_IPV4_LEN, 0, (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr));
			}
		}
	}
	styx_socket_destroy(udp_handle);
	styx_socket_destroy(tcp_client_handle);
}

static void reload_config()
{
	FILE *fp;
	char buffer[BUFFER_SIZE];
	cJSON *json;
	cJSON *settings;

	cJSON *pingObj;
	cJSON *tracertObj;
	cJSON *localDNSObj;

	cJSON *pingEnabled;
	cJSON *maxLatency;

	cJSON *trEnabled;

	cJSON *locDNSEnabled;

	fp = europa_project_root_fopen(PROJECT_ROOT_FOLDER_NAME, "settings/settings.json", "rb");
	if (fp == NULL)
	{
		talos_print_error("Error opening settings.json file!");
		return;
	}

	fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);

	json = cJSON_Parse(buffer);
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			printf("CJSON Error: %s: line %d file %s\n", error_ptr, __LINE__, __FILE__);
		}
		cJSON_Delete(json);
		return;
	}

	settings = cJSON_GetObjectItemCaseSensitive(json, "settings");
	pingObj = cJSON_GetObjectItemCaseSensitive(settings, "ping");
	tracertObj = cJSON_GetObjectItemCaseSensitive(settings, "traceroute");
	localDNSObj = cJSON_GetObjectItemCaseSensitive(settings, "localDNS");
	maxLatency = cJSON_GetObjectItemCaseSensitive(pingObj, "maxLatency");
	pingEnabled = cJSON_GetObjectItemCaseSensitive(pingObj, "pingEnabled");
	trEnabled = cJSON_GetObjectItemCaseSensitive(tracertObj, "trEnabled");
	locDNSEnabled = cJSON_GetObjectItemCaseSensitive(localDNSObj, "locDNSEnabled");

	if (!cJSON_IsObject(settings)
		|| (!cJSON_IsObject(pingObj)
			|| !cJSON_IsNumber(maxLatency)
			|| !cJSON_IsBool(pingEnabled))
		|| (!cJSON_IsObject(tracertObj)
			|| !cJSON_IsBool(trEnabled))
		|| (!cJSON_IsObject(localDNSObj)
			|| !cJSON_IsBool(locDNSEnabled))
		)
	{
		printf("Error: JSON structure invalid!\n");
		cJSON_Delete(json);
		return;
	}

	sharedContext_var_set_maxRtt(&maxLatency->valuedouble);
	sharedContext_callback_set_ping(cJSON_IsTrue(pingEnabled));
	sharedContext_callback_set_traceroute(cJSON_IsTrue(trEnabled));
	sharedContext_callback_set_dnsResolve(!cJSON_IsTrue(locDNSEnabled));

	cJSON_Delete(json);
	return;
}

static void _restart(int signum)
{
	g_reload_settings = 1;
}

static void _shutdown(int signum)
{
	_stop();
}

static int proxy_printf(const char *format, ...)
{
	va_list args;
	char msg[1024] = "\x1b[35m[Proxy]\x1b[0m ";
	int ret;

	strcat(msg, format);

	va_start(args, format);
	ret = vprintf(msg, args);
	va_end(args);
	fflush(stdout);

	return ret;
}