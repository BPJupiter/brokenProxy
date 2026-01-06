#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <errno.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "dnslookup.h"
#ifdef C_MEMORY_DEBUG
#include "Callisto/callisto.h"
#endif

static int dns_printf(const char *format, ...);
#define printf dns_printf

typedef union
{
    struct sockaddr_in ipv4;
    struct sockaddr_in6 ipv6;
} sockaddr_inet;

struct DNS_HEADER
{
    unsigned short id;

    unsigned int rd : 1;
    unsigned int tc : 1;
    unsigned int aa : 1;
    unsigned int opcode : 4;
    unsigned int qr : 1;

    unsigned int rcode : 4;
    unsigned int cd : 1;
    unsigned int ad : 1;
    unsigned int z : 1;
    unsigned int ra : 1;

    unsigned short q_count;
    unsigned short ans_count;
    unsigned short auth_count;
    unsigned short add_count;
};

struct SOA_DATA
{
    unsigned char *mname;
    unsigned char *rname;
    unsigned int serial;
    unsigned int refresh;
    unsigned int retry;
    unsigned int expire;
    unsigned int minimum;
};

/* Constant sized fields of query structure */
struct QUERY
{
    unsigned short qtype;
    unsigned short qclass;
};

/* Constant sized fields of the resource record structure */
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

/* Pointers to resource record contents */
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

/* Structure of a query */
struct QUESTION
{
    char *name;
    struct QUERY *ques;
};

static short         dns_recursive_worker(char *host, int query_type, char *current_ns_ip, unsigned char ***answer_index,
                                          int depth);
static void          change_to_dns_name_format(unsigned char *, char *);
static unsigned char *read_name(unsigned char *, unsigned char *, int *);
static void          dns_free_mem(struct DNS_HEADER *dns, struct RES_RECORD *answers, struct RES_RECORD *auth,
                                  struct RES_RECORD *addit);
static int           external_dns_is_blocked(void);
static void          set_to_local_dns(void);

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

double (*dns_traceroute)(const char *ip, char *out_buf, size_t out_size) = NULL;
#ifndef DNS_PROGRAM
extern double rtt_cutoff;
#endif
char current_root_ip[16];
int root_server_found = 0;

#ifdef DNS_PROGRAM
#include "traceroute.h"
double rtt_cutoff = 80.0f;
int main(int argc, char *argv[])
{
    dns_init(ping);
    unsigned char hostname[100];

    /* Get the hostname from the terminal */
    printf("Enter Hostname to Lookup : ");
    scanf("%s", hostname);

    /* Now get the ip of this hostname, A record */
    unsigned char **answers;
    int n_ans = dns_resolve(hostname, &answers);
    for (int i = 0; i < n_ans; i++)
    {
        free(answers[i]);
    }

    if (n_ans > 0) free(answers);

#ifdef C_MEMORY_DEBUG
    c_debug_mem_print(0);
#endif
    return 0;
}
#endif

void dns_init(double (*tracert)(const char *ip, char *out_buf, size_t out_size))
{
    dns_traceroute = tracert;
}

short localhost(unsigned char ***answer_index)
{
    *answer_index = (unsigned char **)malloc(1 * sizeof(unsigned char *));
    (*answer_index)[0] = (unsigned char *)malloc(256 * sizeof(unsigned char));
    strcpy((char *)(*answer_index)[0], "127.0.0.1");
    return 1;
}

short dns_resolve(char *host, unsigned char ***answer_index)
{
    short n_ans = 0;
    int i = 0;
    if (strcmp(host, "/") == 0
        || strcmp(host, "/favicon.ico") == 0
        || strcmp(host, "/style.css") == 0
        || strcmp(host, "/script.js") == 0
        || strcmp(host, "/settings.json") == 0
        || strstr(host, "/settings?rtt=") != NULL)
    {
        return localhost(answer_index);
    }

    if (!root_server_found)
    {
        if (dns_traceroute != NULL)
        {
            for (i = 0; i < RSI_COUNT; i++)
            {
                char traceroute_out[1024];
                double latency = dns_traceroute(RootServers[i], traceroute_out, 1024);
                if (latency >= rtt_cutoff)
                {
                    printf("%s exceeded %.2f ms! Changing root server\n", RootServers[i], rtt_cutoff);
                    continue;
                }
                break;
                /* TODO: Query database to avoid excessive traceroute */
                /* TODO: Determine cable and whether or not to foward packet */
            }
        }
        strcpy(current_root_ip, RootServers[i]);
        if (external_dns_is_blocked())
        {
            set_to_local_dns();
        }
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

/* Perform a DNS query by sending a packet */
short dns_recursive_worker(char *host, int query_type, char *ns_ip, unsigned char ***answer_index, int depth)
{
    unsigned char buf[65536], *qname, *reader;
    struct RES_RECORD answers[20], auth[20], addit[20]; /* Replies from DNS server */
    struct DNS_HEADER *dns = NULL;
    struct QUERY *qinfo = NULL;
    int s;

    sockaddr_inet a;
    struct sockaddr_in dest;
    struct timeval timeout;
    size_t send_size;
    int bytes_recvd;

    int i, j, stop;

    if (depth > 10)
    {
        printf("\x1b[31m[Loop Detected]\x1b[0m Maximum recursion depth exceeded for %s\n", host);
        return 0;
    }

    memset(answers, 0, sizeof(answers));
    memset(auth, 0, sizeof(auth));
    memset(addit, 0, sizeof(addit));

    printf("Starting iterative resolution for: %s\n", host);

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr(ns_ip);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
        perror("Failed setting socket options! : ");
    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
        perror("Failed setting socket options! : ");

    /* Set the DNS structure to standard queries */
    dns = (struct DNS_HEADER *)&buf;

    dns->id = (unsigned short)htons(getpid());
    dns->qr = 0;
    dns->opcode = 0;
    dns->aa = 0;
    dns->tc = 0;
    dns->rd = 1;
    dns->ra = 0;
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); /* we have 1 question */
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (unsigned char *)&buf[sizeof(struct DNS_HEADER)];

    change_to_dns_name_format(qname, host);
    qinfo = (struct QUERY *)&buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1)]; /* fill */

    qinfo->qtype = htons(query_type);
    qinfo->qclass = htons(1); /* it is indeed internet */

    printf("Querying NS: %s\n", inet_ntoa(dest.sin_addr));

    send_size = sizeof(struct DNS_HEADER) + (strlen((const char *)qname ) + 1) + sizeof(struct QUERY);
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

    close(s);

    if (bytes_recvd < 0)
    {
        if (errno == EAGAIN) printf("\x1b[33m[Timeout]\x1b[0m Recvieve from %s timed out.\n", inet_ntoa(dest.sin_addr));
        else
            perror("recvfrom failed");

        dns_free_mem(dns, answers, auth, addit);
        return 0;
    }

    dns = (struct DNS_HEADER *)buf;

    /* move ahead of dns header and query field */
    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(struct QUERY)];

#ifdef DNS_DEBUG
    print_response_info((void *)dns);
#endif

    if (ntohs(dns->ans_count) > 20)  dns->ans_count = htons(20);
    if (ntohs(dns->auth_count) > 20) dns->auth_count = htons(20);
    if (ntohs(dns->add_count) > 20)  dns->add_count = htons(20);

    stop = 0;

    /* Start reading answers */
    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
        answers[i].name = read_name(reader, buf, &stop);
        reader = reader + stop;

        answers[i].resource = (struct R_DATA *)reader;
        reader = reader + sizeof(struct R_DATA);

        switch (ntohs(answers[i].resource->type))
        {
            case T_A:
                answers[i].rdata = (unsigned char *)malloc(ntohs(answers[i].resource->data_len) + 1);
                for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
                    answers[i].rdata[j] = reader[j];
                answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
                reader = reader + ntohs(answers[i].resource->data_len);
            break;
            case T_AAAA:
                answers[i].rdata = (unsigned char *)malloc(ntohs(answers[i].resource->data_len) + 1);
                for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
                    answers[i].rdata[j] = reader[j];
                answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
                reader = reader + ntohs(answers[i].resource->data_len);
            break;
            default:
                answers[i].rdata = read_name(reader, buf, &stop);
                reader = reader + stop;
            break;
        }
    }

    /* read authorities */
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
        auth[i].name = read_name(reader, buf, &stop);
        reader += stop;

        auth[i].resource = (struct R_DATA *)reader;
        reader += sizeof(struct R_DATA);

        switch (ntohs(auth[i].resource->type))
        {
        unsigned char *rname;
            case T_NS:
                auth[i].rdata = read_name(reader, buf, &stop);
                reader += stop;
            break;
            case T_SOA:
                /* Read MNAME */
                auth[i].rdata = read_name(reader, buf, &stop);
                reader += stop;
                /* Read RNAME */
                rname = read_name(reader, buf, &stop);
                free(rname);
                reader += stop;
                /* Adv 20 bytes */
                reader += (5 * sizeof(unsigned int));
                /* NOTE: May need to access SOA fields later? */
            break;
            default:
                auth[i].rdata = read_name(reader, buf, &stop);
                reader = reader + stop;
            break;
        }
    }

    /* read Additional */
    for (i = 0; i < ntohs(dns->add_count); i++)
    {
        addit[i].name = read_name(reader, buf, &stop);
        reader += stop;

        addit[i].resource = (struct R_DATA *)reader;
        reader += sizeof(struct R_DATA);

        switch (ntohs(addit[i].resource->type))
        {
            case T_A:
                addit[i].rdata = (unsigned char *)malloc(ntohs(addit[i].resource->data_len) + 1);
                for (j = 0; j < ntohs(addit[i].resource->data_len); j++)
                    addit[i].rdata[j] = reader[j];

                addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
                reader += ntohs(addit[i].resource->data_len);
            break;
            case T_AAAA:
                addit[i].rdata = (unsigned char *)malloc(ntohs(addit[i].resource->data_len) + 1);
                for (j = 0; j < ntohs(addit[i].resource->data_len); j++)
                    addit[i].rdata[j] = reader[j];

                addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
                reader += ntohs(addit[i].resource->data_len);
            break;
            default:
                addit[i].rdata = read_name(reader, buf, &stop);
                reader = reader + stop;
            break;
        }
    }

/* print answers */
#ifdef DNS_DEBUG
    print_response_contents((void *)dns, (void *)answers, (void *)auth, (void *)addit, (void *)&a);
#endif

    if (ntohs(dns->ans_count) > 0)
    {
        switch (ntohs(answers[0].resource->type)) /* TODO: make sure final resource type is the same as the req type. */
        {
        char cname_target[256];
            case T_A:
                printf("Found A record.\n");
                *answer_index = (unsigned char **)malloc(ntohs(dns->ans_count) * sizeof(unsigned char *));
                for (i = 0; i < ntohs(dns->ans_count); i++)
                {
                    long *p;
                    p = (long *)answers[i].rdata;
                    a.ipv4.sin_addr.s_addr = (*p);

                    (*answer_index)[i] = (unsigned char *)malloc(256 * sizeof(unsigned char));
                    strcpy((char *)(*answer_index)[i], inet_ntoa(a.ipv4.sin_addr));
                }

                dns_free_mem(dns, answers, auth, addit);
                return ntohs(dns->ans_count);
            break;
            case T_CNAME:
                strcpy(cname_target, (char *)answers[0].rdata);
                printf("Found CNAME alias: %s. Requerying...\n", answers[0].rdata);
                dns_free_mem(dns, answers, auth, addit);
                return dns_recursive_worker(cname_target, query_type, current_root_ip, answer_index, depth++);
            break;
            case T_AAAA:
                printf("Found AAAA record.\n");
                *answer_index = (unsigned char **)malloc(ntohs(dns->ans_count) * sizeof(unsigned char *));
                for (i = 0; i < ntohs(dns->ans_count); i++)
                {
                    unsigned char *p;
                    const char *r;

                    p = (unsigned char *)answers[i].rdata;
                    memcpy(&a.ipv6.sin6_addr, p, sizeof(struct in6_addr));

                    (*answer_index)[i] = (unsigned char *)malloc(256 * sizeof(unsigned char));
                    r = inet_ntop(AF_INET6, &a.ipv6.sin6_addr, (char *)(*answer_index)[i], INET6_ADDRSTRLEN);
                    if (r == NULL) strcpy((char *)(*answer_index[i]), "IPv6 Conversion Error");
                }

                dns_free_mem(dns, answers, auth, addit);
                return ntohs(dns->ans_count);
            break;
            default:
                printf("Unknown RR Type code : %d\n", ntohs(answers[0].resource->type));

                dns_free_mem(dns, answers, auth, addit);
                return ntohs(dns->ans_count);
            break;
        }
    }

    if (ntohs(dns->auth_count == 0))
    {
        printf("No answer or authority records found. Cannot resolve iteratively.\n");
        print_response_contents((void *)dns, (void *)answers, (void *)auth, (void *)addit, (void *)&a);

        dns_free_mem(dns, answers, auth, addit);
        return 0;
    }

    /* Find next nameserver */
    {
        char next_ns_name[256] = "\0";
        char next_ns_ip[256] = {0};
        int found_glue = 0;
        int bad_latency = 0;
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

            strcpy(next_ns_name, (char *)auth[i].rdata);

            for (j = 0; j < ntohs(dns->add_count); j++)
            {
                if (strcmp((char *)addit[j].name, next_ns_name) != 0)
                    continue;

                switch (ntohs(addit[j].resource->type))
                {
                long *p;
                    case T_A:
                        p = (long *)addit[j].rdata;
                        a.ipv4.sin_addr.s_addr = (*p);
                        strcpy(next_ns_ip, inet_ntoa(a.ipv4.sin_addr));
                        printf("Found IPv4 glue record: %s\n", next_ns_ip);
                        found_glue = 1;
                    break;
                    case T_AAAA:
                        /*
                           unsigned char *p;
                           p = (unsigned char *)addit[j].rdata;
                           memcpy(&a.ipv6.sin6_addr, p, sizeof(struct in6_addr));
                           const char *r = inet_ntop(AF_INET6, &a.ipv6.sin6_addr, next_ns_ip, INET6_ADDRSTRLEN);
                           printf("Found IPv6 glue record: %s\n", next_ns_ip);
                           found_glue = 1;
                         */
                    break;
                }
                /* Is that nameserver reachable? */
                if (dns_traceroute != NULL && found_glue)
                {
                    char traceroute_out[1024];
                    double latency = dns_traceroute(next_ns_ip, traceroute_out, 1024);
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
            if (bad_latency)
            {
                continue;
            }
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
                unsigned char **ns_answers;

                int ns_count;
                short result;

                int k;
                if (strcmp(host, next_ns_name) == 0)
                {
                    printf("\x1b[31m[Abort]\x1b[0m Circular dependency: Need %s to resolve %s\n", next_ns_name, host);
                    continue;
                }
                /*TODO: iter through a next_ns array rather than the last found NS */
                printf("No glue record for %s. Recrusively resolving NS IP...\n", next_ns_name);

                /*int ns_count = dns_recursive_worker((char *)next_ns_name, T_A, current_root_ip, &ns_answers); */
                ns_count = dns_resolve(next_ns_name, &ns_answers);

                if (ns_count > 0)
                {
                    for (k = 0; k < ns_count; k++)
                    {
                        strcpy(next_ns_ip, (char *)ns_answers[k]);

                        if (dns_traceroute != NULL)
                        {
                            char traceroute_out[1024];
                            double latency = dns_traceroute(next_ns_ip, traceroute_out, 1024);
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
    }

    dns_free_mem(dns, answers, auth, addit);
    return 0;
}

unsigned char *read_name(unsigned char *reader, unsigned char *buffer, int *count)
{
    unsigned char *name;
    unsigned int p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;
    name = (unsigned char *)malloc(256);

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

void change_to_dns_name_format(unsigned char *dns, char *hostname)
{
    /* convert www.google.com to 3www6google3com0 */
    unsigned long lock = 0;
    char host[256];
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

int dns_printf(const char *format, ...)
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

void dns_free_mem(struct DNS_HEADER *dns, struct RES_RECORD *answers, struct RES_RECORD *auth, struct RES_RECORD *addit)
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

/* debug printing */
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
    sockaddr_inet a = *((sockaddr_inet *)a_ptr);

    int i;

    printf("Answer Records : %d \n", ntohs(dns->ans_count));
    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
        printf("Name : %s ", answers[i].name);

        switch (ntohs(answers[i].resource->type))
        {
        long *p;
        unsigned char *p6;
        char ipv6_str[INET6_ADDRSTRLEN];
            case T_A:
                p = (long *)answers[i].rdata;
                a.ipv4.sin_addr.s_addr = (*p);
                fprintf(stdout, "has IPv4 address : %s", inet_ntoa(a.ipv4.sin_addr));
            break;
            case T_CNAME:
                /* Canonical name for an alias */
                fprintf(stdout, "has alias name : %s", answers[i].rdata);
            break;
            case T_AAAA:
                p6 = (unsigned char *)answers[i].rdata;
                memcpy(&a.ipv6.sin6_addr, p6, sizeof(struct in6_addr));
                fprintf(stdout, "has IPv6 address: %s", inet_ntop(AF_INET6, &a.ipv6.sin6_addr, ipv6_str, INET6_ADDRSTRLEN));
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
        unsigned char *p6;
        char ipv6_str[INET6_ADDRSTRLEN];
            case T_A:
                p = (long *)addit[i].rdata;
                a.ipv4.sin_addr.s_addr = (*p);
                fprintf(stdout, "has IPv4 address : %s", inet_ntoa(a.ipv4.sin_addr));
            break;
            case T_AAAA:
                p6 = (unsigned char *)addit[i].rdata;
                memcpy(&a.ipv6.sin6_addr, p6, sizeof(struct in6_addr));
                fprintf(stdout, "has IPv6 address : %s", inet_ntop(AF_INET6, &a.ipv6.sin6_addr, ipv6_str, INET6_ADDRSTRLEN));
            break;
            default:
                fprintf(stdout, "RR Type code %d", ntohs(addit[i].resource->type));
            break;
        }

        fprintf(stdout, "\n");
    }
}

static int external_dns_is_blocked(void)
{
    int n_ans, i;
    unsigned char **answers;
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
    struct __res_state dns;
    struct in_addr addr;
    res_ninit(&dns);

    addr = dns.nsaddr_list[0].sin_addr;
    /* should check string length here...  */
    strcpy(current_root_ip, inet_ntoa(addr));
    return;
}
