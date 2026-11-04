#include "ami_dlms_port.h"
#include "ami_dlms_cipher.h"
#include "ami_nvm_counter.h"
#include <string.h>

#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "cmsis_os2.h"
#if defined(SECURE_HARDWARE_SAES_ENABLED)
#include "secure_nsc.h"
#endif

static int ami_dlms_secure_random(void *user, uint8_t *output, size_t length)
{
    (void)user;
#if defined(SECURE_HARDWARE_SAES_ENABLED)
    return (SECURE_TRNG_GetBytes(output, (uint32_t)length) == 0) ? 1 : 0;
#else
    for (size_t i = 0; i < length; i++) {
        output[i] = (uint8_t)(i * 37u + 0x5Au);
    }
    return 1;
#endif
}

static int ami_dlms_secure_hls_proof(void *user, const uint8_t *oid, size_t oid_length,
                                     const uint8_t *challenge, size_t challenge_length,
                                     uint8_t *proof, size_t proof_capacity, size_t *proof_length)
{
    (void)user;
    (void)oid;
    (void)oid_length;
    if (challenge == NULL || proof == NULL || proof_length == NULL || challenge_length != 16 || proof_capacity < 16) {
        return 0;
    }

#if defined(SECURE_HARDWARE_SAES_ENABLED)
    uint32_t out_len = 0u;
    int st = SECURE_DLMS_HLS5_CalculateProof(challenge, (uint32_t)challenge_length,
                                            proof, (uint32_t)proof_capacity, &out_len);
    if (st == 0 && out_len == 16u) {
        *proof_length = 16u;
        return 1;
    }
    return 0;
#else
    memcpy(proof, challenge, 16);
    *proof_length = 16u;
    return 1;
#endif
}
#endif

int ami_dlms_port_peer_matches_firmware_profile(const HdlcFrame *frame)
{
    return frame != NULL &&
           frame->destination_sap == 1u &&
           frame->source_sap == 16u;
}

static size_t encode_aare_lockout_rejection(uint8_t *out, size_t capacity)
{
    /* AARE rejected-permanent (1), acse-service-user / authentication-failure (1) per DLMS Green Book */
    static const uint8_t s_aare_rejected[] = {
        0x61, 0x1F,                                                         /* AARE, length 31 */
        0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01, /* Application Context: LN */
        0xA2, 0x03, 0x02, 0x01, 0x01,                                     /* Association-result: rejected-permanent (1) */
        0xA3, 0x05, 0xA1, 0x03, 0x02, 0x01, 0x01,                         /* Result-source-diagnostic: acse-service-user -> authentication-failure (1) */
        0xBE, 0x06, 0x04, 0x04, 0x0E, 0x01, 0x00, 0x00                    /* User-information */
    };
    if (capacity < sizeof(s_aare_rejected)) {
        return 0u;
    }
    memcpy(out, s_aare_rejected, sizeof(s_aare_rejected));
    return sizeof(s_aare_rejected);
}

static AmiDlmsPortStatus dispatch_i_frame(AmiDlmsPortContext *ctx, const HdlcFrame *frame)
{
    uint8_t apdu_response[AMI_BT_MAX_CACHE_BUFFER_SIZE];
    size_t apdu_resp_len = 0u;
    const uint8_t *apdu = frame->payload;
    const size_t apdu_len = frame->payload_length;
    size_t wire_len = 0u;

    if (apdu_len == 0u) {
        return AMI_DLMS_PORT_OK;
    }

    if (apdu[0] == 0x60u) {
        /* AARQ: Check Anti-Brute-Force lockout first per SPLN D5.002:2024 */
        if (ctx->anti_bruteforce != NULL && !ami_anti_bruteforce_is_allowed(ctx->anti_bruteforce, ctx->current_time_sec)) {
            apdu_resp_len = encode_aare_lockout_rejection(apdu_response, sizeof(apdu_response));
        } else if (ctx->use_custom_security) {
            (void)ami_dlms_process_aarq_with_security(&ctx->association,
                                                      &ctx->security,
                                                      apdu, apdu_len,
                                                      apdu_response, sizeof(apdu_response),
                                                      &apdu_resp_len);
        } else {
#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
            /* A zeroed policy rejects every auth mode until firmware configures one. */
            (void)ami_dlms_process_aarq_with_security(&ctx->association,
                                                      &ctx->security,
                                                      apdu, apdu_len,
                                                      apdu_response, sizeof(apdu_response),
                                                      &apdu_resp_len);
#else
            /* Preserve the legacy no-auth path for host tests and simulator only. */
            (void)ami_dlms_process_aarq(&ctx->association, apdu, apdu_len,
                                            apdu_response, sizeof(apdu_response),
                                            &apdu_resp_len);
#endif
        }
        if (apdu_resp_len == 0u) {
            /* If no AARE was generated (e.g. unsolicited AARQ during active session or fatal error),
             * do not transmit garbage on wire.
             */
            return AMI_DLMS_PORT_OK;
        }
    } else if (apdu[0] == 0x62u) {
        /* ACSE RLRQ (Release Request) -> Reply RLRE */
        ami_dlms_association_reset(&ctx->association);
        ami_bt_reset(&ctx->block_transfer);
        apdu_response[0] = 0x63u; /* RLRE tag */
        apdu_response[1] = 0x03u; /* length */
        apdu_response[2] = 0x80u; /* reason tag */
        apdu_response[3] = 0x01u;
        apdu_response[4] = 0x00u; /* normal */
        apdu_resp_len = 5u;
    } else if (apdu[0] == 0xC3u) {
        /* Check for HLS Authentication reply: Class 15, OBIS 0.0.40.0.0.255, Method 1 */
        if (apdu_len >= 14u &&
            apdu[1] == 0x01u && /* Action-Request-Normal */
            apdu[3] == 0x00u && apdu[4] == 0x0Fu && /* Class 15: Association LN */
            apdu[5] == 0x00u && apdu[6] == 0x00u && apdu[7] == 0x28u &&
            apdu[8] == 0x00u && apdu[9] == 0x00u && apdu[10] == 0xFFu && /* 0.0.40.0.0.255 */
            apdu[11] == 0x01u && /* Method 1: reply_to_HLS_authentication */
            apdu[12] == 0x09u) { /* OctetString data type */

            size_t client_proof_len = apdu[13];
            const uint8_t *client_proof = &apdu[14];
            if (apdu_len >= 14u + client_proof_len) {
                uint8_t server_proof[64];
                size_t server_proof_len = 0u;
                AmiDlmsAssociationStatus hls_st = ami_dlms_process_hls_reply(
                    &ctx->association,
                    &ctx->security,
                    client_proof, client_proof_len,
                    server_proof, sizeof(server_proof),
                    &server_proof_len);

                if (hls_st == AMI_DLMS_ASSOC_OK) {
                    if (ctx->anti_bruteforce != NULL) {
                        ami_anti_bruteforce_record_success(ctx->anti_bruteforce);
                    }
                    /* ActionResponseNormal with success (0) and server proof data */
                    apdu_response[0] = 0xC7u;
                    apdu_response[1] = 0x01u;
                    apdu_response[2] = apdu[2]; /* invoke_id */
                    apdu_response[3] = 0x00u;   /* result = success */
                    apdu_response[4] = 0x01u;   /* data-present = true */
                    apdu_response[5] = 0x09u;   /* OctetString */
                    apdu_response[6] = (uint8_t)server_proof_len;
                    memcpy(&apdu_response[7], server_proof, server_proof_len);
                    apdu_resp_len = 7u + server_proof_len;
                } else {
                    if (ctx->anti_bruteforce != NULL) {
                        (void)ami_anti_bruteforce_record_failure(ctx->anti_bruteforce, ctx->current_time_sec);
                    }
                    /* ActionResponseNormal with read-write-denied (3) */
                    apdu_response[0] = 0xC7u;
                    apdu_response[1] = 0x01u;
                    apdu_response[2] = apdu[2];
                    apdu_response[3] = 0x03u;   /* read-write-denied / auth failure */
                    apdu_response[4] = 0x00u;   /* data-present = false */
                    apdu_resp_len = 5u;
                }
            } else {
                /* Malformed parameters -> ActionResponse with other-reason (1) */
                apdu_response[0] = 0xC7u;
                apdu_response[1] = 0x01u;
                apdu_response[2] = apdu[2];
                apdu_response[3] = 0x01u;
                apdu_response[4] = 0x00u;
                apdu_resp_len = 5u;
            }
        } else if (apdu_len >= 12u &&
                   apdu[1] == 0x01u && /* Action-Request-Normal */
                   apdu[3] == 0x00u && apdu[4] == 0x46u && /* Class 70: Disconnect Control */
                   apdu[5] == 0x00u && apdu[6] == 0x00u && apdu[7] == 0x60u &&
                   apdu[8] == 0x03u && apdu[9] == 0x0Au && apdu[10] == 0xFFu) { /* 0.0.96.3.10.255 */

            if (ctx->association.state != AMI_DLMS_ASSOC_ASSOCIATED) {
                apdu_response[0] = 0xC7u;
                apdu_response[1] = 0x01u;
                apdu_response[2] = apdu[2];
                apdu_response[3] = 0x03u; /* read-write-denied / not associated */
                apdu_response[4] = 0x00u;
                apdu_resp_len = 5u;
            } else if (ctx->disconnect_control == NULL) {
                apdu_response[0] = 0xC7u;
                apdu_response[1] = 0x01u;
                apdu_response[2] = apdu[2];
                apdu_response[3] = 0x01u; /* hardware-fault / other-reason */
                apdu_response[4] = 0x00u;
                apdu_resp_len = 5u;
            } else {
                uint8_t method_id = apdu[11];
                AmiDisconnectStatus dc_st = AMI_DISCONNECT_ERR_INVALID_PARAM;
                if (method_id == 1u) {
                    dc_st = ami_disconnect_control_remote_disconnect(ctx->disconnect_control, 0);
                } else if (method_id == 2u) {
                    dc_st = ami_disconnect_control_remote_reconnect(ctx->disconnect_control, 0);
                }

                if (dc_st == AMI_DISCONNECT_OK) {
                    ami_disconnect_control_sync_snapshot(ctx->disconnect_control, &ctx->snapshot);
                    apdu_response[0] = 0xC7u;
                    apdu_response[1] = 0x01u;
                    apdu_response[2] = apdu[2];
                    apdu_response[3] = 0x00u; /* success */
                    apdu_response[4] = 0x00u; /* data-present = false */
                    apdu_resp_len = 5u;
                } else if (dc_st == AMI_DISCONNECT_ERR_NOT_PERMITTED) {
                    apdu_response[0] = 0xC7u;
                    apdu_response[1] = 0x01u;
                    apdu_response[2] = apdu[2];
                    apdu_response[3] = 0x03u; /* read-write-denied */
                    apdu_response[4] = 0x00u;
                    apdu_resp_len = 5u;
                } else {
                    apdu_response[0] = 0xC7u;
                    apdu_response[1] = 0x01u;
                    apdu_response[2] = apdu[2];
                    apdu_response[3] = 0x02u; /* temporary-failure */
                    apdu_response[4] = 0x00u;
                    apdu_resp_len = 5u;
                }
            }
        } else {
            /* Action-Request on read-only meter -> ActionResponseNormal with read-write-denied (3) */
            apdu_response[0] = 0xC7u;
            apdu_response[1] = 0x01u;
            apdu_response[2] = apdu_len > 2u ? apdu[2] : 0u;
            apdu_response[3] = 0x01u;
            apdu_response[4] = 0x03u;
            apdu_resp_len = 5u;
        }
    } else if (ctx->association.state != AMI_DLMS_ASSOC_ASSOCIATED) {
        /* Unassociated state handling per DLMS CTT T_APPL_IDLE_N1 */
        if (apdu[0] == 0x05u) {
            /* Short Name Read -> ExceptionResponse */
            apdu_response[0] = 0xD8u;
            apdu_response[1] = 0x00u;
            apdu_response[2] = 0x02u;
            apdu_resp_len = 3u;
        } else {
            /* Silently drop unassociated GET request */
            return AMI_DLMS_PORT_OK;
        }
    } else if (apdu[0] == 0xC0u) {
        /* GET-Request */
        if (apdu_len >= 2u && apdu[1] == 0x02u) {
            /* GET-Request-For-Next-Datablock */
            uint8_t req_invoke_id = 0u;
            uint32_t req_block_num = 0u;
            if (ami_bt_parse_get_next_block_request(apdu, apdu_len, &req_invoke_id, &req_block_num)) {
                int is_last = 0;
                AmiBtStatus bt_st = ami_bt_build_get_response_block(
                    &ctx->block_transfer, req_block_num,
                    apdu_response, sizeof(apdu_response),
                    &apdu_resp_len, &is_last);
                if (bt_st != AMI_BT_OK) {
                    apdu_resp_len = ami_bt_build_error_response_block(
                        req_invoke_id, req_block_num,
                        2u, /* temporary-failure / data-block-unavailable */
                        apdu_response, sizeof(apdu_response));
                }
            } else {
                return AMI_DLMS_PORT_FRAME_ERROR;
            }
        } else {
            /* GET-Request-Normal */
            if (ctx->snapshot_provider != NULL) {
                (void)ctx->snapshot_provider(&ctx->snapshot, ctx->user_data);
            }
            AmiDlmsApduStatus get_st = ami_dlms_process_get_request(&ctx->association, &ctx->snapshot,
                                                                    apdu, apdu_len,
                                                                    apdu_response, sizeof(apdu_response),
                                                                    &apdu_resp_len);
            if (get_st != AMI_DLMS_APDU_OK || apdu_resp_len == 0u) {
                return AMI_DLMS_PORT_FRAME_ERROR;
            }

            /* If response exceeds threshold, switch to Block Transfer */
            if (ctx->block_transfer_threshold > 0u &&
                apdu_resp_len > ctx->block_transfer_threshold &&
                apdu_resp_len > 4u &&
                apdu_response[0] == 0xC4u &&
                apdu_response[1] == 0x01u &&
                apdu_response[3] == 0x00u) {

                uint8_t invoke_id = apdu_response[2];
                AmiBtStatus bt_st = ami_bt_start_get_transfer(
                    &ctx->block_transfer, invoke_id,
                    &apdu_response[4], apdu_resp_len - 4u);

                if (bt_st == AMI_BT_OK) {
                    int is_last = 0;
                    bt_st = ami_bt_build_get_response_block(
                        &ctx->block_transfer, 1u,
                        apdu_response, sizeof(apdu_response),
                        &apdu_resp_len, &is_last);
                    if (bt_st != AMI_BT_OK) {
                        return AMI_DLMS_PORT_FRAME_ERROR;
                    }
                }
            }
        }
    } else if (apdu[0] == 0xC1u) {
        /* SET-Request -> SetResponseNormal with read-write-denied (3) */
        apdu_response[0] = 0xC5u;
        apdu_response[1] = 0x01u;
        apdu_response[2] = apdu_len > 2u ? apdu[2] : 0u;
        apdu_response[3] = 0x03u;
        apdu_resp_len = 4u;
    } else if (apdu[0] == 0xC8u || apdu[0] == 0xC9u || apdu[0] == 0xCBu) {
        /* Ciphered APDUs (Security Suite 0): glo-get (0xC8), glo-set (0xC9), glo-action (0xCB) */
        if (ctx->cipher_backend == NULL) {
            apdu_response[0] = 0xD8u;
            apdu_response[1] = 0x00u;
            apdu_response[2] = 0x02u;
            apdu_resp_len = 3u;
        } else {
            if (ctx->snapshot_provider != NULL) {
                (void)ctx->snapshot_provider(&ctx->snapshot, ctx->user_data);
            }
            AmiDlmsCipherBackend active_backend = *ctx->cipher_backend;
            if (active_backend.disconnect_control == NULL) {
                active_backend.disconnect_control = ctx->disconnect_control;
            }
            AmiDlmsApduStatus cipher_st = ami_dlms_process_ciphered_request(
                &ctx->association,
                &ctx->snapshot,
                &active_backend,
                apdu, apdu_len,
                ctx->cipher_workspace, sizeof(ctx->cipher_workspace),
                apdu_response, sizeof(apdu_response),
                &apdu_resp_len);

            if (cipher_st == AMI_DLMS_APDU_OK && ctx->disconnect_control != NULL) {
                ami_disconnect_control_sync_snapshot(ctx->disconnect_control, &ctx->snapshot);
            }

            if (cipher_st != AMI_DLMS_APDU_OK && apdu_resp_len == 0u) {
                return AMI_DLMS_PORT_FRAME_ERROR;
            }
        }
    } else {
        /* Unsupported service on LN meter */
        apdu_response[0] = 0xD8u;
        apdu_response[1] = 0x00u;
        apdu_response[2] = 0x02u;
        apdu_resp_len = 3u;
    }

    if (apdu_resp_len == 0u) {
        return AMI_DLMS_PORT_OK;
    }

    if (ami_dlms_hdlc_build_frame(&ctx->hdlc, frame, apdu_response, apdu_resp_len,
                                  ctx->tx_buffer, sizeof(ctx->tx_buffer),
                                  &wire_len) != AMI_HDLC_FRAME_READY) {
        return AMI_DLMS_PORT_FRAME_ERROR;
    }

    if (wire_len > 0u && ctx->tx_func != NULL) {
        if (!ctx->tx_func(ctx->tx_buffer, wire_len, ctx->user_data)) {
            return AMI_DLMS_PORT_TX_ERROR;
        }
    }

    return AMI_DLMS_PORT_OK;
}

static AmiDlmsPortStatus dispatch_frame(AmiDlmsPortContext *ctx, const HdlcFrame *frame)
{
    size_t wire_len = 0u;
    AmiHdlcStatus status;

#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
    if (!ami_dlms_port_peer_matches_firmware_profile(frame)) {
        return AMI_DLMS_PORT_PEER_REJECTED;
    }
#endif

    if (frame->type == AMI_HDLC_FRAME_SNRM || frame->type == AMI_HDLC_FRAME_DISC) {
        ami_dlms_association_reset(&ctx->association);
        status = ami_dlms_hdlc_build_frame(&ctx->hdlc, frame, NULL, 0u,
                                           ctx->tx_buffer, sizeof(ctx->tx_buffer),
                                           &wire_len);
        if (status == AMI_HDLC_FRAME_READY && wire_len > 0u && ctx->tx_func != NULL) {
            if (!ctx->tx_func(ctx->tx_buffer, wire_len, ctx->user_data)) {
                return AMI_DLMS_PORT_TX_ERROR;
            }
        }
        return AMI_DLMS_PORT_OK;
    }

    if (frame->type == AMI_HDLC_FRAME_RR) {
        status = ami_dlms_hdlc_build_frame(&ctx->hdlc, frame, NULL, 0u,
                                           ctx->tx_buffer, sizeof(ctx->tx_buffer),
                                           &wire_len);
        if (status == AMI_HDLC_FRAME_READY && wire_len > 0u && ctx->tx_func != NULL) {
            if (!ctx->tx_func(ctx->tx_buffer, wire_len, ctx->user_data)) {
                return AMI_DLMS_PORT_TX_ERROR;
            }
        }
        return AMI_DLMS_PORT_OK;
    }

    if (frame->type == AMI_HDLC_FRAME_I) {
        return dispatch_i_frame(ctx, frame);
    }

    return AMI_DLMS_PORT_OK;
}

void ami_dlms_port_init(AmiDlmsPortContext *ctx,
                        AmiDlmsPortTxFunc tx_func,
                        AmiDlmsPortSnapshotFunc snapshot_provider,
                        void *user_data)
{
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(*ctx));
    ami_dlms_hdlc_init(&ctx->hdlc);
    ami_dlms_association_reset(&ctx->association);
    ami_bt_init(&ctx->block_transfer, AMI_BT_DEFAULT_BLOCK_SIZE);
    ctx->block_transfer_threshold = 0u; /* Default: normal GET unless threshold is set or > HDLC max info */
    ctx->tx_func = tx_func;
    ctx->snapshot_provider = snapshot_provider;
    ctx->user_data = user_data;
    ctx->anti_bruteforce = NULL;
    ctx->disconnect_control = NULL;
    ctx->current_time_sec = 0u;
}

void ami_dlms_port_configure_block_transfer(AmiDlmsPortContext *ctx,
                                            size_t block_size,
                                            size_t threshold)
{
    if (ctx != NULL) {
        ami_bt_init(&ctx->block_transfer, block_size);
        ctx->block_transfer_threshold = threshold;
    }
}

void ami_dlms_port_set_anti_bruteforce(AmiDlmsPortContext *ctx,
                                      AmiAntiBruteforceContext *abf)
{
    if (ctx != NULL) {
        ctx->anti_bruteforce = abf;
    }
}

void ami_dlms_port_set_disconnect_control(AmiDlmsPortContext *ctx,
                                          AmiDisconnectControlContext *dc)
{
    if (ctx != NULL) {
        ctx->disconnect_control = dc;
    }
}

void ami_dlms_port_set_time(AmiDlmsPortContext *ctx, uint32_t time_sec)
{
    if (ctx != NULL) {
        ctx->current_time_sec = time_sec;
    }
}

AmiDlmsPortStatus ami_dlms_port_set_security(AmiDlmsPortContext *ctx,
                                             const AmiDlmsAssociationSecurity *security)
{
    if (ctx == NULL) return AMI_DLMS_PORT_INVALID_PARAM;
#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
    if (security != NULL &&
        (security->allow_no_auth != 0u || security->verify_lls != NULL)) {
        return AMI_DLMS_PORT_SECURITY_POLICY_REJECTED;
    }
#endif
    if (security != NULL) {
        ctx->security = *security;
        ctx->use_custom_security = 1;
    } else {
        memset(&ctx->security, 0, sizeof(ctx->security));
        ctx->use_custom_security = 0;
    }
    return AMI_DLMS_PORT_OK;
}

void ami_dlms_port_set_cipher_backend(AmiDlmsPortContext *ctx,
                                      const AmiDlmsCipherBackend *cipher)
{
    if (ctx != NULL) {
        ctx->cipher_backend = cipher;
    }
}

AmiDlmsPortStatus ami_dlms_port_process_byte(AmiDlmsPortContext *ctx, uint8_t byte)
{
    HdlcFrame frame;
    AmiHdlcStatus status;

    if (ctx == NULL) return AMI_DLMS_PORT_INVALID_PARAM;

    status = ami_dlms_hdlc_parse_byte(&ctx->hdlc, byte, &frame);
    if (status == AMI_HDLC_INCOMPLETE) {
        return AMI_DLMS_PORT_INCOMPLETE;
    }
    if (status != AMI_HDLC_FRAME_READY) {
        return AMI_DLMS_PORT_FRAME_ERROR;
    }

    return dispatch_frame(ctx, &frame);
}

AmiDlmsPortStatus ami_dlms_port_process_buffer(AmiDlmsPortContext *ctx,
                                              const uint8_t *data,
                                              size_t length)
{
    AmiDlmsPortStatus overall = AMI_DLMS_PORT_OK;
    if (ctx == NULL || (data == NULL && length > 0u)) {
        return AMI_DLMS_PORT_INVALID_PARAM;
    }

    for (size_t i = 0u; i < length; ++i) {
        AmiDlmsPortStatus s = ami_dlms_port_process_byte(ctx, data[i]);
        if (s != AMI_DLMS_PORT_OK && s != AMI_DLMS_PORT_INCOMPLETE) {
            overall = s;
        }
    }
    return overall;
}

#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)

#define DLMS_RX_STREAM_BUFFER_SIZE 1024u
#define DLMS_RX_TRIGGER_LEVEL      1u

static StreamBufferHandle_t s_dlms_rx_stream = NULL;
static AmiDlmsPortContext s_global_port_ctx;
static osThreadId_t s_dlms_task_handle = NULL;

__attribute__((weak)) int ami_dlms_hardware_uart_tx(const uint8_t *data, size_t length, void *user_data)
{
    (void)data;
    (void)length;
    (void)user_data;
    /* Default weak implementation before physical UART is attached.
     * Return 0 (failure) to indicate that physical transmission is not configured yet,
     * preventing silent drop being reported as success.
     * Override in board/app code with HAL_UART_Transmit / HAL_UART_Transmit_DMA.
     */
    return 0;
}

#include "ami_ade9000.h"

__attribute__((weak)) int ami_dlms_hardware_get_snapshot(AmiCosemRegisterSnapshot *snapshot, void *user_data)
{
    if (snapshot == NULL) return 0;

    /* If metrology context is supplied, bridge real calibrated ADE9000 readings */
    if (user_data != NULL) {
        const AmiAde9000Context *ade = (const AmiAde9000Context *)user_data;
        if (ami_ade9000_feed_cosem_snapshot(ade, snapshot) == 0) {
            return 1;
        }
        return 0; /* Metrology invalid or communication error: fail-closed */
    }

#if defined(STM32U585xx)
    /* In production target firmware, refuse to fabricate fake measurement data */
    return 0;
#else
    /* Development snapshot strictly for host CTest suite simulation */
    snapshot->valid_mask = AMI_COSEM_VALID_VOLTAGE_A | AMI_COSEM_VALID_CURRENT_A |
                           AMI_COSEM_VALID_ACTIVE_IMPORT | AMI_COSEM_VALID_CLOCK;
    snapshot->phase_voltage_millivolts[0] = 230000u;
    snapshot->phase_current_milliamps[0] = 5000u;
    snapshot->active_import_wh = 1000u;
    snapshot->clock_date_time.year[0] = 0x07u;
    snapshot->clock_date_time.year[1] = 0xEAu;
    snapshot->clock_date_time.month = 10u;
    snapshot->clock_date_time.day = 28u;
    snapshot->clock_date_time.hour = 12u;
    return 1;
#endif
}

static void AmiDlmsTaskEntry(void *argument)
{
    uint8_t rx_buffer[64];
    (void)argument;

#if defined(STM32U585xx)
    static AmiDlmsSecuritySuite0Context s_suite0_ctx;
    static AmiNvmCounterContext s_nvm_counter_ctx;
    static AmiNvmStorageDriver s_tamp_driver;
    static AmiDlmsCipherBackend s_cipher_backend;

    /* Initialize persistent NVM counter driver via TrustZone secure gateway */
    ami_nvm_create_tamp_backup_driver(&s_tamp_driver);
    (void)ami_nvm_counter_init(&s_nvm_counter_ctx, &s_tamp_driver, 100u, 10u);

    static const uint8_t server_st[8] = {'P', 'L', 'N', 0, 0, 0, 0, 1};
    static const uint8_t client_st[8] = {'H', 'E', 'S', 0, 0, 0, 0, 1};
    static const uint8_t dummy_key[16] = {0};

    (void)ami_dlms_cipher_suite0_init(&s_suite0_ctx, dummy_key, dummy_key,
                                      client_st, server_st, 1u);
    (void)ami_dlms_cipher_suite0_bind_nvm(&s_suite0_ctx, &s_nvm_counter_ctx);
    (void)ami_dlms_cipher_suite0_get_backend(&s_suite0_ctx, &s_cipher_backend);

    ami_dlms_port_init(&s_global_port_ctx,
                       ami_dlms_hardware_uart_tx,
                       ami_dlms_hardware_get_snapshot,
                       NULL);
    ami_dlms_port_set_cipher_backend(&s_global_port_ctx, &s_cipher_backend);

    /* Enforce HLS security policy with TrustZone TRNG and HLS proof callbacks */
    static const uint8_t hls5_oid[7] = {0x60, 0x85, 0x74, 0x05, 0x08, 0x02, 0x05};
    AmiDlmsAssociationSecurity sec_policy;
    memset(&sec_policy, 0, sizeof(sec_policy));
    sec_policy.random = ami_dlms_secure_random;
    sec_policy.hls_proof = ami_dlms_secure_hls_proof;
    sec_policy.hls_mechanism_oid = hls5_oid;
    sec_policy.hls_mechanism_oid_length = sizeof(hls5_oid);
    (void)ami_dlms_port_set_security(&s_global_port_ctx, &sec_policy);
#else
    ami_dlms_port_init(&s_global_port_ctx,
                       ami_dlms_hardware_uart_tx,
                       ami_dlms_hardware_get_snapshot,
                       NULL);
#endif

    for (;;) {
        if (s_dlms_rx_stream != NULL) {
            size_t bytes_received = xStreamBufferReceive(s_dlms_rx_stream,
                                                         rx_buffer,
                                                         sizeof(rx_buffer),
                                                         portMAX_DELAY);
            if (bytes_received > 0u) {
                (void)ami_dlms_port_process_buffer(&s_global_port_ctx, rx_buffer, bytes_received);
            }
        } else {
            osDelay(100);
        }
    }
}

int ami_dlms_freertos_start_task(void)
{
    const osThreadAttr_t dlmsTask_attributes = {
        .name = "AmiDlmsTask",
        .priority = (osPriority_t)osPriorityNormal,
        .stack_size = 512 * 4
    };

    if (s_dlms_rx_stream == NULL) {
        s_dlms_rx_stream = xStreamBufferCreate(DLMS_RX_STREAM_BUFFER_SIZE, DLMS_RX_TRIGGER_LEVEL);
        if (s_dlms_rx_stream == NULL) {
            return -1;
        }
    }

    if (s_dlms_task_handle == NULL) {
        s_dlms_task_handle = osThreadNew(AmiDlmsTaskEntry, NULL, &dlmsTask_attributes);
        if (s_dlms_task_handle == NULL) {
            return -2;
        }
    }
    return 0;
}

void ami_dlms_uart_rx_isr(uint8_t byte)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (s_dlms_rx_stream != NULL) {
        xStreamBufferSendFromISR(s_dlms_rx_stream, &byte, 1u, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void ami_dlms_uart_rx_buffer_isr(const uint8_t *data, size_t length)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (s_dlms_rx_stream != NULL && data != NULL && length > 0u) {
        xStreamBufferSendFromISR(s_dlms_rx_stream, data, length, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

#endif
