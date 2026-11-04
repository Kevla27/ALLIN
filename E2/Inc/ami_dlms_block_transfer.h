/**
 * @file ami_dlms_block_transfer.h
 * @brief DLMS/COSEM xDLMS Block Transfer & General Block Transfer (GBT) Engine.
 *
 * Implements IEC 62056-5-3 / DLMS Green Book block transfer mechanisms:
 * - GET-Request-For-Next-Datablock (0xC0 0x02)
 * - GET-Response-With-Datablock (0xC4 0x02)
 * - ACTION-Request-With-Datablock (0xC3 0x02)
 * - ACTION-Response-With-Datablock (0xC7 0x02)
 *
 * Provides chunking and reassembly state machine for large COSEM Profile Generic
 * buffer reads (Load Profile, Event Log, Billing History) exceeding Max PDU size.
 */

#ifndef AMI_DLMS_BLOCK_TRANSFER_H
#define AMI_DLMS_BLOCK_TRANSFER_H

#include <stddef.h>
#include <stdint.h>
#include "ami_cosem_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_BT_DEFAULT_BLOCK_SIZE     128u
#define AMI_BT_MAX_BLOCK_SIZE         512u
#define AMI_BT_MAX_CACHE_BUFFER_SIZE  2048u

/* xDLMS APDU Tags */
#define AMI_DLMS_TAG_GET_REQUEST                 0xC0u
#define AMI_DLMS_TAG_GET_RESPONSE                0xC4u
#define AMI_DLMS_TAG_ACTION_REQUEST              0xC3u
#define AMI_DLMS_TAG_ACTION_RESPONSE             0xC7u

/* Service choices */
#define AMI_DLMS_GET_REQ_NORMAL                  0x01u
#define AMI_DLMS_GET_REQ_NEXT_DATABLOCK          0x02u
#define AMI_DLMS_GET_RESP_NORMAL                 0x01u
#define AMI_DLMS_GET_RESP_WITH_DATABLOCK         0x02u
#define AMI_DLMS_ACTION_REQ_NORMAL               0x01u
#define AMI_DLMS_ACTION_REQ_WITH_DATABLOCK       0x02u
#define AMI_DLMS_ACTION_RESP_NORMAL              0x01u
#define AMI_DLMS_ACTION_RESP_WITH_DATABLOCK      0x02u

/* DataBlock-Result choices */
#define AMI_DLMS_DATABLOCK_RESULT_RAW_DATA       0x00u
#define AMI_DLMS_DATABLOCK_RESULT_DATA_ACCESS    0x01u

typedef enum {
    AMI_BT_STATE_IDLE = 0,
    AMI_BT_STATE_TRANSFERRING,
    AMI_BT_STATE_COMPLETED,
    AMI_BT_STATE_ERROR
} AmiBtState;

typedef enum {
    AMI_BT_OK = 0,
    AMI_BT_INVALID_PARAM,
    AMI_BT_BUFFER_TOO_SMALL,
    AMI_BT_BLOCK_OUT_OF_SEQUENCE,
    AMI_BT_NOT_ACTIVE,
    AMI_BT_OVERFLOW,
    AMI_BT_ERROR_RESULT
} AmiBtStatus;

/**
 * @brief Context for managing block transfer sessions.
 */
typedef struct {
    AmiBtState state;
    uint8_t invoke_id;
    uint32_t current_block_number; /* 1-based block number */
    size_t block_size;             /* Target chunk size per block (e.g. 128 bytes) */
    size_t total_length;           /* Total payload bytes to transmit */
    size_t bytes_sent;             /* Bytes sent in completed blocks */
    uint8_t cache[AMI_BT_MAX_CACHE_BUFFER_SIZE];
} AmiBlockTransferContext;

/**
 * @brief Initialize block transfer context with specified block chunk size.
 */
void ami_bt_init(AmiBlockTransferContext *ctx, size_t block_size);

/**
 * @brief Reset block transfer state machine back to IDLE.
 */
void ami_bt_reset(AmiBlockTransferContext *ctx);

/**
 * @brief Set the block chunk size (clamped between 32 and AMI_BT_MAX_BLOCK_SIZE).
 */
void ami_bt_set_block_size(AmiBlockTransferContext *ctx, size_t block_size);

/**
 * @brief Start a GET block transfer by caching the full data payload.
 *
 * @param ctx Context pointer
 * @param invoke_id Invoke ID and priority from client request
 * @param data Full raw data payload (excluding GET header)
 * @param data_length Total length of raw data
 * @return AMI_BT_OK on success, error code otherwise.
 */
AmiBtStatus ami_bt_start_get_transfer(AmiBlockTransferContext *ctx,
                                     uint8_t invoke_id,
                                     const uint8_t *data,
                                     size_t data_length);

/**
 * @brief Generate GET-Response-With-Datablock (0xC4 0x02) for the given block number.
 *
 * Supports retransmission of the current block or advancing to current_block + 1.
 *
 * @param ctx Context pointer
 * @param requested_block_number Block number requested by client (1-based)
 * @param response Output buffer for generated APDU
 * @param response_capacity Output buffer capacity
 * @param response_length Pointer to store actual bytes written
 * @param is_last_block Pointer to store flag: 1 if this is the last block, 0 otherwise
 * @return AMI_BT_OK on success, error code otherwise.
 */
AmiBtStatus ami_bt_build_get_response_block(AmiBlockTransferContext *ctx,
                                           uint32_t requested_block_number,
                                           uint8_t *response,
                                           size_t response_capacity,
                                           size_t *response_length,
                                           int *is_last_block);

/**
 * @brief Parse a GET-Request-For-Next-Datablock (0xC0 0x02).
 *
 * @param request Pointer to request APDU
 * @param request_length Length of request APDU
 * @param invoke_id Pointer to store parsed invoke ID
 * @param block_number Pointer to store parsed block number
 * @return 1 if valid GET-Request-For-Next-Datablock, 0 otherwise.
 */
int ami_bt_parse_get_next_block_request(const uint8_t *request,
                                        size_t request_length,
                                        uint8_t *invoke_id,
                                        uint32_t *block_number);

/**
 * @brief Build an error response with datablock (0xC4 0x02 with DataAccessResult).
 *
 * @param invoke_id Invoke ID
 * @param block_number Block number
 * @param data_access_result DLMS DataAccessResult error code
 * @param response Output buffer
 * @param response_capacity Output buffer capacity
 * @return Length of encoded APDU in bytes.
 */
size_t ami_bt_build_error_response_block(uint8_t invoke_id,
                                         uint32_t block_number,
                                         uint8_t data_access_result,
                                         uint8_t *response,
                                         size_t response_capacity);

/**
 * @brief Helper to encode DLMS A-XDR length field.
 *
 * @param length Value length
 * @param out Output buffer
 * @param capacity Output capacity
 * @return Number of bytes written (1, 2, or 3), or 0 on error.
 */
size_t ami_bt_encode_axdr_length(size_t length, uint8_t *out, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* AMI_DLMS_BLOCK_TRANSFER_H */

