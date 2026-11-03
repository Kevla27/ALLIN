#include "ami_security.h"

#include <string.h>

#define AMI_MAX_INPUT_BYTES 0x7fffffffu

/* Standard DLMS HLS 5 GMAC mechanism OID: 2.16.756.5.8.2.5 */
static const uint8_t dlms_hls5_gmac_oid[] = {
    0x60u, 0x85u, 0x74u, 0x05u, 0x08u, 0x02u, 0x05u
};

/* FIPS 197 Standard AES S-Box */
static const uint8_t sbox[256] = {
    0x63u, 0x7cu, 0x77u, 0x7bu, 0xf2u, 0x6bu, 0x6fu, 0xc5u, 0x30u, 0x01u, 0x67u, 0x2bu, 0xfeu, 0xd7u, 0xabu, 0x76u,
    0xcau, 0x82u, 0xc9u, 0x7du, 0xfau, 0x59u, 0x47u, 0xf0u, 0xadu, 0xd4u, 0xa2u, 0xafu, 0x9cu, 0xa4u, 0x72u, 0xc0u,
    0xb7u, 0xfdu, 0x93u, 0x26u, 0x36u, 0x3fu, 0xf7u, 0xccu, 0x34u, 0xa5u, 0xe5u, 0xf1u, 0x71u, 0xd8u, 0x31u, 0x15u,
    0x04u, 0xc7u, 0x23u, 0xc3u, 0x18u, 0x96u, 0x05u, 0x9au, 0x07u, 0x12u, 0x80u, 0xe2u, 0xebu, 0x27u, 0xb2u, 0x75u,
    0x09u, 0x83u, 0x2cu, 0x1au, 0x1bu, 0x6eu, 0x5au, 0xa0u, 0x52u, 0x3bu, 0xd6u, 0xb3u, 0x29u, 0xe3u, 0x2fu, 0x84u,
    0x53u, 0xd1u, 0x00u, 0xedu, 0x20u, 0xfcu, 0xb1u, 0x5bu, 0x6au, 0xcbu, 0xbeu, 0x39u, 0x4au, 0x4cu, 0x58u, 0xcfu,
    0xd0u, 0xefu, 0xaau, 0xfbu, 0x43u, 0x4du, 0x33u, 0x85u, 0x45u, 0xf9u, 0x02u, 0x7fu, 0x50u, 0x3cu, 0x9fu, 0xa8u,
    0x51u, 0xa3u, 0x40u, 0x8fu, 0x92u, 0x9du, 0x38u, 0xf5u, 0xbcu, 0xb6u, 0xdau, 0x21u, 0x10u, 0xffu, 0xf3u, 0xd2u,
    0xcdu, 0x0cu, 0x13u, 0xecu, 0x5fu, 0x97u, 0x44u, 0x17u, 0xc4u, 0xa7u, 0x7eu, 0x3du, 0x64u, 0x5du, 0x19u, 0x73u,
    0x60u, 0x81u, 0x4fu, 0xdcu, 0x22u, 0x2au, 0x90u, 0x88u, 0x46u, 0xeeu, 0xb8u, 0x14u, 0xdeu, 0x5eu, 0x0bu, 0xdbu,
    0xe0u, 0x32u, 0x3au, 0x0au, 0x49u, 0x06u, 0x24u, 0x5cu, 0xc2u, 0xd3u, 0xacu, 0x62u, 0x91u, 0x95u, 0xe4u, 0x79u,
    0xe7u, 0xc8u, 0x37u, 0x6du, 0x8du, 0xd5u, 0x4eu, 0xa9u, 0x6cu, 0x56u, 0xf4u, 0xeau, 0x65u, 0x7au, 0xaeu, 0x08u,
    0xbau, 0x78u, 0x25u, 0x2eu, 0x1cu, 0xa6u, 0xb4u, 0xc6u, 0xe8u, 0xddu, 0x74u, 0x1fu, 0x4bu, 0xbdu, 0x8bu, 0x8au,
    0x70u, 0x3eu, 0xb5u, 0x66u, 0x48u, 0x03u, 0xf6u, 0x0eu, 0x61u, 0x35u, 0x57u, 0xb9u, 0x86u, 0xc1u, 0x1du, 0x9eu,
    0xe1u, 0xf8u, 0x98u, 0x11u, 0x69u, 0xd9u, 0x8eu, 0x94u, 0x9bu, 0x1eu, 0x87u, 0xe9u, 0xceu, 0x55u, 0x28u, 0xdfu,
    0x8cu, 0xa1u, 0x89u, 0x0du, 0xbfu, 0xe6u, 0x42u, 0x68u, 0x41u, 0x99u, 0x2du, 0x0fu, 0xb0u, 0x54u, 0xbbu, 0x16u
};

static const uint8_t rcon[10] = {
    0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x20u, 0x40u, 0x80u, 0x1bu, 0x36u
};

static void secure_zeroize(void *ptr, size_t len)
{
    if (ptr == NULL || len == 0u) return;
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) *p++ = 0u;
}

static int constant_time_memcmp(const void *a, const void *b, size_t len)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t diff = 0u;
    for (size_t i = 0u; i < len; ++i) {
        diff |= (uint8_t)(pa[i] ^ pb[i]);
    }
    return diff == 0u;
}

static int bytes_ok(const uint8_t *p, size_t n)
{
    return n == 0u || p != NULL;
}

static int supported_algorithm(AmiAeadAlgorithm algorithm)
{
    return algorithm == AMI_AEAD_AES_128_GCM ||
           algorithm == AMI_AEAD_AES_256_GCM;
}

/* ========================================================================= */
/* AES-128 Core Functions                                                    */
/* ========================================================================= */

static void aes128_key_expansion(const uint8_t *key, uint8_t *round_keys)
{
    memcpy(round_keys, key, 16u);
    for (int i = 4; i < 44; ++i) {
        uint8_t temp[4];
        memcpy(temp, &round_keys[(i - 1) * 4], 4u);
        if (i % 4 == 0) {
            uint8_t t = temp[0];
            temp[0] = sbox[temp[1]] ^ rcon[(i / 4) - 1];
            temp[1] = sbox[temp[2]];
            temp[2] = sbox[temp[3]];
            temp[3] = sbox[t];
        }
        for (int j = 0; j < 4; ++j) {
            round_keys[i * 4 + j] = (uint8_t)(round_keys[(i - 4) * 4 + j] ^ temp[j]);
        }
    }
}

static uint8_t xtime(uint8_t x)
{
    return (uint8_t)((x << 1) ^ (((x >> 7) & 1u) * 0x1bu));
}

static void aes128_encrypt_block(const uint8_t *round_keys, const uint8_t *in, uint8_t *out)
{
    uint8_t state[16];
    for (int i = 0; i < 16; ++i) state[i] = in[i] ^ round_keys[i];

    for (int round = 1; round <= 10; ++round) {
        /* SubBytes */
        for (int i = 0; i < 16; ++i) state[i] = sbox[state[i]];

        /* ShiftRows */
        uint8_t temp = state[1];
        state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = temp;
        temp = state[2]; uint8_t temp2 = state[6];
        state[2] = state[10]; state[6] = state[14]; state[10] = temp; state[14] = temp2;
        temp = state[15];
        state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = temp;

        /* MixColumns (rounds 1 to 9) */
        if (round < 10) {
            for (int col = 0; col < 4; ++col) {
                int idx = col * 4;
                uint8_t a0 = state[idx], a1 = state[idx + 1], a2 = state[idx + 2], a3 = state[idx + 3];
                uint8_t t = a0 ^ a1 ^ a2 ^ a3;
                uint8_t u = a0;
                state[idx]     ^= t ^ xtime(a0 ^ a1);
                state[idx + 1] ^= t ^ xtime(a1 ^ a2);
                state[idx + 2] ^= t ^ xtime(a2 ^ a3);
                state[idx + 3] ^= t ^ xtime(a3 ^ u);
            }
        }

        /* AddRoundKey */
        const uint8_t *rk = &round_keys[round * 16];
        for (int i = 0; i < 16; ++i) state[i] ^= rk[i];
    }
    memcpy(out, state, 16u);
    secure_zeroize(state, sizeof(state));
}

/* ========================================================================= */
/* Galois Field GF(2^128) & GHASH Functions (NIST SP 800-38D)                */
/* ========================================================================= */

static void gf128_mul(const uint8_t *x, const uint8_t *y, uint8_t *res)
{
    uint8_t v[16];
    uint8_t z[16] = {0};
    memcpy(v, y, 16u);

    for (int i = 0; i < 16; ++i) {
        uint8_t b = x[i];
        for (int bit = 7; bit >= 0; --bit) {
            if ((b >> bit) & 1u) {
                for (int j = 0; j < 16; ++j) z[j] ^= v[j];
            }
            int lsb = v[15] & 1u;
            for (int j = 15; j > 0; --j) {
                v[j] = (uint8_t)((v[j] >> 1) | (v[j - 1] << 7));
            }
            v[0] >>= 1;
            if (lsb) {
                v[0] ^= 0xe1u; /* Field polynomial: R = 11100001 || 0^120 */
            }
        }
    }
    memcpy(res, z, 16u);
    secure_zeroize(v, sizeof(v));
    secure_zeroize(z, sizeof(z));
}

static void ghash(const uint8_t *h, const uint8_t *aad, size_t aad_len,
                  const uint8_t *ct, size_t ct_len, uint8_t *out)
{
    uint8_t y[16] = {0};

    /* Process AAD blocks */
    size_t pos = 0u;
    while (pos < aad_len) {
        size_t chunk = (aad_len - pos >= 16u) ? 16u : (aad_len - pos);
        for (size_t i = 0u; i < chunk; ++i) y[i] ^= aad[pos + i];
        gf128_mul(y, h, y);
        pos += chunk;
    }

    /* Process Ciphertext blocks */
    pos = 0u;
    while (pos < ct_len) {
        size_t chunk = (ct_len - pos >= 16u) ? 16u : (ct_len - pos);
        for (size_t i = 0u; i < chunk; ++i) y[i] ^= ct[pos + i];
        gf128_mul(y, h, y);
        pos += chunk;
    }

    /* Process Length block: len(A) bits || len(C) bits as 64-bit Big-Endian */
    uint64_t a_bits = (uint64_t)aad_len * 8u;
    uint64_t c_bits = (uint64_t)ct_len * 8u;
    uint8_t len_block[16];
    for (int i = 0; i < 8; ++i) {
        len_block[i] = (uint8_t)(a_bits >> (56 - i * 8));
        len_block[8 + i] = (uint8_t)(c_bits >> (56 - i * 8));
    }

    for (int i = 0; i < 16; ++i) y[i] ^= len_block[i];
    gf128_mul(y, h, y);

    memcpy(out, y, 16u);
    secure_zeroize(y, sizeof(y));
    secure_zeroize(len_block, sizeof(len_block));
}

static void inc32(uint8_t *block)
{
    for (int i = 15; i >= 12; --i) {
        if (++block[i] != 0u) break;
    }
}

/* ========================================================================= */
/* Standalone AES-128-GCM Implementation                                     */
/* ========================================================================= */

AmiSecStatus ami_sec_aes_128_gcm_encrypt(
    const uint8_t *key_128,
    const uint8_t *nonce_96, size_t nonce_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *plaintext, size_t plaintext_len,
    uint8_t *ciphertext,
    uint8_t *tag_128, size_t tag_len)
{
    if (key_128 == NULL || nonce_96 == NULL || nonce_len != AMI_GCM_NONCE_BYTES ||
        tag_128 == NULL || tag_len != AMI_GCM_TAG_BYTES ||
        !bytes_ok(aad, aad_len) || !bytes_ok(plaintext, plaintext_len) ||
        (plaintext_len != 0u && ciphertext == NULL) ||
        aad_len > AMI_MAX_INPUT_BYTES || plaintext_len > AMI_MAX_INPUT_BYTES) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    uint8_t rk[176];
    aes128_key_expansion(key_128, rk);

    /* Compute Hash Subkey H = AES_K(0^16) */
    uint8_t zero[16] = {0};
    uint8_t h[16];
    aes128_encrypt_block(rk, zero, h);

    /* Form J0 = IV || 0^31 || 1 */
    uint8_t j0[16];
    memcpy(j0, nonce_96, 12u);
    j0[12] = 0u; j0[13] = 0u; j0[14] = 0u; j0[15] = 1u;

    /* CTR encryption starting at inc32(J0) */
    uint8_t ctr[16];
    memcpy(ctr, j0, 16u);

    size_t pos = 0u;
    while (pos < plaintext_len) {
        inc32(ctr);
        uint8_t enc_ctr[16];
        aes128_encrypt_block(rk, ctr, enc_ctr);
        size_t chunk = (plaintext_len - pos >= 16u) ? 16u : (plaintext_len - pos);
        for (size_t i = 0u; i < chunk; ++i) {
            ciphertext[pos + i] = plaintext[pos + i] ^ enc_ctr[i];
        }
        pos += chunk;
    }

    /* Compute GHASH(H, AAD, Ciphertext) */
    uint8_t s[16];
    ghash(h, aad, aad_len, ciphertext, plaintext_len, s);

    /* Tag = S ^ AES_K(J0) */
    uint8_t enc_j0[16];
    aes128_encrypt_block(rk, j0, enc_j0);
    for (int i = 0; i < 16; ++i) tag_128[i] = s[i] ^ enc_j0[i];

    secure_zeroize(rk, sizeof(rk));
    secure_zeroize(h, sizeof(h));
    secure_zeroize(j0, sizeof(j0));
    secure_zeroize(ctr, sizeof(ctr));
    secure_zeroize(s, sizeof(s));
    secure_zeroize(enc_j0, sizeof(enc_j0));
    return AMI_SEC_OK;
}

AmiSecStatus ami_sec_aes_128_gcm_decrypt(
    const uint8_t *key_128,
    const uint8_t *nonce_96, size_t nonce_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *tag_128, size_t tag_len,
    uint8_t *plaintext)
{
    if (key_128 == NULL || nonce_96 == NULL || nonce_len != AMI_GCM_NONCE_BYTES ||
        tag_128 == NULL || tag_len != AMI_GCM_TAG_BYTES ||
        !bytes_ok(aad, aad_len) || !bytes_ok(ciphertext, ciphertext_len) ||
        (ciphertext_len != 0u && plaintext == NULL) ||
        aad_len > AMI_MAX_INPUT_BYTES || ciphertext_len > AMI_MAX_INPUT_BYTES) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    uint8_t rk[176];
    aes128_key_expansion(key_128, rk);

    uint8_t zero[16] = {0};
    uint8_t h[16];
    aes128_encrypt_block(rk, zero, h);

    uint8_t j0[16];
    memcpy(j0, nonce_96, 12u);
    j0[12] = 0u; j0[13] = 0u; j0[14] = 0u; j0[15] = 1u;

    uint8_t s[16];
    ghash(h, aad, aad_len, ciphertext, ciphertext_len, s);

    uint8_t enc_j0[16];
    aes128_encrypt_block(rk, j0, enc_j0);
    uint8_t expected_tag[16];
    for (int i = 0; i < 16; ++i) expected_tag[i] = s[i] ^ enc_j0[i];

    if (!constant_time_memcmp(expected_tag, tag_128, 16u)) {
        if (plaintext != NULL && ciphertext_len != 0u) {
            secure_zeroize(plaintext, ciphertext_len);
        }
        secure_zeroize(rk, sizeof(rk));
        secure_zeroize(h, sizeof(h));
        secure_zeroize(expected_tag, sizeof(expected_tag));
        secure_zeroize(j0, sizeof(j0));
        secure_zeroize(s, sizeof(s));
        secure_zeroize(enc_j0, sizeof(enc_j0));
        return AMI_SEC_AUTHENTICATION_FAILURE;
    }

    uint8_t ctr[16];
    memcpy(ctr, j0, 16u);
    size_t pos = 0u;
    while (pos < ciphertext_len) {
        inc32(ctr);
        uint8_t enc_ctr[16];
        aes128_encrypt_block(rk, ctr, enc_ctr);
        size_t chunk = (ciphertext_len - pos >= 16u) ? 16u : (ciphertext_len - pos);
        for (size_t i = 0u; i < chunk; ++i) {
            plaintext[pos + i] = ciphertext[pos + i] ^ enc_ctr[i];
        }
        pos += chunk;
    }

    secure_zeroize(rk, sizeof(rk));
    secure_zeroize(h, sizeof(h));
    secure_zeroize(j0, sizeof(j0));
    secure_zeroize(ctr, sizeof(ctr));
    secure_zeroize(s, sizeof(s));
    secure_zeroize(enc_j0, sizeof(enc_j0));
    secure_zeroize(expected_tag, sizeof(expected_tag));
    return AMI_SEC_OK;
}

AmiSecStatus ami_sec_aes_128_gmac(
    const uint8_t *key_128,
    const uint8_t *nonce_96, size_t nonce_len,
    const uint8_t *data, size_t data_len,
    uint8_t *tag_128, size_t tag_len)
{
    return ami_sec_aes_128_gcm_encrypt(
        key_128, nonce_96, nonce_len, data, data_len, NULL, 0u, NULL, tag_128, tag_len);
}

/* ========================================================================= */
/* DLMS HLS 5 GMAC Challenge-Response Implementation                         */
/* ========================================================================= */

AmiSecStatus ami_sec_dlms_hls5_gmac_calculate(
    const uint8_t *auth_key,
    const uint8_t *system_title,
    uint32_t invocation_counter,
    const uint8_t *challenge, size_t challenge_len,
    uint8_t *out_proof, size_t out_capacity, size_t *out_len)
{
    if (out_len != NULL) *out_len = 0u;
    if (auth_key == NULL || system_title == NULL || challenge == NULL ||
        challenge_len == 0u || challenge_len > 64u ||
        out_proof == NULL || out_len == NULL) {
        return AMI_SEC_INVALID_ARGUMENT;
    }
    if (out_capacity < AMI_GCM_TAG_BYTES) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    /* IV (12 bytes) = SystemTitle (8 bytes) || InvocationCounter (4 bytes Big-Endian) */
    uint8_t iv[AMI_GCM_NONCE_BYTES];
    memcpy(iv, system_title, 8u);
    iv[8]  = (uint8_t)(invocation_counter >> 24);
    iv[9]  = (uint8_t)(invocation_counter >> 16);
    iv[10] = (uint8_t)(invocation_counter >> 8);
    iv[11] = (uint8_t)(invocation_counter);

    /* AAD = SC (0x10) || AK (16 bytes) || Challenge */
    uint8_t aad[1u + AMI_AES_128_KEY_BYTES + 64u];
    aad[0] = AMI_DLMS_HLS5_SC_AUTH_ONLY;
    memcpy(aad + 1u, auth_key, AMI_AES_128_KEY_BYTES);
    memcpy(aad + 1u + AMI_AES_128_KEY_BYTES, challenge, challenge_len);
    size_t aad_len = 1u + AMI_AES_128_KEY_BYTES + challenge_len;

    uint8_t tag[AMI_GCM_TAG_BYTES];
    AmiSecStatus status = ami_sec_aes_128_gmac(
        auth_key, iv, sizeof(iv), aad, aad_len, tag, sizeof(tag));
    secure_zeroize(aad, sizeof(aad));
    if (status != AMI_SEC_OK) {
        secure_zeroize(tag, sizeof(tag));
        return status;
    }

    /* Form full 21-byte proof: SC (1B) || IC (4B) || Tag (16B) if capacity permits */
    if (out_capacity >= AMI_DLMS_HLS5_PROOF_LEN) {
        out_proof[0] = AMI_DLMS_HLS5_SC_AUTH_ONLY;
        out_proof[1] = iv[8];
        out_proof[2] = iv[9];
        out_proof[3] = iv[10];
        out_proof[4] = iv[11];
        memcpy(out_proof + 5u, tag, AMI_GCM_TAG_BYTES);
        *out_len = AMI_DLMS_HLS5_PROOF_LEN;
    } else {
        memcpy(out_proof, tag, AMI_GCM_TAG_BYTES);
        *out_len = AMI_GCM_TAG_BYTES;
    }
    secure_zeroize(tag, sizeof(tag));
    return AMI_SEC_OK;
}

AmiSecStatus ami_sec_dlms_hls5_gmac_verify(
    const uint8_t *auth_key,
    const uint8_t *system_title,
    const uint8_t *received_proof, size_t received_proof_len,
    const uint8_t *challenge, size_t challenge_len)
{
    if (auth_key == NULL || system_title == NULL || received_proof == NULL ||
        challenge == NULL || challenge_len == 0u || challenge_len > 64u) {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    uint32_t ic = 0u;
    const uint8_t *expected_tag = NULL;

    if (received_proof_len == AMI_DLMS_HLS5_PROOF_LEN) {
        if (received_proof[0] != AMI_DLMS_HLS5_SC_AUTH_ONLY) {
            return AMI_SEC_AUTHENTICATION_FAILURE;
        }
        ic = ((uint32_t)received_proof[1] << 24) |
             ((uint32_t)received_proof[2] << 16) |
             ((uint32_t)received_proof[3] << 8)  |
             ((uint32_t)received_proof[4]);
        expected_tag = received_proof + 5u;
    } else if (received_proof_len == AMI_GCM_TAG_BYTES) {
        ic = 0u;
        expected_tag = received_proof;
    } else {
        return AMI_SEC_INVALID_ARGUMENT;
    }

    uint8_t iv[AMI_GCM_NONCE_BYTES];
    memcpy(iv, system_title, 8u);
    iv[8]  = (uint8_t)(ic >> 24);
    iv[9]  = (uint8_t)(ic >> 16);
    iv[10] = (uint8_t)(ic >> 8);
    iv[11] = (uint8_t)(ic);

    uint8_t aad[1u + AMI_AES_128_KEY_BYTES + 64u];
    aad[0] = AMI_DLMS_HLS5_SC_AUTH_ONLY;
    memcpy(aad + 1u, auth_key, AMI_AES_128_KEY_BYTES);
    memcpy(aad + 1u + AMI_AES_128_KEY_BYTES, challenge, challenge_len);
    size_t aad_len = 1u + AMI_AES_128_KEY_BYTES + challenge_len;

    uint8_t tag_calc[AMI_GCM_TAG_BYTES];
    AmiSecStatus status = ami_sec_aes_128_gmac(
        auth_key, iv, sizeof(iv), aad, aad_len, tag_calc, sizeof(tag_calc));
    secure_zeroize(aad, sizeof(aad));
    if (status != AMI_SEC_OK) {
        secure_zeroize(tag_calc, sizeof(tag_calc));
        return status;
    }

    int ok = constant_time_memcmp(expected_tag, tag_calc, AMI_GCM_TAG_BYTES);
    secure_zeroize(tag_calc, sizeof(tag_calc));
    return ok ? AMI_SEC_OK : AMI_SEC_AUTHENTICATION_FAILURE;
}

int ami_sec_hls5_gmac_proof_callback(
    void *user,
    const uint8_t *mechanism_oid, size_t mechanism_oid_length,
    const uint8_t *challenge, size_t challenge_length,
    uint8_t *proof, size_t proof_capacity, size_t *proof_length)
{
    if (user == NULL || mechanism_oid == NULL || challenge == NULL ||
        proof == NULL || proof_length == NULL) {
        return 0;
    }
    if (mechanism_oid_length != sizeof(dlms_hls5_gmac_oid) ||
        memcmp(mechanism_oid, dlms_hls5_gmac_oid, mechanism_oid_length) != 0) {
        return 0;
    }

    AmiHls5GmacContext *ctx = (AmiHls5GmacContext *)user;
    AmiSecStatus status = ami_sec_dlms_hls5_gmac_calculate(
        ctx->auth_key, ctx->system_title, ctx->invocation_counter,
        challenge, challenge_length, proof, proof_capacity, proof_length);

    return (status == AMI_SEC_OK) ? 1 : 0;
}

/* ========================================================================= */
/* Software Backend Adapter & Existing Facade Implementation                */
/* ========================================================================= */

static int sw_random(void *user, uint8_t *out, size_t out_len)
{
    (void)user;
    /* Basic pseudo-random PRNG for software/test fallback when hardware TRNG is absent */
    static uint32_t seed = 0x12345678u;
    for (size_t i = 0u; i < out_len; ++i) {
        seed = seed * 1664525u + 1013904223u;
        out[i] = (uint8_t)(seed >> 24);
    }
    return 1;
}

static AmiSecStatus sw_aead_encrypt(void *user, AmiAeadAlgorithm algorithm,
                                   AmiKeyHandle key, const uint8_t *nonce,
                                   size_t nonce_len, const uint8_t *aad,
                                   size_t aad_len, const uint8_t *plaintext,
                                   size_t plaintext_len, uint8_t *ciphertext,
                                   uint8_t *tag, size_t tag_len)
{
    (void)key;
    if (algorithm != AMI_AEAD_AES_128_GCM || user == NULL) {
        return AMI_SEC_UNSUPPORTED;
    }
    const uint8_t *key_bytes = (const uint8_t *)user;
    return ami_sec_aes_128_gcm_encrypt(
        key_bytes, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len,
        ciphertext, tag, tag_len);
}

static AmiSecStatus sw_aead_decrypt(void *user, AmiAeadAlgorithm algorithm,
                                   AmiKeyHandle key, const uint8_t *nonce,
                                   size_t nonce_len, const uint8_t *aad,
                                   size_t aad_len, const uint8_t *ciphertext,
                                   size_t ciphertext_len, const uint8_t *tag,
                                   size_t tag_len, uint8_t *plaintext)
{
    (void)key;
    if (algorithm != AMI_AEAD_AES_128_GCM || user == NULL) {
        return AMI_SEC_UNSUPPORTED;
    }
    const uint8_t *key_bytes = (const uint8_t *)user;
    return ami_sec_aes_128_gcm_decrypt(
        key_bytes, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len,
        tag, tag_len, plaintext);
}

AmiSecStatus ami_sec_init_software_backend(AmiSecBackend *backend, const uint8_t *key_128)
{
    if (backend == NULL || key_128 == NULL) return AMI_SEC_INVALID_ARGUMENT;
    backend->user = (void *)key_128;
    backend->random = sw_random;
    backend->aead_encrypt = sw_aead_encrypt;
    backend->aead_decrypt = sw_aead_decrypt;
    return AMI_SEC_OK;
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
