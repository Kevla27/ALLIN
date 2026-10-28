#include "ami_security.h"

#include <string.h>

#define AMI_GCM_NONCE_BYTES 12u
#define AMI_GCM_TAG_BYTES 16u
#define AMI_MAX_INPUT_BYTES 0x7fffffffu

static int bytes_ok(const uint8_t *p, size_t n)
{
    return n == 0u || p != NULL;
}

static int supported_algorithm(AmiAeadAlgorithm algorithm)
{
    return algorithm == AMI_AEAD_AES_128_GCM ||
           algorithm == AMI_AEAD_AES_256_GCM;
}

AmiSecStatus ami_sec_backend_validate(const AmiSecBackend *backend)
{
    if (backend == NULL || backend->random == NULL ||
        backend->aead_encrypt == NULL || backend->aead_decrypt == NULL) {
        return AMI_SEC_INVALID_ARGUMENT;
    }
    return AMI_SEC_OK;
}

AmiSecStatus ami_sec_random(const AmiSecBackend *backend,
                            uint8_t *out, size_t out_len)
{
    if (ami_sec_backend_validate(backend) != AMI_SEC_OK ||
        out == NULL || out_len == 0u || out_len > AMI_MAX_INPUT_BYTES) {
        return AMI_SEC_INVALID_ARGUMENT;
    }
    if (backend->random(backend->user, out, out_len) != 1) {
        memset(out, 0, out_len);
        return AMI_SEC_BACKEND_FAILURE;
    }
    return AMI_SEC_OK;
}

AmiSecStatus ami_sec_aead_encrypt(const AmiSecBackend *backend,
                                 AmiAeadAlgorithm algorithm,
                                 AmiKeyHandle key,
                                 const uint8_t *nonce, size_t nonce_len,
                                 const uint8_t *aad, size_t aad_len,
                                 const uint8_t *plaintext, size_t plaintext_len,
                                 uint8_t *ciphertext,
                                 uint8_t *tag, size_t tag_len)
{
    if (ami_sec_backend_validate(backend) != AMI_SEC_OK ||
        !supported_algorithm(algorithm) || key == 0u || nonce == NULL ||
        nonce_len != AMI_GCM_NONCE_BYTES || tag == NULL ||
        tag_len != AMI_GCM_TAG_BYTES ||
        !bytes_ok(aad, aad_len) || !bytes_ok(plaintext, plaintext_len) ||
        (plaintext_len != 0u && ciphertext == NULL) ||
        aad_len > AMI_MAX_INPUT_BYTES || plaintext_len > AMI_MAX_INPUT_BYTES) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    AmiSecStatus result = backend->aead_encrypt(
        backend->user, algorithm, key, nonce, nonce_len, aad, aad_len,
        plaintext, plaintext_len, ciphertext, tag, tag_len);
    if (result != AMI_SEC_OK) {
        if (ciphertext != NULL && plaintext_len != 0u) {
            memset(ciphertext, 0, plaintext_len);
        }
        memset(tag, 0, tag_len);
        return result == AMI_SEC_AUTHENTICATION_FAILURE
            ? AMI_SEC_AUTHENTICATION_FAILURE : AMI_SEC_BACKEND_FAILURE;
    }
    return AMI_SEC_OK;
}

AmiSecStatus ami_sec_aead_decrypt(const AmiSecBackend *backend,
                                 AmiAeadAlgorithm algorithm,
                                 AmiKeyHandle key,
                                 const uint8_t *nonce, size_t nonce_len,
                                 const uint8_t *aad, size_t aad_len,
                                 const uint8_t *ciphertext,
                                 size_t ciphertext_len,
                                 const uint8_t *tag, size_t tag_len,
                                 uint8_t *plaintext)
{
    if (ami_sec_backend_validate(backend) != AMI_SEC_OK ||
        !supported_algorithm(algorithm) || key == 0u || nonce == NULL ||
        nonce_len != AMI_GCM_NONCE_BYTES || tag == NULL ||
        tag_len != AMI_GCM_TAG_BYTES ||
        !bytes_ok(aad, aad_len) || !bytes_ok(ciphertext, ciphertext_len) ||
        (ciphertext_len != 0u && plaintext == NULL) ||
        aad_len > AMI_MAX_INPUT_BYTES || ciphertext_len > AMI_MAX_INPUT_BYTES) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    AmiSecStatus result = backend->aead_decrypt(
        backend->user, algorithm, key, nonce, nonce_len, aad, aad_len,
        ciphertext, ciphertext_len, tag, tag_len, plaintext);
    if (result != AMI_SEC_OK) {
        if (plaintext != NULL && ciphertext_len != 0u) {
            memset(plaintext, 0, ciphertext_len);
        }
        return result == AMI_SEC_AUTHENTICATION_FAILURE
            ? AMI_SEC_AUTHENTICATION_FAILURE : AMI_SEC_BACKEND_FAILURE;
    }
    return AMI_SEC_OK;
}
