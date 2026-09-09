// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

internal String8
dns_string_from_class(DNS_Class v)
{
String8 result = str8_lit("<Unknown DNS_Class>");
switch(v)
{
default:{}break;
case DNS_Class_IN:{result = str8_lit("IN");}break;
case DNS_Class_CS:{result = str8_lit("CS");}break;
case DNS_Class_CH:{result = str8_lit("CH");}break;
case DNS_Class_HS:{result = str8_lit("HS");}break;
case DNS_Class_NONE:{result = str8_lit("NONE");}break;
case DNS_Class_ANY:{result = str8_lit("ANY");}break;
}
return result;
}

internal String8
dns_friendly_name_from_class(DNS_Class v)
{
String8 result = str8_lit("<Unknown DNS_Class>");
switch(v)
{
default:{}break;
case DNS_Class_IN:{result = str8_lit("Internet");}break;
case DNS_Class_CS:{result = str8_lit("Computer Science Network");}break;
case DNS_Class_CH:{result = str8_lit("Chaos");}break;
case DNS_Class_HS:{result = str8_lit("Hesiod");}break;
case DNS_Class_NONE:{result = str8_lit("None");}break;
case DNS_Class_ANY:{result = str8_lit("Any");}break;
}
return result;
}

internal String8
dns_rfc_references_from_class(DNS_Class v)
{
String8 result = str8_lit("<Unknown DNS_Class>");
switch(v)
{
default:{}break;
case DNS_Class_IN:{result = str8_lit("1035");}break;
case DNS_Class_CS:{result = str8_lit("1035");}break;
case DNS_Class_CH:{result = str8_lit("1035");}break;
case DNS_Class_HS:{result = str8_lit("1035");}break;
case DNS_Class_NONE:{result = str8_lit("2136");}break;
case DNS_Class_ANY:{result = str8_lit("1035");}break;
}
return result;
}

internal String8
dns_string_from_type(DNS_Type v)
{
String8 result = str8_lit("<Unknown DNS_Type>");
switch(v)
{
default:{}break;
case DNS_Type_A:{result = str8_lit("A");}break;
case DNS_Type_NS:{result = str8_lit("NS");}break;
case DNS_Type_MD:{result = str8_lit("MD");}break;
case DNS_Type_MF:{result = str8_lit("MF");}break;
case DNS_Type_CNAME:{result = str8_lit("CNAME");}break;
case DNS_Type_SOA:{result = str8_lit("SOA");}break;
case DNS_Type_MB:{result = str8_lit("MB");}break;
case DNS_Type_MG:{result = str8_lit("MG");}break;
case DNS_Type_MR:{result = str8_lit("MR");}break;
case DNS_Type_NULL:{result = str8_lit("NULL");}break;
case DNS_Type_WKS:{result = str8_lit("WKS");}break;
case DNS_Type_PTR:{result = str8_lit("PTR");}break;
case DNS_Type_HINFO:{result = str8_lit("HINFO");}break;
case DNS_Type_MINFO:{result = str8_lit("MINFO");}break;
case DNS_Type_MX:{result = str8_lit("MX");}break;
case DNS_Type_TXT:{result = str8_lit("TXT");}break;
case DNS_Type_RP:{result = str8_lit("RP");}break;
case DNS_Type_AFSDB:{result = str8_lit("AFSDB");}break;
case DNS_Type_X25:{result = str8_lit("X25");}break;
case DNS_Type_ISDN:{result = str8_lit("ISDN");}break;
case DNS_Type_RT:{result = str8_lit("RT");}break;
case DNS_Type_NSAP:{result = str8_lit("NSAP");}break;
case DNS_Type_NSAPPTR:{result = str8_lit("NSAPPTR");}break;
case DNS_Type_SIG:{result = str8_lit("SIG");}break;
case DNS_Type_KEY:{result = str8_lit("KEY");}break;
case DNS_Type_PX:{result = str8_lit("PX");}break;
case DNS_Type_GPOS:{result = str8_lit("GPOS");}break;
case DNS_Type_AAAA:{result = str8_lit("AAAA");}break;
case DNS_Type_LOC:{result = str8_lit("LOC");}break;
case DNS_Type_NXT:{result = str8_lit("NXT");}break;
case DNS_Type_EID:{result = str8_lit("EID");}break;
case DNS_Type_NIMLOC:{result = str8_lit("NIMLOC");}break;
case DNS_Type_SRV:{result = str8_lit("SRV");}break;
case DNS_Type_ATMA:{result = str8_lit("ATMA");}break;
case DNS_Type_NAPTR:{result = str8_lit("NAPTR");}break;
case DNS_Type_KX:{result = str8_lit("KX");}break;
case DNS_Type_CERT:{result = str8_lit("CERT");}break;
case DNS_Type_A6:{result = str8_lit("A6");}break;
case DNS_Type_DNAME:{result = str8_lit("DNAME");}break;
case DNS_Type_SINK:{result = str8_lit("SINK");}break;
case DNS_Type_OPT:{result = str8_lit("OPT");}break;
case DNS_Type_APL:{result = str8_lit("APL");}break;
case DNS_Type_DS:{result = str8_lit("DS");}break;
case DNS_Type_SSHFP:{result = str8_lit("SSHFP");}break;
case DNS_Type_IPSECKEY:{result = str8_lit("IPSECKEY");}break;
case DNS_Type_RRSIG:{result = str8_lit("RRSIG");}break;
case DNS_Type_NSEC:{result = str8_lit("NSEC");}break;
case DNS_Type_DNSKEY:{result = str8_lit("DNSKEY");}break;
case DNS_Type_DHCID:{result = str8_lit("DHCID");}break;
case DNS_Type_NSEC3:{result = str8_lit("NSEC3");}break;
case DNS_Type_NSEC3PARAM:{result = str8_lit("NSEC3PARAM");}break;
case DNS_Type_TLSA:{result = str8_lit("TLSA");}break;
case DNS_Type_SMIMEA:{result = str8_lit("SMIMEA");}break;
case DNS_Type_HIP:{result = str8_lit("HIP");}break;
case DNS_Type_NINFO:{result = str8_lit("NINFO");}break;
case DNS_Type_RKEY:{result = str8_lit("RKEY");}break;
case DNS_Type_TALINK:{result = str8_lit("TALINK");}break;
case DNS_Type_CDS:{result = str8_lit("CDS");}break;
case DNS_Type_CDNSKEY:{result = str8_lit("CDNSKEY");}break;
case DNS_Type_OPENPGPKEY:{result = str8_lit("OPENPGPKEY");}break;
case DNS_Type_CSYNC:{result = str8_lit("CSYNC");}break;
case DNS_Type_ZONEMD:{result = str8_lit("ZONEMD");}break;
case DNS_Type_SVCB:{result = str8_lit("SVCB");}break;
case DNS_Type_HTTPS:{result = str8_lit("HTTPS");}break;
case DNS_Type_DSYNC:{result = str8_lit("DSYNC");}break;
case DNS_Type_HHIT:{result = str8_lit("HHIT");}break;
case DNS_Type_BRID:{result = str8_lit("BRID");}break;
case DNS_Type_SPF:{result = str8_lit("SPF");}break;
case DNS_Type_UINFO:{result = str8_lit("UINFO");}break;
case DNS_Type_UID:{result = str8_lit("UID");}break;
case DNS_Type_GID:{result = str8_lit("GID");}break;
case DNS_Type_UNSPEC:{result = str8_lit("UNSPEC");}break;
case DNS_Type_NID:{result = str8_lit("NID");}break;
case DNS_Type_L32:{result = str8_lit("L32");}break;
case DNS_Type_L64:{result = str8_lit("L64");}break;
case DNS_Type_LP:{result = str8_lit("LP");}break;
case DNS_Type_EUI48:{result = str8_lit("EUI48");}break;
case DNS_Type_EUI64:{result = str8_lit("EUI64");}break;
case DNS_Type_NXNAME:{result = str8_lit("NXNAME");}break;
case DNS_Type_TKEY:{result = str8_lit("TKEY");}break;
case DNS_Type_TSIG:{result = str8_lit("TSIG");}break;
case DNS_Type_IXFR:{result = str8_lit("IXFR");}break;
case DNS_Type_AXFR:{result = str8_lit("AXFR");}break;
case DNS_Type_MAILB:{result = str8_lit("MAILB");}break;
case DNS_Type_MAILA:{result = str8_lit("MAILA");}break;
case DNS_Type_ANY:{result = str8_lit("ANY");}break;
case DNS_Type_URI:{result = str8_lit("URI");}break;
case DNS_Type_CAA:{result = str8_lit("CAA");}break;
case DNS_Type_AVC:{result = str8_lit("AVC");}break;
case DNS_Type_DOA:{result = str8_lit("DOA");}break;
case DNS_Type_AMTRELAY:{result = str8_lit("AMTRELAY");}break;
case DNS_Type_RESINFO:{result = str8_lit("RESINFO");}break;
case DNS_Type_WALLET:{result = str8_lit("WALLET");}break;
case DNS_Type_CLA:{result = str8_lit("CLA");}break;
case DNS_Type_IPN:{result = str8_lit("IPN");}break;
case DNS_Type_TA:{result = str8_lit("TA");}break;
case DNS_Type_DLV:{result = str8_lit("DLV");}break;
}
return result;
}

internal String8
dns_string_from_opcode(DNS_OpCode v)
{
String8 result = str8_lit("<Unknown DNS_OpCode>");
switch(v)
{
default:{}break;
case DNS_OpCode_Query:{result = str8_lit("Query");}break;
case DNS_OpCode_IQuery:{result = str8_lit("IQuery");}break;
case DNS_OpCode_Status:{result = str8_lit("Status");}break;
case DNS_OpCode_Notify:{result = str8_lit("Notify");}break;
case DNS_OpCode_Update:{result = str8_lit("Update");}break;
case DNS_OpCode_DSO:{result = str8_lit("DSO");}break;
}
return result;
}

internal String8
dns_string_from_rcode(DNS_RCode v)
{
String8 result = str8_lit("<Unknown DNS_RCode>");
switch(v)
{
default:{}break;
case DNS_RCode_NoError:{result = str8_lit("NoError");}break;
case DNS_RCode_FormErr:{result = str8_lit("FormErr");}break;
case DNS_RCode_ServFail:{result = str8_lit("ServFail");}break;
case DNS_RCode_NXDomain:{result = str8_lit("NXDomain");}break;
case DNS_RCode_NotImp:{result = str8_lit("NotImp");}break;
case DNS_RCode_Refused:{result = str8_lit("Refused");}break;
case DNS_RCode_YXDomain:{result = str8_lit("YXDomain");}break;
case DNS_RCode_YXRRSet:{result = str8_lit("YXRRSet");}break;
case DNS_RCode_NXRRSet:{result = str8_lit("NXRRSet");}break;
case DNS_RCode_NotAuth:{result = str8_lit("NotAuth");}break;
case DNS_RCode_NotZone:{result = str8_lit("NotZone");}break;
case DNS_RCode_DSOTYPENI:{result = str8_lit("DSOTYPENI");}break;
case DNS_RCode_BADSIG:{result = str8_lit("BADSIG");}break;
case DNS_RCode_BADKEY:{result = str8_lit("BADKEY");}break;
case DNS_RCode_BADTIME:{result = str8_lit("BADTIME");}break;
case DNS_RCode_BADMODE:{result = str8_lit("BADMODE");}break;
case DNS_RCode_BADNAME:{result = str8_lit("BADNAME");}break;
case DNS_RCode_BADALG:{result = str8_lit("BADALG");}break;
case DNS_RCode_BADTRUNC:{result = str8_lit("BADTRUNC");}break;
case DNS_RCode_BADCOOKIE:{result = str8_lit("BADCOOKIE");}break;
}
return result;
}

internal String8
dns_dname_of_root_server(DNS_RootServer v)
{
String8 result = str8_lit("<Unknown DNS_RootServer>");
switch(v)
{
default:{}break;
case DNS_RootServer_A:{result = str8_lit("a.root-servers.net");}break;
case DNS_RootServer_B:{result = str8_lit("b.root-servers.net");}break;
case DNS_RootServer_C:{result = str8_lit("c.root-servers.net");}break;
case DNS_RootServer_D:{result = str8_lit("d.root-servers.net");}break;
case DNS_RootServer_E:{result = str8_lit("e.root-servers.net");}break;
case DNS_RootServer_F:{result = str8_lit("f.root-servers.net");}break;
case DNS_RootServer_G:{result = str8_lit("g.root-servers.net");}break;
case DNS_RootServer_H:{result = str8_lit("h.root-servers.net");}break;
case DNS_RootServer_I:{result = str8_lit("i.root-servers.net");}break;
case DNS_RootServer_J:{result = str8_lit("j.root-servers.net");}break;
case DNS_RootServer_K:{result = str8_lit("k.root-servers.net");}break;
case DNS_RootServer_L:{result = str8_lit("l.root-servers.net");}break;
case DNS_RootServer_M:{result = str8_lit("m.root-servers.net");}break;
}
return result;
}

internal String8
dns_ipv4_string_of_root_server(DNS_RootServer v)
{
String8 result = str8_lit("<Unknown DNS_RootServer>");
switch(v)
{
default:{}break;
case DNS_RootServer_A:{result = str8_lit("198.41.0.4");}break;
case DNS_RootServer_B:{result = str8_lit("170.247.170.2");}break;
case DNS_RootServer_C:{result = str8_lit("192.33.4.12");}break;
case DNS_RootServer_D:{result = str8_lit("199.7.91.13");}break;
case DNS_RootServer_E:{result = str8_lit("192.203.230.10");}break;
case DNS_RootServer_F:{result = str8_lit("192.5.5.241");}break;
case DNS_RootServer_G:{result = str8_lit("192.112.36.4");}break;
case DNS_RootServer_H:{result = str8_lit("198.97.190.53");}break;
case DNS_RootServer_I:{result = str8_lit("192.36.148.17");}break;
case DNS_RootServer_J:{result = str8_lit("192.58.128.30");}break;
case DNS_RootServer_K:{result = str8_lit("193.0.14.129");}break;
case DNS_RootServer_L:{result = str8_lit("199.7.83.42");}break;
case DNS_RootServer_M:{result = str8_lit("202.12.27.33");}break;
}
return result;
}

internal String8
dns_ipv6_string_of_root_server(DNS_RootServer v)
{
String8 result = str8_lit("<Unknown DNS_RootServer>");
switch(v)
{
default:{}break;
case DNS_RootServer_A:{result = str8_lit("2001:503:ba3e::2:30");}break;
case DNS_RootServer_B:{result = str8_lit("2801:1b8:10::b");}break;
case DNS_RootServer_C:{result = str8_lit("2001:500:2::c");}break;
case DNS_RootServer_D:{result = str8_lit("2001:500:2d::d");}break;
case DNS_RootServer_E:{result = str8_lit("2001:500:a8::e");}break;
case DNS_RootServer_F:{result = str8_lit("2001:500:2f::f");}break;
case DNS_RootServer_G:{result = str8_lit("2001:500:12::d0d");}break;
case DNS_RootServer_H:{result = str8_lit("2001:500:1::53");}break;
case DNS_RootServer_I:{result = str8_lit("2001:7fe::53");}break;
case DNS_RootServer_J:{result = str8_lit("2001:503:c27::2:30");}break;
case DNS_RootServer_K:{result = str8_lit("2001:7fd::1");}break;
case DNS_RootServer_L:{result = str8_lit("2001:500:9f::42");}break;
case DNS_RootServer_M:{result = str8_lit("2001:dc3::35");}break;
}
return result;
}

internal String8
dns_operator_of_root_server(DNS_RootServer v)
{
String8 result = str8_lit("<Unknown DNS_RootServer>");
switch(v)
{
default:{}break;
case DNS_RootServer_A:{result = str8_lit("Verisign, Inc.");}break;
case DNS_RootServer_B:{result = str8_lit("University of Southern California, Information Sciences Institutde");}break;
case DNS_RootServer_C:{result = str8_lit("Cogent Communications");}break;
case DNS_RootServer_D:{result = str8_lit("University of Maryland");}break;
case DNS_RootServer_E:{result = str8_lit("NASA (Ames Research Center)");}break;
case DNS_RootServer_F:{result = str8_lit("Internet Systems Consortium, Inc.");}break;
case DNS_RootServer_G:{result = str8_lit("US Department of Defense (NIC)");}break;
case DNS_RootServer_H:{result = str8_lit("US Army (Research Lab)");}break;
case DNS_RootServer_I:{result = str8_lit("Netnod");}break;
case DNS_RootServer_J:{result = str8_lit("Verisign, Inc.");}break;
case DNS_RootServer_K:{result = str8_lit("RIPE NCC");}break;
case DNS_RootServer_L:{result = str8_lit("ICANN");}break;
case DNS_RootServer_M:{result = str8_lit("WIDE Project");}break;
}
return result;
}

internal bool32
dns_is_obsolete_class(DNS_Class v)
{
bool32 result = 0;
switch(v)
{
default:{}break;
case DNS_Class_IN:{result = 0;}break;
case DNS_Class_CS:{result = 1;}break;
case DNS_Class_CH:{result = 0;}break;
case DNS_Class_HS:{result = 0;}break;
case DNS_Class_NONE:{result = 0;}break;
case DNS_Class_ANY:{result = 0;}break;
}
return result;
}

internal bool32
dns_is_query_exclusive_class(DNS_Class v)
{
bool32 result = 0;
switch(v)
{
default:{}break;
case DNS_Class_IN:{result = 0;}break;
case DNS_Class_CS:{result = 0;}break;
case DNS_Class_CH:{result = 0;}break;
case DNS_Class_HS:{result = 0;}break;
case DNS_Class_NONE:{result = 1;}break;
case DNS_Class_ANY:{result = 1;}break;
}
return result;
}

internal bool32
dns_is_obsolete_type(DNS_Type v)
{
bool32 result = 0;
switch(v)
{
default:{}break;
case DNS_Type_A:{result = 0;}break;
case DNS_Type_NS:{result = 0;}break;
case DNS_Type_MD:{result = 1;}break;
case DNS_Type_MF:{result = 1;}break;
case DNS_Type_CNAME:{result = 0;}break;
case DNS_Type_SOA:{result = 0;}break;
case DNS_Type_MB:{result = 1;}break;
case DNS_Type_MG:{result = 1;}break;
case DNS_Type_MR:{result = 1;}break;
case DNS_Type_NULL:{result = 1;}break;
case DNS_Type_WKS:{result = 1;}break;
case DNS_Type_PTR:{result = 0;}break;
case DNS_Type_HINFO:{result = 0;}break;
case DNS_Type_MINFO:{result = 1;}break;
case DNS_Type_MX:{result = 0;}break;
case DNS_Type_TXT:{result = 0;}break;
case DNS_Type_RP:{result = 1;}break;
case DNS_Type_AFSDB:{result = 0;}break;
case DNS_Type_X25:{result = 1;}break;
case DNS_Type_ISDN:{result = 1;}break;
case DNS_Type_RT:{result = 1;}break;
case DNS_Type_NSAP:{result = 1;}break;
case DNS_Type_NSAPPTR:{result = 1;}break;
case DNS_Type_SIG:{result = 1;}break;
case DNS_Type_KEY:{result = 1;}break;
case DNS_Type_PX:{result = 1;}break;
case DNS_Type_GPOS:{result = 1;}break;
case DNS_Type_AAAA:{result = 0;}break;
case DNS_Type_LOC:{result = 0;}break;
case DNS_Type_NXT:{result = 1;}break;
case DNS_Type_EID:{result = 1;}break;
case DNS_Type_NIMLOC:{result = 1;}break;
case DNS_Type_SRV:{result = 0;}break;
case DNS_Type_ATMA:{result = 1;}break;
case DNS_Type_NAPTR:{result = 0;}break;
case DNS_Type_KX:{result = 0;}break;
case DNS_Type_CERT:{result = 0;}break;
case DNS_Type_A6:{result = 1;}break;
case DNS_Type_DNAME:{result = 0;}break;
case DNS_Type_SINK:{result = 1;}break;
case DNS_Type_OPT:{result = 0;}break;
case DNS_Type_APL:{result = 1;}break;
case DNS_Type_DS:{result = 0;}break;
case DNS_Type_SSHFP:{result = 0;}break;
case DNS_Type_IPSECKEY:{result = 0;}break;
case DNS_Type_RRSIG:{result = 0;}break;
case DNS_Type_NSEC:{result = 0;}break;
case DNS_Type_DNSKEY:{result = 0;}break;
case DNS_Type_DHCID:{result = 0;}break;
case DNS_Type_NSEC3:{result = 0;}break;
case DNS_Type_NSEC3PARAM:{result = 0;}break;
case DNS_Type_TLSA:{result = 0;}break;
case DNS_Type_SMIMEA:{result = 0;}break;
case DNS_Type_HIP:{result = 0;}break;
case DNS_Type_NINFO:{result = 1;}break;
case DNS_Type_RKEY:{result = 1;}break;
case DNS_Type_TALINK:{result = 1;}break;
case DNS_Type_CDS:{result = 0;}break;
case DNS_Type_CDNSKEY:{result = 0;}break;
case DNS_Type_OPENPGPKEY:{result = 0;}break;
case DNS_Type_CSYNC:{result = 0;}break;
case DNS_Type_ZONEMD:{result = 0;}break;
case DNS_Type_SVCB:{result = 0;}break;
case DNS_Type_HTTPS:{result = 0;}break;
case DNS_Type_DSYNC:{result = 0;}break;
case DNS_Type_HHIT:{result = 0;}break;
case DNS_Type_BRID:{result = 0;}break;
case DNS_Type_SPF:{result = 1;}break;
case DNS_Type_UINFO:{result = 1;}break;
case DNS_Type_UID:{result = 1;}break;
case DNS_Type_GID:{result = 1;}break;
case DNS_Type_UNSPEC:{result = 1;}break;
case DNS_Type_NID:{result = 0;}break;
case DNS_Type_L32:{result = 0;}break;
case DNS_Type_L64:{result = 0;}break;
case DNS_Type_LP:{result = 0;}break;
case DNS_Type_EUI48:{result = 0;}break;
case DNS_Type_EUI64:{result = 0;}break;
case DNS_Type_NXNAME:{result = 0;}break;
case DNS_Type_TKEY:{result = 0;}break;
case DNS_Type_TSIG:{result = 0;}break;
case DNS_Type_IXFR:{result = 0;}break;
case DNS_Type_AXFR:{result = 0;}break;
case DNS_Type_MAILB:{result = 0;}break;
case DNS_Type_MAILA:{result = 1;}break;
case DNS_Type_ANY:{result = 0;}break;
case DNS_Type_URI:{result = 0;}break;
case DNS_Type_CAA:{result = 0;}break;
case DNS_Type_AVC:{result = 0;}break;
case DNS_Type_DOA:{result = 1;}break;
case DNS_Type_AMTRELAY:{result = 0;}break;
case DNS_Type_RESINFO:{result = 0;}break;
case DNS_Type_WALLET:{result = 0;}break;
case DNS_Type_CLA:{result = 0;}break;
case DNS_Type_IPN:{result = 0;}break;
case DNS_Type_TA:{result = 0;}break;
case DNS_Type_DLV:{result = 1;}break;
}
return result;
}

internal bool32
dns_is_query_exclusive_type(DNS_Type v)
{
bool32 result = 0;
switch(v)
{
default:{}break;
case DNS_Type_A:{result = 0;}break;
case DNS_Type_NS:{result = 0;}break;
case DNS_Type_MD:{result = 0;}break;
case DNS_Type_MF:{result = 0;}break;
case DNS_Type_CNAME:{result = 0;}break;
case DNS_Type_SOA:{result = 0;}break;
case DNS_Type_MB:{result = 0;}break;
case DNS_Type_MG:{result = 0;}break;
case DNS_Type_MR:{result = 0;}break;
case DNS_Type_NULL:{result = 0;}break;
case DNS_Type_WKS:{result = 0;}break;
case DNS_Type_PTR:{result = 0;}break;
case DNS_Type_HINFO:{result = 0;}break;
case DNS_Type_MINFO:{result = 0;}break;
case DNS_Type_MX:{result = 0;}break;
case DNS_Type_TXT:{result = 0;}break;
case DNS_Type_RP:{result = 0;}break;
case DNS_Type_AFSDB:{result = 0;}break;
case DNS_Type_X25:{result = 0;}break;
case DNS_Type_ISDN:{result = 0;}break;
case DNS_Type_RT:{result = 0;}break;
case DNS_Type_NSAP:{result = 0;}break;
case DNS_Type_NSAPPTR:{result = 0;}break;
case DNS_Type_SIG:{result = 0;}break;
case DNS_Type_KEY:{result = 0;}break;
case DNS_Type_PX:{result = 0;}break;
case DNS_Type_GPOS:{result = 0;}break;
case DNS_Type_AAAA:{result = 0;}break;
case DNS_Type_LOC:{result = 0;}break;
case DNS_Type_NXT:{result = 0;}break;
case DNS_Type_EID:{result = 0;}break;
case DNS_Type_NIMLOC:{result = 0;}break;
case DNS_Type_SRV:{result = 0;}break;
case DNS_Type_ATMA:{result = 0;}break;
case DNS_Type_NAPTR:{result = 0;}break;
case DNS_Type_KX:{result = 0;}break;
case DNS_Type_CERT:{result = 0;}break;
case DNS_Type_A6:{result = 0;}break;
case DNS_Type_DNAME:{result = 0;}break;
case DNS_Type_SINK:{result = 0;}break;
case DNS_Type_OPT:{result = 0;}break;
case DNS_Type_APL:{result = 0;}break;
case DNS_Type_DS:{result = 0;}break;
case DNS_Type_SSHFP:{result = 0;}break;
case DNS_Type_IPSECKEY:{result = 0;}break;
case DNS_Type_RRSIG:{result = 0;}break;
case DNS_Type_NSEC:{result = 0;}break;
case DNS_Type_DNSKEY:{result = 0;}break;
case DNS_Type_DHCID:{result = 0;}break;
case DNS_Type_NSEC3:{result = 0;}break;
case DNS_Type_NSEC3PARAM:{result = 0;}break;
case DNS_Type_TLSA:{result = 0;}break;
case DNS_Type_SMIMEA:{result = 0;}break;
case DNS_Type_HIP:{result = 0;}break;
case DNS_Type_NINFO:{result = 0;}break;
case DNS_Type_RKEY:{result = 0;}break;
case DNS_Type_TALINK:{result = 0;}break;
case DNS_Type_CDS:{result = 0;}break;
case DNS_Type_CDNSKEY:{result = 0;}break;
case DNS_Type_OPENPGPKEY:{result = 0;}break;
case DNS_Type_CSYNC:{result = 0;}break;
case DNS_Type_ZONEMD:{result = 0;}break;
case DNS_Type_SVCB:{result = 0;}break;
case DNS_Type_HTTPS:{result = 0;}break;
case DNS_Type_DSYNC:{result = 0;}break;
case DNS_Type_HHIT:{result = 0;}break;
case DNS_Type_BRID:{result = 0;}break;
case DNS_Type_SPF:{result = 0;}break;
case DNS_Type_UINFO:{result = 0;}break;
case DNS_Type_UID:{result = 0;}break;
case DNS_Type_GID:{result = 0;}break;
case DNS_Type_UNSPEC:{result = 0;}break;
case DNS_Type_NID:{result = 0;}break;
case DNS_Type_L32:{result = 0;}break;
case DNS_Type_L64:{result = 0;}break;
case DNS_Type_LP:{result = 0;}break;
case DNS_Type_EUI48:{result = 0;}break;
case DNS_Type_EUI64:{result = 0;}break;
case DNS_Type_NXNAME:{result = 0;}break;
case DNS_Type_TKEY:{result = 0;}break;
case DNS_Type_TSIG:{result = 0;}break;
case DNS_Type_IXFR:{result = 1;}break;
case DNS_Type_AXFR:{result = 1;}break;
case DNS_Type_MAILB:{result = 1;}break;
case DNS_Type_MAILA:{result = 1;}break;
case DNS_Type_ANY:{result = 1;}break;
case DNS_Type_URI:{result = 0;}break;
case DNS_Type_CAA:{result = 0;}break;
case DNS_Type_AVC:{result = 0;}break;
case DNS_Type_DOA:{result = 0;}break;
case DNS_Type_AMTRELAY:{result = 0;}break;
case DNS_Type_RESINFO:{result = 0;}break;
case DNS_Type_WALLET:{result = 0;}break;
case DNS_Type_CLA:{result = 0;}break;
case DNS_Type_IPN:{result = 0;}break;
case DNS_Type_TA:{result = 0;}break;
case DNS_Type_DLV:{result = 0;}break;
}
return result;
}

