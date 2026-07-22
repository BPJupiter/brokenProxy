
#ifndef SOCKS5_H
#define SOCKS5_H

// NOTE: ONLY Socks5 is supported.
typedef enum Socks_Version {
    Socks_Version_Reserved = 0x00,
    Socks_Version_4        = 0x04,
    Socks_Version_5        = 0x05,
} Socks_Version;

// rfc 1928: "Compliant implementations MUST support GSSAPI
//            and SHOULD support USERNAME/PASSWORD authentication methods."
// NOTE: 0x03 - 0x7F are IANA assigned.
//       0x80 - 0xFE are reserved for private methods.
typedef enum Socks_AuthMethod {
    Socks_AuthMethod_NoAuth   = 0x00,
    Socks_AuthMethod_GssApi   = 0x01,
    Socks_AuthMethod_UserPass = 0x02,
    Socks_AUthMethod_NoMethod = 0xff,
} Socks_AuthMethod;

typedef enum Socks_Command {
    Socks_Command_Connect      = 0x01,
    Socks_Command_Bind         = 0x02,
    Socks_Command_UdpAssociate = 0x03,
} Socks_Command;

typedef enum Socks_AddressType {
    Socks_AddressType_Ipv4       = 0x01,
    Socks_AddressType_DomainName = 0x03,
    Socks_AddressType_Ipv6       = 0x04,
} Socks_AddressType;

// NODE: 0x09 - 0xFF unassigned.
typedef enum Socks_ReplyCode {
    Socks_ReplyCode_Succeeded,
    Socks_ReplyCode_GeneralFailure,
    Socks_ReplyCode_NotAllowed,
    Socks_ReplyCode_NetworkUnreachable,
    Socks_ReplyCode_HostUnreachable,
    Socks_ReplyCode_ConnectionRefused,
    Socks_ReplyCode_TtlExpired,
    Socks_ReplyCode_UnsupportedCommand,
    Socks_ReplyCode_UnsupportedAddressType,
    Socks_ReplyCode_COUNT
} Socks_ReplyCode;



#endif // SOCKS5_H
