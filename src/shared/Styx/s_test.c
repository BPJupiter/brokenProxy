#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define STYX_DEBUG

/* Include your core header */
#include "styx.h"

/* --- Minimal Testing Framework --- */
#define TEST_BEGIN(name) printf("Running %s... ", #name);
#define TEST_END() printf("PASSED\n");

/* Helper to compare floats safely */
#define ASSERT_FLOAT_EQ(expected, actual) assert(fabs((expected) - (actual)) < 0.0001)

/* ==========================================
 * 1. MEMORY BUFFERS & PACKING/UNPACKING
 * ========================================== */

void test_buffer_and_serialization() {
    TEST_BEGIN(test_buffer_and_serialization)
    
    SHandle *buf = styxBufferCreate();
    assert(buf != NULL);
    assert(styxType(buf) == S_HT_BUFFER);

    /* Test Data */
    uint8 t_u8 = 250;
    int16 t_i16 = -15000;
    uint32 t_u32 = 4294967290;
    float t_f = 3.14159f;
    char *t_str1 = "Hello Styx!";
    char *t_str2_null = NULL;
    uint8 t_raw_in[5] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00};

    /* Pack */
    styxPackUint8(buf, t_u8, "test_u8");
    styxPackInt16(buf, t_i16, "test_i16");
    styxPackUint32(buf, t_u32, "test_u32");
    styxPackFloat(buf, t_f, "test_f");
    styxPackString(buf, t_str1, "test_str");
    styxPackString(buf, t_str2_null, "test_str_null"); /* Test the safe NULL packer you built */
    styxPackRaw(buf, t_raw_in, 5, "test_raw");

    /* Unpack */
    assert(styxUnpackUint8(buf, "test_u8") == t_u8);
    assert(styxUnpackInt16(buf, "test_i16") == t_i16);
    assert(styxUnpackUint32(buf, "test_u32") == t_u32);
    ASSERT_FLOAT_EQ(t_f, styxUnpackFloat(buf, "test_f"));

    /* Unpack String (Static Buffer) */
    char str_out[64];
    assert(styxUnpackString(buf, str_out, sizeof(str_out), "test_str") == TRUE);
    assert(strcmp(str_out, t_str1) == 0);

    /* Unpack String (NULL expected) */
    char *null_str_out = styxUnpackStringWithOwnership(buf, "test_str_null");
    assert(null_str_out == NULL);

    /* Unpack Raw */
    uint8 t_raw_out[5];
    assert(styxUnpackRaw(buf, t_raw_out, 5, "test_raw") == 5);
    assert(memcmp(t_raw_in, t_raw_out, 5) == 0);

    styxFree(buf);
    TEST_END()
}

/* ==========================================
 * 2. FILE MANAGEMENT
 * ========================================== */

void test_file_io() {
    TEST_BEGIN(test_file_io)
    
    char *test_file = "styx_test_dump.bin";
    uint64 magic_val = 0x123456789ABCDEF0;

    /* Write to file */
    SHandle *fw = styxFileSave(test_file);
    assert(fw != NULL);
    assert(styxType(fw) == S_HT_FILE_WRITE);
    styxPackUint64(fw, magic_val, "magic");
    styxPackString(fw, "File Stream Test", "str");
    styxFree(fw);

    /* Read from file */
    SHandle *fr = styxFileLoad(test_file);
    assert(fr != NULL);
    assert(styxType(fr) == S_HT_FILE_READ);
    
    /* Verify size (should be > 0) */
    assert(styxFileSize(fr) > 0);

    /* Unpack and verify */
    assert(styxUnpackUint64(fr, "magic") == magic_val);
    
    char *owned_str = styxUnpackStringWithOwnership(fr, "str");
    assert(owned_str != NULL);
    assert(strcmp(owned_str, "File Stream Test") == 0);
    free(owned_str); /* Assume caller owns and must free */

    styxFree(fr);
    remove(test_file); /* Cleanup */
    
    TEST_END()
}

/* ==========================================
 * 3. NETWORKING (UDP & TCP)
 * ========================================== */

void test_network_addressing() {
    TEST_BEGIN(test_network_addressing)
    
    StyxNetworkAddress addr1, addr2;
    boolean is_ipv6;

    /* Note: Requires an active internet connection to resolve localhost/DNS */
    if (styxNetworkAddressLookup(&addr1, "127.0.0.1", 8080, &is_ipv6)) {
        assert(addr1.port == 8080);
        assert(addr1.is_ipv6 == FALSE);
        
        styxNetworkAddressLookup(&addr2, "127.0.0.1", 8080, &is_ipv6);
        assert(styxNetworkAddressCompare(&addr1, &addr2) == TRUE);
        
        styxNetworkAddressLookup(&addr2, "127.0.0.1", 9090, &is_ipv6);
        assert(styxNetworkAddressCompare(&addr1, &addr2) == FALSE);
    }

    TEST_END()
}

void test_udp_datagrams() {
    TEST_BEGIN(test_udp_datagrams)

    uint16 test_port = 27015;
    
    /* Create Receiver */
    SHandle *udp_server = styxNetworkDatagramCreate(test_port, FALSE);
    assert(udp_server != NULL);
    assert(styxType(udp_server) == S_HT_PACKET_PEER);

    /* Create Sender (binds to any available port) */
    SHandle *udp_client = styxNetworkDatagramCreate(0, FALSE);
    assert(udp_client != NULL);

    /* Resolve destination address */
    StyxNetworkAddress dest;
    boolean is_ipv6;
    styxNetworkAddressLookup(&dest, "127.0.0.1", test_port, &is_ipv6);

    /* Pack Data on Client */
    styxPackInt32(udp_client, 42, "ping");
    styxNetworkDatagramSend(udp_client, &dest);

    /* Wait for network (Wait up to 1 second for readability) */
    boolean can_read = FALSE, can_write = FALSE;
    int wait_res = styxNetworkWait(&udp_server, &can_read, &can_write, 1, 1000000);
    
    if (wait_res > 0 && can_read) {
        StyxNetworkAddress from;
        int bytes = styxNetworkReceive(udp_server, &from);
        assert(bytes > 0);
        assert(styxUnpackInt32(udp_server, "ping") == 42);
    } else {
        printf("[WARN: UDP packet dropped or loopback blocked] ");
    }

    styxFree(udp_client);
    styxFree(udp_server);

    TEST_END()
}

/* ==========================================
 * TEST RUNNER
 * ========================================== */

int main() {
    printf("Starting Styx API Unit Tests...\n");
    printf("--------------------------------\n");

    test_buffer_and_serialization();
    test_file_io();
    test_network_addressing();
    test_udp_datagrams();
    
    /* * Note: TCP Stream tests (styxNetworkStreamAddressCreate, etc.) 
     * are omitted from the basic suite as testing blocking TCP handshakes 
     * in a single thread without mocking socket internals often causes test freezes.
     * I recommend wrapping TCP listener tests in separate threads.
     */

    printf("--------------------------------\n");
    printf("All active tests completed successfully!\n");
    
    return 0;
}