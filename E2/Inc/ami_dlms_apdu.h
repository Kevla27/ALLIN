#ifndef AMI_DLMS_APDU_H
#define AMI_DLMS_APDU_H

#include "ami_cosem_model.h"
#include "ami_dlms_association.h"
#include "ami_security.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AMI_DLMS_APDU_OK = 0,
    AMI_DLMS_APDU_NOT_ASSOCIATED,
    AMI_DLMS_APDU_MALFORMED,
    AMI_DLMS_APDU_ACCESS_ERROR,
    AMI_DLMS_APDU_BUFFER_TOO_SMALL,
    AMI_DLMS_APDU_INVALID_ARGUMENT,
    AMI_DLMS_APDU_CIPHER_REQUIRED,
    AMI_DLMS_APDU_AUTHENTICATION_FAILURE,
    AMI_DLMS_APDU_COUNTER_REJECTED,
    AMI_DLMS_APDU_BACKEND_FAILURE
} AmiDlmsApduStatus;

#define AMI_DLMS_GCM_TAG_LENGTH 16u

typedef struct {
    uint8_t key_type; /* DLMS enum: 0 = global unicast, 2 = authentication. */
    const uint8_t *wrapped_key;
    size_t wrapped_key_length;
} AmiDlmsWrappedKey;

/* Secure-world SAL callbacks. They must use AES-GCM with the negotiated
 * DLMS profile, construct nonce/AAD from the peer/local system titles and
 * security header, and keep keys in Secure storage. Counter callbacks must
 * perform durable anti-replay check/commit and monotonic TX allocation. */
typedef struct {
    void *user;
    uint8_t expected_security_control;
    AmiSecStatus (*decrypt)(void *user, uint8_t security_control,
        uint32_t invocation_counter, const uint8_t *ciphertext, size_t ciphertext_length,
        const uint8_t tag[AMI_DLMS_GCM_TAG_LENGTH], uint8_t *plaintext,
        size_t plaintext_capacity, size_t *plaintext_length);
    AmiSecStatus (*encrypt)(void *user, uint8_t security_control,
        uint32_t invocation_counter, const uint8_t *plaintext, size_t plaintext_length,
        uint8_t *ciphertext, size_t ciphertext_capacity, size_t *ciphertext_length,
        uint8_t tag[AMI_DLMS_GCM_TAG_LENGTH]);
    int (*accept_rx_counter)(void *user, uint32_t invocation_counter);
    int (*next_tx_counter)(void *user, uint32_t *invocation_counter);
    /* Suite-0 SAL must unwrap 24-byte AES-128-wrapped values using its secure
     * KEK and atomically persist the batch; plaintext keys must never leave it. */
    AmiSecStatus (*transfer_keys)(void *user,
        const AmiDlmsWrappedKey *keys, size_t key_count);
} AmiDlmsCipherBackend;

/* Processes GET-Request-Normal (C0 01). Class-7 buffer selective access
 * supports selector 1 (Clock date-time range) and selector 2 (entry range,
 * full rows only). Unsupported descriptors fail with scope-of-access-violated. */
AmiDlmsApduStatus ami_dlms_process_get_request(
    const AmiDlmsAssociation *association,
    const AmiCosemRegisterSnapshot *snapshot,
    const uint8_t *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *response_length);

/* For an HLS-associated session, accepts only glo-get-request (C8) or
 * glo-set-request (C9), decrypts/authenticates, checks replay, dispatches the
 * read-only service, and returns a ciphered glo-* response (CC/CD). workspace
 * is caller-owned RAM for the decrypted APDU and plaintext response. */
AmiDlmsApduStatus ami_dlms_process_ciphered_request(
    const AmiDlmsAssociation *association,
    const AmiCosemRegisterSnapshot *snapshot,
    const AmiDlmsCipherBackend *cipher,
    const uint8_t *request, size_t request_length,
    uint8_t *workspace, size_t workspace_capacity,
    uint8_t *response, size_t response_capacity, size_t *response_length);

#ifdef __cplusplus
}
#endif

#endif
