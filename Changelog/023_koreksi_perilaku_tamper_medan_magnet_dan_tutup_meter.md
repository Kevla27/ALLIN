# Log Perubahan 023: Koreksi Perilaku Sabotase Medan Magnet & Tutup Meter (SPLN D3.006: 2021 Tabel 6)

## 1. Latar Belakang & Analisis Dokumen Acuan
Sesuai dengan ketentuan **SPLN D3.006: 2021 Tabel 6 (Hal. 22)** dan **Catatan 4**:
1. **Induksi Medan Magnet $\le 500\text{ mT}$ (`ERR25` / No. 7)**:
   - Sesuai Catatan 4: *"Dalam hal terjadi pengaruh medan magnet meter tetap mengukur normal dan operasi relai tidak terpengaruh."*
   - Status kolom LED Kuning pada tabel bertanda `-` (TIDAK aktif).
   - Relai kontaktor **TIDAK BOLEH MEMBUKA / TIDAK TRIP**, dan LED Kuning **TIDAK MENYALA**.
2. **Pembukaan Tutup Meter (`RUSAK` / No. 1)**:
   - Status operasi relai: **Buka (TRIP)**.
   - Status kolom LED Kuning bertanda `-` (TIDAK aktif).
   - Relai kontaktor **TRIP**, tetapi LED Kuning **TIDAK MENYALA**.
3. **Kondisi Sabotase Lainnya**:
   - Tutup Terminal (`ERR20`), Urutan Fase Terbalik (`ERR21`), Pengawatan Silang (`ERR22`), Hilang Tegangan 1/2 Fase (`ERR24`): Relai **TRIP** dan LED Kuning **MENYALA**.
   - Netral Putus/Bypass (`ERR23`), Arus Balik (`ERR26`), Kuadran 4 (`ERR27`): Relai **TIDAK TRIP** dan LED Kuning **MENYALA**.

Sebelumnya, firmware memperlakukan seluruh tamper secara seragam (`ctx->active_tamper_mask != 0`) sehingga kejadian medan magnet (`ERR25`) keliru memicu trip relai dan menyalakan LED kuning, sedangkan tutup meter (`RUSAK`) juga keliru menyalakan LED kuning.

---

## 2. File dan Komponen yang Terdampak
1. [`E3/app/tamper/inc/tamper_manager.h`](file:///g:/PPPP/ALLIN/E3/app/tamper/inc/tamper_manager.h):
   - Penambahan bitmask selektif `TAMPER_MASK_RELAY_TRIP` dan `TAMPER_MASK_LED_YELLOW`.
   - Deklarasi fungsi evaluator `tamper_should_trip_relay()` dan `tamper_should_turn_on_led()`.
2. [`E3/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/ALLIN/E3/app/tamper/src/tamper_manager.c):
   - Implementasi `tamper_should_trip_relay()` dan `tamper_should_turn_on_led()`.
3. [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c):
   - Pemisahan evaluasi aktivasi `RELAY_TRIP_Pin` (hanya jika `tamper_should_trip_relay()` bernilai `true`) dan `LED_ALM_Pin` (mengikuti `tamper_should_turn_on_led()`).
4. [`E3/tests/test_e3_app.c`](file:///g:/PPPP/ALLIN/E3/tests/test_e3_app.c):
   - Penambahan test suite `test_spln_tabel6_relay_and_led_behavior()` yang memverifikasi kesembilan jenis sabotase Tabel 6 secara individual dan kombinasinya.

---

## 3. Rincian Perubahan Kode

### A. [`tamper_manager.h`](file:///g:/PPPP/ALLIN/E3/app/tamper/inc/tamper_manager.h)
```c
/**
 * @brief Mask vektor sabotase yang berhak memicu pembukaan relai kontaktor (SPLN D3.006: 2021 Tabel 6)
 * Hanya Case Open, Terminal Open, Urutan Salah, Cross Phase, dan Hilang Fasa.
 * (Catatan 4: Medan magnet <= 500mT, Netral Hilang, Reverse Power, dan Quad 4 TIDAK memutus relai).
 */
#define TAMPER_MASK_RELAY_TRIP ( \
    TAMPER_VECTOR_CASE_OPEN      | \
    TAMPER_VECTOR_TERMINAL_OPEN  | \
    TAMPER_VECTOR_WRONG_SEQUENCE | \
    TAMPER_VECTOR_CROSS_PHASE    | \
    TAMPER_VECTOR_PHASE_LOSS       \
)

/**
 * @brief Mask vektor sabotase yang menyalakan lampu indikator LED Kuning (SPLN D3.006: 2021 Tabel 6)
 * (Pembukaan Tutup Meter dan Medan Magnet bernilai '-' / TIDAK menyalakan LED kuning).
 */
#define TAMPER_MASK_LED_YELLOW ( \
    TAMPER_VECTOR_TERMINAL_OPEN  | \
    TAMPER_VECTOR_WRONG_SEQUENCE | \
    TAMPER_VECTOR_CROSS_PHASE    | \
    TAMPER_VECTOR_NEUTRAL_BYPASS | \
    TAMPER_VECTOR_PHASE_LOSS     | \
    TAMPER_VECTOR_REVERSE_POWER  | \
    TAMPER_VECTOR_LOW_PF_QUAD4     \
)

bool tamper_should_trip_relay(const tamper_context_t *ctx);
bool tamper_should_turn_on_led(const tamper_context_t *ctx);
```

### B. [`tamper_manager.c`](file:///g:/PPPP/ALLIN/E3/app/tamper/src/tamper_manager.c)
```c
bool tamper_should_trip_relay(const tamper_context_t *ctx) {
    if (ctx == NULL) return false;
    return ((ctx->active_tamper_mask & TAMPER_MASK_RELAY_TRIP) != 0) || ctx->alarm_relay_trigger;
}

bool tamper_should_turn_on_led(const tamper_context_t *ctx) {
    if (ctx == NULL) return false;
    return ((ctx->active_tamper_mask & TAMPER_MASK_LED_YELLOW) != 0);
}
```

### C. [`e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)
```c
    /* Sesuai SPLN D3.006: 2021 Tabel 6:
     * 1. Relai membuka hanya pada: Overload/FSM, Case Open, Terminal Open,
     *    Urutan Fasa Terbalik, Cross Phase, dan Hilang Fasa.
     *    (Medan Magnet <=500mT, Netral Hilang, Reverse Power, dan Quad 4 TIDAK memutus relai).
     * 2. LED Kuning aktif pada: Overload trip, Terminal Open, Urutan Salah,
     *    Cross Phase, Netral Hilang, Hilang Fasa, Reverse Power, Quad 4.
     *    (Tutup Meter dibuka dan Medan Magnet bernilai '-' / TIDAK menyalakan LED kuning). */
    bool is_trip_commanded = (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED ||
                              tamper_should_trip_relay(&s_tamper_ctx));
    bool is_yellow_led_on  = tamper_should_turn_on_led(&s_tamper_ctx) ||
                             (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED);

    if (is_trip_commanded)
    {
        HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, is_yellow_led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
...
```

---

## 4. Hasil Verifikasi & Pengujian
1. **Host CTest (x86_64 GCC / Ninja)**:
   - Seluruh 7 suite pengujian lulus (`100% tests passed, 0 tests failed`).
   - `test_spln_tabel6_relay_and_led_behavior()` sukses mengonfirmasi:
     - `ERR25` (Medan Magnet): Relai TRIP = `false`, LED Kuning = `false`.
     - `RUSAK` (Tutup Meter): Relai TRIP = `true`, LED Kuning = `false`.
     - `ERR20` (Tutup Terminal): Relai TRIP = `true`, LED Kuning = `true`.
     - `ERR23` (Netral Putus): Relai TRIP = `false`, LED Kuning = `true`.
2. **Kompilasi Target ARM Cortex-M33 NonSecure (`ALLIN_NS.elf`)**:
   - Kompiler: `arm-none-eabi-gcc 14.3.1`
   - Memory Region Footprint:
     - **RAM**: 62,432 B / 512 KB (11.91%)
     - **ROM**: 131,056 B / 1 MB (12.50%)
   - **0 Error, 0 Warning**.

