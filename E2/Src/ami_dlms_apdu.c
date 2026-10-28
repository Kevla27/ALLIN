#include "ami_dlms_apdu.h"

#include <limits.h>
#include <string.h>

/* Volatile stores prevent the compiler from removing sensitive-data wipes. */
static void secure_zeroize(void *memory, size_t length)
{
    volatile uint8_t *bytes = (volatile uint8_t *)memory;
    while (length-- != 0u) *bytes++ = 0u;
}

#define GET_REQUEST_TAG 0xC0u
#define SET_REQUEST_TAG 0xC1u
#define GET_RESPONSE_TAG 0xC4u
#define SET_RESPONSE_TAG 0xC5u
#define ACTION_REQUEST_TAG 0xC3u
#define ACTION_RESPONSE_TAG 0xC7u
#define GLO_GET_REQUEST_TAG 0xC8u
#define GLO_SET_REQUEST_TAG 0xC9u
#define GLO_ACTION_REQUEST_TAG 0xCBu
#define GLO_GET_RESPONSE_TAG 0xCCu
#define GLO_SET_RESPONSE_TAG 0xCDu
#define GLO_ACTION_RESPONSE_TAG 0xCEu
#define GET_NORMAL_CHOICE 0x01u
#define SECURITY_HEADER_LENGTH 5u /* security-control + invocation-counter */
#define GLO_HEADER_LENGTH 6u /* tag + security header */

static int encode_data(const AmiCosemData *data, uint8_t *out,
                       size_t capacity, size_t *written)
{
    size_t offset = 0u;
    if (data == NULL || out == NULL || written == NULL) {
        return 0;
    }

    switch (data->type) {
    case AMI_COSEM_TYPE_ENCODED:
        if (data->length > capacity || data->length > sizeof(data->value.encoded)) {
            return 0;
        }
        memcpy(out, data->value.encoded, data->length);
        offset = data->length;
        break;
    case AMI_COSEM_TYPE_OCTET_STRING:
        if (data->length > UINT8_MAX || data->length > capacity ||
            capacity - data->length < 2u) {
            return 0;
        }
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_OCTET_STRING;
        out[offset++] = (uint8_t)data->length;
        if (data->length == AMI_COSEM_LOGICAL_NAME_LENGTH) {
            memcpy(out + offset, data->value.logical_name, data->length);
        } else if (data->length == 8u) {
            memcpy(out + offset, data->value.octets, data->length);
        } else if (data->length == sizeof(AmiCosemDateTime)) {
            memcpy(out + offset, &data->value.date_time, data->length);
        } else {
            return 0;
        }
        offset += data->length;
        break;
    case AMI_COSEM_TYPE_VISIBLE_STRING:
        if (data->length > UINT8_MAX || data->length > capacity ||
            capacity - data->length < 2u) {
            return 0;
        }
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_VISIBLE_STRING;
        out[offset++] = (uint8_t)data->length;
        memcpy(out + offset, data->value.octets, data->length);
        offset += data->length;
        break;
    case AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED:
        if (capacity < 5u) return 0;
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED;
        out[offset++] = (uint8_t)(data->value.double_long_unsigned >> 24);
        out[offset++] = (uint8_t)(data->value.double_long_unsigned >> 16);
        out[offset++] = (uint8_t)(data->value.double_long_unsigned >> 8);
        out[offset++] = (uint8_t)data->value.double_long_unsigned;
        break;
    case AMI_COSEM_TYPE_LONG64_UNSIGNED:
        if (capacity < 9u) return 0;
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_LONG64_UNSIGNED;
        for (int shift = 56; shift >= 0; shift -= 8) {
            out[offset++] = (uint8_t)(data->value.long64_unsigned >> shift);
        }
        break;
    case AMI_COSEM_TYPE_DOUBLE_LONG:
        if (capacity < 5u) return 0;
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_DOUBLE_LONG;
        out[offset++] = (uint8_t)((uint32_t)data->value.double_long >> 24);
        out[offset++] = (uint8_t)((uint32_t)data->value.double_long >> 16);
        out[offset++] = (uint8_t)((uint32_t)data->value.double_long >> 8);
        out[offset++] = (uint8_t)data->value.double_long;
        break;
    case AMI_COSEM_TYPE_UNSIGNED:
    case AMI_COSEM_TYPE_ENUM:
        if (capacity < 2u) return 0;
        out[offset++] = (uint8_t)data->type;
        out[offset++] = data->value.unsigned_integer;
        break;
    case AMI_COSEM_TYPE_STRUCTURE:
        if (capacity < 6u) return 0;
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_STRUCTURE;
        out[offset++] = 2u;
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_INTEGER;
        out[offset++] = (uint8_t)data->value.scaler_unit.scaler;
        out[offset++] = (uint8_t)AMI_COSEM_TYPE_UNSIGNED;
        out[offset++] = data->value.scaler_unit.unit;
        break;
    default:
        return 0;
    }
    *written = offset;
    return 1;
}

static uint8_t access_error(AmiCosemStatus status)
{
    switch (status) {
    case AMI_COSEM_OBJECT_NOT_FOUND: return 4u; /* object-undefined */
    case AMI_COSEM_ATTRIBUTE_NOT_FOUND: return 3u; /* read-write-denied */
    case AMI_COSEM_VALUE_UNAVAILABLE: return 2u; /* temporary-failure */
    case AMI_COSEM_SCOPE_VIOLATED: return 11u; /* scope-of-access-violated */
    default: return 3u; /* read-write-denied */
    }
}

static int read_be_u16(const uint8_t *input, size_t length, size_t *offset,
                       uint16_t *value)
{
    if (*offset > length || length - *offset < 2u) return 0;
    *value = (uint16_t)(((uint16_t)input[*offset] << 8) | input[*offset + 1u]);
    *offset += 2u;
    return 1;
}

static int read_be_u32(const uint8_t *input, size_t length, size_t *offset,
                       uint32_t *value)
{
    if (*offset > length || length - *offset < 4u) return 0;
    *value = ((uint32_t)input[*offset] << 24) |
             ((uint32_t)input[*offset + 1u] << 16) |
             ((uint32_t)input[*offset + 2u] << 8) | input[*offset + 3u];
    *offset += 4u;
    return 1;
}

static int compare_profile_datetime(const AmiCosemDateTime *left,
                                    const AmiCosemDateTime *right)
{
    const uint8_t left_fields[] = {
        left->year[0], left->year[1], left->month, left->day,
        left->hour, left->minute, left->second, left->hundredths
    };
    const uint8_t right_fields[] = {
        right->year[0], right->year[1], right->month, right->day,
        right->hour, right->minute, right->second, right->hundredths
    };
    /* Handle DLMS wildcards: 0xFF in fields indicates unspecified/wildcard */
    for (size_t i = 0u; i < sizeof(left_fields); ++i) {
        if (i < 2u && ((left->year[0] == 0xFFu && left->year[1] == 0xFFu) ||
                       (right->year[0] == 0xFFu && right->year[1] == 0xFFu))) {
            i = 1u;
            continue;
        }
        if (left_fields[i] == 0xFFu || right_fields[i] == 0xFFu) continue;
        if (left_fields[i] < right_fields[i]) return -1;
        if (left_fields[i] > right_fields[i]) return 1;
    }
    return 0;
}
/* Supports selector 1 (Clock range) and selector 2 (entry range, all columns).
 * Unknown descriptors are rejected rather than returning the whole buffer. */
static int parse_profile_selection(const uint8_t *request, size_t request_length,
                                   size_t offset, AmiCosemProfileSelection *selection)
{
    if (offset >= request_length || selection == NULL) return 0;
    memset(selection, 0, sizeof(*selection));
    selection->selector = request[offset++];
    if (offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_STRUCTURE ||
        request[offset++] != 4u) return 0;

    if (selection->selector == AMI_COSEM_PROFILE_SELECTOR_ENTRY) {
        uint32_t from_entry, to_entry;
        uint16_t from_column, to_column;
        if (offset >= request_length || request[offset++] != AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED ||
            !read_be_u32(request, request_length, &offset, &from_entry) ||
            offset >= request_length || request[offset++] != AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED ||
            !read_be_u32(request, request_length, &offset, &to_entry) ||
            offset >= request_length || request[offset++] != AMI_COSEM_TYPE_LONG_UNSIGNED ||
            !read_be_u16(request, request_length, &offset, &from_column) ||
            offset >= request_length || request[offset++] != AMI_COSEM_TYPE_LONG_UNSIGNED ||
            !read_be_u16(request, request_length, &offset, &to_column) ||
            offset != request_length || from_column > 1u || to_column > 1u) return 0;
        /* Column projection is not supported: full rows are always returned
         * (from_column 0/1 with to_column 0 means all columns per Green Book). */
        selection->from_entry = from_entry;
        selection->to_entry = to_entry;
        return 1;
    }

    if (selection->selector == AMI_COSEM_PROFILE_SELECTOR_RANGE) {
        uint16_t restricting_class, data_index;
        uint8_t restricting_ln[AMI_COSEM_LOGICAL_NAME_LENGTH];
        int8_t restricting_attribute;
        static const uint8_t clock_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {0, 0, 1, 0, 0, 255};
        if (offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_STRUCTURE ||
            request[offset++] != 4u || offset >= request_length ||
            request[offset++] != AMI_COSEM_TYPE_LONG_UNSIGNED ||
            !read_be_u16(request, request_length, &offset, &restricting_class) ||
            offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_OCTET_STRING ||
            request[offset++] != AMI_COSEM_LOGICAL_NAME_LENGTH ||
            request_length - offset < AMI_COSEM_LOGICAL_NAME_LENGTH) return 0;
        memcpy(restricting_ln, request + offset, sizeof(restricting_ln));
        offset += sizeof(restricting_ln);
        if (offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_INTEGER) return 0;
        restricting_attribute = (int8_t)request[offset++];
        if (offset >= request_length || request[offset++] != AMI_COSEM_TYPE_LONG_UNSIGNED ||
            !read_be_u16(request, request_length, &offset, &data_index) ||
            restricting_class != AMI_COSEM_CLOCK_CLASS_ID ||
            memcmp(restricting_ln, clock_ln, sizeof(clock_ln)) != 0 ||
            restricting_attribute != 2 || data_index != 0u) return 0;

        if (offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_OCTET_STRING ||
            request[offset++] != sizeof(AmiCosemDateTime) ||
            request_length - offset < sizeof(AmiCosemDateTime)) return 0;
        memcpy(&selection->from_date, request + offset, sizeof(selection->from_date));
        offset += sizeof(selection->from_date);
        if (offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_OCTET_STRING ||
            request[offset++] != sizeof(AmiCosemDateTime) ||
            request_length - offset < sizeof(AmiCosemDateTime)) return 0;
        memcpy(&selection->to_date, request + offset, sizeof(selection->to_date));
        offset += sizeof(selection->to_date);
        /* selected_values must be an empty array; column projection isn't supported. */
        if (offset + 2u != request_length || request[offset++] != 1u || request[offset++] != 0u ||
            compare_profile_datetime(&selection->from_date, &selection->to_date) > 0) return 0;
        return 1;
    }
    return 0;
}

static size_t build_action_result(uint8_t invoke_id, uint8_t has_error,
                                  uint8_t error, uint8_t *out, size_t capacity)
{
    const size_t length = has_error ? 5u : 5u;
    if (capacity < length) return 0u;
    out[0] = ACTION_RESPONSE_TAG;
    out[1] = 0x01u;
    out[2] = invoke_id;
    out[3] = has_error ? 1u : 0u;
    out[4] = has_error ? error : 0u; /* absent return-parameters on success */
    return length;
}

static AmiDlmsApduStatus process_security_key_transfer(
    const AmiDlmsAssociation *association, const AmiDlmsCipherBackend *cipher,
    uint8_t *request, size_t request_length,
    size_t workspace_capacity, size_t *response_plain_length)
{
    static const uint8_t security_setup_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] =
        {0, 0, 43, 0, 0, 255};
    if (request_length < 15u || request[0] != ACTION_REQUEST_TAG || request[1] != 1u) {
        return AMI_DLMS_APDU_MALFORMED;
    }
    const uint8_t invoke_id = request[2];
    const uint16_t class_id = (uint16_t)(((uint16_t)request[3] << 8) | request[4]);
    const uint8_t method_id = request[11];
    if (class_id != AMI_COSEM_SECURITY_SETUP_CLASS_ID ||
        memcmp(request + 5u, security_setup_ln, sizeof(security_setup_ln)) != 0 ||
        method_id != 2u || request[12] != 1u || request[13] != 1u ||
        request[14] == 0u || request[14] > 2u) {
        *response_plain_length = build_action_result(invoke_id, 1u, 11u,
            request, workspace_capacity);
        return *response_plain_length != 0u
            ? AMI_DLMS_APDU_ACCESS_ERROR : AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    const size_t key_count = request[14];
    size_t offset = 15u;
    AmiDlmsWrappedKey keys[2] = {{0}};
    uint8_t seen_key_types = 0u;
    for (size_t i = 0; i < key_count; ++i) {
        if (offset + 4u > request_length || request[offset++] != AMI_COSEM_TYPE_STRUCTURE ||
            request[offset++] != 2u || request[offset++] != AMI_COSEM_TYPE_ENUM) goto malformed;
        const uint8_t key_type = request[offset++];
        if ((key_type != 0u && key_type != 2u) ||
            (seen_key_types & (uint8_t)(1u << key_type)) != 0u ||
            offset + 2u > request_length || request[offset++] != AMI_COSEM_TYPE_OCTET_STRING ||
            request[offset++] != 24u || request_length - offset < 24u) goto malformed;
        seen_key_types |= (uint8_t)(1u << key_type);
        keys[i].key_type = key_type;
        keys[i].wrapped_key = request + offset;
        keys[i].wrapped_key_length = 24u; /* AES-128 RFC 3394 wrapped key */
        offset += 24u;
    }
    if (offset != request_length) goto malformed;
    if (association->state != AMI_DLMS_ASSOC_ASSOCIATED ||
        association->mechanism != AMI_DLMS_AUTH_HLS || cipher->transfer_keys == NULL) {
        *response_plain_length = build_action_result(invoke_id, 1u, 3u,
            request, workspace_capacity);
        return *response_plain_length != 0u
            ? AMI_DLMS_APDU_ACCESS_ERROR : AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    if (cipher->transfer_keys(cipher->user, keys, key_count) != AMI_SEC_OK) {
        *response_plain_length = build_action_result(invoke_id, 1u, 15u,
            request, workspace_capacity);
        return *response_plain_length != 0u
            ? AMI_DLMS_APDU_ACCESS_ERROR : AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    *response_plain_length = build_action_result(invoke_id, 0u, 0u,
        request, workspace_capacity);
    return *response_plain_length != 0u
        ? AMI_DLMS_APDU_OK : AMI_DLMS_APDU_BUFFER_TOO_SMALL;

malformed:
    *response_plain_length = build_action_result(invoke_id, 1u, 11u,
        request, workspace_capacity);
    return *response_plain_length != 0u
        ? AMI_DLMS_APDU_ACCESS_ERROR : AMI_DLMS_APDU_BUFFER_TOO_SMALL;
}

static AmiDlmsApduStatus build_error(uint8_t invoke_id, uint8_t error,
                                     uint8_t *response, size_t capacity,
                                     size_t *response_length)
{
    if (response == NULL || response_length == NULL || capacity < 5u) {
        return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    response[0] = GET_RESPONSE_TAG;
    response[1] = GET_NORMAL_CHOICE;
    response[2] = invoke_id;
    response[3] = 1u; /* data-access-error alternative */
    response[4] = error;
    *response_length = 5u;
    return AMI_DLMS_APDU_ACCESS_ERROR;
}

typedef struct {
    uint16_t class_id;
    uint8_t version;
    uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH];
} AssociationViewObject;

/* Host/firmware association view: the objects this read-only meter actually
 * serves. Keep it bounded so it fits one negotiated HDLC information block. */
static const AssociationViewObject association_view[] = {
    {15u, 0u, {0, 0, 40, 0, 0, 255}}, /* Association LN */
    {8u,  0u, {0, 0, 1, 0, 0, 255}},  /* Clock */
    {3u,  0u, {1, 0, 32, 7, 0, 255}}, /* L1 voltage */
    {3u,  0u, {1, 0, 52, 7, 0, 255}}, /* L2 voltage */
    {3u,  0u, {1, 0, 72, 7, 0, 255}}, /* L3 voltage */
    {3u,  0u, {1, 0, 31, 7, 0, 255}}, /* L1 current */
    {3u,  0u, {1, 0, 51, 7, 0, 255}}, /* L2 current */
    {3u,  0u, {1, 0, 71, 7, 0, 255}}, /* L3 current */
    {3u,  0u, {1, 0, 1, 8, 0, 255}},  /* Active import */
    {3u,  0u, {1, 0, 2, 8, 0, 255}},  /* Active export */
    {7u,  0u, {0, 0, 99, 98, 0, 255}},/* Event log */
    {7u,  0u, {1, 0, 98, 1, 0, 255}} /* Billing profile */
};

static int view_put(uint8_t *out, size_t capacity, size_t *offset,
                    uint8_t value)
{
    if (*offset >= capacity) return 0;
    out[(*offset)++] = value;
    return 1;
}

static int view_put_bytes(uint8_t *out, size_t capacity, size_t *offset,
                          const uint8_t *bytes, size_t length)
{
    if (length > capacity - *offset) return 0;
    memcpy(out + *offset, bytes, length);
    *offset += length;
    return 1;
}

static AmiDlmsApduStatus build_association_object_list(
    uint8_t invoke_id, uint8_t *response, size_t capacity, size_t *length)
{
    size_t offset = 0u;
    const size_t object_count = sizeof(association_view) / sizeof(association_view[0]);
    if (capacity < 6u) return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    response[offset++] = GET_RESPONSE_TAG;
    response[offset++] = GET_NORMAL_CHOICE;
    response[offset++] = invoke_id;
    response[offset++] = 0u; /* data choice */
    response[offset++] = 1u; /* array */
    response[offset++] = (uint8_t)object_count;
    for (size_t i = 0u; i < object_count; ++i) {
        const AssociationViewObject *object = &association_view[i];
        if (!view_put(response, capacity, &offset, 2u) || /* structure */
            !view_put(response, capacity, &offset, 4u) ||
            !view_put(response, capacity, &offset, 18u) || /* long-unsigned */
            !view_put(response, capacity, &offset, (uint8_t)(object->class_id >> 8)) ||
            !view_put(response, capacity, &offset, (uint8_t)object->class_id) ||
            !view_put(response, capacity, &offset, 17u) || /* unsigned */
            !view_put(response, capacity, &offset, object->version) ||
            !view_put(response, capacity, &offset, 9u) || /* octet-string LN */
            !view_put(response, capacity, &offset, AMI_COSEM_LOGICAL_NAME_LENGTH) ||
            !view_put_bytes(response, capacity, &offset, object->logical_name,
                            AMI_COSEM_LOGICAL_NAME_LENGTH) ||
            !view_put(response, capacity, &offset, 2u) || /* access_rights */
            !view_put(response, capacity, &offset, 2u) ||
            !view_put(response, capacity, &offset, 1u) || /* attributes array */
            !view_put(response, capacity, &offset, 1u))
            return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
        /* Advertise the read path used by this meter (attribute 2) only. */
        if (!view_put(response, capacity, &offset, 2u) ||
            !view_put(response, capacity, &offset, 3u) ||
            !view_put(response, capacity, &offset, 15u) ||
            !view_put(response, capacity, &offset, 2u) ||
            !view_put(response, capacity, &offset, 22u) ||
            !view_put(response, capacity, &offset, 1u) ||
            !view_put(response, capacity, &offset, 1u) ||
            !view_put(response, capacity, &offset, 0u))
            return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
        /* No methods are exposed by this read-only association view. */
        if (!view_put(response, capacity, &offset, 1u) ||
            !view_put(response, capacity, &offset, 0u))
            return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    *length = offset;
    return AMI_DLMS_APDU_OK;
}

static AmiDlmsApduStatus process_get_request_plain(
    const AmiDlmsAssociation *association,
    const AmiCosemRegisterSnapshot *snapshot,
    const uint8_t *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *response_length,
    int decrypted_hls_apdu)
{
    if (response_length != NULL) {
        *response_length = 0u;
    }
    if (association == NULL || snapshot == NULL || request == NULL ||
        response == NULL || response_length == NULL) {
        return AMI_DLMS_APDU_INVALID_ARGUMENT;
    }
    if (association->state != AMI_DLMS_ASSOC_ASSOCIATED) {
        return AMI_DLMS_APDU_NOT_ASSOCIATED;
    }
    if (association->mechanism == AMI_DLMS_AUTH_HLS && !decrypted_hls_apdu) {
        return AMI_DLMS_APDU_CIPHER_REQUIRED;
    }
    if (request_length < 3u || request[0] != GET_REQUEST_TAG ||
        request[1] != GET_NORMAL_CHOICE) {
        return build_error(request_length > 2u ? request[2] : 0u, 3u,
                           response, response_capacity, response_length);
    }
    if (request_length < 13u || request[12] > 1u) {
        return build_error(request[2], 3u, response,
                           response_capacity, response_length);
    }

    uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH];
    uint16_t class_id = (uint16_t)(((uint16_t)request[3] << 8) | request[4]);
    memcpy(logical_name, request + 5u, sizeof(logical_name));
    uint8_t attribute_id = request[11];
    static const uint8_t association_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] =
        {0u, 0u, 40u, 0u, 0u, 255u};
    if (class_id == 15u) {
        if (memcmp(logical_name, association_ln, sizeof(association_ln)) != 0) {
            return build_error(request[2], 3u, response, response_capacity, response_length);
        }
        if (request[12] == 0u && request_length == 13u) {
        if (attribute_id == 1u) {
            if (response_capacity < 12u) return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
            response[0] = GET_RESPONSE_TAG;
            response[1] = GET_NORMAL_CHOICE;
            response[2] = request[2];
            response[3] = 0u; /* data alternative */
            response[4] = (uint8_t)AMI_COSEM_TYPE_OCTET_STRING;
            response[5] = AMI_COSEM_LOGICAL_NAME_LENGTH;
            memcpy(response + 6u, association_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
            *response_length = 12u;
            return AMI_DLMS_APDU_OK;
        }
        if (attribute_id == 2u) {
            return build_association_object_list(request[2], response,
                                                 response_capacity, response_length);
        }
        }
        return build_error(request[2], 3u, response, response_capacity, response_length);
    }
    AmiCosemProfileSelection selection;
    const AmiCosemProfileSelection *selection_pointer = NULL;
    if (request[12] == 0u) {
        if (request_length != 13u) {
            return build_error(request[2], 15u, response,
                               response_capacity, response_length);
        }
    } else {
        if (class_id != AMI_COSEM_PROFILE_GENERIC_CLASS_ID || attribute_id != 2u) {
            return build_error(request[2], 11u, response,
                               response_capacity, response_length);
        }
        if (!parse_profile_selection(request, request_length, 13u, &selection)) {
            return build_error(request[2], 11u, response,
                               response_capacity, response_length);
        }
        selection_pointer = &selection;
    }
    AmiCosemData value;
    AmiCosemStatus read_status = ami_cosem_register_read_selected(
        snapshot, class_id, logical_name, attribute_id, selection_pointer, &value);
    if (read_status != AMI_COSEM_OK) {
        return build_error(request[2], access_error(read_status), response,
                           response_capacity, response_length);
    }

    if (response_capacity < 4u) {
        return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    response[0] = GET_RESPONSE_TAG;
    response[1] = GET_NORMAL_CHOICE;
    response[2] = request[2];
    response[3] = 0u; /* data alternative */
    size_t encoded_length = 0u;
    if (!encode_data(&value, response + 4u, response_capacity - 4u,
                     &encoded_length)) {
        *response_length = 0u;
        return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    *response_length = 4u + encoded_length;
    return AMI_DLMS_APDU_OK;
}

AmiDlmsApduStatus ami_dlms_process_get_request(
    const AmiDlmsAssociation *association,
    const AmiCosemRegisterSnapshot *snapshot,
    const uint8_t *request, size_t request_length,
    uint8_t *response, size_t response_capacity, size_t *response_length)
{
    return process_get_request_plain(association, snapshot, request, request_length,
        response, response_capacity, response_length, 0);
}

static uint32_t read_u32_be(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static void write_u32_be(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static int ranges_overlap(const void *left, size_t left_length,
                          const void *right, size_t right_length)
{
    uintptr_t left_start = (uintptr_t)left;
    uintptr_t right_start = (uintptr_t)right;
    if (left_length > UINTPTR_MAX - left_start ||
        right_length > UINTPTR_MAX - right_start) return 1;
    uintptr_t left_end = left_start + left_length;
    uintptr_t right_end = right_start + right_length;
    return left_start < right_end && right_start < left_end;
}

AmiDlmsApduStatus ami_dlms_process_ciphered_request(
    const AmiDlmsAssociation *association,
    const AmiCosemRegisterSnapshot *snapshot,
    const AmiDlmsCipherBackend *cipher,
    const uint8_t *request, size_t request_length,
    uint8_t *workspace, size_t workspace_capacity,
    uint8_t *response, size_t response_capacity, size_t *response_length)
{
    if (response_length != NULL) *response_length = 0u;
    if (association == NULL || snapshot == NULL || cipher == NULL || request == NULL ||
        workspace == NULL || response == NULL || response_length == NULL) {
        return AMI_DLMS_APDU_INVALID_ARGUMENT;
    }
    if (ranges_overlap(request, request_length, workspace, workspace_capacity) ||
        ranges_overlap(response, response_capacity, workspace, workspace_capacity)) {
        return AMI_DLMS_APDU_INVALID_ARGUMENT;
    }
    if (association->state != AMI_DLMS_ASSOC_ASSOCIATED ||
        association->mechanism != AMI_DLMS_AUTH_HLS) {
        return AMI_DLMS_APDU_NOT_ASSOCIATED;
    }
    if (cipher->decrypt == NULL || cipher->encrypt == NULL ||
        cipher->accept_rx_counter == NULL || cipher->next_tx_counter == NULL) {
        return AMI_DLMS_APDU_INVALID_ARGUMENT;
    }
    if (request_length <= GLO_HEADER_LENGTH + AMI_DLMS_GCM_TAG_LENGTH ||
        (request[0] != GLO_GET_REQUEST_TAG && request[0] != GLO_SET_REQUEST_TAG &&
         request[0] != GLO_ACTION_REQUEST_TAG)) {
        return AMI_DLMS_APDU_MALFORMED;
    }
    const uint8_t security_control = request[1];
    if (security_control != cipher->expected_security_control ||
        (security_control & 0x30u) != 0x30u) {
        return AMI_DLMS_APDU_AUTHENTICATION_FAILURE;
    }
    const uint32_t rx_counter = read_u32_be(request + 2u);
    const size_t ciphertext_length = request_length - GLO_HEADER_LENGTH - AMI_DLMS_GCM_TAG_LENGTH;
    const uint8_t *ciphertext = request + GLO_HEADER_LENGTH;
    const uint8_t *tag = ciphertext + ciphertext_length;
    if (ciphertext_length > workspace_capacity) return AMI_DLMS_APDU_BUFFER_TOO_SMALL;

    size_t plaintext_length = 0u;
    AmiSecStatus sec_status = cipher->decrypt(cipher->user, security_control, rx_counter,
        ciphertext, ciphertext_length, tag, workspace, workspace_capacity, &plaintext_length);
    if (sec_status != AMI_SEC_OK || plaintext_length == 0u ||
        plaintext_length > workspace_capacity) {
        secure_zeroize(workspace, workspace_capacity);
        return sec_status == AMI_SEC_AUTHENTICATION_FAILURE
            ? AMI_DLMS_APDU_AUTHENTICATION_FAILURE : AMI_DLMS_APDU_BACKEND_FAILURE;
    }
    /* Commit only after tag verification; callback must atomically reject replay. */
    if (cipher->accept_rx_counter(cipher->user, rx_counter) != 1) {
        secure_zeroize(workspace, workspace_capacity);
        return AMI_DLMS_APDU_COUNTER_REJECTED;
    }

    const uint8_t expected_plain_tag = request[0] == GLO_GET_REQUEST_TAG
        ? GET_REQUEST_TAG : (request[0] == GLO_SET_REQUEST_TAG
            ? SET_REQUEST_TAG : ACTION_REQUEST_TAG);
    AmiDlmsApduStatus service_status;
    size_t response_plain_length = 0u;
    if (workspace[0] != expected_plain_tag) {
        secure_zeroize(workspace, workspace_capacity);
        return AMI_DLMS_APDU_MALFORMED;
    }
    if (expected_plain_tag == GET_REQUEST_TAG) {
        service_status = process_get_request_plain(association, snapshot, workspace,
            plaintext_length, workspace, workspace_capacity,
            &response_plain_length, 1);
    } else if (expected_plain_tag == SET_REQUEST_TAG) {
        /* Sprint 1 exposes read-only COSEM objects; SET is authenticated but denied. */
        if (plaintext_length < 14u || workspace[1] != 0x01u || workspace[12] != 0u) {
            secure_zeroize(workspace, workspace_capacity);
            return AMI_DLMS_APDU_MALFORMED;
        }
        if (workspace_capacity < 4u) {
            secure_zeroize(workspace, workspace_capacity);
            return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
        }
        workspace[0] = SET_RESPONSE_TAG;
        workspace[1] = 0x01u;
        /* workspace[2] already holds the invoke-id. */
        workspace[3] = 3u; /* read-write-denied: meter is read-only */
        response_plain_length = 4u;
        service_status = AMI_DLMS_APDU_ACCESS_ERROR;
    } else {
        service_status = process_security_key_transfer(association, cipher, workspace,
            plaintext_length, workspace_capacity, &response_plain_length);
    }
    if (response_plain_length == 0u) {
        secure_zeroize(workspace, workspace_capacity);
        return service_status;
    }

    if (response_capacity < GLO_HEADER_LENGTH + response_plain_length + AMI_DLMS_GCM_TAG_LENGTH) {
        secure_zeroize(workspace, workspace_capacity);
        return AMI_DLMS_APDU_BUFFER_TOO_SMALL;
    }
    uint32_t tx_counter = 0u;
    if (cipher->next_tx_counter(cipher->user, &tx_counter) != 1) {
        secure_zeroize(workspace, workspace_capacity);
        return AMI_DLMS_APDU_BACKEND_FAILURE;
    }
    const uint8_t response_tag = expected_plain_tag == GET_REQUEST_TAG
        ? GLO_GET_RESPONSE_TAG : (expected_plain_tag == SET_REQUEST_TAG
            ? GLO_SET_RESPONSE_TAG : GLO_ACTION_RESPONSE_TAG);
    response[0] = response_tag;
    response[1] = security_control;
    write_u32_be(response + 2u, tx_counter);
    size_t encrypted_length = 0u;
    sec_status = cipher->encrypt(cipher->user, security_control, tx_counter,
        workspace, response_plain_length, response + GLO_HEADER_LENGTH,
        response_capacity - GLO_HEADER_LENGTH - AMI_DLMS_GCM_TAG_LENGTH,
        &encrypted_length, response + GLO_HEADER_LENGTH + response_plain_length);
    secure_zeroize(workspace, workspace_capacity);
    if (sec_status != AMI_SEC_OK || encrypted_length != response_plain_length) {
        memset(response, 0, response_capacity);
        return AMI_DLMS_APDU_BACKEND_FAILURE;
    }
    *response_length = GLO_HEADER_LENGTH + encrypted_length + AMI_DLMS_GCM_TAG_LENGTH;
    return service_status;
}
