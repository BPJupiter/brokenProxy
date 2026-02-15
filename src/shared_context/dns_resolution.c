#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "Clay/clay.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "shared_context/shared_context.h"

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
    uint ttl;
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

static DnsResult dns_iterative_root_worker(const char *host, int query_type, char *current_ns_ip, uint depth); /* Confusing naming; Function is recursive in the programming sense. Overall resolves DNS through the iterative resolution process. */
static size_t init_dns_header(uint8 *buf, const char *host, int query_type);
static void read_answers(DNS_HEADER *dns, RES_RECORD *answers, uint8 **reader, uint8 *buf, int *stop);
static void read_authorities(DNS_HEADER *dns, RES_RECORD *auth, uint8 **reader, uint8 *buf, int *stop);
static void read_additional(DNS_HEADER *dns, RES_RECORD *addit, uint8 **reader, uint8 *buf, int *stop);
static DnsResult handle_found_answers(DNS_HEADER *dns,
                                      RES_RECORD *answers,
                                      RES_RECORD *auth,
                                      RES_RECORD *addit,
                                      int query_type, uint depth);
static DnsResult process_auth_records(DNS_HEADER *dns,
                                      RES_RECORD *answers,
                                      RES_RECORD *auth,
                                      RES_RECORD *addit,
                                      const char *host, int query_type, uint depth);
static DnsResult localhost();
static void  change_to_dns_name_format(uint8 *, const char *);
static void read_name(uint8 name[256], uint8 *, uint8 *, int *);
static void  dns_free_mem(DNS_HEADER *dns, RES_RECORD *answers, RES_RECORD *auth, RES_RECORD *addit);
static boolean external_dns_is_blocked(void);
static void  set_to_local_dns(void);

void print_response_contents(void *dns_ptr, void *answers_ptr, void *auth_ptr, void *addit_ptr, void *a_ptr);

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

#ifdef DNS_PROGRAM
int main()
{
    char hostname[100];
    uchar **answers;
    argType a;
    retType r;
    int n_ans;
    double max_rtt;
    int i;
    max_rtt = 80.0;
    sharedContext_init();
    sharedContext_setVariable(SCV_MAX_RTT, &max_rtt);
    sharedContext_toggleCb(SCC_RESOLVE, 1);

    /* Get the hostname from the terminal */
    printf("Enter Hostname to Lookup : ");
    scanf("%s", hostname);

    /* Now get the ip of this hostname, A record */
    a.resolve.hostname = hostname;
    a.resolve.answer_index = &answers;
    sharedContext_execCb(SCC_RESOLVE, &r, &a);
    n_ans = r.resolve;
    for (i = 0; i < n_ans; i++)
    {
        free(answers[i]);
    }

    if (n_ans > 0) free(answers);

#ifdef C_MEMORY_DEBUG
    c_debug_mem_print(0);
#endif
    sharedContext_destroy();
    return 0;
}
#endif

void DnsResult_free(DnsResult *dns_result)
{
    uint i;
    for (i = 0; i < dns_result->nAns; i++)
        free(dns_result->answers[i]);
    free(dns_result->answers);

    dns_result->status = DNS_OK;
    dns_result->nAns = 0;
    dns_result->answers = NULL;
}

DnsResult dns_resolve_recursive_local(const char *host)
{
    DnsResult result = {0};
    StyxNetworkAddress address;
    char ipstr[16];
    if (host[0] == '/')
    {
        return localhost();
    }

    if (!styx_network_address_lookup(&address, host, 443))
    {
        printf("Quick Resolve: Could not resolve!\n");
        result.status = DNS_ERR_GENERIC;
        result.nAns = 0;
        result.answers = NULL;
        return result;
    }

    result.answers = malloc(sizeof(*result.answers));
    talos_malloc_assert(result.answers);

    result.answers[0] = malloc((sizeof **result.answers) * 256);
    talos_malloc_assert(result.answers[0]);

    styx_ipv4_to_string(ipstr, &address);
    c_text_copy(strlen(ipstr) + 1, result.answers[0], ipstr);

    result.status = DNS_OK;
    result.nAns = 1;
    return result;
}

DnsResult dns_resolve_iterative_root(const char *host)
{
    DnsResult result = { 0 };
    int i = 0;

    if (host[0] == '/')
    {
        return localhost();
    }

    if (!root_server_found && strcmp(host, "127.0.0.1") != 0)
    {
        for (i = 0; i < RSI_COUNT; i++)
        {
            if (!sharedContext_latency_isgood(RootServers[i]))
            {
				printf("%s exceeded max latency! Changing root server\n", RootServers[i]);
				continue;
            }
            else if (!sharedContext_cable_isgood(RootServers[i]))
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
    result = dns_iterative_root_worker(host, T_A, current_root_ip, 0);
    if (result.nAns <= 0)
    {
        printf("Could not resolve!\n");
    }
    return result;
}

/* --------- UNIMPLEMENTED ---------- */
DnsResult dns_resolve_iterative_local(const char *host)
{
    DnsResult result = { 0 };
    if (host[0] == '/')
    {
        return localhost();
    }
    result.status = DNS_ERR_GENERIC;
    result.nAns = 0;
    result.answers = NULL;
    return result;
}

static DnsResult localhost()
{
    DnsResult result = { 0 };
    char lh[] = "127.0.0.1";

    result.answers = malloc(1 * (sizeof *result.answers));
    talos_malloc_assert(result.answers);

    result.answers[0] = malloc(256 * sizeof(**result.answers));
    talos_malloc_assert(result.answers[0]);

    c_text_copy(strlen(lh) + 1, result.answers[0], lh);

    result.status = DNS_OK;
    result.nAns = 1;
    return result;
}

/* Perform a DNS query by sending a packet */
static DnsResult dns_iterative_root_worker(const char *host, int query_type, char *ns_ip, uint depth)
{
    DnsResult result;
    uint8 buf[65536] = {0};
    uint8 *qname, *reader;
    RES_RECORD answers[MAX_RECORDS] = {0};
    RES_RECORD auth[MAX_RECORDS] = {0};
    RES_RECORD addit[MAX_RECORDS] = {0};
    DNS_HEADER *dns = NULL;
    VSocket s;

    StyxSockaddrInet a = {0};
    StyxSockaddrInet dest = {0};
    size_t send_size;
    int bytes_recvd;

    int i, stop;

    if (depth > 10)
    {
        printf("\x1b[31m[Loop Detected]\x1b[0m Maximum recursion depth exceeded for %s\n", host);

        result.status = DNS_ERR_RECURSION_LIMIT;
        result.nAns = 0;
        result.answers = NULL;
        return result;
    }

    printf("Starting iterative resolution for: %s\n", host);

    s = styx_socket_create(FALSE, 0);

    dest.Ipv4.sin_family = AF_INET;
    dest.Ipv4.sin_port = htons(53);
    dest.Ipv4.sin_addr.s_addr = inet_addr(ns_ip);

    styx_socket_set_timeout(s, 5);

    send_size = init_dns_header(buf, host, query_type);
    dns = (DNS_HEADER *)buf;
    qname = &buf[sizeof(DNS_HEADER)];

    printf("Querying NS: %s\n", inet_ntoa(dest.Ipv4.sin_addr));

    if (sendto(s, (char *)buf, send_size, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
    {
        talos_print_error("sendto failed");

        dns_free_mem(dns, answers, auth, addit);

        result.status = DNS_ERR_FAIL_SYSCALL;
        result.nAns = 0;
        result.answers = NULL;
        return result;
    }

    /* Recv ans */
    i = sizeof(dest);
    do
    {
        bytes_recvd = recvfrom(s, (char *)buf, 65536, 0, (struct sockaddr *)&dest, (socklen_t *)&i);
    } while (bytes_recvd < 0 && errno == EINTR);

    styx_socket_destroy(s);

    if (bytes_recvd < 0)
    {
        if (errno == EAGAIN)
        {
            printf("\x1b[33m[Timeout]\x1b[0m Recvieve from %s timed out.\n", inet_ntoa(dest.Ipv4.sin_addr));
            result.status = DNS_ERR_TIMEOUT;
        }
        else
        {
            talos_print_error("recvfrom failed");
            result.status = DNS_ERR_FAIL_SYSCALL;
        }

        dns_free_mem(dns, answers, auth, addit);

        result.nAns = 0;
        result.answers = NULL;
        return result;
    }

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

    if (ntohs(dns->ans_count) > 0)
        return handle_found_answers(dns, answers, auth, addit, query_type, depth);

    if (ntohs(dns->auth_count) > 0)
        return process_auth_records(dns, answers, auth, addit, host, query_type, depth);

    printf("No answer or authority records found. Cannot resolve iteratively.\n");
    print_response_contents((void *)dns, (void *)answers, (void *)auth, (void *)addit, (void *)&a);

    dns_free_mem(dns, answers, auth, addit);

    result.status = DNS_ERR_GENERIC;
    result.nAns = 0;
    result.answers = NULL;
    return result;
}

static size_t init_dns_header(uint8 *buf, const char *host, int query_type)
{
    DNS_HEADER *dns = (DNS_HEADER *)buf;
    uint8 *qname;
    uint16 f = 0;
    QUERY *qinfo = {0};

    dns->id = (uint16)htons(getpid());
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
            default:
                answers[i].rdata = NULL;
                *reader += answers[i].resource->data_len;
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
                *reader += auth[i].resource->data_len;
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
                *reader += addit[i].resource->data_len;
            break;
        }
    }
}

static DnsResult handle_found_answers(DNS_HEADER *dns,
                                      RES_RECORD *answers,
                                      RES_RECORD *auth,
                                      RES_RECORD *addit,
                                      int query_type, uint depth)
{
    DnsResult result = { 0 };
    StyxSockaddrInet a = {0};
    char cname_target[256] = {0};
    int ans_count;
    int i;

    ans_count = ntohs(dns->ans_count);

    switch (ntohs(answers[0].resource->type)) /* TODO: make sure final resource type is the same as the req type. */
    {
        case T_A:
            printf("Found A record.\n");
            result.answers = malloc((sizeof *result.answers) * ans_count);
            talos_malloc_assert(result.answers);

            for (i = 0; i < ans_count; i++)
            {
                long *p;
                char *addr;
                p = (long *)answers[i].rdata;
                a.Ipv4.sin_addr.s_addr = (*p);
                addr = inet_ntoa(a.Ipv4.sin_addr);

                result.answers[i] = malloc(256 * sizeof(**result.answers));
                talos_malloc_assert(result.answers[i]);

                c_text_copy(strlen(addr) + 1, result.answers[i], addr);
            }

            dns_free_mem(dns, answers, auth, addit);
            result.status = DNS_OK;
            result.nAns = ans_count;
            return result;
        break;
        case T_CNAME:
            c_text_copy(strlen((char *)answers[0].rdata) + 1, cname_target, (char *)answers[0].rdata);
            printf("Found CNAME alias: %s. Requerying...\n", answers[0].rdata);
            dns_free_mem(dns, answers, auth, addit);
            return dns_iterative_root_worker(cname_target, query_type, current_root_ip, depth + 1);
        break;
        case T_AAAA:
            printf("Found AAAA record.\n");
            result.answers = malloc(ans_count * sizeof(*result.answers));
            talos_malloc_assert(result.answers);

            for (i = 0; i < ans_count; i++)
            {
                uint8 *p;
                const char *r;

                p = (uint8 *)answers[i].rdata;
                memcpy(&a.Ipv6.sin6_addr, p, sizeof(struct in6_addr));

                result.answers[i] = malloc(256 * sizeof(**result.answers));
                talos_malloc_assert(result.answers[i]);

                r = inet_ntop(AF_INET6, &a.Ipv6.sin6_addr, result.answers[i], INET6_ADDRSTRLEN);
                if (r == NULL) strcpy(result.answers[i], "IPv6 Conversion Error");
            }

            dns_free_mem(dns, answers, auth, addit);
            result.status = DNS_OK;
            result.nAns = ans_count;
            return result;
        break;
        default:
            printf("Unknown RR Type code : %d\n", ntohs(answers[0].resource->type));

            dns_free_mem(dns, answers, auth, addit);
            result.status = DNS_OK;
            result.nAns = ans_count;
            return result;
        break;
    }
}

static DnsResult process_auth_records(DNS_HEADER *dns,
                                      RES_RECORD *answers,
                                      RES_RECORD *auth,
                                      RES_RECORD *addit,
                                      const char *host, int query_type, uint depth)
{
    /* This function is disgusting. */
    DnsResult result = { 0 };
    StyxSockaddrInet a = {0};
    char next_ns_ip[16] = {0};
    char next_ns_name[256] = "\0";
    int found_glue = 0, bad_latency = 0;
    uint i, j, k;

    uint candidates_found = 0;
    uint rtt_rejections = 0;

    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        bad_latency = 0;
        /* currently finds first matching nammeserver that is ipv4 and has a glue record */
        if (ntohs(auth[i].resource->type) == T_SOA)
        {
            printf("SOA Record: %s does not exist.\n", host);
            dns_free_mem(dns, answers, auth, addit);
            result.status = DNS_ERR_GENERIC;
            result.nAns = 0;
            result.answers = NULL;
            return result;
        }
        if (ntohs(auth[i].resource->type) != T_NS) continue;

        c_text_copy(strlen((char *)auth[i].rdata) + 1, next_ns_name, (char *)auth[i].rdata);

        found_glue = 0;
        for (j = 0; j < ntohs(dns->add_count); j++)
        {
            if (strcmp((char *)addit[j].name, next_ns_name) != 0)
                continue;

            switch (ntohs(addit[j].resource->type))
            {
            long *p;
                case T_A:
                    p = (long *)addit[j].rdata;
                    a.Ipv4.sin_addr.s_addr = (*p);
                    strcpy(next_ns_ip, inet_ntoa(a.Ipv4.sin_addr));
                    printf("Found IPv4 glue record: %s\n", next_ns_ip);
                    found_glue = 1;
                break;
                case T_AAAA:
                    /*
                       uchar *p;
                       p = (uchar *)addit[j].rdata;
                       memcpy(&a.ipv6.sin6_addr, p, sizeof(struct in6_addr));
                       const char *r = inet_ntop(AF_INET6, &a.ipv6.sin6_addr, next_ns_ip, INET6_ADDRSTRLEN);
                       printf("Found IPv6 glue record: %s\n", next_ns_ip);
                       found_glue = 1;
                     */
                break;
            }
            /* Is that nameserver reachable? */
            if (found_glue && strcmp(next_ns_ip, "127.0.0.1") != 0)
            {
                candidates_found++;

                if (!sharedContext_latency_isgood(next_ns_ip))
                {
					printf("%s exceeded max latency! Skipping.\n", next_ns_ip);
					rtt_rejections++;
					found_glue = 0;
					bad_latency = 1;
					break;
				}
                if (!sharedContext_cable_isgood(next_ns_ip))
                {
                    /*rtt_rejections++;*/
                    found_glue = 0;
                    /*bad_latency = 1;*/
                    break;
                }
            }
            if (found_glue) break;
        }
        if (bad_latency) continue;
        if (found_glue)
        {
            /* Recursion */
            result = dns_iterative_root_worker(host, query_type, next_ns_ip, depth + 1);

            if (result.nAns > 0)
            {
                dns_free_mem(dns, answers, auth, addit);
                return result;
            }

            if (result.status == DNS_ERR_RTT_EXHAUSTED)
                rtt_rejections++;
        }
        else
        {
            DnsResult ns_result;

            if (strcmp(host, next_ns_name) == 0)
            {
                printf("\x1b[31m[Abort]\x1b[0m Circular dependency: Need %s to resolve %s\n", next_ns_name, host);
                continue;
            }
            printf("No glue record for %s. Recrusively resolving NS IP...\n", next_ns_name);

            ns_result = dns_resolve_iterative_root(next_ns_name);

            if (ns_result.nAns > 0)
            {
                for (k = 0; k < ns_result.nAns; k++)
                {
                    c_text_copy(strlen(ns_result.answers[k]) + 1, next_ns_ip, ns_result.answers[k]);

                    if (strcmp(next_ns_ip, "127.0.0.1") != 0)
                    {
                        candidates_found++;

                        if (!sharedContext_latency_isgood(next_ns_ip))
						{
							printf("%s exceeded max latency! Skipping.\n", next_ns_ip);
							rtt_rejections++;
							continue;
						}
                        if (!sharedContext_cable_isgood(next_ns_ip))
                        {
                            continue;
                        }
                    }

                    result = dns_iterative_root_worker(host, query_type, next_ns_ip, depth + 1);
                    if (result.nAns > 0)
                    {
                        DnsResult_free(&ns_result);

                        dns_free_mem(dns, answers, auth, addit);
                        return result;
                    }
                    if (result.status == DNS_ERR_RTT_EXHAUSTED)
                        rtt_rejections++;
                }
                DnsResult_free(&ns_result);
            }
        }
    }

    if (candidates_found > 0 && candidates_found == rtt_rejections)
    {
        printf("\x1b[31m[DNS Rejected]\x1b[0m Failed to resolve %s: all %d nameservers exceed max RTT.\n", host, candidates_found);
        result.status = DNS_ERR_RTT_EXHAUSTED;
    }
    else
    {
        result.status = DNS_ERR_GENERIC;
    }
    dns_free_mem(dns, answers, auth, addit);
    result.nAns = 0;
    result.answers = NULL;
    return result;
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

static boolean external_dns_is_blocked(void)
{
    DnsResult result = { 0 };
    uint i;
    result = dns_iterative_root_worker("www.google.com", T_A, current_root_ip, 0);
    if (result.nAns <= 0)
    {
        DnsResult_free(&result);
        return TRUE;
    }
    DnsResult_free(&result);
    return FALSE;
}

static void set_to_local_dns(void)
{
    char local_dns[16];
    styx_local_dns_server_get(local_dns, sizeof(local_dns));
    c_text_copy(strlen(local_dns) + 1, current_root_ip, local_dns);
    return;
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

void print_response_contents(void *dns_ptr, void *answers_ptr, void *auth_ptr, void *addit_ptr, void *a_ptr)
{
    DNS_HEADER *dns = (DNS_HEADER *)dns_ptr;
    RES_RECORD *answers = (RES_RECORD *)answers_ptr;
    RES_RECORD *auth = (RES_RECORD *)auth_ptr;
    RES_RECORD *addit = (RES_RECORD *)addit_ptr;
    StyxSockaddrInet a = *((StyxSockaddrInet *)a_ptr);

    int i;

    printf("Answer Records : %d \n", ntohs(dns->ans_count));
    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
        printf("Name : %s ", answers[i].name);

        switch (ntohs(answers[i].resource->type))
        {
        long *p;
        uint8 *p6;
        char ipv6_str[INET6_ADDRSTRLEN];
            case T_A:
                p = (long *)answers[i].rdata;
                a.Ipv4.sin_addr.s_addr = (*p);
                fprintf(stdout, "has IPv4 address : %s", inet_ntoa(a.Ipv4.sin_addr));
            break;
            case T_CNAME:
                /* Canonical name for an alias */
                fprintf(stdout, "has alias name : %s", answers[i].rdata);
            break;
            case T_AAAA:
                p6 = (uint8 *)answers[i].rdata;
                memcpy(&a.Ipv6.sin6_addr, p6, sizeof(struct in6_addr));
                fprintf(stdout, "has IPv6 address: %s", inet_ntop(AF_INET6, &a.Ipv6.sin6_addr, ipv6_str, INET6_ADDRSTRLEN));
            break;
        }

        fprintf(stdout, "\n");
    }

    /* print authorities */
    printf("Authoritative Records : %d \n", ntohs(dns->auth_count));
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        printf("Name : %s ", auth[i].name);
        if (ntohs(auth[i].resource->type) == T_NS)
        {
            fprintf(stdout, "has nameserver : %s", auth[i].rdata);
        }
        else
        {
            fprintf(stdout, "RR Type code %d", ntohs(auth[i].resource->type));
        }
        fprintf(stdout, "\n");
    }

    /* print additional resource records */
    printf("Additional Records : %d \n", ntohs(dns->add_count));
    for (i = 0; i < ntohs(dns->add_count); i++)
    {
        printf("Name : %s ", addit[i].name);
        switch (ntohs(addit[i].resource->type))
        {
        long *p;
        uint8 *p6;
        char ipv6_str[INET6_ADDRSTRLEN];
            case T_A:
                p = (long *)addit[i].rdata;
                a.Ipv4.sin_addr.s_addr = (*p);
                fprintf(stdout, "has IPv4 address : %s", inet_ntoa(a.Ipv4.sin_addr));
            break;
            case T_AAAA:
                p6 = (uint8 *)addit[i].rdata;
                memcpy(&a.Ipv6.sin6_addr, p6, sizeof(struct in6_addr));
                fprintf(stdout, "has IPv6 address : %s", inet_ntop(AF_INET6, &a.Ipv6.sin6_addr, ipv6_str, INET6_ADDRSTRLEN));
            break;
            default:
                fprintf(stdout, "RR Type code %d", ntohs(addit[i].resource->type));
            break;
        }

        fprintf(stdout, "\n");
    }
}
