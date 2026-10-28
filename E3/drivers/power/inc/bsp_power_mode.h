/**
 * @file bsp_power_mode.h
 * @brief Driver Manajemen Hemat Daya Ekstrem STM32U5 (Stop 2 Mode saat Listrik Padam)
 * @details Mengelola transisi status daya sistem cerdas (Smart Power State Machine):
 *          - METER_POWER_MODE_RUN: Operasi normal PLN AC (PLL 160 MHz, seluruh periferal aktif)
 *          - METER_POWER_MODE_BATTERY_ACTIVE: Layar aktif pada baterai (MSIS 4 MHz, timer 10 detik, ~1.5 mA)
 *          - METER_POWER_MODE_STOP2: Mode deep sleep hemat daya ekstrem (~4.5 uA, CPU & clock mati, SRAM retensi penuh)
 *
 * Sumber Wake-up Stop 2:
 *          1. EXTI3  (PC3)  : AC_FAIL_DET Rising Edge (PLN Hidup Kembali)
 *          2. EXTI0  (PA0)  : Tombol Navigasi Layar Atas (BTN_SCROLL_UP)
 *          3. EXTI6  (PB6)  : Tombol Navigasi Layar Bawah (BTN_SCROLL_DN)
 *          4. EXTI13 (PC13) : Tamper Tutup Meter (TAMP_CASE_OPEN)
 *          5. EXTI6  (PE6)  : Tamper Tutup Terminal (TAMP_TERM_COVER)
 *          6. EXTI4  (PE4)  : Tamper Medan Magnet (SENSOR_MAGNET)
 *          7. RTC Wakeup Timer : Pengingat Periodik (Billing / Tamper Scan)
 *
 * Target: STM32U585VIT6 (ARM Cortex-M33 TrustZone) & Host Test Native
 */

#ifndef BSP_POWER_MODE_H
#define BSP_POWER_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Status Moda Konsumsi Daya Smart Meter
 */
typedef enum {
    METER_POWER_MODE_RUN = 0,         /**< Mode Normal PLN (160 MHz PLL, seluruh periferal aktif) */
    METER_POWER_MODE_BATTERY_ACTIVE,  /**< Mode Darurat Layar Aktif pada Baterai (MSIS 4 MHz, Timer 10 detik) */
    METER_POWER_MODE_STOP2            /**< Mode Hemat Daya Ekstrem Stop 2 (~4.5 uA, CPU Deep Sleep) */
} meter_power_mode_t;

/**
 * @brief Identifikasi Sumber Pemicu Bangun (Wakeup Trigger) dari Stop 2
 */
typedef enum {
    METER_WAKEUP_REASON_NONE = 0,     /**< Belum ada event bangun */
    METER_WAKEUP_REASON_AC_RESTORED,  /**< Listrik PLN pulih normal (PC3 AC_FAIL_DET rising edge) */
    METER_WAKEUP_REASON_BUTTON_PRESS, /**< Penekanan tombol navigasi display (PA0 / PB6) */
    METER_WAKEUP_REASON_TAMPER,       /**< Pemicu sakelar proteksi sabotase (PC13, PE6, PE4) */
    METER_WAKEUP_REASON_RTC_PERIODIC  /**< Timer pengingat bangun periodik RTC */
} meter_wakeup_reason_t;

/**
 * @brief Menginisialisasi subsistem manajemen daya STM32U5 & konfigurasi Stop 2.
 */
void bsp_power_mode_init(void);

/**
 * @brief Mengambil status mode daya yang sedang aktif saat ini.
 * @return Nilai enumerasi meter_power_mode_t.
 */
meter_power_mode_t bsp_power_mode_get_current(void);

/**
 * @brief Mengambil representasi nama string dari mode daya.
 * @param mode Mode daya yang ingin dibaca namanya.
 * @return String deskriptif mode daya.
 */
const char *bsp_power_mode_get_name(meter_power_mode_t mode);

/**
 * @brief Mengambil alasan bangun (wake-up reason) terakhir dari Stop 2.
 * @return Nilai enumerasi meter_wakeup_reason_t.
 */
meter_wakeup_reason_t bsp_power_mode_get_last_wakeup_reason(void);

/**
 * @brief Mengambil jumlah kumulatif siklus bangun dari mode Stop 2.
 * @return Total siklus bangun (wake-up count).
 */
uint32_t bsp_power_mode_get_wakeup_count(void);

/**
 * @brief Memberitahu status suplai AC listrik PLN (AC Fail vs AC Normal).
 * @param failed true jika listrik PLN padam, false jika suplai AC normal.
 */
void bsp_power_mode_set_ac_failed(bool failed);

/**
 * @brief Memeriksa apakah status suplai AC sedang padam.
 * @return true jika padam, false jika aktif normal.
 */
bool bsp_power_mode_is_ac_failed(void);

/**
 * @brief Masuk ke mode hemat daya ekstrem STM32U5 Stop 2 (~4.5 uA).
 *        Fungsi ini mematikan periferal analog (ADC4), mematikan relai daya display,
 *        menghentikan FreeRTOS tick, dan memanggil HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI).
 *        Setelah bangun oleh EXTI/RTC, jam dipulihkan secara adaptif:
 *        - Jika AC pulih: Mengaktifkan kembali PLL 160 MHz.
 *        - Jika tombol/tamper: Tetap pada MSIS 4 MHz untuk meminimalkan konsumsi baterai.
 * @return Alasan terbangun dari mode Stop 2.
 */
meter_wakeup_reason_t bsp_power_enter_stop2(void);

/**
 * @brief Mengonfigurasi / memulihkan clock ke kecepatan penuh (PLL 160 MHz, Scale 1).
 */
void bsp_power_restore_run_clock(void);

/**
 * @brief Mengonfigurasi clock ke frekuensi hemat energi baterai (MSIS 4 MHz, Scale 4).
 */
void bsp_power_set_battery_clock_msis(void);

/**
 * @brief Mengonfigurasi RTC Wakeup Timer periodik (detik).
 * @param period_sec Periode dalam detik (misal: 60 untuk 1 menit).
 */
void bsp_power_configure_rtc_wakeup_timer(uint32_t period_sec);

#if !defined(EMBEDDED_HARDWARE_TARGET)
/**
 * @brief Fungsi simulasi pengujian host CTest untuk menyimulasikan pemicu bangun.
 * @param reason Alasan bangun yang disimulasikan.
 */
void bsp_power_mock_simulate_wakeup(meter_wakeup_reason_t reason);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BSP_POWER_MODE_H */
