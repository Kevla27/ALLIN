#include "ami_dlms_port.h"
#include <string.h>

#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "cmsis_os2.h"
#endif

int ami_dlms_port_peer_matches_firmware_profile(const HdlcFrame *frame)
{
    return frame != NULL &&
           frame->destination_sap == 1u &&
           frame->source_sap == 16u;
}

static AmiDlmsPortStatus dispatch_i_frame(AmiDlmsPortContext *ctx, const HdlcFrame *frame)
{
    uint8_t apdu_response[AMI_HDLC_MAX_INFO_LENGTH];
    size_t apdu_resp_len = 0u;
    const uint8_t *apdu = frame->payload;
    const size_t apdu_len = frame->payload_length;
    size_t wire_len = 0u;

    if (apdu_len == 0u) {
        return AMI_DLMS_PORT_OK;
    }

    if (apdu[0] == 0x60u) {
        /* AARQ: Do NOT reset existing association before validating.
         * T_APPL_IDLE_N1 / T_APPL_OPEN: If already associated, unsolicited AARQ is rejected
         * without tearing down the existing authenticated session.
         */
        if (ctx->use_custom_security) {
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
        apdu_response[0] = 0x63u; /* RLRE tag */
        apdu_response[1] = 0x03u; /* length */
        apdu_response[2] = 0x80u; /* reason tag */
        apdu_response[3] = 0x01u;
        apdu_response[4] = 0x00u; /* normal */
        apdu_resp_len = 5u;
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
    } else if (apdu[0] == 0xC1u) {
        /* SET-Request -> SetResponseNormal with read-write-denied (3) */
        apdu_response[0] = 0xC5u;
        apdu_response[1] = 0x01u;
        apdu_response[2] = apdu_len > 2u ? apdu[2] : 0u;
        apdu_response[3] = 0x03u;
        apdu_resp_len = 4u;
    } else if (apdu[0] == 0xC3u) {
        /* Action-Request -> ActionResponseNormal with read-write-denied (3) */
        apdu_response[0] = 0xC7u;
        apdu_response[1] = 0x01u;
        apdu_response[2] = apdu_len > 2u ? apdu[2] : 0u;
        apdu_response[3] = 0x01u;
        apdu_response[4] = 0x03u;
        apdu_resp_len = 5u;
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
            AmiDlmsApduStatus cipher_st = ami_dlms_process_ciphered_request(
                &ctx->association,
                &ctx->snapshot,
                ctx->cipher_backend,
                apdu, apdu_len,
                ctx->cipher_workspace, sizeof(ctx->cipher_workspace),
                apdu_response, sizeof(apdu_response),
                &apdu_resp_len);

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
    ctx->tx_func = tx_func;
    ctx->snapshot_provider = snapshot_provider;
    ctx->user_data = user_data;
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

__attribute__((weak)) int ami_dlms_hardware_get_snapshot(AmiCosemRegisterSnapshot *snapshot, void *user_data)
{
    (void)user_data;
    if (snapshot == NULL) return 0;
    /* Default development snapshot if metrology hardware not connected yet */
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
}

static void AmiDlmsTaskEntry(void *argument)
{
    uint8_t rx_buffer[64];
    (void)argument;

    ami_dlms_port_init(&s_global_port_ctx,
                       ami_dlms_hardware_uart_tx,
                       ami_dlms_hardware_get_snapshot,
                       NULL);

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
