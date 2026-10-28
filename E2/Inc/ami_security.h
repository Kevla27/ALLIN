#ifndef AMI_SECURITY_H
#define AMI_SECURITY_H

/*
 * Platform-neutral security boundary for a postpaid AMI/DLMS application.
 * This layer deliberately accepts protocol-produced nonce/AAD values: the
 * DLMS security suite and invocation-counter rules must come from the approved
 * SPLN/DLMS profile, not from a second, project-specific wire format.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AMI_SEC_OK = 0,
    AMI_SEC_INVALID_ARGUMENT,
    AMI_SEC_UNSUPPORTED,
    AMI_SEC_BACKEND_FAILURE,
    AMI_SEC_AUTHENTICATION_FAILURE
} AmiSecStatus;

typedef enum {
    AMI_AEAD_AES_128_GCM = 1,
    AMI_AEAD_AES_256_GCM = 2
} AmiAeadAlgorithm;

/* The key identifier refers to a key held by the secure backend. Callers
 * never receive key bytes through this interface. */
typedef uint32_t AmiKeyHandle;

typedef struct {
    void *user;
    int (*random)(void *user, uint8_t *out, size_t out_len);
    AmiSecStatus (*aead_encrypt)(void *user, AmiAeadAlgorithm algorithm,
                        AmiKeyHandle key, const uint8_t *nonce,
                        size_t nonce_len, const uint8_t *aad,
                        size_t aad_len, const uint8_t *plaintext,
                        size_t plaintext_len, uint8_t *ciphertext,
                        uint8_t *tag, size_t tag_len);
    AmiSecStatus (*aead_decrypt)(void *user, AmiAeadAlgorithm algorithm,
                        AmiKeyHandle key, const uint8_t *nonce,
                        size_t nonce_len, const uint8_t *aad,
                        size_t aad_len, const uint8_t *ciphertext,
                        size_t ciphertext_len, const uint8_t *tag,
                        size_t tag_len, uint8_t *plaintext);
} AmiSecBackend;

/* Validate that every required cryptographic operation is present. */
AmiSecStatus ami_sec_backend_validate(const AmiSecBackend *backend);

/* Obtain random material from the selected secure backend. */
AmiSecStatus ami_sec_random(const AmiSecBackend *backend,
                            uint8_t *out, size_t out_len);

/* Authenticated encryption/decryption using a backend-held key. The nonce,
 * AAD, algorithm, tag size, and invocation counter are supplied by the
 * protocol adapter and must follow the approved profile. */
AmiSecStatus ami_sec_aead_encrypt(const AmiSecBackend *backend,
                                 AmiAeadAlgorithm algorithm,
                                 AmiKeyHandle key,
                                 const uint8_t *nonce, size_t nonce_len,
                                 const uint8_t *aad, size_t aad_len,
                                 const uint8_t *plaintext, size_t plaintext_len,
                                 uint8_t *ciphertext,
                                 uint8_t *tag, size_t tag_len);

AmiSecStatus ami_sec_aead_decrypt(const AmiSecBackend *backend,
                                 AmiAeadAlgorithm algorithm,
                                 AmiKeyHandle key,
                                 const uint8_t *nonce, size_t nonce_len,
                                 const uint8_t *aad, size_t aad_len,
                                 const uint8_t *ciphertext,
                                 size_t ciphertext_len,
                                 const uint8_t *tag, size_t tag_len,
                                 uint8_t *plaintext);

#ifdef __cplusplus
}
#endif

#endif
