#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include "dnslookup.h"
#include "Callisto/callisto.h"

static int dns_printf(const char *format, ...);
#define printf(...) dns_printf(__VA_ARGS__)

typedef union {
  struct sockaddr_in ipv4;
  struct sockaddr_in6 ipv6;
} sockaddr_inet;

struct DNS_HEADER
{
  unsigned short id;

  unsigned char rd :1;
  unsigned char tc :1;
  unsigned char aa :1;
  unsigned char opcode :4;
  unsigned char qr :1;

  unsigned char rcode :4;
  unsigned char cd :1;
  unsigned char ad :1;
  unsigned char z :1;
  unsigned char ra :1;

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

//Constant sized fields of query structure
struct QUESTION
{
  unsigned short qtype;
  unsigned short qclass;
};

//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
  unsigned short type;
  unsigned short _class;
  unsigned int ttl;
  unsigned short data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
  unsigned char *name;
  struct R_DATA *resource;
  unsigned char *rdata;
};

//Structure of a query
typedef struct
{
  unsigned char *name;
  struct QUESTION *ques;
} QUERY;

static void change_to_dns_name_format(unsigned char*, unsigned char*);
static unsigned char* read_name(unsigned char*, unsigned char*, int*);
static void dns_free_mem(struct DNS_HEADER *dns, struct RES_RECORD *answers, struct RES_RECORD *auth, struct RES_RECORD *addit);

void print_response_contents(void* dns_ptr, void* answers_ptr, void* auth_ptr, void* addit_ptr, void* a_ptr);

char RootServers[][16] = {
  "198.41.0.4",//A
  "170.247.170.2",//B
  "192.33.4.12",//C
  "199.7.91.13",//D
  "192.203.230.10",//E
  "192.5.5.241",//F
  "192.112.36.4",//G
  "198.97.190.53",//H
  "192.36.148.17",//I
  "192.58.128.30",//J
  "193.0.14.129",//K
  "199.7.83.42",//L
  "202.12.27.33",//M
};
int dns_server_count = 16;

double (*dns_traceroute)(const char* ip, char* out_buf, size_t out_size) = NULL;
double dns_rtt_cutoff = 9999.0f;

#ifdef DNS_PROGRAM
int main(int argc, char *argv[])
{
  unsigned char hostname[100];

  //Get the hostname from the terminal
  printf("Enter Hostname to Lookup : ");
  scanf("%s", hostname);

  //Now get the ip of this hostname, A record
  unsigned char** answers;
  int n_ans = dns_resolve(hostname, T_A, J, &answers);
  for (int i = 0; i < n_ans; i++) {
    free(answers[i]);
  }

  if (n_ans > 0) free(answers);

  #ifdef C_MEMORY_DEBUG
  c_debug_mem_print(0);
  #endif
  return 0;
}
#endif

void dns_init(double (*tracert)(const char* ip, char* out_buf, size_t out_size), double rtt_cutoff)
{
  dns_traceroute = tracert;
  dns_rtt_cutoff = rtt_cutoff;
}

//Perform a DNS query by sending a packet
short dns_resolve(unsigned char *host, int query_type, RootServerIndex root_server, unsigned char*** answer_index)
{
  if (strcmp(host, "/") == 0 || strcmp(host, "/favicon.ico") == 0) {
    *answer_index = (unsigned char**)malloc(1 * sizeof(unsigned char*));
    (*answer_index)[0] = (unsigned char*)malloc(256 * sizeof(unsigned char));
    strcpy((char*)(*answer_index)[0], "127.0.0.1");
    return 1;
  }
  unsigned char buf[65536], *qname, *reader, current_nameserver[256];
  int i, j, stop, s;

  sockaddr_inet a;

  struct RES_RECORD answers[20], auth[20], addit[20]; //Replies from DNS server
  struct sockaddr_in dest;

  struct DNS_HEADER *dns = NULL;
  struct QUESTION *qinfo = NULL;

  memset(answers, 0, sizeof(answers));
  memset(auth, 0, sizeof(auth));
  memset(addit, 0, sizeof(addit));

  printf("Starting iterative resolution for: %s\n", host);

  s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  dest.sin_family = AF_INET;
  dest.sin_port = htons(53);

  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0)
    perror("Failed setting socket options! : ");
  if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0)
    perror("Failed setting socket options! : ");

  strcpy(current_nameserver, RootServers[root_server]);

  while (1)
  {
    if (dns != NULL) {
      dns_free_mem(dns, answers, auth, addit);
      memset(answers, 0, sizeof(answers));
      memset(auth, 0, sizeof(auth));
      memset(addit, 0, sizeof(addit));
    }
    //Set the DNS structure to standard queries
    dns = (struct DNS_HEADER *)&buf;

    //is current nameserver reachable? (traceroute)
    if (dns_traceroute != NULL)
    {
      char traceroute_out[1024];
      double latency = dns_traceroute(current_nameserver, traceroute_out, 1024);
      if (latency > dns_rtt_cutoff) {
        dns->ans_count = -1;
        break;
      }
      //TODO: Query database to avoid excessive traceroute
      //TODO: Determine cable and whether or not to foward packet
    }
    dest.sin_addr.s_addr = inet_addr(current_nameserver);

    dns->id = (unsigned short) htons(getpid());
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
    dns->q_count = htons(1); // we have 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (unsigned char*)&buf[sizeof(struct DNS_HEADER)];

    change_to_dns_name_format(qname, host);
    qinfo = (struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; // fill

    qinfo->qtype = htons(query_type);
    qinfo->qclass = htons(1); // it is indeed internet

    printf("Querying NS: %s\n", inet_ntoa(dest.sin_addr));
    if (sendto(s, (char*)buf, sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0)
    {
      perror("sendto failed\n");

      dns_free_mem(dns, answers, auth, addit);
      return 0;
    }

    //Recv ans
    i = sizeof(dest);
    int bytes_recvd;
    do {
      bytes_recvd = recvfrom(s, (char*)buf, 65536, 0, (struct sockaddr*)&dest, (socklen_t*)&i);
    } while (bytes_recvd < 0 && errno == EINTR);

    if (bytes_recvd < 0)
    {
      if (errno == EAGAIN)
        printf("\x1b[33m[Timeout]\x1b[0m Recvieve from %s timed out.\n", inet_ntoa(dest.sin_addr));
      else
        perror("recvfrom failed\n");

      dns_free_mem(dns, answers, auth, addit);
      return 0;
    }
    
    dns = (struct DNS_HEADER*) buf;

    // move ahead of dns header and query field
    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

    #ifdef DNS_DEBUG
    print_response_info((void*)dns);
    #endif

    // Start reading answers
    stop = 0;

    if (ntohs(dns->ans_count) > 20) dns->ans_count = htons(20);
    if (ntohs(dns->auth_count) > 20) dns->auth_count = htons(20);
    if (ntohs(dns->add_count) > 20) dns->add_count = htons(20);

    for (i = 0; i < ntohs(dns->ans_count); i++)
    {
      answers[i].name = read_name(reader, buf, &stop);
      reader = reader + stop;

      answers[i].resource = (struct R_DATA*)reader;
      reader = reader + sizeof(struct R_DATA);

      switch (ntohs(answers[i].resource->type))
      {
        case T_A:
          answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len)+1);
          //defer(free, answers[i].rdata);
          for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
            answers[i].rdata[j] = reader[j];
          answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
          reader = reader + ntohs(answers[i].resource->data_len);
        break;
        case T_AAAA:
          answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len)+1);
          //defer(free, answers[i].rdata);
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

    // read authorities
    if (ntohs(dns->auth_count) > 20)
      printf("AUTH OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOODIWOAHDIOWAHIODAWHIODHAWIODHIOAWHDIOWAHDIH");
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
      auth[i].name = read_name(reader, buf, &stop);
      reader += stop;

      auth[i].resource = (struct R_DATA*)reader;
      reader += sizeof(struct R_DATA);

      switch (ntohs(auth[i].resource->type))
      {
        case T_NS:
          auth[i].rdata = read_name(reader, buf, &stop);
          reader += stop;
        break;
        case T_SOA:
          //Read MNAME
          auth[i].rdata = read_name(reader, buf, &stop);
          reader += stop;
          //Read RNAME
          unsigned char *rname = read_name(reader, buf, &stop);
          free(rname);
          reader += stop;
          //Adv 20 bytes
          reader += (5 * sizeof(unsigned int));
          //NOTE: May need to access SOA fields later?
        break;
        default:
          auth[i].rdata = read_name(reader, buf, &stop);
          reader = reader + stop;
        break;
      }
    }

    //read Additional
    if (ntohs(dns->add_count) > 20)
      printf("ADDDITIONAL OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOODIWOAHDIOWAHIODAWHIODHAWIODHIOAWHDIOWAHDIH");
    for (i = 0; i < ntohs(dns->add_count); i++)
    {
      addit[i].name = read_name(reader, buf, &stop);
      reader += stop;

      addit[i].resource = (struct R_DATA*)reader;
      reader += sizeof(struct R_DATA);

      switch (ntohs(addit[i].resource->type))
      {
        case T_A:
          addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len)+1);
          //defer(free, addit[i].rdata);
          for (j = 0; j < ntohs(addit[i].resource->data_len); j++)
            addit[i].rdata[j] = reader[j];

          addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
          reader += ntohs(addit[i].resource->data_len);
        break;
        case T_AAAA:
          addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len)+1);
          //defer(free, addit[i].rdata);
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

    //print answers
    #ifdef DNS_DEBUG
    print_response_contents((void*)dns, (void*)answers, (void*)auth, (void*)addit, (void*)&a);
    #endif

    if (ntohs(dns->ans_count) > 0)
    {
      switch (ntohs(answers[0].resource->type)) // TODO: make sure final resource type is the same as the requested type.
      {
        case T_A:
          printf("Found A record.\n");
          *answer_index = (unsigned char**)malloc(ntohs(dns->ans_count) * sizeof(unsigned char*));
          for (i = 0; i < ntohs(dns->ans_count); i++)
          {
            long *p;
            p = (long*)answers[i].rdata;
            a.ipv4.sin_addr.s_addr = (*p);

            (*answer_index)[i] = (unsigned char*)malloc(256 * sizeof(unsigned char));
            strcpy((char*)(*answer_index)[i], inet_ntoa(a.ipv4.sin_addr));
          }

          dns_free_mem(dns, answers, auth, addit);
          return ntohs(dns->ans_count);
        break;
        case T_CNAME:
          printf("Found CNAME alias: %s. Requerying...\n", answers[0].rdata);
          unsigned char* new_host = answers[0].rdata;
          short final_ans_count = dns_resolve(new_host, query_type, root_server, answer_index);

          dns_free_mem(dns, answers, auth, addit);
          return final_ans_count;
        break;
        case T_AAAA:
          printf("Found AAAA record.\n");
          *answer_index = (unsigned char**)malloc(ntohs(dns->ans_count) * sizeof(unsigned char*));
          for (i = 0; i < ntohs(dns->ans_count); i++)
          {
            unsigned char *p;
            p = (unsigned char*)answers[i].rdata;
            memcpy(&a.ipv6.sin6_addr, p, sizeof(struct in6_addr));

            (*answer_index)[i] = (unsigned char*)malloc(256 * sizeof(unsigned char));
            const char *r = inet_ntop(AF_INET6, &a.ipv6.sin6_addr, (char*)(*answer_index)[i], INET6_ADDRSTRLEN);
            if (r == NULL)
              strcpy((char*)(*answer_index[i]), "IPv6 Conversion Error");
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
      print_response_contents((void*)dns, (void*)answers, (void*)auth, (void*)addit, (void*)&a);

      dns_free_mem(dns, answers, auth, addit);
      return 0;
    }

    char next_ns_name[256] = "\0";
    int found_ip = 0;
    for (i = 0; i < ntohs(dns->auth_count); i++)
    {
      // currently finds first matching nammeserver that is ipv4 and has a glue record
      if (ntohs(auth[i].resource->type) != T_NS) continue;

      strcpy(next_ns_name, (char*)auth[i].rdata);

      for (j = 0; j < ntohs(dns->add_count); j++)
      {
        if (strcmp((char*)addit[j].name, next_ns_name) == 0 )
        {
          if (ntohs(addit[j].resource->type) == T_A)
          {
            long *p;
            p = (long*)addit[j].rdata;
            a.ipv4.sin_addr.s_addr = (*p);
            strcpy(current_nameserver, inet_ntoa(a.ipv4.sin_addr));
            printf("Found IPv4 glue record: %s\n", current_nameserver);
            found_ip = 1;
            break;
          }
          if (0)//(ntohs(addit[j].resource->type) == T_AAAA)
          {
            unsigned char *p;
            p = (unsigned char*)addit[j].rdata;
            memcpy(&a.ipv6.sin6_addr, p, sizeof(struct in6_addr));
            const char* r = inet_ntop(AF_INET6, &a.ipv6.sin6_addr, current_nameserver, INET6_ADDRSTRLEN);
            printf("Found IPv6 glue record: %s\n", current_nameserver);
            found_ip = 1;
            break;
          }
        }
      }
      if (found_ip)
        break;
    }
    if (!found_ip && strlen(next_ns_name) > 0) {
      printf("No glue record found. Recrusively resolving nameserver: %s\n", next_ns_name);

      unsigned char** ns_answers;
      int ns_count = dns_resolve((unsigned char*)next_ns_name, T_A, root_server, &ns_answers);

      if (ns_count > 0)
      {
        strcpy(current_nameserver, (char*)ns_answers[0]);
        printf("Resolved nameserver to: %s\n", current_nameserver);
        found_ip = 1;

        for (i = 0; i < ns_count; i++)
          free(ns_answers[i]);
        free(ns_answers);
      }
    }

    if (!found_ip)
    {
      printf("No answer or authority with matching additional record found. OR SOA. Cannot resolve.\n");
      //print_response_contents((void*)dns, (void*)answers, (void*)auth, (void*)addit, (void*)&a);

      dns_free_mem(dns, answers, auth, addit);
      return 0;
    }
  }

  dns_free_mem(dns, answers, auth, addit);
  return ntohs(dns->ans_count);
}

u_char* read_name(unsigned char* reader, unsigned char* buffer, int* count)
{
  unsigned char* name;
  unsigned int p = 0, jumped = 0, offset;
  int i, j;

  *count = 1;
  name = (unsigned char*)malloc(256);
  //defer(free, name);

  name[0] = '\0';

  // Handle message compression
  while (*reader != 0)
  {
    if ((*reader & 0b11000000) == 0b11000000)
    {
      // grab the 14 bit offset after the two 1s
      offset = (*reader)*256 + *(reader+1) - 49152; // 49152 = 11000000 00000000
      reader = buffer + offset - 1;
      jumped = 1; // we have jumped to another location so counting wont go up
    }
    else
    {
      if (p < 256) {
        name[p++] = *reader;
      }
      else {}
    }

    reader++;

    if (jumped == 0)
    {
      (*count)++;
    }
  }

  name[p] = '\0'; // string complete
  if (jumped == 1)
  {
    (*count)++; //number of steps weve moved forward in the packet;
  }

  //convert www6gooogle3com to www.google.com
  for (i = 0; i < (int)strlen((const char*)name); i++)
  {
    p = name[i];
    for (j = 0; j < (int)p; j++)
    {
      name[i] = name[i+1];
      i++;
    }
    name[i] = '.';
  }
  name[i-1] = '\0'; //remove the last dot
  return name;
}

void change_to_dns_name_format(unsigned char* dns, unsigned char* hostname)
{
  unsigned char* dnsstart = dns;
  int lock = 0, i;
  unsigned char host[256];
  strcpy(host, hostname);
  strcat((char*)host,".");

  for (i = 0; i < strlen((char*) host); i++)
  {
    if (host[i]=='.')
    {
      *dns++ = i - lock;
      for (; lock < i; lock++)
      {
        *dns++ = host[lock];
      }
      lock++; // or lock=i+1
    }
  }
  *dns++ = '\0';
}

int dns_printf(const char *format, ...)
{
  va_list args;
  int ret;
  char* msg = "\x1b[34m[DNS]\x1b[0m ";
  fprintf(stdout, "%s", msg);

  va_start(args, format);

  ret = vprintf(format, args);

  va_end(args);

  return ret;
}

void dns_free_mem(struct DNS_HEADER *dns, struct RES_RECORD *answers, struct RES_RECORD *auth, struct RES_RECORD *addit)
{
  int i;
  int ans_count = ntohs(dns->ans_count);
  int auth_count = ntohs(dns->auth_count);
  int add_count = ntohs(dns->add_count);

  for (i = 0; i < 20; i++)
  {
    if (i < ans_count) {
      if (answers[i].name != NULL) {
        free(answers[i].name);
        answers[i].name = NULL;
      }
      if (answers[i].rdata != NULL) {
        free(answers[i].rdata);
        answers[i].rdata = NULL;
      }
    }
    if (i < auth_count) {
      if (auth[i].name != NULL) {
        free(auth[i].name);
        auth[i].name = NULL;
      }
      if (auth[i].rdata != NULL) {
        free(auth[i].rdata);
        auth[i].rdata = NULL;
      }
    }
    if (i < add_count) {
      if (addit[i].name != NULL) {
        free(addit[i].name);
        addit[i].name = NULL;
      }
      if (addit[i].rdata != NULL) {
        free(addit[i].rdata);
        addit[i].rdata = NULL;
      }
    }
  }
}

// debug printing
void print_response_info(void* dns_ptr)
{
  struct DNS_HEADER* dns = (struct DNS_HEADER*)dns_ptr;
  printf("The response contains : \n");
  printf("%d Questions.\n", ntohs(dns->q_count));
  printf("%d Answers.\n", ntohs(dns->ans_count));
  printf("%d Authoritative Servers.\n", ntohs(dns->auth_count));
  printf("%d Additional Records.\n\n", ntohs(dns->add_count));
}

void print_response_contents(void* dns_ptr, void* answers_ptr, void* auth_ptr, void* addit_ptr, void* a_ptr)
{
  struct DNS_HEADER* dns = (struct DNS_HEADER*)dns_ptr;
  struct RES_RECORD* answers = (struct RES_RECORD*)answers_ptr;
  struct RES_RECORD* auth = (struct RES_RECORD*)auth_ptr;
  struct RES_RECORD* addit = (struct RES_RECORD*)addit_ptr;
  sockaddr_inet a = *((sockaddr_inet*)a_ptr);

  int i;

  printf("Answer Records : %d \n", ntohs(dns->ans_count));
  for (i = 0; i < ntohs(dns->ans_count); i++)
  {
    printf("Name : %s ", answers[i].name);

    switch (ntohs(answers[i].resource->type))
    {
      case T_A:
        long *p;
        p = (long*)answers[i].rdata;
        a.ipv4.sin_addr.s_addr = (*p);
        fprintf(stdout, "has IPv4 address : %s", inet_ntoa(a.ipv4.sin_addr));
      break;
      case T_CNAME:
        //Canonical name for an alias
        fprintf(stdout, "has alias name : %s", answers[i].rdata);
      break;
      case T_AAAA:
        unsigned char *p6;
        p6 = (unsigned char*)answers[i].rdata;
        memcpy(&a.ipv6.sin6_addr, p6, sizeof(struct in6_addr));
        char ipv6_str[INET6_ADDRSTRLEN];
        fprintf(stdout, "has IPv6 address: %s", inet_ntop(AF_INET6, &a.ipv6.sin6_addr, ipv6_str, INET6_ADDRSTRLEN));
      break;
    }

    fprintf(stdout, "\n");
  }

  //print authorities
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

  //print additional resource records
  printf("Additional Records : %d \n", ntohs(dns->add_count));
  for (i = 0; i < ntohs(dns->add_count); i++)
  {
    printf("Name : %s ", addit[i].name);
    switch (ntohs(addit[i].resource->type))
    {
      case T_A:
        long *p;
        p = (long*)addit[i].rdata;
        a.ipv4.sin_addr.s_addr = (*p);
        fprintf(stdout, "has IPv4 address : %s", inet_ntoa(a.ipv4.sin_addr));
      break;
      case T_AAAA:
        unsigned char *p6;
        p6 = (unsigned char*)addit[i].rdata;
        memcpy(&a.ipv6.sin6_addr, p6, sizeof(struct in6_addr));
        char ipv6_str[INET6_ADDRSTRLEN];
        fprintf(stdout, "has IPv6 address : %s", inet_ntop(AF_INET6, &a.ipv6.sin6_addr, ipv6_str, INET6_ADDRSTRLEN));
      break;
      default:
        fprintf(stdout, "RR Type code %d", ntohs(addit[i].resource->type));
      break;
    }

    fprintf(stdout, "\n");
  }
}
