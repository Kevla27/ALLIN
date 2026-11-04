#ifndef AMI_DLMS_CIPHER_H
#define AMI_DLMS_CIPHER_H

#include <stddef.h>
#include <stdint.h>
#include "ami_dlms_apdu.h"
#include "ami_security.h"
#include "ami_nvm_counter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_DLMS_SECURITY_CONTROL_SUITE_0  0x30u /* Auth + Encrypt, Suite 0 */
#define AMI_DLMS_SYSTEM_TITLE_LENGTH       8u
#define AMI_DLMS_KEY_LENGTH                16u

typedef struct {
    uint8_t encryption_key[AMI_DLMS_KEY_LENGTH];     /* Global Unicast / Dedicated Key */
    uint8_t authentication_key[AMI_DLMS_KEY_LENGTH]; /* Authentication Key (AK) */
    uint8_t master_kek[AMI_DLMS_KEY_LENGTH];         /* Master Key Encrypting Key for Key Transfer */
    uint8_t client_system_title[AMI_DLMS_SYSTEM_TITLE_LENGTH];
    uint8_t server_system_title[AMI_DLMS_SYSTEM_TITLE_LENGTH];
    uint32_t last_rx_counter;                        /* Anti-replay: must strictly increase */
    uint32_t current_tx_counter;                     /* Monotonically increasing TX counter */
    AmiNvmCounterContext *nvm_counter;               /* Optional persistent NVM counter manager */
    uint8_t expected_security_control;
    int is_initialized;
} AmiDlmsSecuritySuite0Context;

/**
 * @brief Initialize the DLMS Security Suite 0 context with keys and system titles.
 */
int ami_dlms_cipher_suite0_init(AmiDlmsSecuritySuite0Context *ctx,
                                const uint8_t encryption_key[AMI_DLMS_KEY_LENGTH],
                                const uint8_t authentication_key[AMI_DLMS_KEY_LENGTH],
                                const uint8_t client_system_title[AMI_DLMS_SYSTEM_TITLE_LENGTH],
                                const uint8_t server_system_title[AMI_DLMS_SYSTEM_TITLE_LENGTH],
                                uint32_t initial_tx_counter);

/**
 * @brief Populate an AmiDlmsCipherBackend interface using this Suite 0 context.
 */
int ami_dlms_cipher_suite0_get_backend(AmiDlmsSecuritySuite0Context *ctx,
                                       AmiDlmsCipherBackend *backend);

/**
 * @brief Bind an NVM counter manager to the DLMS Security Suite 0 context.
 * Enables persistent strictly monotonic TX counters & persistent anti-replay tracking.
 */
int ami_dlms_cipher_suite0_bind_nvm(AmiDlmsSecuritySuite0Context *ctx,
                                    AmiNvmCounterContext *nvm);

/**
 * @brief Configure Master Key (KEK) in Suite 0 context for RFC 3394 key unwrapping.
 */
int ami_dlms_cipher_suite0_set_kek(AmiDlmsSecuritySuite0Context *ctx,
                                   const uint8_t kek[AMI_DLMS_KEY_LENGTH]);

/**
 * @brief Helper to wrap an operational key using the Master KEK according to RFC 3394.
 */
int ami_dlms_cipher_suite0_wrap_key(const AmiDlmsSecuritySuite0Context *ctx,
                                    const uint8_t *plain_key,
                                    size_t plain_len,
                                    uint8_t *out_wrapped,
                                    size_t out_capacity,
                                    size_t *out_len);

/**
 * @brief Helper to encrypt a plaintext APDU into a wire glo-* request (e.g. for testing/client).
 */
int ami_dlms_cipher_suite0_build_glo_request(
    const AmiDlmsSecuritySuite0Context *ctx,
    uint8_t glo_tag,
    uint32_t invocation_counter,
    const uint8_t *plain_apdu,
    size_t plain_len,
    uint8_t *out_wire,
    size_t out_capacity,
    size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* AMI_DLMS_CIPHER_H */

