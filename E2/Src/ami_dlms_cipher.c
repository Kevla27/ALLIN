#include "ami_dlms_cipher.h"
#include <string.h>

#if defined(STM32U585xx)
#include "secure_nsc.h"
#endif

static AmiSecStatus suite0_decrypt_cb(void *user, uint8_t security_control,
    uint32_t invocation_counter, const uint8_t *ciphertext, size_t ciphertext_length,
    const uint8_t tag[AMI_DLMS_GCM_TAG_LENGTH], uint8_t *plaintext,
    size_t plaintext_capacity, size_t *plaintext_length)
{
    AmiDlmsSecuritySuite0Context *ctx = (AmiDlmsSecuritySuite0Context *)user;
    if (ctx == NULL || plaintext == NULL || plaintext_length == NULL ||
        (ciphertext_length > 0 && ciphertext == NULL)) {
        return AMI_SEC_INVALID_ARGUMENT;
    }
    if (plaintext_capacity < ciphertext_length) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    /* 1. Build 12-byte Nonce: Client System Title (8B) || Invocation Counter (4B BE) */
    uint8_t nonce[AMI_GCM_NONCE_BYTES];
    memcpy(nonce, ctx->client_system_title, AMI_DLMS_SYSTEM_TITLE_LENGTH);
    nonce[8]  = (uint8_t)(invocation_counter >> 24);
    nonce[9]  = (uint8_t)(invocation_counter >> 16);
    nonce[10] = (uint8_t)(invocation_counter >> 8);
    nonce[11] = (uint8_t)invocation_counter;

    /* 2. Build 17-byte AAD: Security Control (1B) || Authentication Key (16B) */
    uint8_t aad[1u + AMI_DLMS_KEY_LENGTH];
    aad[0] = security_control;
    memcpy(&aad[1], ctx->authentication_key, AMI_DLMS_KEY_LENGTH);

#if defined(STM32U585xx)
    /* Hardware-isolated Secure World SAES via TrustZone CMSE */
    SECURE_AesGcmParamsTypeDef params = {
        .key_id = SECURE_KEY_GUEK,
        .is_encrypt = 0,
        .iv = nonce,
        .iv_len = sizeof(nonce),
        .aad = aad,
        .aad_len = sizeof(aad),
        .input = ciphertext,
        .in_len = (uint32_t)ciphertext_length,
        .output = plaintext,
        .tag = (uint8_t *)tag
    };
    if (SECURE_DLMS_AES_GCM_Process(&params) != 0) {
        return AMI_SEC_AUTHENTICATION_FAILURE;
    }
    *plaintext_length = ciphertext_length;
    return AMI_SEC_OK;
#else
    /* Standalone NIST SP 800-38D software AES-128-GCM engine */
    AmiSecStatus status = ami_sec_aes_128_gcm_decrypt(
        ctx->encryption_key,
        nonce, sizeof(nonce),
        aad, sizeof(aad),
        ciphertext, ciphertext_length,
        tag, AMI_DLMS_GCM_TAG_LENGTH,
        plaintext);

    if (status == AMI_SEC_OK) {
        *plaintext_length = ciphertext_length;
    }
    return status;
#endif
}

static AmiSecStatus suite0_encrypt_cb(void *user, uint8_t security_control,
    uint32_t invocation_counter, const uint8_t *plaintext, size_t plaintext_length,
    uint8_t *ciphertext, size_t ciphertext_capacity, size_t *ciphertext_length,
    uint8_t tag[AMI_DLMS_GCM_TAG_LENGTH])
{
    AmiDlmsSecuritySuite0Context *ctx = (AmiDlmsSecuritySuite0Context *)user;
    if (ctx == NULL || ciphertext == NULL || ciphertext_length == NULL ||
        (plaintext_length > 0 && plaintext == NULL) || tag == NULL) {
        return AMI_SEC_INVALID_ARGUMENT;
    }
    if (ciphertext_capacity < plaintext_length) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    /* 1. Build 12-byte Nonce: Server System Title (8B) || Invocation Counter (4B BE) */
    uint8_t nonce[AMI_GCM_NONCE_BYTES];
    memcpy(nonce, ctx->server_system_title, AMI_DLMS_SYSTEM_TITLE_LENGTH);
    nonce[8]  = (uint8_t)(invocation_counter >> 24);
    nonce[9]  = (uint8_t)(invocation_counter >> 16);
    nonce[10] = (uint8_t)(invocation_counter >> 8);
    nonce[11] = (uint8_t)invocation_counter;

    /* 2. Build 17-byte AAD: Security Control (1B) || Authentication Key (16B) */
    uint8_t aad[1u + AMI_DLMS_KEY_LENGTH];
    aad[0] = security_control;
    memcpy(&aad[1], ctx->authentication_key, AMI_DLMS_KEY_LENGTH);

#if defined(STM32U585xx)
    SECURE_AesGcmParamsTypeDef params = {
        .key_id = SECURE_KEY_GUEK,
        .is_encrypt = 1,
        .iv = nonce,
        .iv_len = sizeof(nonce),
        .aad = aad,
        .aad_len = sizeof(aad),
        .input = plaintext,
        .in_len = (uint32_t)plaintext_length,
        .output = ciphertext,
        .tag = tag
    };
    if (SECURE_DLMS_AES_GCM_Process(&params) != 0) {
        return AMI_SEC_BACKEND_FAILURE;
    }
    *ciphertext_length = plaintext_length;
    return AMI_SEC_OK;
#else
    AmiSecStatus status = ami_sec_aes_128_gcm_encrypt(
        ctx->encryption_key,
        nonce, sizeof(nonce),
        aad, sizeof(aad),
        plaintext, plaintext_length,
        ciphertext,
        tag, AMI_DLMS_GCM_TAG_LENGTH);

    if (status == AMI_SEC_OK) {
        *ciphertext_length = plaintext_length;
    }
    return status;
#endif
}

static int suite0_accept_rx_counter_cb(void *user, uint32_t invocation_counter)
{
    AmiDlmsSecuritySuite0Context *ctx = (AmiDlmsSecuritySuite0Context *)user;
    if (ctx == NULL) return 0;

    /* Anti-replay: counter must be strictly monotonically increasing */
    if (invocation_counter <= ctx->last_rx_counter) {
        return 0; /* Replay detected -> Reject! */
    }

    ctx->last_rx_counter = invocation_counter;
    return 1;
}

static int suite0_next_tx_counter_cb(void *user, uint32_t *invocation_counter)
{
    AmiDlmsSecuritySuite0Context *ctx = (AmiDlmsSecuritySuite0Context *)user;
    if (ctx == NULL || invocation_counter == NULL) return 0;

    ctx->current_tx_counter++;
    *invocation_counter = ctx->current_tx_counter;
    return 1;
}

static AmiSecStatus suite0_transfer_keys_cb(void *user,
                                           const AmiDlmsWrappedKey *keys,
                                           size_t key_count)
{
    AmiDlmsSecuritySuite0Context *ctx = (AmiDlmsSecuritySuite0Context *)user;
    if (ctx == NULL || keys == NULL || key_count == 0 || key_count > 2) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < key_count; i++) {
        if (keys[i].wrapped_key == NULL || keys[i].wrapped_key_length != 24u) {
            return AMI_SEC_INVALID_ARGUMENT;
        }
        /* In production, 24-byte AES-wrapped key is unwrapped using KEK.
         * For unwrap payload update: */
        if (keys[i].key_type == 0u) {
            /* Global Unicast Encryption Key (GUEK) */
            memcpy(ctx->encryption_key, keys[i].wrapped_key, AMI_DLMS_KEY_LENGTH);
        } else if (keys[i].key_type == 2u) {
            /* Authentication Key (AK) */
            memcpy(ctx->authentication_key, keys[i].wrapped_key, AMI_DLMS_KEY_LENGTH);
        } else {
            return AMI_SEC_INVALID_ARGUMENT;
        }
    }

    return AMI_SEC_OK;
}

int ami_dlms_cipher_suite0_init(AmiDlmsSecuritySuite0Context *ctx,
                                const uint8_t encryption_key[AMI_DLMS_KEY_LENGTH],
                                const uint8_t authentication_key[AMI_DLMS_KEY_LENGTH],
                                const uint8_t client_system_title[AMI_DLMS_SYSTEM_TITLE_LENGTH],
                                const uint8_t server_system_title[AMI_DLMS_SYSTEM_TITLE_LENGTH],
                                uint32_t initial_tx_counter)
{
    if (ctx == NULL || encryption_key == NULL || authentication_key == NULL ||
        client_system_title == NULL || server_system_title == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    memcpy(ctx->encryption_key, encryption_key, AMI_DLMS_KEY_LENGTH);
    memcpy(ctx->authentication_key, authentication_key, AMI_DLMS_KEY_LENGTH);
    memcpy(ctx->client_system_title, client_system_title, AMI_DLMS_SYSTEM_TITLE_LENGTH);
    memcpy(ctx->server_system_title, server_system_title, AMI_DLMS_SYSTEM_TITLE_LENGTH);

    ctx->current_tx_counter = initial_tx_counter;
    ctx->last_rx_counter = 0u;
    ctx->expected_security_control = AMI_DLMS_SECURITY_CONTROL_SUITE_0;
    ctx->is_initialized = 1;

    return 0;
}

int ami_dlms_cipher_suite0_get_backend(AmiDlmsSecuritySuite0Context *ctx,
                                       AmiDlmsCipherBackend *backend)
{
    if (ctx == NULL || backend == NULL || !ctx->is_initialized) {
        return -1;
    }

    memset(backend, 0, sizeof(*backend));
    backend->user = ctx;
    backend->expected_security_control = ctx->expected_security_control;
    backend->decrypt = suite0_decrypt_cb;
    backend->encrypt = suite0_encrypt_cb;
    backend->accept_rx_counter = suite0_accept_rx_counter_cb;
    backend->next_tx_counter = suite0_next_tx_counter_cb;
    backend->transfer_keys = suite0_transfer_keys_cb;

    return 0;
}

int ami_dlms_cipher_suite0_build_glo_request(
    const AmiDlmsSecuritySuite0Context *ctx,
    uint8_t glo_tag,
    uint32_t invocation_counter,
    const uint8_t *plain_apdu,
    size_t plain_len,
    uint8_t *out_wire,
    size_t out_capacity,
    size_t *out_len)
{
    if (ctx == NULL || plain_apdu == NULL || out_wire == NULL || out_len == NULL) {
        return -1;
    }

    size_t total_wire_len = 6u + plain_len + AMI_DLMS_GCM_TAG_LENGTH;
    if (out_capacity < total_wire_len) {
        return -2;
    }

    out_wire[0] = glo_tag;
    out_wire[1] = ctx->expected_security_control;
    out_wire[2] = (uint8_t)(invocation_counter >> 24);
    out_wire[3] = (uint8_t)(invocation_counter >> 16);
    out_wire[4] = (uint8_t)(invocation_counter >> 8);
    out_wire[5] = (uint8_t)invocation_counter;

    /* Build client nonce and AAD */
    uint8_t nonce[AMI_GCM_NONCE_BYTES];
    memcpy(nonce, ctx->client_system_title, AMI_DLMS_SYSTEM_TITLE_LENGTH);
    nonce[8]  = (uint8_t)(invocation_counter >> 24);
    nonce[9]  = (uint8_t)(invocation_counter >> 16);
    nonce[10] = (uint8_t)(invocation_counter >> 8);
    nonce[11] = (uint8_t)invocation_counter;

    uint8_t aad[1u + AMI_DLMS_KEY_LENGTH];
    aad[0] = ctx->expected_security_control;
    memcpy(&aad[1], ctx->authentication_key, AMI_DLMS_KEY_LENGTH);

    uint8_t *ciphertext = out_wire + 6u;
    uint8_t *tag = ciphertext + plain_len;

#if defined(STM32U585xx)
    SECURE_AesGcmParamsTypeDef params = {
        .key_id = SECURE_KEY_GUEK,
        .is_encrypt = 1,
        .iv = nonce,
        .iv_len = sizeof(nonce),
        .aad = aad,
        .aad_len = sizeof(aad),
        .input = plain_apdu,
        .in_len = (uint32_t)plain_len,
        .output = ciphertext,
        .tag = tag
    };
    if (SECURE_DLMS_AES_GCM_Process(&params) != 0) {
        return -3;
    }
#else
    AmiSecStatus sec_st = ami_sec_aes_128_gcm_encrypt(
        ctx->encryption_key,
        nonce, sizeof(nonce),
        aad, sizeof(aad),
        plain_apdu, plain_len,
        ciphertext,
        tag, AMI_DLMS_GCM_TAG_LENGTH);

    if (sec_st != AMI_SEC_OK) {
        return -3;
    }
#endif

    *out_len = total_wire_len;
    return 0;
}
