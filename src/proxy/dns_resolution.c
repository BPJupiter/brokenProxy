#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#endif
#include <sys/types.h>

#include "Clay/clay.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "network_tools.h"

static int dns_printf(const char *format, ...);
#define printf dns_printf

typedef struct
{
    uint16 id;
    uint16 flags;
    uint16 q_count;
    uint16 ans_count;
    uint16 auth_count;
    uint16 add_count;
} DNS_HEADER;

/**   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
 *   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *   |QR|   Opcode  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
 *   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */

#define DNS_FLAG_QR     0x8000
#define DNS_FLAG_OPCODE 0x7800
#define DNS_FLAG_AA     0x0400
#define DNS_FLAG_TC     0x0200
#define DNS_FLAG_RD     0x0100
#define DNS_FLAG_RA     0x0080
#define DNS_FLAG_Z      0x0040
#define DNS_FLAG_AD     0x0020
#define DNS_FLAG_CD     0x0010
#define DNS_FLAG_RCODE  0x000F

typedef struct
{
    uint8 *mname;
    uint8 *rname;
    uint serial;
    uint refresh;
    uint retry;
    uint expire;
    uint minimum;
} SOA_DATA;

/* Constant sized fields of query structure */
typedef struct
{
    uint16 qtype;
    uint16 qclass;
} QUERY;

/* Constant sized fields of the resource record structure */
#pragma pack(push, 1)
typedef struct
{
    uint16 type;
    uint16 _class;
    uint32 ttl;
    uint16 data_len;
} R_DATA;
#pragma pack(pop)

/* Pointers to resource record contents */
typedef struct
{
    uint8 name[256]; /* domain names have a max length of 255 octets, as per rfc 1035 */
    R_DATA *resource;
    uint8 *rdata;
} RES_RECORD;

/* Structure of a query */
typedef struct
{
    char *name;
    QUERY *ques;
} QUESTION;

static boolean dns_iterative_root_worker(StyxNetworkAddress *dest, const char *dns_name, uint16 query_type, const char *current_ns_ip, uint32 max_rtt_ns, uint depth); /* Confusing naming; Function is recursive in the programming sense. Overall resolves DNS through the iterative resolution process. */
static size_t init_dns_header(uint8 *buf, const char *host, uint16 query_type);
static void read_answers(DNS_HEADER *dns, RES_RECORD *answers, uint8 **reader, uint8 *buf, int *stop);
static void read_authorities(DNS_HEADER *dns, RES_RECORD *auth, uint8 **reader, uint8 *buf, int *stop);
static void read_additional(DNS_HEADER *dns, RES_RECORD *addit, uint8 **reader, uint8 *buf, int *stop);
static boolean handle_found_answers(StyxNetworkAddress *out,
                                    DNS_HEADER *dns,
                                    RES_RECORD *answers,
                                    RES_RECORD *auth,
                                    RES_RECORD *addit,
                                    uint16 query_type,
                                    uint32 max_rtt_ns,
                                    uint depth);
static boolean process_auth_records(StyxNetworkAddress *out,
                                    DNS_HEADER *dns,
                                    RES_RECORD *answers,
                                    RES_RECORD *auth,
                                    RES_RECORD *addit,
                                    const char *host,
                                    uint16 query_type,
                                    uint32 max_rtt_ns,
                                    uint depth);
static boolean find_ipv4_glue(RES_RECORD *addit, int add_count, const char *ns_name, char *out_ip);
static boolean try_ns_ip(StyxNetworkAddress *out, const char *ns_ip, const char *host, uint16 query_type, uint32 max_rtt_ns, uint depth, uint *rtt_rejections);
static boolean localhost(StyxNetworkAddress *address);
static void  change_to_dns_name_format(uint8 *, const char *);
static void read_name(uint8 name[256], uint8 *, uint8 *, int *);
static void  dns_free_mem(DNS_HEADER *dns, RES_RECORD *answers, RES_RECORD *auth, RES_RECORD *addit);
static void free_addresses(StyxNetworkAddress *addr);

void print_response_contents(void *dns_ptr, void *answers_ptr, void *auth_ptr, void *addit_ptr);

char RootServers[][16] = {
    "198.41.0.4",     /* A */
    "170.247.170.2",  /* B */
    "192.33.4.12",    /* C */
    "199.7.91.13",    /* D */
    "192.203.230.10", /* E */
    "192.5.5.241",    /* F */
    "192.112.36.4",   /* G */
    "198.97.190.53",  /* H */
    "192.36.148.17",  /* I */
    "192.58.128.30",  /* J */
    "193.0.14.129",   /* K */
    "199.7.83.42",    /* L */
    "202.12.27.33",   /* M */
};

typedef enum RootServerIndex
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    RSI_COUNT
} RootServerIndex;

#ifndef _ARPA_NAMESER_H_

#define T_A 1     /* Ipv4 address */
#define T_NS 2    /* Nameserver */
#define T_CNAME 5 /* Canonical name */
#define T_SOA 6   /* Start of authority zone */
#define T_PTR 12  /* Domain name pointer */
#define T_MX 15   /* Mail server */
#define T_AAAA 28 /* IPv6 address */

#endif

#define MAX_RECORDS 20

int dns_server_count = 16;

char current_root_ip[16];
int root_server_found = 0;

boolean dns_resolve_iterative_root_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6, uint32 max_rtt_ns)
{
    boolean result = TRUE;
    int i = 0;
    UNUSED(default_port);
    UNUSED(do_ipv6);

    if (dns_name[0] == '/')
    {
        return localhost(dest);
    }

    if (!root_server_found && strcmp(dns_name, "127.0.0.1") != 0)
    {
        for (i = 0; i < RSI_COUNT; i++)
        {
            if (!verify_latency(RootServers[i], max_rtt_ns))
            {
                printf("%s exceeded max latency! Changing root server\n", RootServers[i]);
                continue;
            }
            else if (!verify_cable(RootServers[i]))
            {
                printf("%s uses a disabled cable! Changing root server\n", RootServers[i]);
                continue;
            }
            /** Seems like UoA wifi only blocks some root DNS servers?
            * i.e. Root server A is blocked. Root server M is not.
            * Need more sophisticated way to see if local wifi will block DNS request packets to a given root server?
            */
            else
                break;
        }
        strcpy(current_root_ip, RootServers[i]);
        root_server_found = 1;
    }
    result = dns_iterative_root_worker(dest, dns_name, T_A, current_root_ip, max_rtt_ns, 0);
    if (!result) {
        printf("Could not resolve!\n");
    }
    return result;
}

/* --------- UNIMPLEMENTED ---------- */
boolean dns_resolve_iterative_local_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6)
{
    if (dns_name[0] == '/')
    {
        return localhost(dest);
    }
    UNUSED(default_port);
    UNUSED(do_ipv6);
    return FALSE;
}

static boolean localhost(StyxNetworkAddress *address)
{
    address->ip.v4 = 0x7F000001;

    return TRUE;
}

/* Perform a DNS query by sending a packet */
static boolean dns_iterative_root_worker(StyxNetworkAddress *out, const char *dns_name, uint16 query_type, const char *ns_ip, uint32 max_rtt_ns, uint depth)
{
    uint8 buf[65536] = {0};
    uint8 *qname, *reader;
    RES_RECORD answers[MAX_RECORDS] = {0};
    RES_RECORD auth[MAX_RECORDS] = {0};
    RES_RECORD addit[MAX_RECORDS] = {0};
    DNS_HEADER *dns = NULL;
    SHandle *h;
    boolean r = FALSE, w = FALSE;

    StyxNetworkAddress dest_addr, sender_addr;
    size_t send_size;
    int bytes_recvd;
    int stop;

    if (depth > 10)
    {
        printf("\x1b[31m[Loop Detected]\x1b[0m Maximum recursion depth exceeded for %s\n", dns_name);

        return FALSE;
    }

    printf("Starting iterative resolution for: %s\n", dns_name);

    h = styx_network_datagram_create(0, FALSE);
    if (!h) {
        printf("FUCK\n");
        return FALSE;
    }

    dest_addr.ip.v4 = ntohl(inet_addr(ns_ip));
    dest_addr.port = 53;
    dest_addr.is_ipv6 = FALSE;

    send_size = init_dns_header(buf, dns_name, query_type);

    printf("Querying NS: %s\n", ns_ip);

    styx_pack_raw(h, buf, send_size, NULL);
    if (styx_network_datagram_send(h, &dest_addr) < 0) {
        talos_print_error("sendto failed");
        dns_free_mem(dns, answers, auth, addit);
        styx_free(h);
        return FALSE;
    }

    bytes_recvd = styx_network_receive(h, &sender_addr);
    if (bytes_recvd <= 0)
    {
        talos_print_error("recvfrom failed");
        dns_free_mem(dns, answers, auth, addit);
        styx_free(h);
        return FALSE;
    }

    styx_unpack_raw(h, buf, bytes_recvd, NULL);
    styx_free(h);

    dns = (DNS_HEADER *)buf;
    qname = &buf[sizeof(DNS_HEADER)];

    if (ntohs(dns->ans_count) > MAX_RECORDS)  dns->ans_count = htons(MAX_RECORDS);
    if (ntohs(dns->auth_count) > MAX_RECORDS) dns->auth_count = htons(MAX_RECORDS);
    if (ntohs(dns->add_count) > MAX_RECORDS)  dns->add_count = htons(MAX_RECORDS);

#ifdef DNS_DEBUG
    print_response_info((void *)dns);
#endif

    /* move ahead of dns header and query field */
    reader = &buf[sizeof(DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(QUERY)];
    stop = 0;

    read_answers(dns, answers, &reader, buf, &stop);
    read_authorities(dns, auth, &reader, buf, &stop);
    read_additional(dns, addit, &reader, buf, &stop);

#ifdef DNS_DEBUG
    print_response_contents((void *)dns, (void *)answers, (void *)auth, (void *)addit, (void *)&a);
#endif

    if (ntohs(dns->ans_count) > 0) {
        boolean res = handle_found_answers(out, dns, answers, auth, addit, query_type, max_rtt_ns, depth);
        dns_free_mem(dns, answers, auth, addit);
        return res;
    }

    if (ntohs(dns->auth_count) > 0) {
        boolean res =  process_auth_records(out, dns, answers, auth, addit, dns_name, query_type, max_rtt_ns, depth);
        dns_free_mem(dns, answers, auth, addit);
        return res;
    }

    printf("No answer or authority records found. Cannot resolve iteratively.\n");
    dns_free_mem(dns, answers, auth, addit);

    return FALSE;
}

static size_t init_dns_header(uint8 *buf, const char *host, uint16 query_type)
{
    DNS_HEADER *dns = (DNS_HEADER *)buf;
    uint8 *qname;
    uint16 f = 0;
    QUERY *qinfo = {0};

    dns->id = (uint16)htons(13709); /* smiley on a numpad :) */
    f |= DNS_FLAG_RD;
    dns->flags = htons(f);
    dns->q_count = htons(1); /* we have 1 question */
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (uint8 *)&buf[sizeof(DNS_HEADER)];
    change_to_dns_name_format(qname, host);

    qinfo = (QUERY *)&buf[sizeof(DNS_HEADER) + (strlen((const char *)qname) + 1)]; /* fill */

    qinfo->qtype = htons(query_type);
    qinfo->qclass = htons(1); /* it is indeed internet */

    return sizeof(DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(QUERY);
}

static void read_answers(DNS_HEADER *dns, RES_RECORD *answers, uint8 **reader, uint8 *buf, int *stop)
{
    int i, j;
    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
        read_name(answers[i].name, *reader, buf, stop);
        *reader = *reader + *stop;

        answers[i].resource = (R_DATA *)*reader;
        *reader = *reader + sizeof(R_DATA);

        UNUSED(answers[i].resource->ttl);
        /* NOTE(frances): We may want to do DNS chaching here.
         * We will want to store the entire DNS lookup chain for a single answer record
         * so that we can check the DNS lookup for outage criteria.
         */

        switch (ntohs(answers[i].resource->type))
        {
            case T_A:
                answers[i].rdata = (uint8 *)malloc(ntohs(answers[i].resource->data_len) + 1);
                talos_malloc_assert(answers[i].rdata);
                for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
                    answers[i].rdata[j] = (*reader)[j];
                answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
                *reader = *reader + ntohs(answers[i].resource->data_len);
            break;
            case T_AAAA:
                answers[i].rdata = (uint8 *)malloc(ntohs(answers[i].resource->data_len) + 1);
                talos_malloc_assert(answers[i].rdata);
                for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
                    answers[i].rdata[j] = (*reader)[j];
                answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
                *reader = *reader + ntohs(answers[i].resource->data_len);
            break;
            case T_CNAME:
                answers[i].rdata = (uint8 *)malloc(256 * sizeof(*(answers[i].rdata)));
                talos_malloc_assert(answers[i].rdata);
                read_name(answers[i].rdata, *reader, buf, stop);
                *reader += *stop;
            break;
            default:
                answers[i].rdata = NULL;
                *reader += ntohs(answers[i].resource->data_len);
            break;
        }
    }
}

static void read_authorities(DNS_HEADER *dns, RES_RECORD *auth, uint8 **reader, uint8 *buf, int *stop)
{
    int i;
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        read_name(auth[i].name, *reader, buf, stop);
        *reader += *stop;

        auth[i].resource = (R_DATA *)*reader;
        *reader += sizeof(R_DATA);

        UNUSED(auth[i].resource->ttl);
        /* NOTE(frances): We may want to do DNS chaching here.
         * We will want to store the entire DNS lookup chain for a single answer record
         * so that we can check the DNS lookup for outage criteria.
         */

        switch (ntohs(auth[i].resource->type))
        {
        uint8 rname[256];
            case T_NS:
                auth[i].rdata = malloc(256 * sizeof(*(auth[i].rdata)));
                talos_malloc_assert(auth[i].rdata);
                read_name(auth[i].rdata, *reader, buf, stop);
                *reader += *stop;
            break;
            case T_SOA:
                /* Read MNAME */
                auth[i].rdata = malloc(256 * sizeof(*(auth[i].rdata)));
                talos_malloc_assert(auth[i].rdata);
                read_name(auth[i].rdata, *reader, buf, stop);
                *reader += *stop;
                /* Read RNAME */
                read_name(rname, *reader, buf, stop);
                *reader += *stop;
                /* Adv 20 bytes */
                *reader += (5 * sizeof(uint));
                /* NOTE: May need to access SOA fields later? */
            break;
            default:
                auth[i].rdata = NULL;
                *reader += ntohs(auth[i].resource->data_len);
            break;
        }
    }
}

static void read_additional(DNS_HEADER *dns, RES_RECORD *addit, uint8 **reader, uint8 *buf, int *stop)
{
    int i, j;
    for (i = 0; i < ntohs(dns->add_count); i++)
    {
        read_name(addit[i].name, *reader, buf, stop);
        *reader += *stop;

        addit[i].resource = (R_DATA *)*reader;
        *reader += sizeof(R_DATA);

        UNUSED(addit[i].resource->ttl);
        /* NOTE(frances): We may want to do DNS chaching here.
         * We will want to store the entire DNS lookup chain for a single answer record
         * so that we can check the DNS lookup for outage criteria.
         */

        switch (ntohs(addit[i].resource->type))
        {
            case T_A:
                addit[i].rdata = (uint8 *)malloc(ntohs(addit[i].resource->data_len) + 1);
                talos_malloc_assert(addit[i].rdata);

                for (j = 0; j < ntohs(addit[i].resource->data_len); j++)
                    addit[i].rdata[j] = (*reader)[j];

                addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
                *reader += ntohs(addit[i].resource->data_len);
            break;
            case T_AAAA:
                addit[i].rdata = (uint8 *)malloc(ntohs(addit[i].resource->data_len) + 1);
                talos_malloc_assert(addit[i].rdata);

                for (j = 0; j < ntohs(addit[i].resource->data_len); j++)
                    addit[i].rdata[j] = (*reader)[j];

                addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
                *reader += ntohs(addit[i].resource->data_len);
            break;
            default:
                addit[i].rdata = NULL;
                *reader += ntohs(addit[i].resource->data_len);
            break;
        }
    }
}

static boolean handle_found_answers(StyxNetworkAddress *out,
                                    DNS_HEADER *dns,
                                    RES_RECORD *answers,
                                    RES_RECORD *auth,
                                    RES_RECORD *addit,
                                    uint16 query_type,
                                    uint32 max_rtt_ns,
                                    uint depth)
{
    char cname_target[256] = {0};
    int ans_count = ntohs(dns->ans_count);
    int i;
    StyxNetworkAddress *tail = out;

    switch (ntohs(answers[0].resource->type)) /* TODO: make sure final resource type is the same as the req type. */
    {
        case T_A:
            printf("Found A record.\n");
            out->ip.v4 = htonl(*(uint32 *)answers[0].rdata);
            out->is_ipv6 = FALSE;
            out->next = NULL;

            for (i = 0; i < ans_count; i++)
            {
                StyxNetworkAddress *node = malloc(sizeof(*node));
                talos_malloc_assert(node);
                memset(node, 0, sizeof(*node));

                node->ip.v4 = htonl(*(uint32 *)answers[i].rdata);
                node->port = out->port;
                node->is_ipv6 = FALSE;
                node->next = NULL;

                tail->next = node;
                tail = node;
            }
            return TRUE;
        break;
        case T_CNAME:
            c_text_copy(strlen((char *)answers[0].rdata) + 1, cname_target, (char *)answers[0].rdata);
            printf("Found CNAME alias: %s. Requerying...\n", answers[0].rdata);
            dns_free_mem(dns, answers, auth, addit);
            return dns_iterative_root_worker(out, cname_target, query_type, current_root_ip, max_rtt_ns, depth + 1);
        break;
        case T_AAAA:
            printf("Found AAAA record.\n");
            memcpy(out->ip.v6, answers[0].rdata, 16);
            out->is_ipv6 = TRUE;
            out->next = NULL;

            for (i = 0; i < ans_count; i++)
            {
                StyxNetworkAddress *node = malloc(sizeof(*node));
                talos_malloc_assert(node);
                memset(node, 0, sizeof(*node));

                memcpy(node->ip.v6, answers[i].rdata, 16);
                node->port = out->port;
                node->is_ipv6 = TRUE;
                node->next = NULL;

                tail->next = node;
                tail = node;
            }
            return TRUE;
        break;
        default:
            printf("Unknown RR Type code : %d\n", ntohs(answers[0].resource->type));
            return FALSE;
        break;
    }
}

static boolean process_auth_records(StyxNetworkAddress *out,
                                    DNS_HEADER *dns,
                                    RES_RECORD *answers,
                                    RES_RECORD *auth,
                                    RES_RECORD *addit,
                                    const char *host,
                                    uint16 query_type,
                                    uint32 max_rtt_ns,
                                    uint depth)
{
    char next_ns_name[256] = "\0";
    char next_ns_ip[16] = "\0";

    uint i;
    uint candidates_found = 0;
    uint rtt_rejections = 0;

    for (i = 0; i < ntohs(dns->auth_count); i++) {
        if (ntohs(auth[i].resource->type) == T_SOA) {
            printf("SOA Record: %s does not exist.\n", host);
            return FALSE;
        }

        if (ntohs(auth[i].resource->type) != T_NS) {
            continue;
        }

        c_text_copy(strlen((char *)auth[i].rdata) + 1, next_ns_name, (char *)auth[i].rdata);

        if (find_ipv4_glue(addit, ntohs(dns->add_count), next_ns_name, next_ns_ip)) {
            candidates_found++;
            if (try_ns_ip(out, next_ns_ip, host, query_type, max_rtt_ns, depth, &rtt_rejections)) {
                dns_free_mem(dns, answers, auth, addit);
                return TRUE;
            }
        }
        else {
            StyxNetworkAddress ns_addr;
            memset(&ns_addr, 0, sizeof(StyxNetworkAddress));

            if (strcmp(host, next_ns_name) == 0) {
                printf("\x1b[31m[Abort]\x1b[0m Circular dependency: Need %s to resolve %s\n", next_ns_name, host);
                continue;
            }

            printf("No glue record for %s. Recursively resolving NS IP...\n", next_ns_name);
            if (dns_iterative_root_worker(&ns_addr, next_ns_name, T_A, current_root_ip, max_rtt_ns, depth + 1)) {
                StyxNetworkAddress *curr = &ns_addr;
                while (curr) {
                    struct in_addr in;
                    in.s_addr = curr->ip.v4;
                    c_text_copy(INET_ADDRSTRLEN, next_ns_ip, inet_ntoa(in));
                    candidates_found++;

                    if (try_ns_ip(out, next_ns_ip, host, query_type, max_rtt_ns, depth, &rtt_rejections)) {
                        free_addresses(&ns_addr);
                        return TRUE;
                    }
                    curr = curr->next;
                }
                free_addresses(&ns_addr);
            }
        }
    }

    if (candidates_found > 0 && candidates_found == rtt_rejections) {
        printf("\x1b[31m[DNS Rejected]\x1b[0m Failed to resolve %s: all %u nameservers exceed max RTT.\n", host, candidates_found);
    }
    return FALSE;
}

static boolean find_ipv4_glue(RES_RECORD *addit, int add_count, const char *ns_name, char *out_ip)
{
    int j;
    struct in_addr a;

    for (j = 0; j < add_count; j++)
    {
        if (strcmp((char *)addit[j].name, ns_name) == 0 && ntohs(addit[j].resource->type) == T_A) {
            long *p = (long *)addit[j].rdata;
            a.s_addr = *p;
            strcpy(out_ip, inet_ntoa(a));
            printf("Found IPv4 glue record: %s\n", out_ip);
            return TRUE;
        }
    }
    return FALSE;
}

static boolean try_ns_ip(StyxNetworkAddress *out, const char *ns_ip, const char *host, uint16 query_type, uint32 max_rtt_ns, uint depth, uint *rtt_rejections)
{
    if (strcmp(ns_ip, "127.0.0.1") == 0) {
        return FALSE;
    }

    if (!verify_latency(ns_ip, max_rtt_ns)) {
        printf("%s exceeded max latency! Skipping.\n", ns_ip);
        (*rtt_rejections)++;
        return FALSE;
    }

    if (!verify_cable(ns_ip)) {
        return FALSE;
    }

    return dns_iterative_root_worker(out, host, query_type, ns_ip, max_rtt_ns, depth + 1);
}

static void read_name(uint8 name[256], uint8 *reader, uint8 *buffer, int *count)
{
    uint p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;

    name[0] = '\0';

    /* Handle message compression */
    while (*reader != 0)
    {
        if ((*reader & 0xC0) == 0xC0)
        {
            /* grab the 14 bit offset after the two 1s */
            offset = (*reader) * 256 + *(reader + 1) - 49152; /* 49152 = C0 00 */
            reader = buffer + offset - 1;
            jumped = 1; /* we have jumped to another location so counting wont go up */
        }
        else
        {
            if (p < 256)
            {
                name[p++] = *reader;
            }
            else
            {
            }
        }

        reader++;

        if (jumped == 0)
        {
            (*count)++;
        }
    }

    name[p] = '\0'; /* string complete */
    if (jumped == 1)
    {
        (*count)++; /* number of steps weve moved forward in the packet; */
    }

    /* convert www6gooogle3com0 to www.google.com */
    for (i = 0; i < (int)strlen((const char *)name); i++)
    {
        p = name[i];
        for (j = 0; j < (int)p; j++)
        {
            name[i] = name[i + 1];
            i++;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0'; /* remove the last dot */
}

static void change_to_dns_name_format(uint8 *dns, const char *hostname)
{
    /* convert www.google.com to 3www6google3com0 */
    unsigned long lock = 0;
    char host[256] = {0};
    unsigned long i;
    c_text_copy(strlen(hostname) + 1, host, hostname);
    strcat((char *)host, ".");

    for (i = 0; i < strlen((char *)host); i++)
    {
        if (host[i] == '.')
        {
            *dns++ = i - lock;
            for (; lock < i; lock++)
            {
                *dns++ = host[lock];
            }
            lock++; /* or lock=i+1 */
        }
    }
    *dns++ = '\0';
}

static int dns_printf(const char *format, ...)
{
    va_list args;
    int ret;
    char msg[1024] = "\x1b[34m[DNS]\x1b[0m ";
    strcat(msg, format);

    va_start(args, format);

    ret = vprintf(msg, args);

    va_end(args);

    return ret;
}

static void free_addresses(StyxNetworkAddress *addr)
{
    StyxNetworkAddress *curr = addr->next;
    if (!addr) return;
    while (curr) {
        StyxNetworkAddress *next = curr->next;
        free(curr);
        curr = next;
    }
    addr->next = NULL;
}

static void dns_free_mem(DNS_HEADER *dns, RES_RECORD *answers, RES_RECORD *auth, RES_RECORD *addit)
{
    int ans_count = ntohs(dns->ans_count);
    int auth_count = ntohs(dns->auth_count);
    int add_count = ntohs(dns->add_count);

    int i;

    for (i = 0; i < MAX_RECORDS; i++)
    {
        if (i < ans_count)
        {
            if (answers[i].rdata != NULL)
            {
                free(answers[i].rdata);
                answers[i].rdata = NULL;
            }
        }
        if (i < auth_count)
        {
            if (auth[i].rdata != NULL)
            {
                free(auth[i].rdata);
                auth[i].rdata = NULL;
            }
        }
        if (i < add_count)
        {
            if (addit[i].rdata != NULL)
            {
                free(addit[i].rdata);
                addit[i].rdata = NULL;
            }
        }
    }
}

/* -------------- debug printing ------------------ */
void print_response_info(void *dns_ptr)
{
    DNS_HEADER *dns = (DNS_HEADER *)dns_ptr;
    printf("The response contains : \n");
    printf("%d Questions.\n", ntohs(dns->q_count));
    printf("%d Answers.\n", ntohs(dns->ans_count));
    printf("%d Authoritative Servers.\n", ntohs(dns->auth_count));
    printf("%d Additional Records.\n\n", ntohs(dns->add_count));
}

void print_response_contents(void *dns_ptr, void *answers_ptr, void *auth_ptr, void *addit_ptr)
{
    DNS_HEADER *dns = (DNS_HEADER *)dns_ptr;
    RES_RECORD *answers = (RES_RECORD *)answers_ptr;
    RES_RECORD *auth = (RES_RECORD *)auth_ptr;
    RES_RECORD *addit = (RES_RECORD *)addit_ptr;
    struct in_addr in4;
    struct in6_addr in6;

    uint32 seconds;

    int i;

    printf("Answer Records : %d \n", ntohs(dns->ans_count));
    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
        seconds = ntohl(answers[i].resource->ttl);
        printf("Name : %s ", answers[i].name);

        switch (ntohs(answers[i].resource->type))
        {
        long *p;
        uint8 *p6;
        char ipv6_str[INET6_ADDRSTRLEN];
            case T_A:
                p = (long *)answers[i].rdata;
                in4.s_addr = (*p);
                fprintf(stdout, "has IPv4 address : %s", inet_ntoa(in4));
            break;
            case T_CNAME:
                /* Canonical name for an alias */
                fprintf(stdout, "has alias name : %s", answers[i].rdata);
            break;
            case T_AAAA:
                p6 = (uint8 *)answers[i].rdata;
                memcpy(&in6, p6, sizeof(struct in6_addr));
                fprintf(stdout, "has IPv6 address: %s", inet_ntop(AF_INET6, &in6, ipv6_str, INET6_ADDRSTRLEN));
            break;
        }
        if (seconds >= 60 * 60) {
            fprintf(stdout, " for %d hours", seconds / (60 * 60));
        }
        else if (seconds >= 60) {
            fprintf(stdout, " for %d minutes", seconds / 60);
        }
        else {
            fprintf(stdout, " for %d seconds", seconds);
        }

        fprintf(stdout, "\n");
    }

    /* print authorities */
    printf("Authoritative Records : %d \n", ntohs(dns->auth_count));
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        seconds = ntohl(auth[i].resource->ttl);
        printf("Name : %s ", auth[i].name);
        if (ntohs(auth[i].resource->type) == T_NS)
        {
            fprintf(stdout, "has nameserver : %s", auth[i].rdata);
        }
        else
        {
            fprintf(stdout, "RR Type code %d", ntohs(auth[i].resource->type));
        }
        if (seconds >= 60 * 60) {
            fprintf(stdout, " for %d hours", seconds / (60 * 60));
        }
        else if (seconds >= 60) {
            fprintf(stdout, " for %d minutes", seconds / 60);
        }
        else {
            fprintf(stdout, " for %d seconds", seconds);
        }
        fprintf(stdout, "\n");
    }

    /* print additional resource records */
    printf("Additional Records : %d \n", ntohs(dns->add_count));
    for (i = 0; i < ntohs(dns->add_count); i++)
    {
        seconds = ntohl(addit[i].resource->ttl);
        printf("Name : %s ", addit[i].name);
        switch (ntohs(addit[i].resource->type))
        {
        long *p;
        uint8 *p6;
        char ipv6_str[INET6_ADDRSTRLEN];
            case T_A:
                p = (long *)addit[i].rdata;
                in4.s_addr = (*p);
                fprintf(stdout, "has IPv4 address : %s", inet_ntoa(in4));
            break;
            case T_AAAA:
                p6 = (uint8 *)addit[i].rdata;
                memcpy(&in6, p6, sizeof(struct in6_addr));
                fprintf(stdout, "has IPv6 address : %s", inet_ntop(AF_INET6, &in6, ipv6_str, INET6_ADDRSTRLEN));
            break;
            default:
                fprintf(stdout, "RR Type code %d", ntohs(addit[i].resource->type));
            break;
        }
        if (seconds >= 60 * 60) {
            fprintf(stdout, " for %d hours", seconds / (60 * 60));
        }
        else if (seconds >= 60) {
            fprintf(stdout, " for %d minutes", seconds / 60);
        }
        else {
            fprintf(stdout, " for %d seconds", seconds);
        }

        fprintf(stdout, "\n");
    }
}
