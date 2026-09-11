// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DNS_META_H
#define DNS_META_H

typedef enum DNS_Class
{
DNS_Class_IN           = 1,
DNS_Class_CS           = 2,
DNS_Class_CH           = 3,
DNS_Class_HS           = 4,
DNS_Class_NONE         = 254,
DNS_Class_ANY          = 255,
} DNS_Class;

typedef enum DNS_Type
{
DNS_Type_A            = 1,
DNS_Type_NS           = 2,
DNS_Type_MD           = 3,
DNS_Type_MF           = 4,
DNS_Type_CNAME        = 5,
DNS_Type_SOA          = 6,
DNS_Type_MB           = 7,
DNS_Type_MG           = 8,
DNS_Type_MR           = 9,
DNS_Type_NULL         = 10,
DNS_Type_WKS          = 11,
DNS_Type_PTR          = 12,
DNS_Type_HINFO        = 13,
DNS_Type_MINFO        = 14,
DNS_Type_MX           = 15,
DNS_Type_TXT          = 16,
DNS_Type_RP           = 17,
DNS_Type_AFSDB        = 18,
DNS_Type_X25          = 19,
DNS_Type_ISDN         = 20,
DNS_Type_RT           = 21,
DNS_Type_NSAP         = 22,
DNS_Type_NSAPPTR      = 23,
DNS_Type_SIG          = 24,
DNS_Type_KEY          = 25,
DNS_Type_PX           = 26,
DNS_Type_GPOS         = 27,
DNS_Type_AAAA         = 28,
DNS_Type_LOC          = 29,
DNS_Type_NXT          = 30,
DNS_Type_EID          = 31,
DNS_Type_NIMLOC       = 32,
DNS_Type_SRV          = 33,
DNS_Type_ATMA         = 34,
DNS_Type_NAPTR        = 35,
DNS_Type_KX           = 36,
DNS_Type_CERT         = 37,
DNS_Type_A6           = 38,
DNS_Type_DNAME        = 39,
DNS_Type_SINK         = 40,
DNS_Type_OPT          = 41,
DNS_Type_APL          = 42,
DNS_Type_DS           = 43,
DNS_Type_SSHFP        = 44,
DNS_Type_IPSECKEY     = 45,
DNS_Type_RRSIG        = 46,
DNS_Type_NSEC         = 47,
DNS_Type_DNSKEY       = 48,
DNS_Type_DHCID        = 49,
DNS_Type_NSEC3        = 50,
DNS_Type_NSEC3PARAM   = 51,
DNS_Type_TLSA         = 52,
DNS_Type_SMIMEA       = 53,
DNS_Type_HIP          = 55,
DNS_Type_NINFO        = 56,
DNS_Type_RKEY         = 57,
DNS_Type_TALINK       = 58,
DNS_Type_CDS          = 59,
DNS_Type_CDNSKEY      = 60,
DNS_Type_OPENPGPKEY   = 61,
DNS_Type_CSYNC        = 62,
DNS_Type_ZONEMD       = 63,
DNS_Type_SVCB         = 64,
DNS_Type_HTTPS        = 65,
DNS_Type_DSYNC        = 66,
DNS_Type_HHIT         = 67,
DNS_Type_BRID         = 68,
DNS_Type_SPF          = 99,
DNS_Type_UINFO        = 100,
DNS_Type_UID          = 101,
DNS_Type_GID          = 102,
DNS_Type_UNSPEC       = 103,
DNS_Type_NID          = 104,
DNS_Type_L32          = 105,
DNS_Type_L64          = 106,
DNS_Type_LP           = 107,
DNS_Type_EUI48        = 108,
DNS_Type_EUI64        = 109,
DNS_Type_NXNAME       = 128,
DNS_Type_TKEY         = 249,
DNS_Type_TSIG         = 250,
DNS_Type_IXFR         = 251,
DNS_Type_AXFR         = 252,
DNS_Type_MAILB        = 253,
DNS_Type_MAILA        = 254,
DNS_Type_ANY          = 255,
DNS_Type_URI          = 256,
DNS_Type_CAA          = 257,
DNS_Type_AVC          = 258,
DNS_Type_DOA          = 259,
DNS_Type_AMTRELAY     = 260,
DNS_Type_RESINFO      = 261,
DNS_Type_WALLET       = 262,
DNS_Type_CLA          = 263,
DNS_Type_IPN          = 264,
DNS_Type_TA           = 32768,
DNS_Type_DLV          = 32769,
} DNS_Type;

typedef enum DNS_OpCode
{
DNS_OpCode_Query        = 0,
DNS_OpCode_IQuery       = 1,
DNS_OpCode_Status       = 2,
DNS_OpCode_Notify       = 4,
DNS_OpCode_Update       = 5,
DNS_OpCode_DSO          = 6,
} DNS_OpCode;

typedef enum DNS_RCode
{
DNS_RCode_NoError      = 0,
DNS_RCode_FormErr      = 1,
DNS_RCode_ServFail     = 2,
DNS_RCode_NXDomain     = 3,
DNS_RCode_NotImp       = 4,
DNS_RCode_Refused      = 5,
DNS_RCode_YXDomain     = 6,
DNS_RCode_YXRRSet      = 7,
DNS_RCode_NXRRSet      = 8,
DNS_RCode_NotAuth      = 9,
DNS_RCode_NotZone      = 10,
DNS_RCode_DSOTYPENI    = 11,
DNS_RCode_BADSIG       = 16,
DNS_RCode_BADKEY       = 17,
DNS_RCode_BADTIME      = 18,
DNS_RCode_BADMODE      = 19,
DNS_RCode_BADNAME      = 20,
DNS_RCode_BADALG       = 21,
DNS_RCode_BADTRUNC     = 22,
DNS_RCode_BADCOOKIE    = 23,
} DNS_RCode;

typedef enum DNS_RootServer
{
DNS_RootServer_A,
DNS_RootServer_B,
DNS_RootServer_C,
DNS_RootServer_D,
DNS_RootServer_E,
DNS_RootServer_F,
DNS_RootServer_G,
DNS_RootServer_H,
DNS_RootServer_I,
DNS_RootServer_J,
DNS_RootServer_K,
DNS_RootServer_L,
DNS_RootServer_M,
DNS_RootServer_COUNT,
} DNS_RootServer;

#undef NULL
typedef union DNS_RData DNS_RData;
union DNS_RData {
struct
{
u32 addr;
} A;
struct
{
String8 ns;
} NS;
struct
{
String8 madname;
} MD;
struct
{
String8 madname;
} MF;
struct
{
String8 target;
} CNAME;
struct
{
String8 master_name;
String8 responsible_name;
u32 serial;
u32 refresh;
u32 retry;
u32 expire;
u32 minimum;
} SOA;
struct
{
String8 madname;
} MB;
struct
{
String8 mgmname;
} MG;
struct
{
String8 newname;
} MR;
struct
{
String8 data;
} NULL;
struct
{
u32 address;
u8 protocol;
String8 bitmap;
} WKS;
struct
{
String8 ptrdname;
} PTR;
struct
{
String8 cpu;
String8 os;
} HINFO;
struct
{
String8 rmailbx;
String8 emailbx;
} MINFO;
struct
{
u16 preference;
String8 exchange;
} MX;
struct
{
String8 txt_data;
} TXT;
struct
{
String8 mbox_dname;
String8 txt_dname;
} RP;
struct
{
u16 subtype;
String8 hostname;
} AFSDB;
struct
{
String8 psdn_addr;
} X25;
struct
{
String8 isdn_addr;
String8 sub_addr;
} ISDN;
struct
{
u16 preference;
String8 intrm_host;
} RT;
struct
{
String8 data;
} NSAP;
struct
{
String8 ptrdname;
} NSAPPTR;
struct
{
u16 type_covered;
u8 algorithm;
u8 labels;
u32 ttl;
u32 sig_expire;
u32 time_signd;
u16 key_ftprnt;
String8 signr_name;
String8 signature;
} SIG;
struct
{
u16 flags;
u8 protocol;
u8 algorithm;
String8 pubkey;
} KEY;
struct
{
u16 preference;
String8 map_822;
String8 map_x400;
} PX;
struct
{
String8 longitude;
String8 latitude;
String8 altitude;
} GPOS;
struct
{
u128 addr;
} AAAA;
struct
{
u8 version;
u8 size;
u8 horiz_pre;
u8 vert_pre;
u32 latitude;
u32 longitude;
u32 altitude;
} LOC;
struct
{
String8 next_dname;
String8 type_bitmap;
} NXT;
struct
{
String8 endpoint_id;
} EID;
struct
{
String8 nimrod_loc;
} NIMLOC;
struct
{
u16 priority;
u16 weight;
u16 port;
String8 target;
} SRV;
struct
{
String8 mctx;
String8 format;
String8 atma;
u16 atma_len;
} ATMA;
struct
{
u16 order;
u16 preference;
String8 flags;
String8 service;
String8 regexp;
String8 replacement;
} NAPTR;
struct
{
u16 preference;
String8 exchanger;
} KX;
struct
{
u16 type;
u16 key_tag;
u8 algorithm;
String8 certificate;
} CERT;
struct
{
u8 prefix_len;
String8 addr_suffx;
String8 prefix_name;
} A6;
struct
{
String8 target;
} DNAME;
struct
{
u8 coding;
u8 subcoding;
String8 data;
} SINK;
struct
{
String8 data;
} OPT;
struct
{
u16 addr_fam;
u8 prefix;
bool32 n;
u8 afdlength;
union{u32 v4; u128 v6;} afdpart;
} APL;
struct
{
u16 key_tag;
u8 algorithm;
u8 digest_t;
String8 digest;
} DS;
struct
{
u8 algorithm;
u8 fp_type;
String8 fingerprint;
} SSHFP;
struct
{
u8 precedence;
u8 gateway_type;
u8 algorithm;
union{u32 v4; u128 v6; String8 dname;} gateway;
String8 public_key;
} IPSECKEY;
struct
{
u16 type_covered;
u8 algorithm;
u8 labels;
u32 orig_ttl;
u32 expiration;
u32 inception;
u16 key_tag;
String8 signr_name;
String8 signature;
} RRSIG;
struct
{
String8 nxt_domain;
String8 type_bitmap;
} NSEC;
struct
{
u16 flags;
u8 protocol;
u8 algorithm;
String8 public_key;
} DNSKEY;
struct
{
u16 id_type_code;
u8 digest_type_code;
String8 digest;
} DHCID;
struct
{
u8 hash;
u8 flags;
u16 iterations;
u8 salt_len;
String8 salt;
u8 hash_len;
String8 nxt_domain;
String8 type_bitmap;
} NSEC3;
struct
{
u8 hash;
u8 flags;
u16 iterations;
u8 salt_len;
String8 salt;
} NSEC3PARAM;
struct
{
u8 usage;
u8 selector;
u8 matching_t;
String8 certificate;
} TLSA;
struct
{
u8 usage;
u8 selector;
u8 matching_t;
String8 certificate;
} SMIMEA;
struct
{
u8 hit_len;
u8 pub_key_alg;
u16 pub_key_len;
String8 hit;
String8 pub_key;
String8 rendezvous_servers;
} HIP;
struct
{
String8 zsdata;
} NINFO;
struct
{
u16 flags;
u8 protocol;
u8 algorithm;
String8 pub_key;
} RKEY;
struct
{
String8 prev_name;
String8 next_name;
} TALINK;
struct
{
u16 key_tag;
u8 algorithm;
u8 digest_t;
String8 digest;
} CDS;
struct
{
u16 flags;
u8 protocol;
u8 algorithm;
String8 pub_key;
} CDNSKEY;
struct
{
String8 pub_key;
} OPENPGPKEY;
struct
{
u32 serial;
u16 flags;
String8 type_bitmap;
} CSYNC;
struct
{
u32 serial;
u8 scheme;
u8 hash;
String8 digest;
} ZONEMD;
struct
{
u16 priority;
String8 target;
String8 pairs;
} SVCB;
struct
{
u16 priority;
String8 target;
String8 pairs;
} HTTPS;
struct
{
u16 type;
u8 scheme;
u16 port;
String8 target;
} DSYNC;
struct
{
u128 prefix;
u32 hid;
u8 hssi;
String8 orchid_hash;
} HHIT;
struct
{
String8 UNKNOWN;
} BRID;
struct
{
String8 text;
} SPF;
struct
{
String8 uinfo;
} UINFO;
struct
{
u32 uid;
} UID;
struct
{
u32 gid;
} GID;
struct
{
String8 UNKNOWN;
} UNSPEC;
struct
{
u16 preference;
u64 node_id;
} NID;
struct
{
u16 preference;
u32 locator_32;
} L32;
struct
{
u16 preference;
u64 locator_64;
} L64;
struct
{
u16 preference;
String8 fqdn;
} LP;
struct
{
String8 address;
} EUI48;
struct
{
String8 address;
} EUI64;
struct
{
String8 EMPTY;
} NXNAME;
struct
{
String8 algorithm;
u32 inception;
u32 expiration;
u16 mode;
u16 error;
u16 key_size;
String8 key;
u16 other_len;
String8 other_data;
} TKEY;
struct
{
String8 algorithm;
u64 time_signed;
u16 fudge;
u16 mac_size;
String8 mac;
u16 orig_id;
u16 error;
u16 other_len;
String8 other_data;
} TSIG;
struct
{
String8 EMPTY;
} IXFR;
struct
{
String8 EMPTY;
} AXFR;
struct
{
String8 EMPTY;
} MAILB;
struct
{
String8 EMPTY;
} MAILA;
struct
{
String8 EMPTY;
} ANY;
struct
{
u16 priority;
u16 weight;
String8 target;
} URI;
struct
{
u8 flag;
String8 tag;
String8 value;
} CAA;
struct
{
String8 text;
} AVC;
struct
{
String8 UNKNOWN;
} DOA;
struct
{
String8 UNKOWN;
} AMTRELAY;
struct
{
String8 text;
} RESINFO;
struct
{
String8 text;
} WALLET;
struct
{
String8 text;
} CLA;
struct
{
u64 node;
} IPN;
struct
{
u16 key_tag;
u8 algorithm;
u8 digest_t;
String8 digest;
} TA;
struct
{
u16 key_tag;
u8 algorithm;
u8 digest_t;
String8 digest;
} DLV;
};
#define NULL ((void*)0)
internal String8 dns_string_from_class(DNS_Class v);
internal String8 dns_friendly_name_from_class(DNS_Class v);
internal String8 dns_rfc_references_from_class(DNS_Class v);
internal String8 dns_string_from_type(DNS_Type v);
internal String8 dns_string_from_opcode(DNS_OpCode v);
internal String8 dns_string_from_rcode(DNS_RCode v);
internal String8 dns_dname_of_root_server(DNS_RootServer v);
internal String8 dns_ipv4_string_of_root_server(DNS_RootServer v);
internal String8 dns_ipv6_string_of_root_server(DNS_RootServer v);
internal String8 dns_operator_of_root_server(DNS_RootServer v);
internal bool32 dns_is_obsolete_class(DNS_Class v);
internal bool32 dns_is_query_exclusive_class(DNS_Class v);
internal bool32 dns_is_obsolete_type(DNS_Type v);
internal bool32 dns_is_query_exclusive_type(DNS_Type v);
#endif // DNS_META_H
