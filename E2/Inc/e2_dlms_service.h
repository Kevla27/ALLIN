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
 * @brief Callback interrupt RX UART: hanya menyalin 1 byte ke ring buffer lock-free ISR-safe & re-arm IRQ
 * @param huart Pointer ke UART handle yang memicu interupsi
 */
void e2_dlms_service_uart_rx_callback(UART_HandleTypeDef *huart);

/**
 * @brief Callback pemulihan error UART (ORE/FE/NE) agar penerimaan RX_IT tetap berjalan
 * @param huart Pointer ke UART handle yang mengalami error
 */
void e2_dlms_service_uart_error_callback(UART_HandleTypeDef *huart);

/**
 * @brief Polling protokol DLMS/HDLC & jabat tangan IEC 62056-21 Mode E dari konteks thread (DLMSTask)
 */
void e2_dlms_service_poll(void);

/**
 * @brief Mengaktifkan/menonaktifkan mode penangkapan balasan AT-Command modem seluler pada USART2
 */
void e2_dlms_service_set_modem_at_active(bool active);

/**
 * @brief Mengirim string AT-Command ke modem seluler (USART2) dan mengaktifkan penangkapan balasan AT
 */
bool e2_dlms_service_send_modem_at(const char *at_cmd);

/**
 * @brief Mengambil 1 byte balasan ASCII AT-Command modem dari FIFO khusus ModemTask
 */
bool e2_dlms_service_pop_modem_rx_byte(uint8_t *out_byte);

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
