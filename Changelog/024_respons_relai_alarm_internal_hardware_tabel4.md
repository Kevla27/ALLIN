# Log Perubahan 024: Respons Buka Relai & Indikator LED Kuning pada Alarm Internal Hardware (SPLN D3.006: 2021 Tabel 4)

## 1. Latar Belakang & Analisis Dokumen Acuan
Sesuai dengan ketentuan **SPLN D3.006: 2021 Tabel 4 (Hal. 20)** ("Respons meter terhadap alarm"):
1. **Kegagalan Hardware Kritis Internal**:
   - `ERR00` (No. 0): Flash memory rusak/error $\rightarrow$ **Relai membuka / perintah shunt trip membuka**, LED kuning: **Aktif**.
   - `ERR01` (No. 1): RAM rusak/error $\rightarrow$ **Relai membuka / perintah shunt trip membuka**, LED kuning: **Aktif**.
   - `ERR02` (No. 2): Clock loss (kembali ke waktu awal chipset) $\rightarrow$ **Relai membuka / perintah shunt trip membuka**, LED kuning: **Aktif**.
   - `ERR04` (No. 4): Mikroprosesor tidak berfungsi normal $\rightarrow$ **Relai membuka / perintah shunt trip membuka**, LED kuning: **Aktif**.
   - `ERR05` (No. 5): Kegagalan sampling data pengukuran pada ADC $\rightarrow$ **Relai membuka / perintah shunt trip membuka**, LED kuning: **Aktif**.
2. **Kegagalan Non-Kritis / Peringatan**:
   - `ERR03` (No. 3): Low battery (threshold 2,7 V) $\rightarrow$ Rekam data, Simbol baterai berkedip, LED kuning: **Aktif**, Relai: **TIDAK MEMBUKA**.
   - `ERR06` (No. 6): Superkapasitor rusak/tidak terhubung/terlepas $\rightarrow$ Rekam data, LED kuning: **Aktif**, Relai: **TIDAK MEMBUKA**.
   - `ERR07` (No. 7): Relai/shunt trip gagal membuka/menutup $\rightarrow$ Rekam data, LED kuning: **Aktif**.

Sebelum perubahan ini, status alarm internal hardware hanya diteruskan ke penampil layar LCD dan belum memicu pembukaan relai kontaktor fisik (`RELAY_TRIP_Pin`) maupun kontrol `LED_ALM_Pin`.

---

## 2. File dan Komponen yang Terdampak
1. [`E3/app/tamper/inc/tamper_manager.h`](file:///g:/PPPP/ALLIN/E3/app/tamper/inc/tamper_manager.h):
   - Penambahan bitmask `SPLN_ALARM_MASK_RELAY_TRIP` (hanya `ERR00`, `ERR01`, `ERR02`, `ERR04`, `ERR05`).
   - Penambahan bitmask `SPLN_ALARM_MASK_LED_YELLOW` (seluruh `ERR00` s.d `ERR07`).
   - Penambahan inline helper `spln_alarm_should_trip_relay(uint32_t alarm_mask)` dan `spln_alarm_should_turn_on_led(uint32_t alarm_mask)`.
2. [`E3/app/display/inc/display.h`](file:///g:/PPPP/ALLIN/E3/app/display/inc/display.h) & [`E3/app/display/src/display.c`](file:///g:/PPPP/ALLIN/E3/app/display/src/display.c):
   - Deklarasi dan implementasi fungsi getter `display_get_internal_alarm()`.
3. [`E3/app/e3_meter_app.h`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.h) & [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c):
   - Penambahan variabel static `s_internal_alarm_mask`.
   - Implementasi API publik `e3_meter_app_set_internal_alarm()` dan `e3_meter_app_get_internal_alarm()`.
   - Penggabungan evaluasi alarm internal ke logika trip relai (`is_trip_commanded`) dan aktivasi LED alarm kuning (`is_yellow_led_on`).
4. [`E3/tests/test_e3_app.c`](file:///g:/PPPP/ALLIN/E3/tests/test_e3_app.c):
   - Penambahan test suite `test_spln_tabel4_internal_alarm_behavior()` yang memverifikasi seluruh respons Tabel 4 secara individual dan kombinasi.

---

## 3. Rincian Perubahan Kode

### A. [`tamper_manager.h`](file:///g:/PPPP/ALLIN/E3/app/tamper/inc/tamper_manager.h)
```c
/**
 * @brief Mask alarm internal hardware yang mewajibkan relai/shunt trip membuka (SPLN D3.006: 2021 Tabel 4)
 * Yaitu ERR00 (Flash), ERR01 (RAM), ERR02 (Clock Loss), ERR04 (MCU), dan ERR05 (ADC Sampling Fail).
 */
#define SPLN_ALARM_MASK_RELAY_TRIP ( \
    SPLN_ALARM_FLASH_ERROR | \
    SPLN_ALARM_RAM_ERROR   | \
    SPLN_ALARM_RTC_ERROR   | \
    SPLN_ALARM_MCU_ERROR   | \
    SPLN_ALARM_ADC_ERROR     \
)

/**
 * @brief Mask alarm internal hardware yang menyalakan lampu indikator LED Kuning (SPLN D3.006: 2021 Tabel 4)
 * Seluruh alarm internal ERR00 s.d ERR07 menyalakan LED Kuning.
 */
#define SPLN_ALARM_MASK_LED_YELLOW ( \
    SPLN_ALARM_FLASH_ERROR   | \
    SPLN_ALARM_RAM_ERROR     | \
    SPLN_ALARM_RTC_ERROR     | \
    SPLN_ALARM_LOW_BATTERY   | \
    SPLN_ALARM_MCU_ERROR     | \
    SPLN_ALARM_ADC_ERROR     | \
    SPLN_ALARM_SUPERCAP_FAIL | \
    SPLN_ALARM_RELAY_FAIL      \
)

static inline bool spln_alarm_should_trip_relay(uint32_t alarm_mask) {
    return ((alarm_mask & SPLN_ALARM_MASK_RELAY_TRIP) != 0U);
}

static inline bool spln_alarm_should_turn_on_led(uint32_t alarm_mask) {
    return ((alarm_mask & SPLN_ALARM_MASK_LED_YELLOW) != 0U);
}
```

### B. [`e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)
```c
    /* Sesuai SPLN D3.006: 2021:
     * 1. Relai membuka (TRIP) jika:
     *    a. Overload / FSM Disconnected
     *    b. Sabotase/Tamper yang berhak trip (Tabel 6)
     *    c. Alarm Internal Kritis Hardware (Tabel 4: ERR00 Flash, ERR01 RAM, ERR02 Clock, ERR04 MCU, ERR05 ADC)
     * 2. LED Kuning aktif jika:
     *    a. Overload / FSM Disconnected
     *    b. Sabotase/Tamper yang menyalakan LED (Tabel 6)
     *    c. Alarm Internal Hardware aktif (Tabel 4: ERR00 s.d ERR07) */
    bool is_hardware_alarm_trip = spln_alarm_should_trip_relay(s_internal_alarm_mask);
    bool is_hardware_alarm_led  = spln_alarm_should_turn_on_led(s_internal_alarm_mask);

    bool is_trip_commanded = (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED ||
                              tamper_should_trip_relay(&s_tamper_ctx) ||
                              is_hardware_alarm_trip);
    bool is_yellow_led_on  = tamper_should_turn_on_led(&s_tamper_ctx) ||
                             (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED) ||
                             is_hardware_alarm_led;
```

---

## 4. Hasil Verifikasi & Pengujian
1. **Host CTest (x86_64 GCC / Ninja)**:
   - 7 dari 7 suite lulus (`100% tests passed, 0 tests failed`).
   - Eksekusi `test_spln_tabel4_internal_alarm_behavior()` memverifikasi:
     - `ERR00`, `ERR01`, `ERR02`, `ERR04`, `ERR05`: Trip = `true`, LED = `true`.
     - `ERR03` (Baterai), `ERR06` (Supercap), `ERR07` (Relai macet): Trip = `false`, LED = `true`.
2. **Kompilasi Target ARM Cortex-M33 NonSecure (`ALLIN_NS.elf`)**:
   - Kompiler: `arm-none-eabi-gcc 14.3.1`
   - Memory Region Footprint:
     - **RAM**: 62,440 B / 512 KB (11.91%)
     - **ROM**: 131,192 B / 1 MB (12.51%)
   - **0 Error, 0 Warning**.

