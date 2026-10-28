/**
 * @file console_io.h
 * @brief Modul Abstraksi Konsol Universal, Retargeting printf & Interactive CLI Parser
 *        Sesuai Kebutuhan Pengujian Fabrikasi, Audit Teknis & Pemeliharaan Lapangan
 */

#ifndef CONSOLE_IO_H
#define CONSOLE_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "display.h"
#include "tamper_manager.h"
#include "power_relay_manager.h"
#include "load_profile.h"
#include "board_profile_config.h"

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
/**
 * @brief Konfigurasi Injeksi Beban Metrologi Tiruan (Synthetic Metrology Injection)
 */
typedef struct {
    bool     active;                /**< true: Injeksi beban aktif, false: Pembacaan fisik ADE9000 */
    int32_t  active_power_w;        /**< Daya aktif total (Watt) */
    int32_t  reactive_power_var;    /**< Daya reaktif total (var) */
    uint16_t power_factor_ppm;      /**< Faktor daya (permille, misal 990 = 0.990) */
    uint32_t voltage_r_dv;          /**< Tegangan Fasa R (desiVolt, 2300 = 230.0 V) */
    uint32_t voltage_s_dv;          /**< Tegangan Fasa S (desiVolt) */
    uint32_t voltage_t_dv;          /**< Tegangan Fasa T (desiVolt) */
    uint32_t current_r_ma;          /**< Arus Fasa R (milliAmpere, 10000 = 10.000 A) */
    uint32_t current_s_ma;          /**< Arus Fasa S (milliAmpere) */
    uint32_t current_t_ma;          /**< Arus Fasa T (milliAmpere) */
    uint32_t current_n_ma;          /**< Arus Kawat Netral N (milliAmpere) */
} synthetic_load_config_t;
#endif

#define CONSOLE_CMD_MAX_LEN 128
#define CONSOLE_MAX_ARGS    8

/**
 * @brief Prototipe fungsi callback pengiriman data ke antarmuka fisik (UART/RS485/USB)
 * @param data Pointer ke buffer data yang akan dikirim
 * @param len Panjang data dalam byte
 */
typedef void (*console_tx_fn_t)(const uint8_t *data, size_t len);

/**
 * @brief Struktur hooks layanan meter untuk interaksi CLI dengan seluruh pilar meter
 */
typedef struct {
    const meter_measurements_t* (*get_measurements)(void);
    uint32_t                    (*get_internal_alarm)(void);
    void                        (*set_internal_alarm)(uint32_t mask);
    void                        (*clear_internal_alarm)(uint32_t mask);
    uint32_t                    (*get_battery_voltage)(void);
    void                        (*set_battery_voltage)(uint32_t vbat_mv);
    const tamper_context_t*     (*get_tamper_context)(void);
    void                        (*inject_tamper)(tamper_vector_t vec, bool asserted);
    relay_state_t               (*get_relay_state)(void);
    relay_reason_t              (*get_relay_disconnect_reason)(void);
    bool                        (*relay_disconnect)(relay_reason_t reason);
    bool                        (*relay_request_reconnect)(void);
    bool                        (*relay_confirm_reconnect)(void);
    const max_demand_tracker_t* (*get_max_demand)(void);
    void                        (*reset_max_demand)(void);
    uint32_t                    (*get_time)(void);
    void                        (*set_time)(uint32_t epoch_sec);
    uint8_t                     (*get_active_tariff)(void);
    void                        (*button_press)(button_dir_t dir);
    void                        (*set_ac_fail)(bool is_fail);
    bool                        (*is_ac_failed)(void);
    uint8_t                     (*get_power_mode)(void);
    uint32_t                    (*get_power_wakeup_count)(void);
    uint8_t                     (*get_power_last_wakeup_reason)(void);
#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    void                        (*set_synthetic_load)(const synthetic_load_config_t *cfg);
    void                        (*get_synthetic_load)(synthetic_load_config_t *out_cfg);
#endif
} console_meter_hooks_t;

/**
 * @brief Prototipe fungsi handler perintah CLI
 */
typedef int (*console_cmd_handler_t)(int argc, char *argv[]);

/**
 * @brief Entri tabel perintah CLI
 */
typedef struct {
    const char           *name;
    const char           *desc;
    console_cmd_handler_t handler;
} console_cmd_entry_t;

/* --- Core Console I/O APIs --- */

/**
 * @brief Inisialisasi modul konsol dengan mendaftarkan driver pengiriman fisik.
 * @param tx_func Fungsi callback untuk mentransmisikan data ke hardware.
 */
void console_io_init(console_tx_fn_t tx_func);

/**
 * @brief Mengatur status mute pada konsol (Fitur Pengaman DLMS E2).
 * @param mute true untuk mematikan output konsol, false untuk mengaktifkan kembali.
 */
void console_io_set_mute(bool mute);

/**
 * @brief Memeriksa apakah konsol saat ini dalam status dibungkam (muted).
 * @return true jika konsol dibungkam, false jika aktif.
 */
bool console_io_is_muted(void);

/**
 * @brief Mencetak banner hardware bring-up ke konsol
 * @param system_title Judul sistem / MCU (NULL untuk default)
 * @param uart_desc Deskripsi port UART (NULL untuk default)
 * @param sysclk_mhz Frekuensi core clock sistem dalam MHz (misal 160)
 */
void console_io_print_boot_banner(const char *system_title, const char *uart_desc, uint32_t sysclk_mhz);

/* --- Interactive Serial CLI APIs --- */

/**
 * @brief Inisialisasi parser perintah CLI interaktif dengan menyematkan hooks meter.
 * @param hooks Pointer ke tabel hook akses meter (dapat NULL jika unhooked).
 */
void console_cli_init(const console_meter_hooks_t *hooks);

/**
 * @brief Memproses 1 byte karakter masukan serial (RX) dari terminal.
 *        Mendukung penanganan backspace, echo, dan pemrosesan baris saat CR/LF.
 * @param byte Karakter masukan (ASCII).
 */
void console_cli_process_rx_byte(uint8_t byte);

/**
 * @brief Memproses buffer stream masukan serial.
 * @param buf Pointer ke buffer data masukan.
 * @param len Jumlah byte data masukan.
 */
void console_cli_process_rx_buffer(const uint8_t *buf, size_t len);

/**
 * @brief Mengeksekusi satu baris perintah string secara langsung (tanpa buffering RX).
 * @param cmd_line String perintah lengkap (null-terminated).
 * @return 0 jika sukses, kode error negatif bila gagal.
 */
int console_cli_execute_command(char *cmd_line);

/**
 * @brief Mencetak prompt konsol serial (`SMART-METER> `).
 */
void console_cli_print_prompt(void);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_IO_H */
