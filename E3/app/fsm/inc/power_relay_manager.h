/**
 * @file power_relay_manager.h
 * @brief Pengelola Asimetri Beban, Pembatas Daya (Power Limiter), Proteksi Tegangan (Cut-Off),
 *        dan State Relai "READY ON" Sesuai Standar SPLN D3.006: 2021 Tabel 5 & Butir 10.3 Tabel 9
 */

#ifndef POWER_RELAY_MANAGER_H
#define POWER_RELAY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ambang Batas Proteksi Cut-Off Tegangan Pasok (SPLN D3.006: 2021 Butir 10.3 Tabel 9)
 * Satuan dalam deci-Volt (0.1 V)
 */
#define SPLN_VOLT_UNDER_CUTOFF_DV    1100U /**< < 110.0 V: Relai Membuka Serempak Semua Fasa */
#define SPLN_VOLT_UNDER_RECLOSE_DV   1150U /**< > 115.0 V: Relai Menutup Kembali (Histeresis 5 V) */
#define SPLN_VOLT_OVER_CUTOFF_DV     2800U /**< > 280.0 V: Relai Membuka Serempak Semua Fasa */
#define SPLN_VOLT_OVER_RECLOSE_DV    2760U /**< < 276.0 V: Relai Menutup Kembali (Histeresis 4 V) */

/**
 * @brief Status Kontak Relai (Bistable Latching Relay)
 */
typedef enum {
    RELAY_STATE_CONNECTED = 0,    /**< Relai tertutup / tersambung (Normal) */
    RELAY_STATE_DISCONNECTED,     /**< Relai terbuka / terputus (Trip / Remote Open / Cut-Off) */
    RELAY_STATE_READY_ON          /**< Perintah Reconnect diterima, menunggu aksi lokal (SPLN Tabel 5 No 12) */
} relay_state_t;

/**
 * @brief Alasan Terakhir Perubahan Status Relai
 */
typedef enum {
    RELAY_REASON_NONE = 0,
    RELAY_REASON_MANUAL,            /**< Perintah manual / CLI */
    RELAY_REASON_POWER_LIMITER,     /**< Trip Pembatas Daya (SPLN Tabel 5 No 13) */
    RELAY_REASON_REMOTE_DISCONNECT, /**< Perintah Remote HES/DLMS */
    RELAY_REASON_REMOTE_RECONNECT,  /**< Perintah Reconnect HES/DLMS */
    RELAY_REASON_LOCAL_CONFIRM,     /**< Konfirmasi tombol lokal di meter */
    RELAY_REASON_UNDER_VOLTAGE,     /**< Cut-Off Tegangan Rendah < 110V (SPLN Butir 10.3) */
    RELAY_REASON_OVER_VOLTAGE       /**< Cut-Off Tegangan Lebih > 280V (SPLN Butir 10.3) */
} relay_reason_t;

/**
 * @brief Struktur Konfigurasi & Status Power & Relay Manager
 */
typedef struct {
    /* Konfigurasi Ambang Batas */
    uint32_t power_limit_w;            /**< Ambang batas daya aktif (Watt) - Default 6600 W */
    uint32_t over_power_duration_sec;  /**< Durasi toleransi kelebihan daya (Detik) - Default 5 s */
    uint32_t asymmetry_threshold_pct;  /**< Ambang batas asimetri arus 3-fase (%) - Default 30% */

    /* Status Operasional */
    relay_state_t  relay_state;          /**< Status relai saat ini */
    relay_reason_t last_reason;          /**< Alasan perubahan status relai */
    bool           is_asymmetry_active;  /**< Flag deteksi beban asimetris */
    uint32_t       current_deviation_pct;/**< Nilai deviasi arus 3-fase saat ini (%) */
    bool           is_limiter_tripped;   /**< Flag kelebihan daya (Power Limiter tripped) */
    uint32_t       over_power_timer_sec; /**< Akumulator durasi kelebihan daya */
    uint32_t       last_event_timestamp; /**< Waktu event terakhir (Epoch UNIX) */

    /* Status Proteksi Tegangan (SPLN Butir 10.3) */
    bool           is_under_voltage_trip;/**< Flag relai trip akibat tegangan < 110V */
    bool           is_over_voltage_trip; /**< Flag relai trip akibat tegangan > 280V */
} power_relay_context_t;

/**
 * @brief Inisialisasi Power & Relay Manager dengan parameter default SPLN
 */
void power_relay_init(power_relay_context_t *ctx, uint32_t power_limit_w, uint32_t trip_sec, uint32_t asym_pct);

/**
 * @brief Evaluasi Asimetri Arus 3-Fase sesuai formula SPLN Tabel 5 No 11
 *        Formula: Deviasi Maksimum / Rata-rata Arus x 100%
 * @param ctx Pointer ke konteks
 * @param i_r_ma Arus fasa R (mA)
 * @param i_s_ma Arus fasa S (mA)
 * @param i_t_ma Arus fasa T (mA)
 * @return true jika terjadi asimetri daya (> threshold 30%), false jika normal
 */
bool power_relay_check_asymmetry(power_relay_context_t *ctx, uint32_t i_r_ma, uint32_t i_s_ma, uint32_t i_t_ma);

/**
 * @brief Evaluasi Daya Aktif terhadap Pembatas Daya (Power Limiter) SPLN Tabel 5 No 13
 * @param ctx Pointer ke konteks
 * @param active_power_w Daya aktif total (Watt)
 * @param timestamp Waktu kejadian (Epoch UNIX)
 * @return true jika terjadi trip pemutusan relai, false jika normal
 */
bool power_relay_process_power(power_relay_context_t *ctx, int32_t active_power_w, uint32_t timestamp);

/**
 * @brief Evaluasi Proteksi Cut-off Tegangan Rendah & Lebih (SPLN D3.006: 2021 Butir 10.3 Tabel 9)
 *        - Tegangan < 110.0 V: Relai membuka serempak semua fasa
 *        - Tegangan > 115.0 V: Relai menutup kembali (histeresis 5 V)
 *        - Tegangan > 280.0 V: Relai membuka serempak semua fasa
 *        - Tegangan < 276.0 V: Relai menutup kembali (histeresis 4 V)
 * @param ctx Pointer ke konteks
 * @param v_r_dv Tegangan fasa R dalam desi-volt (0.1 V)
 * @param v_s_dv Tegangan fasa S dalam desi-volt (0.1 V)
 * @param v_t_dv Tegangan fasa T dalam desi-volt (0.1 V)
 * @param timestamp Waktu kejadian (Epoch UNIX)
 * @return true jika relai di-trip akibat anomali tegangan, false jika normal
 */
bool power_relay_process_voltage(power_relay_context_t *ctx, uint32_t v_r_dv, uint32_t v_s_dv, uint32_t v_t_dv, uint32_t timestamp);

/**
 * @brief Pemutusan relai (Disconnect)
 */
void power_relay_disconnect(power_relay_context_t *ctx, relay_reason_t reason, uint32_t timestamp);

/**
 * @brief Permintaan penyambungan kembali relai via remote DLMS (Remote Reconnect)
 *        Transisi ke state RELAY_STATE_READY_ON (SPLN Tabel 5 No 12)
 */
void power_relay_request_reconnect(power_relay_context_t *ctx, uint32_t timestamp);

/**
 * @brief Aksi konfirmasi lokal pengguna (tekan tombol di meter) untuk menyambungkan relai
 *        Transisi dari RELAY_STATE_READY_ON ke RELAY_STATE_CONNECTED
 * @return true jika berhasil tersambung, false jika tidak dalam state READY ON
 */
bool power_relay_confirm_reconnect(power_relay_context_t *ctx, uint32_t timestamp);

/**
 * @brief Mengambil status relai saat ini
 */
relay_state_t power_relay_get_state(const power_relay_context_t *ctx);

/**
 * @brief Memeriksa apakah relai berada dalam state "READY ON"
 */
bool power_relay_is_ready_on(const power_relay_context_t *ctx);

/**
 * @brief Konversi status relai ke string untuk display / log
 */
const char* power_relay_state_to_string(relay_state_t state);

/**
 * @brief Konversi alasan pemutusan/perubahan relai ke string
 */
const char* power_relay_reason_to_string(relay_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif /* POWER_RELAY_MANAGER_H */
