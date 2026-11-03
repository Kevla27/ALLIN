#ifndef AMI_SECURITY_H
#define AMI_SECURITY_H

/*
 * Platform-neutral security boundary for a postpaid AMI/DLMS application.
 * This layer accepts protocol-produced nonce/AAD values: the DLMS security
 * suite and invocation-counter rules follow the approved SPLN/DLMS profile.
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

#define AMI_AES_128_KEY_BYTES           16u
#define AMI_GCM_NONCE_BYTES             12u
#define AMI_GCM_TAG_BYTES               16u

/* ========================================================================= */
/* Standalone AES-128-GCM & GMAC Primitives (NIST SP 800-38D Standard KAT)   */
/* ========================================================================= */

#define AMI_DLMS_HLS5_GMAC_OID_LEN      7u
#define AMI_DLMS_HLS5_SC_AUTH_ONLY      0x10u
#define AMI_DLMS_HLS5_PROOF_LEN         21u  /* SC (1B) + IC (4B) + Tag (16B) */

/* Raw-key primitives and adapters are only available in host builds. */
AmiSecStatus ami_sec_aes_128_gcm_encrypt(
    const uint8_t *key_128,
    const uint8_t *nonce_96, size_t nonce_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *plaintext, size_t plaintext_len,
    uint8_t *ciphertext,
    uint8_t *tag_128, size_t tag_len);

/* NIST SP 800-38D AES-128-GCM authenticated decryption with constant-time tag
 * verification and volatile buffer zeroization on authentication failure. */
AmiSecStatus ami_sec_aes_128_gcm_decrypt(
    const uint8_t *key_128,
    const uint8_t *nonce_96, size_t nonce_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *tag_128, size_t tag_len,
    uint8_t *plaintext);

/* Galois Message Authentication Code (GMAC) authentication-only calculation. */
AmiSecStatus ami_sec_aes_128_gmac(
    const uint8_t *key_128,
    const uint8_t *nonce_96, size_t nonce_len,
    const uint8_t *data, size_t data_len,
    uint8_t *tag_128, size_t tag_len);

/* ========================================================================= */
/* DLMS/COSEM HLS Mechanism 5 (GMAC) Challenge-Response Implementation       */
/* ========================================================================= */

/* Computes DLMS HLS 5 GMAC response: f(challenge) = SC || IC || GMAC(Key=AK, IV=Title||IC, AAD=SC||AK||Challenge).
 * Generates 21-byte proof if out_capacity >= 21; otherwise outputs 16-byte raw tag if out_capacity == 16. */
AmiSecStatus ami_sec_dlms_hls5_gmac_calculate(
    const uint8_t *auth_key,
    const uint8_t *system_title,
    uint32_t invocation_counter,
    const uint8_t *challenge, size_t challenge_len,
    uint8_t *out_proof, size_t out_capacity, size_t *out_len);

/* Verifies DLMS HLS 5 GMAC proof against the given challenge in constant time.
 * Supports both full 21-byte wire proof (SC + IC + Tag) and 16-byte raw tag. */
AmiSecStatus ami_sec_dlms_hls5_gmac_verify(
    const uint8_t *auth_key,
    const uint8_t *system_title,
    const uint8_t *received_proof, size_t received_proof_len,
    const uint8_t *challenge, size_t challenge_len);

/* Context structure for DLMS HLS 5 GMAC proof callback. */
typedef struct {
    uint8_t auth_key[AMI_AES_128_KEY_BYTES];
    uint8_t system_title[8];
    uint32_t invocation_counter;
} AmiHls5GmacContext;

/* Callback adapter matching AmiDlmsAssociationSecurity.hls_proof signature. */
int ami_sec_hls5_gmac_proof_callback(
    void *user,
    const uint8_t *mechanism_oid, size_t mechanism_oid_length,
    const uint8_t *challenge, size_t challenge_length,
    uint8_t *proof, size_t proof_capacity, size_t *proof_length);

/* Initializes an AmiSecBackend powered by the pure-software AES-128-GCM engine. */
AmiSecStatus ami_sec_init_software_backend(
    AmiSecBackend *backend,
    const uint8_t *key_128);

#ifdef __cplusplus
}
#endif

#endif /* AMI_SECURITY_H */
