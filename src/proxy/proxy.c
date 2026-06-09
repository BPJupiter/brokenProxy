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
#include <errno.h>

#if defined(_WIN32)
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif

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

/* 
* The below is a brainstorm for how I want to restructure my measurement data structure to:
* 1. Increase ease of use
* 2. Reduce data duplication
* 3. Easily map to SQL entries
*/

#define MAX_MEASUREMENTS (1 << 16)
typedef uint midx;

typedef enum {
	MT_PING,
	MT_TRACEROUTE,
	MT_COUNT
} MeasurementType;

typedef enum {
	MF_NIL = (1 << 0),
	MF_PING = (1 << 1),
	MF_TRACEROUTE = (1 << 2),
	MF_MAX_FLAGS = (1 << 31)
} MeasurementFlags;

typedef enum {
	RB_NIL = 0,
	RB_DNS,
	RB_HOP,
	RB_HOST,
} ReachabilityBlame;

typedef enum {
	RT_NIL = 0,
	RT_CABLE_BROKEN,
	RT_EXCEEDS_MAX_LATENCY,
	RT_REACHABLE,
} ReachabilityType;

typedef struct {
	union {
		uint32 v4;
		uint8 v6[16];
	} address; /* objective*/
	char domain[256]; /* objective */
	boolean is_ipv6; /* objective */
	float rtt_min; /* subjective */
	float rtt_avg;
	float rtt_max;
	float rtt_stddev;
	uint8 rtt_samples;
	time_t timestamp[MT_COUNT];

	midx hops[32]; /* objective */
	uint hop_count; /* objective */

	MeasurementFlags measurements_performed; /* objective */
	struct {
		ReachabilityType type;
		struct {
			ReachabilityBlame reason;
			midx host;
		} blame;
	} reachability;
} IpMeasurement;

/* SQL QUESTION:
 * Do I even bother with this? Given how fast an SQLite in-memory database can be.
 * Turns out there is the SQLite Online Backup API that allows syncing between in-memory and on-disk databases.
 * Multi-producer, single consumer queue. Dozens of reader threads, one writer thread, reader threads queue data for the writer thread to batch.
 * SQLite WAL mode?
 * Would end up puttings this in data/data.h
*/
typedef struct {
	IpMeasurement	ip_measurements[MAX_MEASUREMENTS];
	boolean			used[MAX_MEASUREMENTS]; /* may want to move away from C89 to allow for 1 bit array ? Or just use an int array with bitmasking */
	midx			next_empty_slot; /* 0 slot is nil */
	
} Measurements;

midx measurement_add(IpMeasurement measurement);
void meausrement_rem(midx idx);
IpMeasurement *measurement_get(midx idx);

static int proxy_printf(const char *format, ...);
#define printf proxy_printf

static void setup_signals(void);
static void proxy_settings_init(void);
static void proxy_reload_settings(void);
static void thread_handler(void *arg);
static void handle_client(SHandle *client_handle);
static void handle_tcp_connect(SHandle *client_handle);
static int transfer_data(SHandle *from_handle, SHandle *to_handle);
static void handle_udp_associate(SHandle *tcp_client_handle);

static void reload_config(void);

static void _restart(int signum);
static void _shutdown(int signum);

//#define ENABLE_BACKTRACE_PRINTING

int proxy_start(uint16 port)
{
	SHandle *server_handle, *client_handle;
	int opt = 1;
#ifdef ENABLE_BACKTRACE_PRINTING
	printf("%s\n", __FUNCTION__);
#endif

	setup_signals();
	proxy_port = port;

	server_handle = styx_network_stream_address_create(NULL, proxy_port, FALSE);
	if (!server_handle) {
		printf("Could not create server_handle\n");
		exit(1);
	}

	proxy_settings_init();

	printf("SOCKS5 Proxy listening on port: %d...\n", proxy_port);

	while (1) {
		client_handle = styx_network_stream_wait_for_connection(server_handle, NULL);
		if (g_reload_settings) {
			g_reload_settings = 0;
			proxy_reload_settings();
		}
		if (client_handle) {
			EuropaThread thread;
			if ((thread = europa_thread_create(thread_handler, client_handle, NULL)) < 0) {
				printf("Failed to create new thread.\n");
				styx_free(client_handle);
			}
			else {
				europa_thread_detach(thread);
			}
		}
	}
	styx_free(server_handle);

	return 0;
}

void proxy_shutdown(void)
{

}

static void setup_signals(void)
{
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif
}

static void proxy_settings_init(void)
{
	if (!g_settings_mutex) {
		g_settings_mutex = europa_mutex_create();
	}
	proxy_reload_settings();
}

static void proxy_reload_settings(void)
{
	europa_mutex_lock(g_settings_mutex);
	reload_config();
	europa_mutex_unlock(g_settings_mutex);
	printf("Settings reloaded.\n");
}

static void thread_handler(void *arg)
{
#ifdef ENABLE_BACKTRACE_PRINTING
	printf("%s\n", __FUNCTION__);
#endif	
	handle_client((SHandle *)arg);
	return;
}

static void handle_client(SHandle *client_handle)
{
	uint8 version, nmethods, command;
	boolean r = FALSE, w = FALSE;
	uint i;

#ifdef ENABLE_BACKTRACE_PRINTING
	printf("%s\n", __FUNCTION__);
#endif

	r = FALSE;
	while (!r) {
		r = TRUE;
		styx_network_wait(&client_handle, &r, NULL, 1, 1000);
	}

	version = styx_unpack_uint8(client_handle, "VERSION5");
	nmethods = styx_unpack_uint8(client_handle, "NMETHODS");
	for (i = 0; i < nmethods; i++) {
		(void)styx_unpack_uint8(client_handle, "METHODS");
	}

	if (version != VERSION5) {
		goto bad;
	}

	styx_pack_uint8(client_handle, VERSION5, "VERSION5");
	styx_pack_uint8(client_handle, NOAUTH, "NOAUTH");
	if (styx_network_stream_send_force(client_handle) != 2) {
		printf("Failed to send SOCKS5 resp\n");
		goto bad;
	}

	r = FALSE;
	while (!r) {
		r = TRUE;
		styx_network_wait(&client_handle, &r, NULL, 1, 1000);
	}

	version = styx_unpack_uint8(client_handle, "VERSION5");
	if (version != VERSION5) {
		goto bad;
	}

	command = styx_unpack_uint8(client_handle, "CMD");

	switch (command)
	{
		case CONNECT:
			printf("App requested TCP Connection.\n");
			handle_tcp_connect(client_handle);
		break;
		case UDP_ASSOCIATE:
			printf("App requested UDP Association.\n");
			handle_udp_associate(client_handle);
		break;
		default:
			printf("Unsupported SOCKS command: 0x%02x\n", command);
			goto bad;
		break;
	}
bad:
	return;
}

static boolean target_addr_get_from_buffer(SHandle *client_handle, StyxNetworkAddress *target_addr, char *target_ip_str, char *target_domain)
{
	uint i;
	uint8 atyp;

	(void)styx_unpack_uint8(client_handle, NULL);
	atyp = styx_unpack_uint8(client_handle, "ATYP");

	switch (atyp) {
		case IPV4:
			target_addr->is_ipv6 = FALSE;
			target_addr->ip.v4 = styx_unpack_uint32(client_handle, "DST.ADDR");
			target_addr->port = styx_unpack_uint16(client_handle, "DST.PORT");
		break;
		case DOMAIN:
		{
			uint8 domain_len = styx_unpack_uint8(client_handle, "LEN");

			for (i = 0; i < domain_len; i++) {
				target_domain[i] = styx_unpack_uint8(client_handle, NULL);
			}
			target_domain[domain_len] = '\0';

			target_addr->port = styx_unpack_uint16(client_handle, "DST.PORT");

			if (!styx_network_address_lookup(target_addr, target_domain, target_addr->port, NULL)) {
				printf("DNS resolution failed for domain: %s\n", target_domain);
				return FALSE;
			}
		}
		break;
		default:
			printf("Unsupported ATYP: 0x%02x\n", atyp);
			return FALSE;
		break;
	}
	if (target_addr->is_ipv6) {
		inet_ntop(AF_INET6, &target_addr->ip.v6, target_ip_str, INET6_ADDRSTRLEN);
	}
	else {
		inet_ntop(AF_INET, &target_addr->ip.v4, target_ip_str, INET_ADDRSTRLEN);
	}
	return TRUE;
}

static void handle_tcp_connect(SHandle *client_handle)
{
	SHandle *target_handle = NULL;
	SHandle *handles[2];
	boolean read_ready[2] = {FALSE, FALSE};
	StyxNetworkAddress target_addr;
	double rtt_cutoff;
	uint8 rep = OK;
	boolean r = FALSE, w = FALSE;

	char target_ip_str[64];
	char target_domain[256];

	sharedContext_var_get_maxRtt(&rtt_cutoff);

	if (!target_addr_get_from_buffer(client_handle, &target_addr, target_ip_str, target_domain)) {
		goto bad;
	}

	printf("App requested connection to: %s:%d\n", target_domain, target_addr.port);

	if (!verify_latency(target_ip_str)) {
		printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
		rep = TTL_EXPIRED;
	}
	if (!verify_cable(target_ip_str)) {
		printf("Request uses disabled cable! Packet dropped!\n");
		rep = NETWORK_UNREACHABLE;
	}

	target_handle = styx_network_stream_ip_create(target_addr);
	if (target_handle == NULL) {
		printf("Failed to connect to target.\n");
		rep = GENERAL_FAILURE;
	}

	styx_pack_uint8(client_handle, VERSION5, "VERSION5");
	styx_pack_uint8(client_handle, rep, "REP");
	styx_pack_uint8(client_handle, 0x00, "RSV");
	styx_pack_uint8(client_handle, IPV4, "ATYP");
	styx_pack_uint32(client_handle, target_addr.ip.v4, "BND.ADDR");
	styx_pack_uint16(client_handle, target_addr.port, "BND.PORT");
	styx_network_stream_send_force(client_handle);

	if (rep != OK) {
		goto bad;
	}

	printf("Connection established.\n");

	handles[0] = client_handle;
	handles[1] = target_handle;
	read_ready[0] = FALSE;
	read_ready[1] = FALSE;
	while (1) {
		read_ready[0] = TRUE;
		read_ready[1] = TRUE;	
		if (styx_network_wait(handles, read_ready, NULL, 2, 100000) < 0) {
			break;
		}
		if (read_ready[0]) {
			if (transfer_data(client_handle, target_handle) < 0) {
				break;
			}
		}
		if (read_ready[1]) {
			if (transfer_data(target_handle, client_handle) < 0) {
				break;
			}
		}
	}
	printf("Connection to %s is closed.\n", target_domain);
	styx_free(client_handle);
	styx_free(target_handle);
	return;
bad:
	if (client_handle) {
		styx_free(client_handle);
	}
	if (target_handle) {
		styx_free(target_handle);
	}
	return;
}

static int transfer_data(SHandle *from_handle, SHandle *to_handle)
{
	uint8 buffer[8192];
	uint64 bytes_written;
	uint64 bytes_read;
	
	bytes_read = styx_unpack_raw(from_handle, buffer, sizeof(buffer), NULL);

	if (bytes_read <= 0) {
		return -1;
	}

	bytes_written = styx_pack_raw(to_handle, buffer, bytes_read, NULL);
	styx_network_stream_send_force(to_handle);

	return bytes_written;
}

static void handle_udp_associate(SHandle *client_handle)
{
	uint8 buffer[65535];
	SHandle *proxy_handle;
	StyxNetworkAddress bind_addr, peer_addr, sender_addr;
	uint64 bytes;
	boolean peer_known = FALSE;
	boolean r = FALSE;

	proxy_handle = styx_network_datagram_create(13407, FALSE);
	if (!proxy_handle) {
		goto cleanup;
	}

	//bind_addr.ip.v4 = proxy_handle->ip.v4;
	bind_addr.port = proxy_handle->port;

	styx_pack_uint8(client_handle, VERSION5, "VERSION5");
	styx_pack_uint8(client_handle, OK, "OK");
	styx_pack_uint8(client_handle, 0x00, "RSV");
	styx_pack_uint8(client_handle, IPV4, "ATYP");
	styx_pack_uint32(client_handle, 0, "BND.ADDR");
	styx_pack_uint16(client_handle, bind_addr.port, "BND.PORT");

	styx_network_stream_send_force(client_handle);
	printf("UDP ASSOCIATE active. Listening for datagrams on UDP port: %d\n", bind_addr.port);

	r = FALSE;
	while (1) {
		r = TRUE;
		if (styx_network_wait(&client_handle, &r, NULL, 1, 1000) < 0) {
			break;
		}

		if (r) {
			printf("TCP control channel closed or received unexpected data. Terminating UDP ASSOCIATE.\n");
			break;
		}

		bytes = styx_network_receive(proxy_handle, &sender_addr);
		if (bytes <= 0) {
			continue;
		}

		if (!peer_known || (sender_addr.ip.v4 == peer_addr.ip.v4)) {
			StyxNetworkAddress target_addr;
			uint64 payload;
			char target_ip_str[64];
			double rtt_cutoff;
			uint8 frag;
			uint8 atyp;

			peer_addr = sender_addr;
			peer_known = TRUE;

			(void)styx_unpack_uint16(proxy_handle, NULL);
			frag = styx_unpack_uint8(proxy_handle, NULL);
			atyp = styx_unpack_uint8(proxy_handle, NULL);

			if (frag != 0x00 || atyp != IPV4) {
				continue;
			}

			target_addr.is_ipv6 = FALSE;
			target_addr.ip.v4 = styx_unpack_uint32(proxy_handle, NULL);
			target_addr.port = styx_unpack_uint16(proxy_handle, NULL);

			inet_ntop(AF_INET, &target_addr.ip.v4, target_ip_str, INET_ADDRSTRLEN);
			sharedContext_var_get_maxRtt(&rtt_cutoff);

			if (!verify_latency(target_ip_str)) {
				printf("Request RTT Exceeded %.2lf ms! Packet dropped!\n", rtt_cutoff);
				break;
			}
			if (!verify_cable(target_ip_str)) {
				printf("Request uses disabled cable! Packet dropped!\n");
				break;
			}

			payload = bytes - UDP_HEADER_IPV4_LEN;
			styx_unpack_raw(proxy_handle, buffer, payload, NULL);
			styx_pack_raw(proxy_handle, buffer, payload, NULL);
			styx_network_datagram_send(proxy_handle, &target_addr);
		}
		else {
			uint64 payload = bytes;
			styx_unpack_raw(proxy_handle, buffer, payload, NULL);

			styx_pack_uint16(proxy_handle, 0x0000, "RSV");
			styx_pack_uint8(proxy_handle, 0x00, "FRAG");
			styx_pack_uint8(proxy_handle, IPV4, "ATYP");
			styx_pack_uint32(proxy_handle, sender_addr.ip.v4, "SRC.ADDR");
			styx_pack_uint16(proxy_handle, sender_addr.port, "SRC.PORT");

			styx_pack_raw(proxy_handle, buffer, payload, NULL);
			styx_network_datagram_send(proxy_handle, &peer_addr);
		}
	}
cleanup:
	styx_free(proxy_handle);
	styx_free(client_handle);
}

static void reload_config(void)
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
	UNUSED(signum);
	g_reload_settings = 1;
}

static void _shutdown(int signum)
{
	UNUSED(signum);
	return;
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