#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "Clay/clay.h"
#include "Styx/styx.h"
#include "Talos/talos.h"

#include "verify/verify.h"
#include "context.h"

static int dns_printf(const char *format, ...);
#define printf dns_printf

#define DNS_MAX_LABEL_LEN   63
#define DNS_MAX_NAME_LEN    255
#define DNS_MAX_RDATA_LEN   512

typedef char DnsName[DNS_MAX_NAME_LEN + 1];

typedef enum {
    /* RESERVED     =   0,    rfc 6895 */
    DNS_TYPE_A      =   1, /* rfc 1035 */
    DNS_TYPE_NS     =   2, /* rfc 1035 */
    DNS_TYPE_MD     =   3, /* OBSOLETE: rfc 883, 973, 1035 */
    DNS_TYPE_MF     =   4, /* OBSOLETE: rfc 883, 973, 1035 */
    DNS_TYPE_CNAME  =   5, /* rfc 1035 */
    DNS_TYPE_SOA    =   6, /* rfc 1035, 2308 */
    DNS_TYPE_MB     =   7, /* EXPERIMENTAL: rfc 883, 1035, 2505 */
    DNS_TYPE_MG     =   8, /* EXPERIMENTAL: rfc 883, 1035, 2505 */
    DNS_TYPE_MR     =   9, /* EXPERIMENTAL: rfc 883, 1035, 2505 */
    DNS_TYPE_NULL   =  10, /* EXPERIMENTAL: rfc 883, 1035 */
    DNS_TYPE_WKS    =  11, /* EXPERIMENTAL: rfc 883, 1035 */
    DNS_TYPE_PTR    =  12, /* rfc 1035 */
    DNS_TYPE_HINFO  =  13, /* rfc 883, 1035, 8482 */
    DNS_TYPE_MINFO  =  14, /* EXPERIMENTAL: rfc 883, 1035, 2505 */
    DNS_TYPE_MX     =  15, /* rfc 1035, 7505 */
    DNS_TYPE_TXT    =  16, /* rfc 1035 */
    DNS_TYPE_RP     =  17, /* OBSOLETE: rfc 1183 */
    DNS_TYPE_AFSDB  =  18, /* rfc 1183 */
    DNS_TYPE_X25    =  19, /* OBSOLETE: rfc 1183 */
    DNS_TYPE_ISDN   =  20, /* OBSOLETE: rfc 1183 */
    DNS_TYPE_RT     =  21, /* OBSOLETE: rfc 1183 */
    DNS_TYPE_NSAP   =  22, /* OBSOLETE: rfc 1706 */
    DNS_TYPE_NSAPPTR = 23, /* OBSOLETE: rfc 1706 */
    DNS_TYPE_SIG    =  24, /* OBSOLETE: rfc 2065, 2535, 3755 */
    DNS_TYPE_KEY    =  25, /* OBSOLETE: rfc 2065, 2535, 2930, 3755 */
    DNS_TYPE_PX     =  26, /* OBSOLETE: rfc 2163 */
    DNS_TYPE_GPOS   =  27, /* OBSOLETE: rfc 1712 */
    DNS_TYPE_AAAA   =  28, /* rfc 3596 */
    DNS_TYPE_LOC    =  29, /* rfc 1876 */
    DNS_TYPE_NXT    =  30, /* OBSOLETE: rfc 2065, 3755 */
    DNS_TYPE_EID    =  31, /* OBSOLETE: - */
    DNS_TYPE_NIMLOC =  32, /* OBSOLETE: - */
    DNS_TYPE_NB     =  32, /* OBSOLETE: rfc 1002 */
    DNS_TYPE_NBSTAT =  33, /* OBSOLETE: rfc 1002 */
    DNS_TYPE_SRV    =  33, /* rfc 2782 */
    DNS_TYPE_ATMA   =  34, /* OBSOLETE: - */
    DNS_TYPE_NAPTR  =  35, /* rfc 3403 */
    DNS_TYPE_KX     =  36, /* rfc 2230 */
    DNS_TYPE_CERT   =  37, /* rfc 4398 */
    DNS_TYPE_A6     =  38, /* OBSOLETE: rfc 2874, 3363, 6563 */
    DNS_TYPE_DNAME  =  39, /* rfc 6672 */
    DNS_TYPE_SINK   =  40, /* OBSOLETE: - */
    DNS_TYPE_OPT    =  41, /* PSEUDO: rfc 3225, 6891 */
    DNS_TYPE_APL    =  42, /* EXPERIMENTAL: rfc 3123 */
    DNS_TYPE_DS     =  43, /* rfc 4034 */
    DNS_TYPE_SSHFP  =  44, /* rfc 4255 */
    DNS_TYPE_IPSECKEY = 45, /* rfc 4025 */
    DNS_TYPE_RRSIG  =  46, /* rfc 4034 */
    DNS_TYPE_NSEC   =  47, /* rfc 4034 */
    DNS_TYPE_DNSKEY =  48, /* rfc 4034 */
    DNS_TYPE_DHCID  =  49, /* rfc 4701 */
    DNS_TYPE_NSEC3  =  50, /* rfc 5155 */
    DNS_TYPE_NSEC3PARAM = 51, /* rfc 5155 */
    DNS_TYPE_TLSA   =  52, /* rfc 6698 */
    DNS_TYPE_SMIMEA =  53, /* rfc 8162 */
    /* UNASSIGNED   =  54,             */
    DNS_TYPE_HIP    =  55, /* rfc 8005 */
    DNS_TYPE_NINFO  =  56, /* OBSOLETE: - */
    DNS_TYPE_RKEY   =  57, /* OBSOLETE: - */
    DNS_TYPE_TALINK =  58, /* OBSOLETE: - */
    DNS_TYPE_CDS    =  59, /* rfc 7344 */
    DNS_TYPE_CDNSKEY = 60, /* rfc 7344 */
    DNS_TYPE_OPENPGPKEY = 61, /* rfc 7929 */
    DNS_TYPE_CSYNC  =  62, /* rfc 7477 */
    DNS_TYPE_ZONEMD =  63, /* rfc 8976 */
    DNS_TYPE_SVCB   =  64, /* rfc 9460 */
    DNS_TYPE_HTTPS  =  65, /* rfc 9460 */
    DNS_TYPE_DSYNC  =  66, /* rfc 9859 */
    DNS_TYPE_HHIT   =  67, /* rfc 9886 */
    DNS_TYPE_BRID   =  68, /* rfc 9886 */
    /* UNASSIGNED   =  69,
                    =  98,             */
    DNS_TYPE_SPF    =  99, /* OBSOLETE: rfc 4408, 7208 */
    DNS_TYPE_UINFO  = 100, /* OBSOLETE: - */
    DNS_TYPE_UID    = 101, /* OBSOLETE: - */
    DNS_TYPE_GID    = 102, /* OBSOLETE: - */
    DNS_TYPE_UNSPEC = 103, /* OBSOLETE: - */
    DNS_TYPE_NID    = 104, /* EXPERIMENTAL: rfc 6742 */
    DNS_TYPE_L32    = 105, /* EXPERIMENTAL: rfc 6742 */
    DNS_TYPE_L64    = 106, /* EXPERIMENTAL: rfc 6742 */
    DNS_TYPE_LP     = 107, /* EXPERIMENTAL: rfc 6742 */
    DNS_TYPE_EUI48  = 108, /* rfc 7043 */
    DNS_TYPE_EUI64  = 109, /* rfc 7043 */
    /* UNASSIGNED   = 110,
                    = 127,             */
    DNS_TYPE_NXNAME = 128, /* rfc 9824 */
    /* UNASSIGNED   = 129,
                    = 248,             */
    DNS_TYPE_TKEY   = 249, /* rfc 2930 */
    DNS_TYPE_TSIG   = 250, /* rfc 2845 */
    DNS_QTYPE_IXFR  = 251, /* rfc 1996 */
    DNS_QTYPE_AXFR  = 252, /* rfc 1035 */
    DNS_QTYPE_MAILB = 253, /* EXPERIMENTAL: rfc 883, 1035, 2505 */
    DNS_QTYPE_MAILA = 254, /* OBSOLETE: rfc 883, 973, 1035 */
    DNS_QTYPE_ANY   = 255, /* rfc 1035 */
    DNS_TYPE_URI    = 256, /* rfc 7553 */
    DNS_TYPE_CAA    = 257, /* rfc 8659 */
    DNS_TYPE_DOA    = 259, /* OBSOLETE: - */
    DNS_TYPE_AMTRELAY = 260, /* rfc 8777 */
    DNS_TYPE_RESINFO = 261, /* rfc 9606 */
    DNS_TYPE_WALLET = 262, /* https://www.iana.org/assignments/dns-parameters/WALLET/wallet-completed-template */
    DNS_TYPE_CLA    = 263, /* - */
    DNS_TYPE_IPN    = 264, /* - */
    /* UNASSIGNED   = 265,
                  = 32767       */
    DNS_TYPE_TA   = 32768, /* - */
    DNS_TYPE_DLV  = 32769 /* OBSOLETE: rfc 4431, 8749 */
    /* UNASSIGNED = 32770,
                  = 65279,      */
    /* PRIVATE    = 65280,
                  = 65534,      */
    /* RESERVED   = 65535       */
} DnsType;

typedef enum {
    /* RESERVED     =   0,    rfc 6895 */
    DNS_CLASS_IN    =   1, /* rfc 1035 */
    DNS_CLASS_CS    =   2, /* OBSOLETE: rfc 1035 */
    DNS_CLASS_CH    =   3, /* rfc 1035 */
    DNS_CLASS_HS    =   4, /* rfc 1035 */
    /* UNASSIGNED   =   5,
                    = 253,             */
    DNS_QCLASS_NONE = 254, /* rfc 2136 */
    DNS_QCLASS_ANY  = 255  /* rfc 1035 */
    /* UNASSIGNED   = 256,
                    = 65279,           */
    /* PRIVATE      = 65280,
                    = 65534,           */
    /* RESERVED     = 65535            */
} DnsClass;

typedef enum {
    DNS_OPCODE_QUERY    = 0, /* rfc 1035 */
    DNS_OPCODE_IQUERY   = 1, /* rfc 1035 */
    DNS_OPCODE_STATUS   = 2, /* rfc 1035 */
    /* UNASSIGNED       = 3,             */
    DNS_OPCODE_NOTIFY   = 4, /* rfc 1996 */
    DNS_OPCODE_UPDATE   = 5, /* rfc 2136 */
    DNS_OPCODE_DSO      = 6  /* rfc 8490 */
} DnsOpcode;

typedef enum {
    DNS_RCODE_NOERROR   =   0, /* rfc 1035 */
    DNS_RCODE_FORMERR   =   1, /* rfc 1035 */
    DNS_RCODE_SERVFAIL  =   2, /* rfc 1035 */
    DNS_RCODE_NXDOMAIN  =   3, /* rfc 1035 */
    DNS_RCODE_NOTIMP    =   4, /* rfc 1035 */
    DNS_RCODE_REFUSED   =   5, /* rfc 1035 */
    DNS_RCODE_YXDOMAIN  =   6, /* rfc 2136, 6672 */
    DNS_RCODE_YXRRSET   =   7, /* rfc 2136 */
    DNS_RCODE_NXRRSET   =   8, /* rfc 2136 */
    DNS_RCODE_NOTAUTH   =   9, /* rfc 2136, 8945 */
    DNS_RCODE_NOTZONE   =  10, /* rfc 2136 */
    DNS_RCODE_DSOTYPENI =  11, /* rfc 8490 */
    /* UNASSIGNED       =  12,
                        =  15,             */
    DNS_RCODE_BADVERS   =  16, /* rfc 6891 */
    DNS_RCODE_BADSIG    =  16, /* rfc 8945 */
    DNS_RCODE_BADKEY    =  17, /* rfc 8945 */
    DNS_RCODE_BADTIME   =  18, /* rfc 8945 */
    DNS_RCODE_BADMODE   =  19, /* rfc 2930 */
    DNS_RCODE_BADNAME   =  20, /* rfc 2930 */
    DNS_RCODE_BADALG    =  21, /* rfc 2930 */
    DNS_RCODE_BADTRUNC  =  22, /* rfc 8945 */
    DNS_RCODE_BADCOOKIE =  23  /* rfc 7873 */
    /* UNASSIGNED       =  24,
                       = 3840,             */
    /* PRIVATE         = 3841,
                       = 4095,             */
    /* UNASSIGNED      = 4096,
                      = 65534,             */
    /* RESERVED       = 65535              */
} DnsRcode;

typedef struct {
    DnsName name;
    uint16 type;
    uint16 class;
    uint32 ttl;
    uint16 rdlength;
    uint8 rdata[DNS_MAX_RDATA_LEN];
} DnsRecord;

typedef struct {
    uint32 address;
} RdataA;

typedef struct {
    DnsName nsdname;
} RdataNS;

typedef struct {
    DnsName cname;
} RdataCNAME;

typedef struct {
    DnsName mname;
    DnsName rname;
    uint32 serial;
    uint32 refresh;
    uint32 retry;
    uint32 expire;
    uint32 minimum;
} RdataSOA;

typedef struct {
    DnsName ptrdname;
} RdataPTR;

typedef struct {
    uint16 preference;
    DnsName exchange;
} RdataMX;

typedef struct {
    uint8 address[16];
} RdataAAAA;

typedef struct
{
    uint16 id;
    uint16 flags;
    uint16 qdcount;
    uint16 ancount;
    uint16 nscount;
    uint16 arcount;
} DnsHeader;

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

/* Constant sized fields of query structure */
typedef struct
{
    DnsName qname;
    uint16 qtype;
    uint16 qclass;
} DnsQuestion;

typedef struct {
    DnsHeader header;
    DnsQuestion *questions;
    DnsRecord *answers;
    DnsRecord *authorities;
    DnsRecord *resources;
} DnsPacket;

typedef struct {
    uint8 buf[512];
    size_t pos;
} BytePacketBuffer;

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

char current_root_ip[16];

static void buffer_step(BytePacketBuffer *buffer, const size_t steps)
{
    buffer->pos += steps;
}

static void buffer_seek(BytePacketBuffer *buffer, const size_t pos)
{
    buffer->pos = pos;
}

static boolean buffer_read(BytePacketBuffer *buffer, uint8 *out)
{
    if (buffer->pos >= 512) {
        return FALSE;
    }
    *out = buffer->buf[buffer->pos];
    buffer->pos += 1;
    return TRUE;
}

static boolean buffer_get(BytePacketBuffer *buffer, size_t pos, uint8 *out)
{
    if (pos >= 512) {
        return FALSE;
    }
    *out = buffer->buf[pos];
    return TRUE;
}

static boolean buffer_get_range(BytePacketBuffer *buffer, size_t start, uint8 *out_buff, size_t len)
{
    if ((start + len) > 512) {
        return FALSE;
    }
    memcpy(out_buff, &buffer->buf[start], len);
    return TRUE;
}

static boolean buffer_read_uint16(BytePacketBuffer *buffer, uint16 *out)
{
    uint8 n1, n2;
    buffer_read(buffer, &n1);
    buffer_read(buffer, &n2);
    *out = (uint16) n1 << 8;
    *out |= (uint16) n2;
    return TRUE;
}

static boolean buffer_read_uint32(BytePacketBuffer *buffer, uint32 *out)
{
    uint8 n1, n2, n3, n4;
    buffer_read(buffer, &n1);
    buffer_read(buffer, &n2);
    buffer_read(buffer, &n3);
    buffer_read(buffer, &n4);
    *out = (uint32) n1 << 24;
    *out |= (uint32) n2 << 16;
    *out |= (uint32) n3 << 8;
    *out |= (uint32) n4;
    return TRUE;
}

static boolean buffer_read_qname(BytePacketBuffer *buffer, DnsName out)
{
    size_t pos = buffer->pos;
    boolean jumped = FALSE;
    const int max_jumps = 5;
    int jumps_performed = 0;
    uint8 len;
    boolean add_delim = FALSE;
    size_t i, j = 0;

    for (;;) {
        /* Dns packets are untrusted data, so we need to be paranoid.
         * Someone can craft a packet with a cycle in the jump instructions.
         * This guards against such packets
         */
        if (jumps_performed > max_jumps) {
            return FALSE;
        }

        if (!buffer_get(buffer, pos, &len)) {
            return FALSE;
        }
        /* A two byte sequence, where the two highest bits of the first byte is
         * set, represents an offset relative to the start of the buffer.
         * We handle this by jumping to the offset, setting a flag to indicate
         * that we shouldn't update the shared buffer position once done.
         */
        if ((len & 0xC0) == 0xC0) {
            uint8 b2;
            uint16 offset;
            if (!jumped) {
                /* When a jump is performed, we only modify the shared buffer
                 * position once, and avoid making the change later on.
                 */
                buffer_seek(buffer, pos + 2);
            }

            if (!buffer_get(buffer, pos + 1, &b2)) {
                return FALSE;
            }

            offset = ((((uint16)len) ^ 0xC0) << 8) | (uint16)b2;
            pos = (size_t)offset;
            jumped = TRUE;
            jumps_performed += 1;
            continue;
        }

        pos += 1;

        if (len == 0) {
            break;
        }

        if (add_delim) {
            if (j >= DNS_MAX_NAME_LEN) {
                return FALSE;
            }
            out[j++] = '.';
        }

        for (i = 0; i < len; i++, j++) {
            if (j >= DNS_MAX_NAME_LEN) {
                return FALSE;
            }
            if (!buffer_get(buffer, pos + i, (uint8 *)&out[j])) {
                return FALSE;
            }
            out[j] |= 0x20;
        }

        add_delim = TRUE;
        pos += len;
    }

    if (!jumped) {
        buffer_seek(buffer, pos);
    }

    out[j] = '\0';

    return TRUE;
}

static boolean buffer_write(BytePacketBuffer *buffer, uint8 val)
{
    if (buffer->pos >= 512) {
        return FALSE;
    }

    buffer->buf[buffer->pos++] = val;
    return TRUE;
}

static boolean buffer_write_uint16(BytePacketBuffer *buffer, uint16 val)
{
    if (!buffer_write(buffer, (val >> 8) & 0xFF)) {
        return FALSE;
    }
    if (!buffer_write(buffer, val & 0xFF)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_uint32(BytePacketBuffer *buffer, uint32 val)
{
    if (!buffer_write(buffer, (val >> 24) & 0xFF)) {
        return FALSE;
    }
    if (!buffer_write(buffer, (val >> 16) & 0xFF)) {
        return FALSE;
    }
    if (!buffer_write(buffer, (val >> 8) & 0xFF)) {
        return FALSE;
    }
    if (!buffer_write(buffer, val & 0xFF)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_qname(BytePacketBuffer *buffer, const DnsName *qname)
{
    size_t slen = strlen(*qname);
    size_t i, j = 0;

    for (i = 0; i <= slen; i++) {
        if (*qname[i] == '.' || *qname[i] == '\0') {
            size_t len = i - j;

            if (len > 0x3F) {
                return FALSE;
            }

            if (!buffer_write(buffer, i - j)) {
                return FALSE;
            }

            for (; j < i; j++) {
                if (!buffer_write(buffer, *qname[j])) {
                    return FALSE;
                }
            }
            j++;
        }
    }
    buffer_write(buffer, '\0');

    return TRUE;
}

static boolean buffer_set(BytePacketBuffer *buffer, const size_t pos, const uint8 val)
{
    if (pos >= 512) {
        return FALSE;
    }
    buffer->buf[pos] = val;
    return TRUE;
}

static boolean buffer_set_uint16(BytePacketBuffer *buffer, const size_t pos, const uint16 val)
{
    if (!buffer_set(buffer, pos, (val >> 8) & 0xFF)) {
        return FALSE;
    }
    if (!buffer_set(buffer, pos + 1, (val & 0xFF))) {
        return FALSE;
    }

    return TRUE;
}

static boolean buffer_read_dnsheader(BytePacketBuffer *buffer, DnsHeader *out)
{
    if (!buffer_read_uint16(buffer, &out->id)) {
        return FALSE;
    }

    if (!buffer_read_uint16(buffer, &out->flags)) {
        return FALSE;
    }

    if (!buffer_read_uint16(buffer, &out->qdcount)) {
        return FALSE;
    }

    if (!buffer_read_uint16(buffer, &out->ancount)) {
        return FALSE;
    }

    if (!buffer_read_uint16(buffer, &out->nscount)) {
        return FALSE;
    }

    if (!buffer_read_uint16(buffer, &out->arcount)) {
        return FALSE;
    }

    return TRUE;
}

static boolean buffer_write_dnsheader(BytePacketBuffer *buffer, const DnsHeader *in)
{
    if (!buffer_write_uint16(buffer, in->id)) {
        return FALSE;
    }

    if (!buffer_write_uint16(buffer, in->flags)) {
        return FALSE;
    }

    if (!buffer_write_uint16(buffer, in->qdcount)) {
        return FALSE;
    }

    if (!buffer_write_uint16(buffer, in->ancount)) {
        return FALSE;
    }

    if (!buffer_write_uint16(buffer, in->nscount)) {
        return FALSE;
    }

    if (!buffer_write_uint16(buffer, in->arcount)) {
        return FALSE;
    }

    return TRUE;
}

static boolean buffer_read_dnsquestion(BytePacketBuffer *buffer, DnsQuestion *question)
{
    if (!buffer_read_qname(buffer, question->qname)) {
        return FALSE;
    }
    if (!buffer_read_uint16(buffer, &question->qtype)) {
        return FALSE;
    }
    if (!buffer_read_uint16(buffer, &question->qclass)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_dnsquestion(BytePacketBuffer *buffer, const DnsQuestion *question)
{
    if (!buffer_write_qname(buffer, &question->qname)) {
        return FALSE;
    }
    if (!buffer_write_uint16(buffer, question->qtype)) {
        return FALSE;
    }
    if (!buffer_write_uint16(buffer, question->qclass)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_a(BytePacketBuffer *buffer, RdataA *data)
{
    if (!buffer_read_uint32(buffer, &data->address)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_rdata_a(BytePacketBuffer *buffer, const RdataA *data)
{
    if (!buffer_write_uint32(buffer, data->address)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_ns(BytePacketBuffer *buffer, RdataNS *data)
{
    if (!buffer_read_qname(buffer, data->nsdname)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_rdata_ns(BytePacketBuffer *buffer, const RdataNS *data)
{
    if (!buffer_write_qname(buffer, &data->nsdname)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_cname(BytePacketBuffer *buffer, RdataCNAME *data)
{
    if (!buffer_read_qname(buffer, data->cname)) {
        return FALSE;
    }
    return FALSE;
}

static boolean buffer_write_rdata_cname(BytePacketBuffer *buffer, const RdataCNAME *data)
{
    if (!buffer_write_qname(buffer, &data->cname)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_soa(BytePacketBuffer *buffer, RdataSOA *data)
{
    if (!buffer_read_qname(buffer, data->mname)) {
        return FALSE;
    }
    if (!buffer_read_qname(buffer, data->rname)) {
        return FALSE;
    }
    if (!buffer_read_uint32(buffer, &data->serial)) {
        return FALSE;
    }
    if (!buffer_read_uint32(buffer, &data->refresh)) {
        return FALSE;
    }
    if (!buffer_read_uint32(buffer, &data->retry)) {
        return FALSE;
    }
    if (!buffer_read_uint32(buffer, &data->expire)) {
        return FALSE;
    }
    if (!buffer_read_uint32(buffer, &data->minimum)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_rdata_soa(BytePacketBuffer *buffer, const RdataSOA *data)
{
    if (!buffer_write_qname(buffer, &data->mname)) {
        return FALSE;
    }
    if (!buffer_write_qname(buffer, &data->rname)) {
        return FALSE;
    }
    if (!buffer_write_uint32(buffer, data->serial)) {
        return FALSE;
    }
    if (!buffer_write_uint32(buffer, data->refresh)) {
        return FALSE;
    }
    if (!buffer_write_uint32(buffer, data->retry)) {
        return FALSE;
    }
    if (!buffer_write_uint32(buffer, data->expire)) {
        return FALSE;
    }
    if (!buffer_write_uint32(buffer, data->minimum)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_ptr(BytePacketBuffer *buffer, RdataPTR *data)
{
    if (!buffer_read_qname(buffer, data->ptrdname)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_rdata_ptr(BytePacketBuffer *buffer, const RdataPTR *data)
{
    if (!buffer_write_qname(buffer, &data->ptrdname)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_mx(BytePacketBuffer *buffer, RdataMX *data)
{
    if (!buffer_read_uint16(buffer, &data->preference)) {
        return FALSE;
    }
    if (!buffer_read_qname(buffer, data->exchange)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_write_rdata_mx(BytePacketBuffer *buffer, const RdataMX *data)
{
    if (!buffer_write_uint16(buffer, data->preference)) {
        return FALSE;
    }
    if (!buffer_write_qname(buffer, &data->exchange)) {
        return FALSE;
    }
    return TRUE;
}

static boolean buffer_read_rdata_aaaa(BytePacketBuffer *buffer, RdataAAAA *data)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (!buffer_read(buffer, &data->address[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

static boolean buffer_write_rdata_aaaa(BytePacketBuffer *buffer, const RdataAAAA *data)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (!buffer_write(buffer, data->address[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

static boolean buffer_read_dnsrecord(BytePacketBuffer *buffer, DnsRecord *data)
{
    if (!buffer_read_qname(buffer, data->name)) {
        return FALSE;
    } 
    if (!buffer_read_uint16(buffer, &data->type)) {
        return FALSE;
    }
    if (!buffer_read_uint16(buffer, &data->class)) {
        return FALSE;
    }
    if (!buffer_read_uint32(buffer, &data->ttl)) {
        return FALSE;
    }
    if (!buffer_read_uint16(buffer, &data->rdlength)) {
        return FALSE;
    }
    switch (data->type)
    {
        case DNS_TYPE_A:
            if (!buffer_read_rdata_a(buffer, (RdataA *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_NS:
            if (!buffer_read_rdata_ns(buffer, (RdataNS *)&data->rdata)) {
               return FALSE; 
            }
        break;
        case DNS_TYPE_CNAME:
            if (!buffer_read_rdata_cname(buffer, (RdataCNAME *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_SOA:
            if (!buffer_read_rdata_soa(buffer, (RdataSOA *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_PTR:
            if (!buffer_read_rdata_ptr(buffer, (RdataPTR *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_MX:
            if (!buffer_read_rdata_mx(buffer, (RdataMX *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_AAAA:
            if (!buffer_read_rdata_aaaa(buffer, (RdataAAAA *)&data->rdata)) {
                return FALSE;
            }
        break;
        default:
            return FALSE;
        break;
    }
    return TRUE;
}

static boolean buffer_write_dnsrecord(BytePacketBuffer *buffer, const DnsRecord *data)
{
    if (!buffer_write_qname(buffer, &data->name)) {
        return FALSE;
    } 
    if (!buffer_write_uint16(buffer, data->type)) {
        return FALSE;
    }
    if (!buffer_write_uint16(buffer, data->class)) {
        return FALSE;
    }
    if (!buffer_write_uint32(buffer, data->ttl)) {
        return FALSE;
    }
    if (!buffer_write_uint16(buffer, data->rdlength)) {
        return FALSE;
    }
    switch (data->type)
    {
        case DNS_TYPE_A:
            if (!buffer_write_rdata_a(buffer, (RdataA *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_NS:
            if (!buffer_write_rdata_ns(buffer, (RdataNS *)&data->rdata)) {
               return FALSE; 
            }
        break;
        case DNS_TYPE_CNAME:
            if (!buffer_write_rdata_cname(buffer, (RdataCNAME *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_SOA:
            if (!buffer_write_rdata_soa(buffer, (RdataSOA *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_PTR:
            if (!buffer_write_rdata_ptr(buffer, (RdataPTR *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_MX:
            if (!buffer_write_rdata_mx(buffer, (RdataMX *)&data->rdata)) {
                return FALSE;
            }
        break;
        case DNS_TYPE_AAAA:
            if (!buffer_write_rdata_aaaa(buffer, (RdataAAAA *)&data->rdata)) {
                return FALSE;
            }
        break;
        default:
            return FALSE;
        break;
    }
    return TRUE;
}

static boolean buffer_read_dnspacket(BytePacketBuffer *buffer, DnsPacket *data)
{
    uint i;
    if (!buffer_read_dnsheader(buffer, &data->header)) {
        return FALSE;
    }

    data->questions = malloc(sizeof(*data->questions) * data->header.qdcount);
    if (!data->questions) {
        return FALSE;
    }
    data->answers = malloc(sizeof(*data->answers) * data->header.ancount);
    if (!data->answers) {
        free(data->questions);
        return FALSE;
    }
    data->authorities = malloc(sizeof(*data->authorities) * data->header.nscount);
    if (!data->authorities) {
        free(data->questions);
        free(data->answers);
        return FALSE;
    }
    data->resources = malloc(sizeof(*data->resources) * data->header.arcount);
    if (!data->resources) {
        free(data->questions);
        free(data->answers);
        free(data->authorities);
        return FALSE;
    }

    for (i = 0; i < data->header.qdcount; i++) {
        if (!buffer_read_dnsquestion(buffer, &data->questions[i])) {
            return FALSE;
        }
    }
    for (i = 0; i < data->header.ancount; i++) {
        if (!buffer_read_dnsrecord(buffer, &data->answers[i])) {
            return FALSE;
        }
    }
    for (i = 0; i < data->header.nscount; i++) {
        if (!buffer_read_dnsrecord(buffer, &data->authorities[i])) {
            return FALSE;
        }
    }
    for (i = 0; i < data->header.arcount; i++) {
        if (!buffer_read_dnsrecord(buffer, &data->resources[i])) {
            return FALSE;
        }
    }

    return TRUE;
}

static boolean buffer_write_dnspacket(BytePacketBuffer *buffer, const DnsPacket *data)
{
    uint i;

    if (!buffer_write_dnsheader(buffer, &data->header)) {
        return FALSE;
    }
    for (i = 0; i < data->header.qdcount; i++) {
        if (!buffer_write_dnsquestion(buffer, &data->questions[i])) {
            return FALSE;
        }
    }
    for (i = 0; i < data->header.ancount; i++) {
        if (!buffer_write_dnsrecord(buffer, &data->answers[i])) {
            return FALSE;
        }
    }
    for (i = 0; i < data->header.nscount; i++) {
        if (!buffer_write_dnsrecord(buffer, &data->authorities[i])) {
            return FALSE;
        }
    }
    for (i = 0; i < data->header.arcount; i++) {
        if (!buffer_write_dnsrecord(buffer, &data->resources[i])) {
            return FALSE;
        }
    }
    
    return TRUE;
}

static boolean dnspacket_get_first_a(DnsPacket *packet, uint32 *out_addr)
{
    uint i;

    for (i = 0; i < packet->header.ancount; i++) {
        if (packet->answers[i].type == DNS_TYPE_A) {
            memcpy(out_addr, (uint32 *)&packet->answers[i].rdata, sizeof(uint32));
            return TRUE;
        }
    } 

    return FALSE;
}

static boolean dnspacket_get_resolved_ns(DnsPacket *packet, DnsName qname, uint32 *out_addr)
{
    uint i, j;

    for (i = 0; i < packet->header.nscount; i++) {
        if (c_text_compare_case_insensitive(qname, packet->authorities[i].name)) {
            for (j = 0; j < packet->header.arcount; i++) {
                if (c_text_compare_case_insensitive((char *)packet->authorities[i].rdata, packet->resources[j].name)) {
                    *out_addr = ((uint32) packet->resources[j].rdata[0]) << 24;
                    *out_addr |= ((uint32) packet->resources[j].rdata[1]) << 16;
                    *out_addr |= ((uint32) packet->resources[j].rdata[2]) << 8;
                    *out_addr |= (uint32) packet->resources[j].rdata[3];
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static boolean dnspacket_get_unresolved_ns(DnsPacket *packet, DnsName qname, DnsName out_name)
{
    uint i;

    for (i = 0; i < packet->header.nscount; i++) {
        if (c_text_compare_case_insensitive(qname, packet->authorities[i].name)) {
            memcpy(out_name, (DnsName *)&packet->authorities[i].rdata, sizeof(DnsName));
            return TRUE;
        }
    }
    return FALSE;
}

static boolean lookup(DnsPacket *out, const DnsName qname, const DnsType qtype, StyxNetworkAddress server)
{
    SHandle *handle = styx_network_datagram_create(0, server.is_ipv6);
    DnsPacket netbound = {0};
    BytePacketBuffer req_buffer = {0}, res_buffer = {0};
    if (!handle) {
        return FALSE;
    } 

    netbound.header.id = 13709; /* smiley on a numpad :) */
    netbound.header.qdcount = 1;
    netbound.header.flags |= DNS_FLAG_RD;
    netbound.questions = malloc(sizeof(*netbound.questions));
    memcpy(netbound.questions[0].qname, qname, sizeof(DnsName));
    netbound.questions[0].qtype = qtype;
    netbound.questions[0].qclass = DNS_CLASS_IN;

    buffer_write_dnspacket(&req_buffer, &netbound);
    styx_pack_raw(handle, req_buffer.buf, sizeof(res_buffer.buf), NULL);
    styx_network_datagram_send(handle, &server);

    styx_network_receive(handle, &server);
    styx_unpack_raw(handle, res_buffer.buf, sizeof(res_buffer.buf), NULL);
    buffer_read_dnspacket(&res_buffer, out);

    return TRUE;
}

static boolean recursive_lookup(DnsPacket *out, DnsName qname, DnsType qtype, StyxNetworkAddress server)
{
    for (;;) {
        char ns_str[16];
        DnsPacket response, recursive_response;
        DnsName new_qname;
        inet_ntop(AF_INET, &server.ip.v4, ns_str, sizeof(ns_str));
        lookup(&response, qname, qtype, server);

        printf("Starting iterative resolution for: %s with ns: %s\n", qname, ns_str);
        if (response.header.ancount != 0 && (response.header.flags & DNS_FLAG_RCODE) == DNS_RCODE_NOERROR) {
            *out = response;
            return TRUE;
        }

        if ((response.header.flags & DNS_FLAG_RCODE) == DNS_RCODE_NXDOMAIN) {
            *out = response;
            return TRUE;
        }

        if (dnspacket_get_resolved_ns(&response, qname, &server.ip.v4)) {
            continue;
        }

        if (!dnspacket_get_unresolved_ns(&response, qname, new_qname)) {
            *out = response;
            return TRUE;
        }

        recursive_lookup(&recursive_response, new_qname, DNS_TYPE_A, server);

        if (!dnspacket_get_first_a(&recursive_response, &server.ip.v4)) {
            *out = response;
            return TRUE;
        }
    }
}

static boolean handle_query(SHandle *handle)
{
    BytePacketBuffer req_buffer = {0};

    //styx_network_receive(handle, )
}

boolean dns_resolve_iterative_root_func(StyxNetworkAddress *dest, const char *dns_name, uint16 default_port, boolean *do_ipv6)
{
    static boolean root_server_found = FALSE;
    boolean result = TRUE;
    int i = 0;
    UNUSED(default_port);
    UNUSED(do_ipv6);

    if (dns_name[0] == '/')
    {
        return localhost(dest);
    }

    create_rr_cache();

    if (!root_server_found && strcmp(dns_name, "127.0.0.1") != 0)
    {
        for (i = 0; i < RSI_COUNT; i++)
        {
            if (!verify_latency(RootServers[i]))
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
    result = dns_iterative_root_worker(dest, dns_name, DNS_TYPE_A, current_root_ip, 0);
    if (!result) {
        printf("Could not resolve!\n");
    }
    return result;
}