#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "Clay/clay.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "dnslookup.h"
#include "shared_context.h"

static int dns_printf(const char *format, ...);
#define printf dns_printf

struct DNS_HEADER
{
    uint16 id;
    uint16 flags;
	uint16 q_count;
    uint16 ans_count;
    uint16 auth_count;
    uint16 add_count;
};

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

struct SOA_DATA
{
    uint8 *mname;
    uint8 *rname;
    uint serial;
    uint refresh;
    uint retry;
    uint expire;
    uint minimum;
};

/* Constant sized fields of query structure */
struct QUERY
{
    uint16 qtype;
    uint16 qclass;
};

/* Constant sized fields of the resource record structure */
#pragma pack(push, 1)
struct R_DATA
{
    uint16 type;
    uint16 _class;
    uint ttl;
    uint16 data_len;
};
#pragma pack(pop)

/* Pointers to resource record contents */
struct RES_RECORD
{
    uint8 *name;
    struct R_DATA *resource;
    uint8 *rdata;
};

/* Structure of a query */
struct QUESTION
{
    char *name;
    struct QUERY *ques;
};

static short dns_recursive_worker(const char *host, int query_type, char *current_ns_ip, char ***answer_index, int depth);
static size_t init_dns_query(uint8 *buf, const char *host, int query_type);
static void read_answers(struct DNS_HEADER *dns, struct RES_RECORD *answers, uint8 **reader, uint8 *buf, int *stop);
static void read_authorities(struct DNS_HEADER *dns, struct RES_RECORD *auth, uint8 **reader, uint8 *buf, int *stop);
static void read_additional(struct DNS_HEADER *dns, struct RES_RECORD *addit, uint8 **reader, uint8 *buf, int *stop);
static short handle_found_answers(struct DNS_HEADER *dns,
                                  struct RES_RECORD *answers,
                                  struct RES_RECORD *auth,
                                  struct RES_RECORD *addit,
                                  char ***answer_index, int query_type, int depth);
static short process_auth_records(struct DNS_HEADER *dns,
                                  struct RES_RECORD *answers,
                                  struct RES_RECORD *auth,
                                  struct RES_RECORD *addit,
                                  const char *host, int query_type, char ***answer_index, int depth);
static void  change_to_dns_name_format(uint8 *, const char *);
static uint8 *read_name(uint8 *, uint8 *, int *);
static void  dns_free_mem(struct DNS_HEADER *dns, struct RES_RECORD *answers, struct RES_RECORD *auth,
                          struct RES_RECORD *addit);
static int   external_dns_is_blocked(void);
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

short localhost(char ***answer_index)
{
    char lh[] = "127.0.0.1";

    *answer_index = malloc(sizeof **answer_index);
    talos_malloc_assert(*answer_index);

    (*answer_index)[0] = malloc(256 * sizeof(***answer_index));
    talos_malloc_assert((*answer_index)[0]);

    c_text_copy(strlen(lh) + 1, (*answer_index)[0], lh);
    return 1;
}

short quick_resolve(const char *host, char ***answer_index)
{
    StyxNetworkAddress address;
    char ipstr[16];
    if (host[0] == '/')
    {
        return localhost(answer_index);
    }

    if (!styx_network_address_lookup(&address, host, 443))
    {
        printf("Quick Resolve: Could not resolve!\n");
        return 0;
    }

    *answer_index = malloc(sizeof(**answer_index));
    talos_malloc_assert(*answer_index);

    (*answer_index)[0] = malloc((sizeof ***answer_index) * 256);
    talos_malloc_assert((*answer_index)[0]);

    styx_ipv4_to_string(ipstr, &address);
    c_text_copy(strlen(ipstr) + 1, (*answer_index)[0], ipstr);
    return 1;
}

short dns_resolve(const char *host, char ***answer_index)
{
    short n_ans = 0;
    int i = 0;

    if (host[0] == '/')
    {
        return localhost(answer_index);
    }

    if (!root_server_found && strcmp(host, "127.0.0.1") != 0)
    {
        for (i = 0; i < RSI_COUNT; i++)
        {
            double rtt_cutoff, latency;
            argType a = {0};
            retType r = {0};
            a.ping.ip = RootServers[i];
            sharedContext_getVariable(SCV_MAX_RTT, &rtt_cutoff);
            sharedContext_execCb(SCC_PING, &r, &a);
            latency = r.ping;
            if (latency >= rtt_cutoff)
            {
                printf("%s exceeded %.2f ms! Changing root server\n", RootServers[i], rtt_cutoff);
                continue;
            }
            break;
            /* TODO: Query database to avoid excessive traceroute */
            /* TODO: Determine cable and whether or not to foward packet */
        }
        strcpy(current_root_ip, RootServers[i]);
        root_server_found = 1;
    }
    n_ans = dns_recursive_worker(host, T_A, current_root_ip, answer_index, 0);
    if (n_ans <= 0)
    {
        printf("Could not resolve!\n");
        return 0;
    }
    else
        return n_ans;
}

/* --------- UNIMPLEMENTED ---------- */
short local_resolve(const char *host, char ***answer_index)
{
    if (host[0] == '/')
    {
        return localhost(answer_index);
    }
    return 0;
}

/* Perform a DNS query by sending a packet */
static short dns_recursive_worker(const char *host, int query_type, char *ns_ip, char ***answer_index, int depth)
{
    uint8 buf[65536] = {0};
    uint8 *qname, *reader;
    struct RES_RECORD answers[20] = {0};
    struct RES_RECORD auth[20] = {0};
    struct RES_RECORD addit[20] = {0};
    struct DNS_HEADER *dns = NULL;
    VSocket s;

    StyxSockaddrInet a = {0};
    StyxSockaddrInet dest = {0};
    size_t send_size;
    int bytes_recvd;

    int i, stop;

    if (depth > 10)
    {
        printf("\x1b[31m[Loop Detected]\x1b[0m Maximum recursion depth exceeded for %s\n", host);
        return 0;
    }

    printf("Starting iterative resolution for: %s\n", host);

    s = styx_socket_create(FALSE, 0);

    dest.Ipv4.sin_family = AF_INET;
    dest.Ipv4.sin_port = htons(53);
    dest.Ipv4.sin_addr.s_addr = inet_addr(ns_ip);

    styx_socket_set_timeout(s, 5);

    send_size = init_dns_query(buf, host, query_type);
    dns = (struct DNS_HEADER *)buf;
    qname = &buf[sizeof(struct DNS_HEADER)];

    printf("Querying NS: %s\n", inet_ntoa(dest.Ipv4.sin_addr));

    if (sendto(s, (char *)buf, send_size, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
    {
        perror("sendto failed");

        dns_free_mem(dns, answers, auth, addit);
        return 0;
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
        if (errno == EAGAIN) printf("\x1b[33m[Timeout]\x1b[0m Recvieve from %s timed out.\n", inet_ntoa(dest.Ipv4.sin_addr));
        else
            perror("recvfrom failed");

        dns_free_mem(dns, answers, auth, addit);
        return 0;
    }

    if (ntohs(dns->ans_count) > 20)  dns->ans_count = htons(20);
    if (ntohs(dns->auth_count) > 20) dns->auth_count = htons(20);
    if (ntohs(dns->add_count) > 20)  dns->add_count = htons(20);

#ifdef DNS_DEBUG
    print_response_info((void *)dns);
#endif

    /* move ahead of dns header and query field */
    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(struct QUERY)];
    stop = 0;

    read_answers(dns, answers, &reader, buf, &stop);
    read_authorities(dns, auth, &reader, buf, &stop);
    read_additional(dns, addit, &reader, buf, &stop);

#ifdef DNS_DEBUG
    print_response_contents((void *)dns, (void *)answers, (void *)auth, (void *)addit, (void *)&a);
#endif

    if (ntohs(dns->ans_count) > 0)
        return handle_found_answers(dns, answers, auth, addit, answer_index, query_type, depth);

    if (ntohs(dns->auth_count) > 0)
        return process_auth_records(dns, answers, auth, addit, host, query_type, answer_index, depth);

    printf("No answer or authority records found. Cannot resolve iteratively.\n");
    print_response_contents((void *)dns, (void *)answers, (void *)auth, (void *)addit, (void *)&a);

    dns_free_mem(dns, answers, auth, addit);
    return 0;
}

static size_t init_dns_query(uint8 *buf, const char *host, int query_type)
{
    struct DNS_HEADER *dns = (struct DNS_HEADER *)buf;
    uint8 *qname;
    uint16 f = 0;
    struct QUERY *qinfo = {0};

    dns->id = (uint16)htons(getpid());
    f |= DNS_FLAG_RD;
    dns->flags = htons(f);
    dns->q_count = htons(1); /* we have 1 question */
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (uint8 *)&buf[sizeof(struct DNS_HEADER)];
    change_to_dns_name_format(qname, host);

    qinfo = (struct QUERY *)&buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1)]; /* fill */

    qinfo->qtype = htons(query_type);
    qinfo->qclass = htons(1); /* it is indeed internet */

    return sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(struct QUERY);
}

static void read_answers(struct DNS_HEADER *dns, struct RES_RECORD *answers, uint8 **reader, uint8 *buf, int *stop)
{
    int i, j;
    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
        answers[i].name = read_name(*reader, buf, stop);
        *reader = *reader + *stop;

        answers[i].resource = (struct R_DATA *)*reader;
        *reader = *reader + sizeof(struct R_DATA);

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
                answers[i].rdata = read_name(*reader, buf, stop);
                *reader = *reader + *stop;
            break;
        }
    }
}

static void read_authorities(struct DNS_HEADER *dns, struct RES_RECORD *auth, uint8 **reader, uint8 *buf, int *stop)
{
    int i;
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        auth[i].name = read_name(*reader, buf, stop);
        *reader += *stop;

        auth[i].resource = (struct R_DATA *)*reader;
        *reader += sizeof(struct R_DATA);

        switch (ntohs(auth[i].resource->type))
        {
        uint8 *rname;
            case T_NS:
                auth[i].rdata = read_name(*reader, buf, stop);
                *reader += *stop;
            break;
            case T_SOA:
                /* Read MNAME */
                auth[i].rdata = read_name(*reader, buf, stop);
                *reader += *stop;
                /* Read RNAME */
                rname = read_name(*reader, buf, stop);
                free(rname);
                *reader += *stop;
                /* Adv 20 bytes */
                *reader += (5 * sizeof(uint));
                /* NOTE: May need to access SOA fields later? */
            break;
            default:
                auth[i].rdata = read_name(*reader, buf, stop);
                *reader = *reader + *stop;
            break;
        }
    }
}

static void read_additional(struct DNS_HEADER *dns, struct RES_RECORD *addit, uint8 **reader, uint8 *buf, int *stop)
{
    int i, j;
    for (i = 0; i < ntohs(dns->add_count); i++)
    {
        addit[i].name = read_name(*reader, buf, stop);
        *reader += *stop;

        addit[i].resource = (struct R_DATA *)*reader;
        *reader += sizeof(struct R_DATA);

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
                addit[i].rdata = read_name(*reader, buf, stop);
                *reader = *reader + *stop;
            break;
        }
    }
}

static short handle_found_answers(struct DNS_HEADER *dns,
                                  struct RES_RECORD *answers,
                                  struct RES_RECORD *auth,
                                  struct RES_RECORD *addit,
                                  char ***answer_index, int query_type, int depth)
{
    StyxSockaddrInet a = {0};
    char cname_target[256] = {0};
    int ans_count;
    int i;

    ans_count = ntohs(dns->ans_count);

    switch (ntohs(answers[0].resource->type)) /* TODO: make sure final resource type is the same as the req type. */
    {
        case T_A:
            printf("Found A record.\n");
            *answer_index = malloc((sizeof **answer_index) * ans_count);
            talos_malloc_assert(*answer_index);

            for (i = 0; i < ans_count; i++)
            {
                long *p;
                char *addr;
                p = (long *)answers[i].rdata;
                a.Ipv4.sin_addr.s_addr = (*p);
                addr = inet_ntoa(a.Ipv4.sin_addr);

                (*answer_index)[i] = malloc(256 * sizeof(***answer_index));
                talos_malloc_assert((*answer_index)[i]);

                c_text_copy(strlen(addr) + 1, (*answer_index)[i], addr);
            }

            dns_free_mem(dns, answers, auth, addit);
            return ans_count;
        break;
        case T_CNAME:
            c_text_copy(strlen((char *)answers[0].rdata) + 1, cname_target, (char *)answers[0].rdata);
            printf("Found CNAME alias: %s. Requerying...\n", answers[0].rdata);
            dns_free_mem(dns, answers, auth, addit);
            return dns_recursive_worker(cname_target, query_type, current_root_ip, answer_index, depth++);
        break;
        case T_AAAA:
            printf("Found AAAA record.\n");
            *answer_index = malloc(ans_count * sizeof(**answer_index));
            talos_malloc_assert(*answer_index);

            for (i = 0; i < ans_count; i++)
            {
                uint8 *p;
                const char *r;

                p = (uint8 *)answers[i].rdata;
                memcpy(&a.Ipv6.sin6_addr, p, sizeof(struct in6_addr));

                (*answer_index)[i] = malloc(256 * sizeof(***answer_index));
                talos_malloc_assert((*answer_index)[i]);

                r = inet_ntop(AF_INET6, &a.Ipv6.sin6_addr, (*answer_index)[i], INET6_ADDRSTRLEN);
                if (r == NULL) strcpy((*answer_index[i]), "IPv6 Conversion Error");
            }

            dns_free_mem(dns, answers, auth, addit);
            return ans_count;
        break;
        default:
            printf("Unknown RR Type code : %d\n", ntohs(answers[0].resource->type));

            dns_free_mem(dns, answers, auth, addit);
            return ans_count;
        break;
    }
}

static short process_auth_records(struct DNS_HEADER *dns,
                                  struct RES_RECORD *answers,
                                  struct RES_RECORD *auth,
                                  struct RES_RECORD *addit,
                                  const char *host, int query_type, char ***answer_index, int depth)
{
    StyxSockaddrInet a = {0};
    char next_ns_ip[16] = {0};
    char next_ns_name[256] = "\0";
    int found_glue = 0, bad_latency = 0;
    int i, j, k;

    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        bad_latency = 0;
        /* currently finds first matching nammeserver that is ipv4 and has a glue record */
        if (ntohs(auth[i].resource->type) == T_SOA)
        {
            printf("SOA Record: %s does not exist.\n", host);
            dns_free_mem(dns, answers, auth, addit);
            return 0;
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
                double rtt_cutoff, latency;
                argType a = {0};
                retType r = {0};
                a.ping.ip = next_ns_ip;
                sharedContext_getVariable(SCV_MAX_RTT, &rtt_cutoff);
                sharedContext_execCb(SCC_PING, &r, &a);
                latency = r.ping;
                if (latency > rtt_cutoff)
                {
                    printf("%s exceeded %.2lf ms! Skipping.\n", next_ns_ip, rtt_cutoff);
                    found_glue = 0;
                    bad_latency = 1;
                    break;
                }
                /* TODO: Query database to avoid excessive traceroute */
                /* TODO: Determine cable and whether or not to foward packet */
            }
            if (found_glue) break;
        }
        if (bad_latency) continue;
        if (found_glue)
        {
            /* Recursion */
            short ans_count = dns_recursive_worker(host, query_type, next_ns_ip, answer_index, depth++);

            if (ans_count > 0)
            {
                dns_free_mem(dns, answers, auth, addit);
                return ans_count;
            }
        }
        else
        {
            char **ns_answers;

            int ns_count;
            short result;

            if (strcmp(host, next_ns_name) == 0)
            {
                printf("\x1b[31m[Abort]\x1b[0m Circular dependency: Need %s to resolve %s\n", next_ns_name, host);
                continue;
            }
            printf("No glue record for %s. Recrusively resolving NS IP...\n", next_ns_name);

            /*int ns_count = dns_recursive_worker((char *)next_ns_name, T_A, current_root_ip, ns_answers); */
            ns_count = dns_resolve(next_ns_name, &ns_answers);

            if (ns_count > 0)
            {
                double latency, rtt_cutoff;
                argType a = {0};
                retType r = {0};
                for (k = 0; k < ns_count; k++)
                {
                    c_text_copy(strlen(ns_answers[k]) + 1, next_ns_ip, ns_answers[k]);

                    if (strcmp(next_ns_ip, "127.0.0.1") != 0)
                    {
                        a.ping.ip = next_ns_ip;
                        sharedContext_getVariable(SCV_MAX_RTT, &rtt_cutoff);
                        sharedContext_execCb(SCC_PING, &r, &a);
                        latency = r.ping;
                        if (latency > rtt_cutoff)
                        {
                            printf("%s exceeded %.2lf ms! Skipping.\n", next_ns_ip, rtt_cutoff);
                            continue;
                        }
                        /* TODO: Query database to avoid excessive traceroute */
                        /* TODO: Determine cable and whether or not to foward packet */
                    }

                    result = dns_recursive_worker(host, query_type, next_ns_ip, answer_index, depth++);
                    if (result > 0)
                    {
                        for (i = 0; i < ns_count; i++)
                            free(ns_answers[i]);
                        free(ns_answers);

                        dns_free_mem(dns, answers, auth, addit);
                        return result;
                    }
                }
                for (i = 0; i < ns_count; i++)
                    free(ns_answers[i]);
                free(ns_answers);
            }
        }
    }
    dns_free_mem(dns, answers, auth, addit);
    return 0;
}

static uint8 *read_name(uint8 *reader, uint8 *buffer, int *count)
{
    uint8 *name;
    uint p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;
    name = malloc(256);
    talos_malloc_assert(name);

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
    return name;
}

static void change_to_dns_name_format(uint8 *dns, const char *hostname)
{
    /* convert www.google.com to 3www6google3com0 */
    unsigned long lock = 0;
    char host[256] = {0};
    unsigned long i;
    strcpy(host, hostname);
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

static void dns_free_mem(struct DNS_HEADER *dns, struct RES_RECORD *answers, struct RES_RECORD *auth, struct RES_RECORD *addit)
{
    int ans_count = ntohs(dns->ans_count);
    int auth_count = ntohs(dns->auth_count);
    int add_count = ntohs(dns->add_count);

    int i;

    for (i = 0; i < 20; i++)
    {
        if (i < ans_count)
        {
            if (answers[i].name != NULL)
            {
                free(answers[i].name);
                answers[i].name = NULL;
            }
            if (answers[i].rdata != NULL)
            {
                free(answers[i].rdata);
                answers[i].rdata = NULL;
            }
        }
        if (i < auth_count)
        {
            if (auth[i].name != NULL)
            {
                free(auth[i].name);
                auth[i].name = NULL;
            }
            if (auth[i].rdata != NULL)
            {
                free(auth[i].rdata);
                auth[i].rdata = NULL;
            }
        }
        if (i < add_count)
        {
            if (addit[i].name != NULL)
            {
                free(addit[i].name);
                addit[i].name = NULL;
            }
            if (addit[i].rdata != NULL)
            {
                free(addit[i].rdata);
                addit[i].rdata = NULL;
            }
        }
    }
}

static int external_dns_is_blocked(void)
{
    int n_ans, i;
    char **answers;
    n_ans = dns_recursive_worker("www.google.com", T_A, current_root_ip, &answers, 0);
    if (n_ans <= 0)
        return 1;
    for (i = 0; i < n_ans; i++)
        free(answers[i]);
    free(answers);
    return 0;
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
    struct DNS_HEADER *dns = (struct DNS_HEADER *)dns_ptr;
    printf("The response contains : \n");
    printf("%d Questions.\n", ntohs(dns->q_count));
    printf("%d Answers.\n", ntohs(dns->ans_count));
    printf("%d Authoritative Servers.\n", ntohs(dns->auth_count));
    printf("%d Additional Records.\n\n", ntohs(dns->add_count));
}

void print_response_contents(void *dns_ptr, void *answers_ptr, void *auth_ptr, void *addit_ptr, void *a_ptr)
{
    struct DNS_HEADER *dns = (struct DNS_HEADER *)dns_ptr;
    struct RES_RECORD *answers = (struct RES_RECORD *)answers_ptr;
    struct RES_RECORD *auth = (struct RES_RECORD *)auth_ptr;
    struct RES_RECORD *addit = (struct RES_RECORD *)addit_ptr;
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
