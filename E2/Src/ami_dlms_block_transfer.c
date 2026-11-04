/**
 * @file ami_dlms_block_transfer.c
 * @brief Implementation of DLMS/COSEM xDLMS Block Transfer & GBT Engine.
 */

#include "ami_dlms_block_transfer.h"
#include <string.h>

size_t ami_bt_encode_axdr_length(size_t length, uint8_t *out, size_t capacity)
{
    if (out == NULL || capacity == 0u) {
        return 0u;
    }
    if (length < 128u) {
        out[0] = (uint8_t)length;
        return 1u;
    } else if (length <= 255u) {
        if (capacity < 2u) return 0u;
        out[0] = 0x81u;
        out[1] = (uint8_t)length;
        return 2u;
    } else if (length <= 65535u) {
        if (capacity < 3u) return 0u;
        out[0] = 0x82u;
        out[1] = (uint8_t)(length >> 8);
        out[2] = (uint8_t)(length & 0xFFu);
        return 3u;
    }
    return 0u;
}

void ami_bt_init(AmiBlockTransferContext *ctx, size_t block_size)
{
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(*ctx));
    ami_bt_set_block_size(ctx, block_size);
    ctx->state = AMI_BT_STATE_IDLE;
}

void ami_bt_reset(AmiBlockTransferContext *ctx)
{
    if (ctx == NULL) return;
    size_t prev_block_size = ctx->block_size;
    memset(ctx, 0, sizeof(*ctx));
    ctx->block_size = prev_block_size > 0u ? prev_block_size : AMI_BT_DEFAULT_BLOCK_SIZE;
    ctx->state = AMI_BT_STATE_IDLE;
}

void ami_bt_set_block_size(AmiBlockTransferContext *ctx, size_t block_size)
{
    if (ctx == NULL) return;
    if (block_size < 32u) {
        ctx->block_size = AMI_BT_DEFAULT_BLOCK_SIZE;
    } else if (block_size > AMI_BT_MAX_BLOCK_SIZE) {
        ctx->block_size = AMI_BT_MAX_BLOCK_SIZE;
    } else {
        ctx->block_size = block_size;
    }
}

AmiBtStatus ami_bt_start_get_transfer(AmiBlockTransferContext *ctx,
                                     uint8_t invoke_id,
                                     const uint8_t *data,
                                     size_t data_length)
{
    if (ctx == NULL || data == NULL || data_length == 0u) {
        return AMI_BT_INVALID_PARAM;
    }
    if (data_length > AMI_BT_MAX_CACHE_BUFFER_SIZE) {
        return AMI_BT_OVERFLOW;
    }

    memcpy(ctx->cache, data, data_length);
    ctx->total_length = data_length;
    ctx->invoke_id = invoke_id;
    ctx->current_block_number = 1u;
    ctx->bytes_sent = 0u;
    ctx->state = AMI_BT_STATE_TRANSFERRING;

    return AMI_BT_OK;
}

AmiBtStatus ami_bt_build_get_response_block(AmiBlockTransferContext *ctx,
                                           uint32_t requested_block_number,
                                           uint8_t *response,
                                           size_t response_capacity,
                                           size_t *response_length,
                                           int *is_last_block)
{
    if (ctx == NULL || response == NULL || response_length == NULL) {
        return AMI_BT_INVALID_PARAM;
    }
    *response_length = 0u;
    if (is_last_block != NULL) {
        *is_last_block = 0;
    }

    if (ctx->state != AMI_BT_STATE_TRANSFERRING && ctx->state != AMI_BT_STATE_COMPLETED) {
        return AMI_BT_NOT_ACTIVE;
    }

    if (requested_block_number == 0u) {
        return AMI_BT_BLOCK_OUT_OF_SEQUENCE;
    }

    size_t offset = 0u;
    if (requested_block_number == 1u) {
        offset = 0u;
        ctx->current_block_number = 1u;
        ctx->state = AMI_BT_STATE_TRANSFERRING;
    } else if (requested_block_number == ctx->current_block_number) {
        /* Retransmission of current block */
        offset = (size_t)(requested_block_number - 1u) * ctx->block_size;
    } else if (requested_block_number == ctx->current_block_number + 1u) {
        /* Next block in sequence */
        offset = (size_t)(requested_block_number - 1u) * ctx->block_size;
        ctx->current_block_number = requested_block_number;
    } else {
        /* Out-of-sequence request */
        return AMI_BT_BLOCK_OUT_OF_SEQUENCE;
    }

    if (offset >= ctx->total_length) {
        return AMI_BT_BLOCK_OUT_OF_SEQUENCE;
    }

    size_t remaining = ctx->total_length - offset;
    size_t chunk_len = (remaining > ctx->block_size) ? ctx->block_size : remaining;
    int last = (offset + chunk_len >= ctx->total_length) ? 1 : 0;

    if (is_last_block != NULL) {
        *is_last_block = last;
    }

    /* Check output buffer capacity: 9 bytes header + up to 3 bytes len + chunk_len */
    if (response_capacity < 9u + 3u + chunk_len) {
        return AMI_BT_BUFFER_TOO_SMALL;
    }

    size_t w = 0u;
    response[w++] = AMI_DLMS_TAG_GET_RESPONSE;
    response[w++] = AMI_DLMS_GET_RESP_WITH_DATABLOCK;
    response[w++] = ctx->invoke_id;
    response[w++] = last ? 0x01u : 0x00u; /* last-block boolean */
    response[w++] = (uint8_t)(requested_block_number >> 24);
    response[w++] = (uint8_t)(requested_block_number >> 16);
    response[w++] = (uint8_t)(requested_block_number >> 8);
    response[w++] = (uint8_t)(requested_block_number);
    response[w++] = AMI_DLMS_DATABLOCK_RESULT_RAW_DATA; /* 0x00 */

    size_t len_bytes = ami_bt_encode_axdr_length(chunk_len, response + w, response_capacity - w);
    if (len_bytes == 0u) {
        return AMI_BT_BUFFER_TOO_SMALL;
    }
    w += len_bytes;

    memcpy(response + w, ctx->cache + offset, chunk_len);
    w += chunk_len;

    *response_length = w;

    if (last) {
        ctx->state = AMI_BT_STATE_COMPLETED;
    }

    return AMI_BT_OK;
}

int ami_bt_parse_get_next_block_request(const uint8_t *request,
                                        size_t request_length,
                                        uint8_t *invoke_id,
                                        uint32_t *block_number)
{
    if (request == NULL || request_length < 7u || invoke_id == NULL || block_number == NULL) {
        return 0;
    }

    if (request[0] != AMI_DLMS_TAG_GET_REQUEST ||
        request[1] != AMI_DLMS_GET_REQ_NEXT_DATABLOCK) {
        return 0;
    }

    *invoke_id = request[2];
    *block_number = ((uint32_t)request[3] << 24) |
                    ((uint32_t)request[4] << 16) |
                    ((uint32_t)request[5] << 8)  |
                    ((uint32_t)request[6]);

    return 1;
}

size_t ami_bt_build_error_response_block(uint8_t invoke_id,
                                         uint32_t block_number,
                                         uint8_t data_access_result,
                                         uint8_t *response,
                                         size_t response_capacity)
{
    if (response == NULL || response_capacity < 10u) {
        return 0u;
    }

    response[0] = AMI_DLMS_TAG_GET_RESPONSE;
    response[1] = AMI_DLMS_GET_RESP_WITH_DATABLOCK;
    response[2] = invoke_id;
    response[3] = 0x01u; /* last-block = true */
    response[4] = (uint8_t)(block_number >> 24);
    response[5] = (uint8_t)(block_number >> 16);
    response[6] = (uint8_t)(block_number >> 8);
    response[7] = (uint8_t)(block_number);
    response[8] = AMI_DLMS_DATABLOCK_RESULT_DATA_ACCESS; /* 0x01 = data-access-result */
    response[9] = data_access_result;

    return 10u;
}

