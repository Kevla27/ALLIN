/**
 * @file e2_dlms_service.h
 * @brief Layanan Terpadu Pilar E2: Protokol DLMS/COSEM, Driver Port Optik & Modem, dan Snapshot OBIS
 */

#ifndef E2_DLMS_SERVICE_H
#define E2_DLMS_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ami_cosem_model.h"
#include "ami_dlms_hdlc.h"
#include "ami_dlms_association.h"
#include "ami_dlms_apdu.h"
#include "display.h"
#include "main.h"

/**
 * @brief Inisialisasi stack DLMS, parser HDLC, asosiasi, dan buffer RX port optik (USART3)
 * @param optical_huart Pointer ke UART handle untuk port optik IEC 62056-21 (USART3 @ 9600 bps)
 */
void e2_dlms_service_init(UART_HandleTypeDef *optical_huart);

/**
 * @brief Mendaftarkan UART handle untuk port modem seluler/RS-232 (USART2 @ 19200 bps)
 * @param modem_huart Pointer ke UART handle modem (USART2)
 */
void e2_dlms_service_set_modem_uart(UART_HandleTypeDef *modem_huart);

/**
 * @brief Memperbarui snapshot register COSEM/OBIS dari data pengukuran meter
 * @param[out] snapshot Pointer ke objek snapshot register COSEM
 * @param[in]  meas Pointer ke data pengukuran metrologi
 */
void e2_dlms_service_sync_snapshot(AmiCosemRegisterSnapshot *snapshot, const meter_measurements_t *meas);

/**
 * @brief Callback penerimaan byte UART port optik maupun modem (Full HDLC -> AARQ -> GET/RLRQ pipeline)
 * @param huart Pointer ke UART handle yang memicu interupsi
 */
void e2_dlms_service_uart_rx_callback(UART_HandleTypeDef *huart);

/**
 * @brief Mengirimkan paket darurat Last-Gasp DLMS Event Notification (0xC2) melalui port modem saat listrik padam
 * @param timestamp_epoch Timestamp UNIX saat kejadian padam (AC_FAIL_DET)
 * @param active_import_wh Akumulasi energi aktif saat ini (Wh)
 * @return true jika berhasil ditransmisikan ke UART modem/optik
 */
bool e2_dlms_send_last_gasp_push(uint32_t timestamp_epoch, uint64_t active_import_wh);

/**
 * @brief Mendapatkan pointer ke objek snapshot global DLMS/COSEM
 */
AmiCosemRegisterSnapshot *e2_dlms_service_get_snapshot(void);

/**
 * @brief Mengambil 1 byte karakter ASCII non-HDLC dari antrean RX UART konsol/modem (USART2)
 * @param[out] out_byte Pointer ke variabel penerima byte
 * @return true jika ada byte tersedia di FIFO, false jika kosong
 */
bool e2_dlms_service_pop_cli_rx_byte(uint8_t *out_byte);

#ifdef __cplusplus
}
#endif

#endif /* E2_DLMS_SERVICE_H */
