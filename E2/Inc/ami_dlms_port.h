#ifndef AMI_DLMS_PORT_H
#define AMI_DLMS_PORT_H

#include <stddef.h>
#include <stdint.h>
#include "ami_cosem_model.h"
#include "ami_dlms_apdu.h"
#include "ami_dlms_association.h"
#include "ami_dlms_hdlc.h"
#include "ami_anti_bruteforce.h"
#include "ami_dlms_block_transfer.h"
#include "ami_disconnect_control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AMI_DLMS_PORT_OK = 0,
    AMI_DLMS_PORT_INCOMPLETE,
    AMI_DLMS_PORT_FRAME_ERROR,
    AMI_DLMS_PORT_TX_ERROR,
    AMI_DLMS_PORT_INVALID_PARAM,
    AMI_DLMS_PORT_SECURITY_POLICY_REJECTED,
    AMI_DLMS_PORT_PEER_REJECTED
} AmiDlmsPortStatus;

/**
 * @brief Callback to send raw wire bytes to physical interface (e.g. UART).
 * @return 1 on success, 0 on failure.
 */
typedef int (*AmiDlmsPortTxFunc)(const uint8_t *data, size_t length, void *user_data);

/**
 * @brief Callback to obtain latest metrology snapshot.
 * @return 1 on success, 0 on failure.
 */
typedef int (*AmiDlmsPortSnapshotFunc)(AmiCosemRegisterSnapshot *snapshot, void *user_data);

typedef struct {
    HdlcContext hdlc;
    AmiDlmsAssociation association;
    AmiDlmsAssociationSecurity security;
    int use_custom_security;
    const AmiDlmsCipherBackend *cipher_backend;
    uint8_t cipher_workspace[AMI_HDLC_MAX_FRAME_BYTES];
    AmiCosemRegisterSnapshot snapshot;
    AmiDlmsPortTxFunc tx_func;
    AmiDlmsPortSnapshotFunc snapshot_provider;
    void *user_data;
    AmiAntiBruteforceContext *anti_bruteforce;
    AmiDisconnectControlContext *disconnect_control;
    uint32_t current_time_sec;
    AmiBlockTransferContext block_transfer;
    size_t block_transfer_threshold;
    uint8_t tx_buffer[AMI_HDLC_MAX_FRAME_BYTES];
} AmiDlmsPortContext;

/**
 * @brief Initialize the DLMS Port context.
 */
void ami_dlms_port_init(AmiDlmsPortContext *ctx,
                        AmiDlmsPortTxFunc tx_func,
                        AmiDlmsPortSnapshotFunc snapshot_provider,
                        void *user_data);

/**
 * @brief Configure Block Transfer chunk size and activation threshold.
 * @param ctx Port context
 * @param block_size Max chunk size per datablock (e.g. 128 bytes)
 * @param threshold Size above which GET-Response transitions to Datablock (0 to disable)
 */
void ami_dlms_port_configure_block_transfer(AmiDlmsPortContext *ctx,
                                            size_t block_size,
                                            size_t threshold);

/**
 * @brief Configure Anti-Brute-Force protection manager for the port.
 */
void ami_dlms_port_set_anti_bruteforce(AmiDlmsPortContext *ctx,
                                      AmiAntiBruteforceContext *abf);

/**
 * @brief Configure Disconnect Control (Breaker) manager for the port.
 */
void ami_dlms_port_set_disconnect_control(AmiDlmsPortContext *ctx,
                                          AmiDisconnectControlContext *dc);

/**
 * @brief Set current virtual/real timestamp (seconds) for timer calculations.
 */
void ami_dlms_port_set_time(AmiDlmsPortContext *ctx, uint32_t time_sec);

/**
 * @brief Configure cipher backend for handling encrypted APDUs (glo-get, glo-set, glo-action).
 */
void ami_dlms_port_set_cipher_backend(AmiDlmsPortContext *ctx,
                                      const AmiDlmsCipherBackend *cipher);

/**
 * @brief Check the selected firmware HDLC peer and server addresses.
 */
int ami_dlms_port_peer_matches_firmware_profile(const HdlcFrame *frame);

/**
 * @brief Configure security policy for the port association.
 */
AmiDlmsPortStatus ami_dlms_port_set_security(AmiDlmsPortContext *ctx,
                                             const AmiDlmsAssociationSecurity *security);

/**
 * @brief Process a single byte received from physical UART / transport.
 */
AmiDlmsPortStatus ami_dlms_port_process_byte(AmiDlmsPortContext *ctx, uint8_t byte);

/**
 * @brief Process a buffer of bytes received from physical UART / DMA.
 */
AmiDlmsPortStatus ami_dlms_port_process_buffer(AmiDlmsPortContext *ctx,
                                              const uint8_t *data,
                                              size_t length);

#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
/**
 * @brief Start the FreeRTOS DLMS communication task and initialize stream buffers.
 * @return 0 on success, negative value on error.
 */
int ami_dlms_freertos_start_task(void);

/**
 * @brief Interrupt-safe function to push received UART byte into FreeRTOS DLMS stream.
 */
void ami_dlms_uart_rx_isr(uint8_t byte);

/**
 * @brief Interrupt-safe function to push received UART buffer into FreeRTOS DLMS stream.
 */
void ami_dlms_uart_rx_buffer_isr(const uint8_t *data, size_t length);
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMI_DLMS_PORT_H */
