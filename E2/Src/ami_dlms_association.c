#include "ami_dlms_association.h"

#include <string.h>

#define AARQ_TAG 0x60u
#define AARE_TAG 0x61u
#define BER_OID_TAG 0x06u
#define BER_APP_CONTEXT_TAG 0xA1u
#define BER_CALLING_AUTH_VALUE_TAG 0xACu
#define BER_RESPONDING_AUTH_VALUE_TAG 0xAAu

static const uint8_t logical_name_context_oid[] =
    {0x60u, 0x85u, 0x74u, 0x05u, 0x08u, 0x01u, 0x01u};
static const uint8_t lls_mechanism_oid[] =
    {0x60u, 0x85u, 0x74u, 0x05u, 0x08u, 0x02u, 0x01u};

typedef struct {
    const uint8_t *value;
    size_t length;
    size_t encoded_length;
} BerTlv;

typedef struct {
    int context_seen;
    int acse_auth_seen;
    const uint8_t *mechanism_oid;
    size_t mechanism_oid_length;
    const uint8_t *auth_value;
    size_t auth_value_length;
    uint16_t client_max_pdu_size;
    int qos_present;
    uint8_t proposed_qos;
    int dedicated_key_present;
    int conformance_present;
    uint8_t proposed_conformance[3];
} ParsedAarq;

static int read_ber_length(const uint8_t *bytes, size_t available,
                           size_t *length, size_t *length_octets)
{
    if (bytes == NULL || length == NULL || length_octets == NULL || available == 0u) return 0;
    uint8_t first = bytes[0];
    if ((first & 0x80u) == 0u) {
        *length = first;
        *length_octets = 1u;
        return 1;
    }
    size_t count = first & 0x7fu;
    if (count == 0u || count > sizeof(size_t) || count > available - 1u || bytes[1] == 0u) return 0;
    size_t value = 0u;
    for (size_t i = 0u; i < count; ++i) {
        if (value > (SIZE_MAX >> 8)) return 0;
        value = (value << 8) | bytes[1u + i];
    }
    if (value < 128u) return 0;
    *length = value;
    *length_octets = count + 1u;
    return 1;
}

static int read_tlv(const uint8_t *bytes, size_t available, BerTlv *tlv)
{
    size_t length, length_octets;
    if (bytes == NULL || tlv == NULL || available < 2u ||
        !read_ber_length(bytes + 1u, available - 1u, &length, &length_octets) ||
        length > available - 1u - length_octets) return 0;
    tlv->value = bytes + 1u + length_octets;
    tlv->length = length;
    tlv->encoded_length = 1u + length_octets + length;
    return 1;
}

static int parse_auth_value(const uint8_t *bytes, size_t length,
                            const uint8_t **value, size_t *value_length)
{
    BerTlv inner;
    if (!read_tlv(bytes, length, &inner) || inner.encoded_length != length ||
        bytes[0] != 0x80u || inner.length == 0u ||
        inner.length > AMI_DLMS_AUTH_VALUE_MAX) return 0;
    *value = inner.value;
    *value_length = inner.length;
    return 1;
}

static int parse_aarq(const uint8_t *aarq, size_t length, ParsedAarq *parsed)
{
    BerTlv outer;
    if (aarq == NULL || parsed == NULL || length < 2u || aarq[0] != AARQ_TAG ||
        !read_tlv(aarq, length, &outer) || outer.encoded_length != length) return 0;
    memset(parsed, 0, sizeof(*parsed));
    int mechanism_seen = 0, auth_value_seen = 0;
    size_t offset = 0u;
    while (offset < outer.length) {
        BerTlv field;
        if (!read_tlv(outer.value + offset, outer.length - offset, &field)) return 0;
        const uint8_t tag = outer.value[offset];
        if (tag == BER_APP_CONTEXT_TAG) {
            BerTlv oid;
            if (parsed->context_seen || !read_tlv(field.value, field.length, &oid) ||
                oid.encoded_length != field.length || field.value[0] != BER_OID_TAG ||
                oid.length != sizeof(logical_name_context_oid) ||
                memcmp(oid.value, logical_name_context_oid, oid.length) != 0) return 0;
            parsed->context_seen = 1;
        } else if (tag == 0x8Au) {
            if (parsed->acse_auth_seen || field.length != 2u ||
                field.value[0] != 0x07u || field.value[1] != 0x80u) return 0;
            parsed->acse_auth_seen = 1;
        } else if (tag == 0x8Bu) {
            if (mechanism_seen || field.length == 0u || field.length > AMI_DLMS_OID_MAX) return 0;
            mechanism_seen = 1;
            parsed->mechanism_oid = field.value;
            parsed->mechanism_oid_length = field.length;
        } else if (tag == BER_CALLING_AUTH_VALUE_TAG) {
            if (auth_value_seen || !parse_auth_value(field.value, field.length,
                    &parsed->auth_value, &parsed->auth_value_length)) return 0;
            auth_value_seen = 1;
        } else if (tag == 0xBEu) {
            BerTlv user_information;
            size_t available_ui = outer.length - offset > 2u ? outer.length - offset - 2u : field.length;
            const uint8_t *ui_bytes = outer.value + offset + 2u;
            if (available_ui < 2u || ui_bytes[0] != 0x04u ||
                !read_tlv(ui_bytes, available_ui, &user_information) ||
                user_information.length == 0u) return 0;
            field.encoded_length = 2u + user_information.encoded_length;
            /* Parse InitiateRequest from user_information if available.
             * InitiateRequest ::= [1] IMPLICIT SEQUENCE {
             *   dedicated_key [OPTIONAL], response_allowed [OPTIONAL/BOOLEAN],
             *   proposed_quality_of_service [OPTIONAL], proposed_dlms_version,
             *   proposed_conformance, client_max_receive_pdu_size }
             */
            const uint8_t *pdu = user_information.value;
            size_t pdu_len = user_information.length;
            if (pdu_len >= 8u && pdu[0] == 0x01u) {
                size_t ppos = 1u;
                if (ppos < pdu_len && pdu[ppos] == 0x00u) ppos++; /* dedicated_key absent */
                else if (ppos < pdu_len && pdu[ppos] == 0x01u) {
                    ppos++;
                    parsed->dedicated_key_present = 1;
                    if (ppos < pdu_len) { size_t klen = pdu[ppos++]; ppos += klen; }
                }
                if (ppos < pdu_len && pdu[ppos] == 0x00u) ppos++; /* response_allowed absent */
                else if (ppos + 1u < pdu_len && pdu[ppos] == 0x01u) ppos += 2u; /* response_allowed present (flag + bool) */

                if (ppos < pdu_len && pdu[ppos] == 0x00u) {
                    ppos++; /* qos absent */
                    parsed->qos_present = 0;
                } else if (ppos + 1u < pdu_len && pdu[ppos] == 0x01u) {
                    parsed->qos_present = 1;
                    parsed->proposed_qos = pdu[ppos + 1u];
                    ppos += 2u; /* qos present (flag + int8) */
                }
                if (ppos < pdu_len) ppos++; /* dlms_version */
                if (ppos + 2u < pdu_len && pdu[ppos] == 0x5Fu && pdu[ppos + 1u] == 0x1Fu) {
                    ppos += 2u;
                    if (ppos < pdu_len) {
                        size_t clen = pdu[ppos++];
                        if (clen >= 1u && ppos + clen <= pdu_len) {
                            parsed->conformance_present = 1;
                            parsed->proposed_conformance[0] = (clen > 1u) ? pdu[ppos + 1u] : 0u;
                            parsed->proposed_conformance[1] = (clen > 2u) ? pdu[ppos + 2u] : 0u;
                            parsed->proposed_conformance[2] = (clen > 3u) ? pdu[ppos + 3u] : 0u;
                        }
                        ppos += clen;
                    }
                }
                if (ppos + 1u < pdu_len) {
                    parsed->client_max_pdu_size = (uint16_t)(((uint16_t)pdu[ppos] << 8) | pdu[ppos + 1u]);
                }
            }
        } else if (tag != 0x80u && tag != 0xA2u && tag != 0xA3u &&
                   tag != 0xA4u && tag != 0xA5u && tag != 0xBDu) {
            /* Unknown optional ACSE tags are tolerated in standard BER decoding */
        }
        offset += field.encoded_length;
    }
    return parsed->context_seen &&
        (parsed->acse_auth_seen == mechanism_seen) &&
        (mechanism_seen == auth_value_seen);
}

static int build_aare(uint8_t *out, size_t capacity, size_t *out_length,
                      uint8_t result, uint8_t diagnostic,
                      const uint8_t *mechanism_oid, size_t mechanism_oid_length,
                      const uint8_t *server_challenge, size_t challenge_length,
                      uint16_t client_max_pdu_size,
                      int qos_present, uint8_t proposed_qos,
                      int conformance_present, const uint8_t *proposed_conformance)
{
    static const uint8_t context_field[] = {
        BER_APP_CONTEXT_TAG, 0x09u, BER_OID_TAG, 0x07u,
        0x60u, 0x85u, 0x74u, 0x05u, 0x08u, 0x01u, 0x01u
    };
    const uint8_t result_field[] = {0xA2u, 0x03u, 0x02u, 0x01u, result};
    const uint8_t diagnostic_field[] = {0xA3u, 0x05u, 0xA1u, 0x03u, 0x02u, 0x01u, diagnostic};
    /* xDLMS InitiateResponse: negotiate server max PDU size (respect client proposal). */
    uint16_t negotiated_pdu = (client_max_pdu_size > 0u && client_max_pdu_size < 500u)
        ? client_max_pdu_size : 500u;
    /* Conformance negotiation: server supported mask = 0x00, 0x50, 0x1F.
     * Negotiated conformance is bitwise AND of proposed and supported. */
    uint8_t conf0 = 0x00u;
    uint8_t conf1 = 0x00u;
    uint8_t conf2 = 0x14u; /* Get (0x10) + Selective-access (0x04) only */
    if (conformance_present && proposed_conformance != NULL) {
        conf0 = (uint8_t)(proposed_conformance[0] & 0x00u);
        conf1 = (uint8_t)(proposed_conformance[1] & 0x00u);
        conf2 = (uint8_t)(proposed_conformance[2] & 0x14u);
    }
    uint8_t initiate_response_field[24];
    size_t init_len = 0u;
    if (qos_present) {
        initiate_response_field[init_len++] = 0xBEu;
        initiate_response_field[init_len++] = 0x11u; /* 17 bytes */
        initiate_response_field[init_len++] = 0x04u;
        initiate_response_field[init_len++] = 0x0Fu; /* 15 bytes */
        initiate_response_field[init_len++] = 0x08u;
        initiate_response_field[init_len++] = 0x01u; /* qos present */
        initiate_response_field[init_len++] = proposed_qos;
    } else {
        initiate_response_field[init_len++] = 0xBEu;
        initiate_response_field[init_len++] = 0x10u; /* 16 bytes */
        initiate_response_field[init_len++] = 0x04u;
        initiate_response_field[init_len++] = 0x0Eu; /* 14 bytes */
        initiate_response_field[init_len++] = 0x08u;
        initiate_response_field[init_len++] = 0x00u; /* qos absent */
    }
    initiate_response_field[init_len++] = 0x06u; /* dlms version 6 */
    initiate_response_field[init_len++] = 0x5Fu;
    initiate_response_field[init_len++] = 0x1Fu;
    initiate_response_field[init_len++] = 0x04u;
    initiate_response_field[init_len++] = 0x00u;
    initiate_response_field[init_len++] = conf0;
    initiate_response_field[init_len++] = conf1;
    initiate_response_field[init_len++] = conf2;
    initiate_response_field[init_len++] = (uint8_t)(negotiated_pdu >> 8);
    initiate_response_field[init_len++] = (uint8_t)negotiated_pdu;
    initiate_response_field[init_len++] = 0x00u; /* vaa name */
    initiate_response_field[init_len++] = 0x07u;

    const size_t auth_extra = challenge_length == 0u ? 0u : 4u + challenge_length;
    const size_t mechanism_extra = challenge_length == 0u ? 0u : 2u + mechanism_oid_length;
    const size_t acse_extra = challenge_length == 0u ? 0u : 4u;
    const size_t user_info_extra = result == 0u ? init_len : 0u;
    const size_t body_length = sizeof(context_field) + sizeof(result_field) +
        sizeof(diagnostic_field) + auth_extra + mechanism_extra + acse_extra + user_info_extra;
    if (out == NULL || out_length == NULL || body_length > 127u ||
        capacity < body_length + 2u ||
        (challenge_length != 0u && (server_challenge == NULL || mechanism_oid == NULL ||
          mechanism_oid_length == 0u || mechanism_oid_length > AMI_DLMS_OID_MAX))) return 0;
    size_t n = 0u;
    out[n++] = AARE_TAG; out[n++] = (uint8_t)body_length;
    memcpy(out + n, context_field, sizeof(context_field)); n += sizeof(context_field);
    memcpy(out + n, result_field, sizeof(result_field)); n += sizeof(result_field);
    memcpy(out + n, diagnostic_field, sizeof(diagnostic_field)); n += sizeof(diagnostic_field);
    if (challenge_length != 0u) {
        out[n++] = 0x88u; out[n++] = 0x02u; out[n++] = 0x07u; out[n++] = 0x80u;
        out[n++] = 0x89u; out[n++] = (uint8_t)mechanism_oid_length;
        memcpy(out + n, mechanism_oid, mechanism_oid_length); n += mechanism_oid_length;
        out[n++] = BER_RESPONDING_AUTH_VALUE_TAG;
        out[n++] = (uint8_t)(challenge_length + 2u);
        out[n++] = 0x80u; out[n++] = (uint8_t)challenge_length;
        memcpy(out + n, server_challenge, challenge_length); n += challenge_length;
    }
    if (result == 0u) {
        memcpy(out + n, initiate_response_field, init_len);
        n += init_len;
    }
    *out_length = n;
    return 1;
}

static int oid_equal(const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len)
{
    return a != NULL && b != NULL && a_len == b_len && memcmp(a, b, a_len) == 0;
}

static int constant_time_equal(const uint8_t *a, const uint8_t *b, size_t length)
{
    uint8_t diff = 0u;
    for (size_t i = 0u; i < length; ++i) diff |= (uint8_t)(a[i] ^ b[i]);
    return diff == 0u;
}

static void clear_challenges(AmiDlmsAssociation *association)
{
    memset(association->client_challenge, 0, sizeof(association->client_challenge));
    memset(association->server_challenge, 0, sizeof(association->server_challenge));
    association->client_challenge_length = 0u;
    association->server_challenge_length = 0u;
    memset(association->hls_mechanism_oid, 0, sizeof(association->hls_mechanism_oid));
    association->hls_mechanism_oid_length = 0u;
}

void ami_dlms_association_reset(AmiDlmsAssociation *association)
{
    if (association != NULL) {
        clear_challenges(association);
        association->mechanism = AMI_DLMS_AUTH_NONE;
        association->state = AMI_DLMS_ASSOC_IDLE;
    }
}

AmiDlmsAssociationStatus ami_dlms_process_aarq_with_security(
    AmiDlmsAssociation *association, const AmiDlmsAssociationSecurity *security,
    const uint8_t *aarq, size_t aarq_length, uint8_t *aare,
    size_t aare_capacity, size_t *aare_length)
{
    if (association == NULL || security == NULL || aarq == NULL || aare == NULL || aare_length == NULL)
        return AMI_DLMS_ASSOC_INVALID_ARGUMENT;
    *aare_length = 0u;
    if (association->state != AMI_DLMS_ASSOC_IDLE) {
        /* An unsolicited AARQ must not tear down an authenticated session. */
        return AMI_DLMS_ASSOC_REJECTED;
    }
    ParsedAarq parsed;
    int parsed_ok = parse_aarq(aarq, aarq_length, &parsed);
    AmiDlmsAuthMechanism mechanism = AMI_DLMS_AUTH_NONE;
    int accepted = 0;
    uint8_t server_challenge[AMI_DLMS_HLS_CHALLENGE_LENGTH] = {0};
    size_t server_challenge_length = 0u;
    if (parsed_ok && parsed.conformance_present &&
        (((parsed.proposed_conformance[0] & 0x00u) |
          (parsed.proposed_conformance[1] & 0x00u) |
          (parsed.proposed_conformance[2] & 0x14u)) == 0u)) {
        /* Empty/incompatible conformance: no common services, reject association */
        accepted = 0;
    } else if (parsed_ok && parsed.client_max_pdu_size != 0u && parsed.client_max_pdu_size < 12u) {
        /* T_APPL_OPEN_14: client max receive PDU size below the 12-byte minimum is rejected */
        accepted = 0;
    } else if (parsed_ok && parsed.dedicated_key_present) {
        /* DLMS CTT T_APPL_OPEN_9: dedicated key when ciphering is not used must be rejected */
        accepted = 0;
    } else if (parsed_ok && parsed.mechanism_oid == NULL && security->allow_no_auth != 0u) {
        accepted = 1;
    } else if (parsed_ok && oid_equal(parsed.mechanism_oid, parsed.mechanism_oid_length,
                                      lls_mechanism_oid, sizeof(lls_mechanism_oid)) &&
               security->verify_lls != NULL) {
        mechanism = AMI_DLMS_AUTH_LLS;
        accepted = security->verify_lls(security->user, parsed.auth_value,
                                        parsed.auth_value_length) == 1;
    } else if (parsed_ok && security->hls_mechanism_oid != NULL &&
               security->hls_mechanism_oid_length > 0u &&
               security->hls_mechanism_oid_length <= AMI_DLMS_OID_MAX &&
               security->random != NULL && security->hls_proof != NULL &&
               oid_equal(parsed.mechanism_oid, parsed.mechanism_oid_length,
                         security->hls_mechanism_oid, security->hls_mechanism_oid_length)) {
        mechanism = AMI_DLMS_AUTH_HLS;
        server_challenge_length = sizeof(server_challenge);
        accepted = security->random(security->user, server_challenge,
                                    server_challenge_length) == 1;
        if (accepted) {
            memcpy(association->client_challenge, parsed.auth_value, parsed.auth_value_length);
            association->client_challenge_length = parsed.auth_value_length;
            memcpy(association->server_challenge, server_challenge, server_challenge_length);
            association->server_challenge_length = server_challenge_length;
            memcpy(association->hls_mechanism_oid, security->hls_mechanism_oid,
                   security->hls_mechanism_oid_length);
            association->hls_mechanism_oid_length = security->hls_mechanism_oid_length;
        }
    }
    const int hls_started = accepted && mechanism == AMI_DLMS_AUTH_HLS;
    const uint8_t result = (uint8_t)(accepted ? 0u : 1u);
    const uint8_t diagnostic = (uint8_t)(accepted ? 0u : 2u);
    if (!build_aare(aare, aare_capacity, aare_length, result, diagnostic,
                    hls_started ? association->hls_mechanism_oid : NULL,
                    hls_started ? association->hls_mechanism_oid_length : 0u,
                    hls_started ? association->server_challenge : NULL,
                    hls_started ? association->server_challenge_length : 0u,
                    parsed.client_max_pdu_size,
                    parsed.qos_present, parsed.proposed_qos,
                    parsed.conformance_present, parsed.proposed_conformance)) {
        clear_challenges(association);
        association->state = AMI_DLMS_ASSOC_ERROR;
        return AMI_DLMS_ASSOC_BUFFER_TOO_SMALL;
    }
    if (accepted) {
        association->mechanism = mechanism;
        association->state = hls_started ? AMI_DLMS_ASSOC_AUTH_PENDING : AMI_DLMS_ASSOC_ASSOCIATED;
        return AMI_DLMS_ASSOC_OK;
    }
    clear_challenges(association);
    association->state = AMI_DLMS_ASSOC_ERROR;
    return parsed_ok ? AMI_DLMS_ASSOC_REJECTED : AMI_DLMS_ASSOC_REJECTED;
}

AmiDlmsAssociationStatus ami_dlms_process_hls_reply(
    AmiDlmsAssociation *association, const AmiDlmsAssociationSecurity *security,
    const uint8_t *client_proof, size_t client_proof_length,
    uint8_t *server_proof, size_t server_proof_capacity, size_t *server_proof_length)
{
    if (server_proof_length != NULL) *server_proof_length = 0u;
    if (association == NULL || security == NULL || client_proof == NULL ||
        server_proof == NULL || server_proof_length == NULL) return AMI_DLMS_ASSOC_INVALID_ARGUMENT;
    if (association->state != AMI_DLMS_ASSOC_AUTH_PENDING ||
        association->mechanism != AMI_DLMS_AUTH_HLS || security->hls_proof == NULL)
        return AMI_DLMS_ASSOC_REJECTED;
    uint8_t expected[AMI_DLMS_AUTH_VALUE_MAX] = {0};
    size_t expected_length = 0u;
    int ok = security->hls_proof(security->user, association->hls_mechanism_oid,
        association->hls_mechanism_oid_length, association->server_challenge,
        association->server_challenge_length, expected, sizeof(expected), &expected_length) == 1;
    if (!ok || expected_length == 0u || expected_length > sizeof(expected) ||
        expected_length != client_proof_length ||
        !constant_time_equal(expected, client_proof, expected_length)) {
        memset(expected, 0, sizeof(expected));
        clear_challenges(association);
        association->state = AMI_DLMS_ASSOC_ERROR;
        return AMI_DLMS_ASSOC_REJECTED;
    }
    uint8_t response[AMI_DLMS_AUTH_VALUE_MAX] = {0};
    size_t response_length = 0u;
    ok = security->hls_proof(security->user, association->hls_mechanism_oid,
        association->hls_mechanism_oid_length, association->client_challenge,
        association->client_challenge_length, response, sizeof(response),
        &response_length) == 1;
    memset(expected, 0, sizeof(expected));
    if (!ok || response_length == 0u || response_length > sizeof(response)) {
        memset(response, 0, sizeof(response));
        clear_challenges(association);
        association->state = AMI_DLMS_ASSOC_ERROR;
        return AMI_DLMS_ASSOC_BACKEND_FAILURE;
    }
    if (response_length > server_proof_capacity) {
        memset(response, 0, sizeof(response));
        return AMI_DLMS_ASSOC_BUFFER_TOO_SMALL;
    }
    memcpy(server_proof, response, response_length);
    memset(response, 0, sizeof(response));
    *server_proof_length = response_length;
    clear_challenges(association);
    association->state = AMI_DLMS_ASSOC_ASSOCIATED;
    return AMI_DLMS_ASSOC_OK;
}

AmiDlmsAssociationStatus ami_dlms_process_aarq(
    AmiDlmsAssociation *association, const uint8_t *aarq, size_t aarq_length,
    uint8_t *aare, size_t aare_capacity, size_t *aare_length)
{
    const AmiDlmsAssociationSecurity no_auth = {.allow_no_auth = 1u};
    return ami_dlms_process_aarq_with_security(association, &no_auth, aarq,
        aarq_length, aare, aare_capacity, aare_length);
}
