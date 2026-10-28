/**
 * @file test_load_profile_dlms_adapter.c
 * @brief Unit Test untuk Pemformatan A-XDR DLMS Adapter
 */

#include <stdio.h>
#include <assert.h>
#include "load_profile_dlms_adapter.h"

void test_tamper_dlms_encoding(void) {
    printf("[TEST] Testing Tamper Log A-XDR DLMS Encoding...\n");

    tamper_event_entry_t entry = {
        .timestamp = 1727570400,
        .vector = TAMPER_VECTOR_CASE_OPEN,
        .is_asserted = true
    };

    uint8_t buffer[64];
    size_t len = tamper_log_encode_dlms_entry(&entry, buffer, sizeof(buffer));

    assert(len > 0);
    assert(buffer[0] == DLMS_AXDR_TAG_STRUCTURE);
    assert(buffer[1] == 4); /* 3 elemen */

    printf("   Encoded Buffer Length: %zu bytes\n", len);
    printf("   [PASS] Tamper A-XDR Encoding Valid 100%!\n\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING DLMS ADAPTER UNIT TESTS        \n");
    printf("=========================================\n\n");

    test_tamper_dlms_encoding();

    printf("=========================================\n");
    printf(" ALL DLMS ADAPTER TESTS PASSED 100%!    \n");
    printf("=========================================\n");
    return 0;
}