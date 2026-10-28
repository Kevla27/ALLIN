/**
 * @file board_profile_config.h
 * @brief Konfigurasi Profil Papan Perangkat Keras (Hardware Board Profile Configuration)
 * @details Menyediakan lapisan abstraksi konfigurasi terpusat untuk memisahkan logika
 *          aplikasi meter (Pilar E3) dan protokol DLMS (Pilar E2) dari variasi fisik
 *          komponen perangkat keras (misal: OLED vs Segment LCD, SPI Flash vs EEPROM,
 *          tipe koil relai, polaritas sensor tamper, dan baudrate port komunikasi).
 *
 * Sesuai Standar SPLN D3.006: 2021 & Spesifikasi Perangkat Keras Meter 3-Fasa.
 */

#ifndef BOARD_PROFILE_CONFIG_H
#define BOARD_PROFILE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * 1. DEFINISI PILIHAN PROFIL PAPAN (BOARD PROFILE SELECTOR)
 * ==================================================================== */
#define BOARD_PROFILE_PROTOTYPE_OLED     1  /**< Papan purwarupa/dev board: Layar OLED 128x32 SSD1306, SPI Flash W25Qxx */
#define BOARD_PROFILE_PRODUCTION_SEGLCD  2  /**< Papan produksi massal: Kaca Segment LCD khusus (HT1621), I2C EEPROM/FRAM */
#define BOARD_PROFILE_HEADLESS_BENCH     3  /**< Mode pengujian pabrik/test bench tanpa layar fisik (Serial CLI only) */

/* Tentukan profil papan aktif di sini (atau lewat build flag compiler -DACTIVE_BOARD_PROFILE=...) */
#ifndef ACTIVE_BOARD_PROFILE
#define ACTIVE_BOARD_PROFILE BOARD_PROFILE_PROTOTYPE_OLED
#endif

/* ====================================================================
 * 2. PROFIL 1: PAPAN PURWARUPA (PROTOTYPE DEV BOARD - OLED SSD1306)
 * ==================================================================== */
#if (ACTIVE_BOARD_PROFILE == BOARD_PROFILE_PROTOTYPE_OLED)

#define BOARD_NAME_STRING               "ALLIN_STM32U585_REV_A"
#define BOARD_HARDWARE_REVISION_MAJOR   1
#define BOARD_HARDWARE_REVISION_MINOR   0

/* --- A. Display Backend (Layar) --- */
#define BOARD_CFG_DISPLAY_BACKEND_OLED  1
#define BOARD_CFG_DISPLAY_BACKEND_SEGLCD 0
#define BOARD_CFG_DISPLAY_I2C_ADDR      0x78U      /**< Alamat 8-bit I2C SSD1306 (0x3C << 1) */
#define BOARD_CFG_DISPLAY_WIDTH         128U
#define BOARD_CFG_DISPLAY_HEIGHT        32U
#define BOARD_CFG_DISPLAY_WAKE_MS       10000U     /**< Layar menyala 10 detik saat listrik padam jika tombol ditekan */
#define BOARD_CFG_DISPLAY_CAROUSEL_MS   5000U      /**< Interval rotasi halaman carousel (5 detik per SPLN) */
#define BOARD_CFG_DISPLAY_ERR_ROTATE_MS 3000U      /**< Interval rotasi kode multi-error (3 detik per SPLN LOG-021) */

/* --- B. Power Relay Subsystem --- */
#define BOARD_CFG_RELAY_DUAL_COIL       1          /**< 1 = Dual-coil latching (SET & RESET terpisah), 0 = Single-coil */
#define BOARD_CFG_RELAY_PULSE_MS        100U       /**< Lebar pulsa koil penggerak (100 ms) */
#define BOARD_CFG_RELAY_FB_CLOSED_LVL   1          /**< 1 = GPIO_PIN_SET saat kontak tertutup, 0 = Active-low */
#define BOARD_CFG_RELAY_UNDERVOLT_DV    1100U      /**< Ambang batas cut-off tegangan rendah (110.0V per SPLN LOG-025) */
#define BOARD_CFG_RELAY_OVERVOLT_DV     2800U      /**< Ambang batas cut-off tegangan tinggi (280.0V per SPLN LOG-025) */
#define BOARD_CFG_RELAY_UNDERVOLT_MS    3000U      /**< Durasi toleransi tegangan rendah sebelum trip (3 detik) */
#define BOARD_CFG_RELAY_RECONNECT_MS    5000U      /**< Durasi tegangan stabil sebelum auto-reconnect (5 detik) */
#define BOARD_CFG_RELAY_DUAL_BTN_MS     1000U      /**< Durasi penekanan tombol simultan konfirmasi reconnect (1 detik) */

/* --- C. Tamper Sensors Polarity & Debounce --- */
#define BOARD_CFG_TAMPER_CASE_OPEN_LVL  1          /**< 1 = GPIO_PIN_SET saat tutup meter dibuka (Normally Open switch) */
#define BOARD_CFG_TAMPER_TERM_OPEN_LVL  1          /**< 1 = GPIO_PIN_SET saat tutup terminal dibuka */
#define BOARD_CFG_TAMPER_MAGNET_LVL     1          /**< 1 = GPIO_PIN_SET saat terdeteksi medan magnet eksternal 500 mT */
#define BOARD_CFG_TAMPER_DEBOUNCE_MS    100U       /**< Filter debounce sakelar mekanik (100 ms) */
#define BOARD_CFG_TAMPER_MAG_PERSIST_MS 3000U      /**< Waktu verifikasi medan magnet stabil sebelum alarm aktif (3 detik) */
#define BOARD_CFG_TAMPER_MAG_CLEAR_MS   10000U     /**< Waktu pemulihan medan magnet hilang sebelum alarm pulih (10 detik) */

/* --- D. Non-Volatile Storage (NVRAM) --- */
#define BOARD_CFG_NVRAM_STORAGE_SPI_FLASH 1        /**< Menggunakan W25Qxx SPI2 Flash eksternal */
#define BOARD_CFG_NVRAM_STORAGE_EEPROM    0
#define BOARD_CFG_LOAD_PROFILE_CAPACITY   100U     /**< Kapasitas ring buffer profil beban 15-menit */
#define BOARD_CFG_FIFO_LOG_CAPACITY       30U      /**< Kapasitas masing-masing FIFO (Alarm, Event, Tamper min 30 entri) */
#define BOARD_CFG_BILLING_SNAPSHOT_MAX    12U      /**< Kapasitas snapshot penutupan tagihan bulanan (12 bulan) */

/* --- E. Audio Buzzer --- */
#define BOARD_CFG_BUZZER_ALARM_ON_MS    200U       /**< Denyut 200 ms ON (SPLN Butir 6.14) */
#define BOARD_CFG_BUZZER_ALARM_OFF_MS   200U       /**< Denyut 200 ms OFF (SPLN Butir 6.14) */
#define BOARD_CFG_BUZZER_KEY_BEEP_MS    50U        /**< Feedback suara 50 ms penekanan tombol (SPLN Butir 6.10) */

/* --- F. Communication Ports --- */
#define BOARD_CFG_OPTICAL_BAUDRATE      9600U      /**< Port optik lokal IEC 62056-21 (USART3) */
#define BOARD_CFG_MODEM_BAUDRATE        19200U     /**< Port modem seluler jarak jauh (USART2) */
#define BOARD_CFG_CLI_BAUDRATE          115200U    /**< Port serial konsol interaktif / CLI */

/* --- G. Metrology & Analog Telemetry --- */
#define BOARD_CFG_METROLOGY_AFE_ADE9000 1          /**< Menggunakan chip analog front-end ADE9000 */
#define BOARD_CFG_NOMINAL_VOLTAGE_DV    2200U      /**< Tegangan nominal 220.0 V (skala 0.1V) */
#define BOARD_CFG_NOMINAL_FREQ_MHZ      50000U     /**< Frekuensi nominal 50.00 Hz (skala 0.001Hz) */
#define BOARD_CFG_ANTI_CREEP_WATTS      3U         /**< Ambang batas tanpa beban / anti-creep (3 Watt) */
#define BOARD_CFG_ADC4_BATT_LOW_MV      2700U      /**< Ambang batas alarm baterai lemah ERR03 (<=2.7V per PIN.docx & SPLN Tabel 4) */
#define BOARD_CFG_ADC4_SUPERCAP_LOW_MV  2000U      /**< Ambang batas alarm superkapasitor lemah ERR06 (<2.0V) */

/* --- H. Power Saving & Ultra-Low-Power Stop 2 Mode --- */
#define BOARD_CFG_ENABLE_STOP2_MODE        1          /**< 1 = Masuk Stop 2 Mode saat listrik padam & layar padam (~4.5 uA) */
#define BOARD_CFG_STOP2_WAKEUP_CLOCK_MSI   1          /**< 1 = Berjalan di MSIS 4MHz saat bangun di mode baterai */
#define BOARD_CFG_RTC_WAKEUP_PERIOD_SEC    60U        /**< Interval periodik RTC Wakeup Timer (opsional, 60 detik) */

/* ====================================================================
 * 3. PROFIL 2: PAPAN PRODUKSI MASSAL (PRODUCTION - CUSTOM SEGMENT LCD)
 * ==================================================================== */
#elif (ACTIVE_BOARD_PROFILE == BOARD_PROFILE_PRODUCTION_SEGLCD)

#define BOARD_NAME_STRING               "ALLIN_STM32U585_REV_B_PROD"
#define BOARD_HARDWARE_REVISION_MAJOR   2
#define BOARD_HARDWARE_REVISION_MINOR   0

/* --- A. Display Backend (Kaca Segment LCD Khusus HT1621) --- */
#define BOARD_CFG_DISPLAY_BACKEND_OLED  0
#define BOARD_CFG_DISPLAY_BACKEND_SEGLCD 1
#define BOARD_CFG_DISPLAY_I2C_ADDR      0x00U      /**< Antarmuka 3-wire serial HT1621 (CS/WR/DATA) */
#define BOARD_CFG_DISPLAY_WIDTH         8U         /**< 8 Digit 7-Segment Utama */
#define BOARD_CFG_DISPLAY_HEIGHT        1U
#define BOARD_CFG_DISPLAY_WAKE_MS       10000U
#define BOARD_CFG_DISPLAY_CAROUSEL_MS   5000U
#define BOARD_CFG_DISPLAY_ERR_ROTATE_MS 3000U

/* --- B. Power Relay Subsystem (Single-Coil H-Bridge Relai) --- */
#define BOARD_CFG_RELAY_DUAL_COIL       0          /**< 0 = Single-coil H-Bridge latching */
#define BOARD_CFG_RELAY_PULSE_MS        80U        /**< Efisiensi pulsa koil 80 ms */
#define BOARD_CFG_RELAY_FB_CLOSED_LVL   0          /**< Active-low kontak bantu NC */
#define BOARD_CFG_RELAY_UNDERVOLT_DV    1100U
#define BOARD_CFG_RELAY_OVERVOLT_DV     2800U
#define BOARD_CFG_RELAY_UNDERVOLT_MS    3000U
#define BOARD_CFG_RELAY_RECONNECT_MS    5000U
#define BOARD_CFG_RELAY_DUAL_BTN_MS     1000U

/* --- C. Tamper Sensors Polarity & Debounce --- */
#define BOARD_CFG_TAMPER_CASE_OPEN_LVL  0          /**< Active-low NC microswitch */
#define BOARD_CFG_TAMPER_TERM_OPEN_LVL  0          /**< Active-low NC microswitch */
#define BOARD_CFG_TAMPER_MAGNET_LVL     0          /**< Active-low open-drain Hall IC */
#define BOARD_CFG_TAMPER_DEBOUNCE_MS    100U
#define BOARD_CFG_TAMPER_MAG_PERSIST_MS 3000U
#define BOARD_CFG_TAMPER_MAG_CLEAR_MS   10000U

/* --- D. Non-Volatile Storage (I2C EEPROM AT24C512 / FRAM) --- */
#define BOARD_CFG_NVRAM_STORAGE_SPI_FLASH 0
#define BOARD_CFG_NVRAM_STORAGE_EEPROM    1
#define BOARD_CFG_LOAD_PROFILE_CAPACITY   100U
#define BOARD_CFG_FIFO_LOG_CAPACITY       30U
#define BOARD_CFG_BILLING_SNAPSHOT_MAX    12U

/* --- E. Audio Buzzer --- */
#define BOARD_CFG_BUZZER_ALARM_ON_MS    200U
#define BOARD_CFG_BUZZER_ALARM_OFF_MS   200U
#define BOARD_CFG_BUZZER_KEY_BEEP_MS    50U

/* --- F. Communication Ports --- */
#define BOARD_CFG_OPTICAL_BAUDRATE      9600U
#define BOARD_CFG_MODEM_BAUDRATE        19200U
#define BOARD_CFG_CLI_BAUDRATE          115200U

/* --- G. Metrology & Analog Telemetry --- */
#define BOARD_CFG_METROLOGY_AFE_ADE9000 1
#define BOARD_CFG_NOMINAL_VOLTAGE_DV    2200U
#define BOARD_CFG_NOMINAL_FREQ_MHZ      50000U
#define BOARD_CFG_ANTI_CREEP_WATTS      3U
#define BOARD_CFG_ADC4_BATT_LOW_MV      2700U
#define BOARD_CFG_ADC4_SUPERCAP_LOW_MV  2000U

/* --- H. Power Saving & Ultra-Low-Power Stop 2 Mode --- */
#define BOARD_CFG_ENABLE_STOP2_MODE        1
#define BOARD_CFG_STOP2_WAKEUP_CLOCK_MSI   1
#define BOARD_CFG_RTC_WAKEUP_PERIOD_SEC    60U

/* ====================================================================
 * 4. PROFIL 3: MODE TEST BENCH / CI HEADLESS (TANPA DISPLAY FISIK)
 * ==================================================================== */
#else

#define BOARD_NAME_STRING               "ALLIN_HEADLESS_TEST_PROFILE"
#define BOARD_HARDWARE_REVISION_MAJOR   0
#define BOARD_HARDWARE_REVISION_MINOR   1

#define BOARD_CFG_DISPLAY_BACKEND_OLED  0
#define BOARD_CFG_DISPLAY_BACKEND_SEGLCD 0
#define BOARD_CFG_DISPLAY_I2C_ADDR      0x00U
#define BOARD_CFG_DISPLAY_WIDTH         0U
#define BOARD_CFG_DISPLAY_HEIGHT        0U
#define BOARD_CFG_DISPLAY_WAKE_MS       10000U
#define BOARD_CFG_DISPLAY_CAROUSEL_MS   5000U
#define BOARD_CFG_DISPLAY_ERR_ROTATE_MS 3000U

#define BOARD_CFG_RELAY_DUAL_COIL       1
#define BOARD_CFG_RELAY_PULSE_MS        100U
#define BOARD_CFG_RELAY_FB_CLOSED_LVL   1
#define BOARD_CFG_RELAY_UNDERVOLT_DV    1100U
#define BOARD_CFG_RELAY_OVERVOLT_DV     2800U
#define BOARD_CFG_RELAY_UNDERVOLT_MS    3000U
#define BOARD_CFG_RELAY_RECONNECT_MS    5000U
#define BOARD_CFG_RELAY_DUAL_BTN_MS     1000U

#define BOARD_CFG_TAMPER_CASE_OPEN_LVL  1
#define BOARD_CFG_TAMPER_TERM_OPEN_LVL  1
#define BOARD_CFG_TAMPER_MAGNET_LVL     1
#define BOARD_CFG_TAMPER_DEBOUNCE_MS    50U
#define BOARD_CFG_TAMPER_MAG_PERSIST_MS 3000U
#define BOARD_CFG_TAMPER_MAG_CLEAR_MS   10000U

#define BOARD_CFG_NVRAM_STORAGE_SPI_FLASH 0
#define BOARD_CFG_NVRAM_STORAGE_EEPROM    0
#define BOARD_CFG_LOAD_PROFILE_CAPACITY   100U
#define BOARD_CFG_FIFO_LOG_CAPACITY       30U
#define BOARD_CFG_BILLING_SNAPSHOT_MAX    12U

#define BOARD_CFG_BUZZER_ALARM_ON_MS    200U
#define BOARD_CFG_BUZZER_ALARM_OFF_MS   200U
#define BOARD_CFG_BUZZER_KEY_BEEP_MS    50U

#define BOARD_CFG_OPTICAL_BAUDRATE      9600U
#define BOARD_CFG_MODEM_BAUDRATE        19200U
#define BOARD_CFG_CLI_BAUDRATE          115200U

#define BOARD_CFG_METROLOGY_AFE_ADE9000 1
#define BOARD_CFG_NOMINAL_VOLTAGE_DV    2200U
#define BOARD_CFG_NOMINAL_FREQ_MHZ      50000U
#define BOARD_CFG_ANTI_CREEP_WATTS      3U
#define BOARD_CFG_ADC4_BATT_LOW_MV      2700U
#define BOARD_CFG_ADC4_SUPERCAP_LOW_MV  2000U

/* --- H. Power Saving & Ultra-Low-Power Stop 2 Mode --- */
#define BOARD_CFG_ENABLE_STOP2_MODE        0
#define BOARD_CFG_STOP2_WAKEUP_CLOCK_MSI   1
#define BOARD_CFG_RTC_WAKEUP_PERIOD_SEC    60U

#endif

/* ====================================================================
 * 5. HELPER INLINE API
 * ==================================================================== */

/**
 * @brief Memeriksa apakah display OLED SSD1306 aktif pada profil papan saat ini
 */
static inline bool board_cfg_is_oled_active(void) {
    return (BOARD_CFG_DISPLAY_BACKEND_OLED != 0);
}

/**
 * @brief Memeriksa apakah display Segment LCD aktif pada profil papan saat ini
 */
static inline bool board_cfg_is_segment_lcd_active(void) {
    return (BOARD_CFG_DISPLAY_BACKEND_SEGLCD != 0);
}

/**
 * @brief Mengambil nama profil papan perangkat keras yang sedang aktif
 */
static inline const char* board_cfg_get_board_name(void) {
    return BOARD_NAME_STRING;
}

/* ====================================================================
 * 6. SAKELAR DIAGNOSTIK & INJEKSI BEBAN TIRUAN (R&D / FACTORY BENCH)
 * ==================================================================== */
#ifndef BOARD_CFG_ENABLE_SYNTHETIC_LOAD
#define BOARD_CFG_ENABLE_SYNTHETIC_LOAD  1  /**< 1 = Aktifkan perintah CLI simulasi beban (load/volt/curr/synth), 0 = Produksi massal / Sertifikasi resmi PLN */
#endif

/**
 * @brief Memeriksa apakah fitur beban tiruan metrologi diaktifkan pada level kompilasi
 */
static inline bool board_cfg_is_synthetic_load_enabled(void) {
    return (BOARD_CFG_ENABLE_SYNTHETIC_LOAD != 0);
}

/* ====================================================================
 * 7. SAKELAR MANAJEMEN DAYA EKSTREM STM32U5 (STOP 2 MODE)
 * ==================================================================== */
#ifndef BOARD_CFG_ENABLE_STOP2_MODE
#define BOARD_CFG_ENABLE_STOP2_MODE      1  /**< 1 = Masuk Stop 2 Mode saat listrik padam & layar padam (~4.5 uA), 0 = Run biasa */
#endif

#ifndef BOARD_CFG_STOP2_WAKEUP_CLOCK_MSI
#define BOARD_CFG_STOP2_WAKEUP_CLOCK_MSI 1  /**< 1 = Berjalan di MSIS 4MHz saat bangun di mode baterai */
#endif

#ifndef BOARD_CFG_RTC_WAKEUP_PERIOD_SEC
#define BOARD_CFG_RTC_WAKEUP_PERIOD_SEC  60U /**< Interval periodik RTC Wakeup Timer (60 detik) */
#endif

/**
 * @brief Memeriksa apakah fitur Stop 2 Mode hemat daya ekstrem diaktifkan
 */
static inline bool board_cfg_is_stop2_mode_enabled(void) {
    return (BOARD_CFG_ENABLE_STOP2_MODE != 0);
}

#ifdef __cplusplus
}
#endif

#endif /* BOARD_PROFILE_CONFIG_H */

