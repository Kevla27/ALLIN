# Log Perubahan 025: Proteksi Cut-Off Relai Tegangan Rendah (<110V) & Tegangan Lebih (>280V) (SPLN D3.006: 2021 Butir 10.3 Tabel 9)

## 1. Latar Belakang & Analisis Dokumen Acuan
Sesuai dengan ketentuan **SPLN D3.006: 2021 Butir 10.3 (Hal. 27)** dan **Tabel 9 ("Julat tegangan")**:
1. **Under-Voltage Cut-Off & Recovery**:
   - *"Untuk meter tersambung langsung, ketika tegangan pasok di bawah 110 V, maka relai akan membuka, dan ketika tegangan pasok di atas 115 V, maka relai akan menutup kembali."*
   - Ambang trip cut-off: $< 110.0\text{ V}$ ($1100\text{ dV}$).
   - Ambang reclose otomatis: $> 115.0\text{ V}$ ($1150\text{ dV}$).
   - Histeresis proteksi: $5\text{ V}$.
2. **Over-Voltage Cut-Off & Recovery**:
   - *"Serta ketika tegangan pasok di atas 280 V, maka relai akan membuka, dan ketika tegangan pasok di bawah 276 V, maka relai akan menutup kembali."*
   - Ambang trip cut-off: $> 280.0\text{ V}$ ($2800\text{ dV}$).
   - Ambang reclose otomatis: $< 276.0\text{ V}$ ($2760\text{ dV}$).
   - Histeresis proteksi: $4\text{ V}$.
3. **Sifat Operasi Relai**:
   - *"Operasi relai memutus semua fase secara serempak."*
   - Jika salah satu fasa (R, S, atau T) melanggar batas operasi, seluruh fasa diputus serempak (`RELAY_TRIP_Pin` aktif).
   - Pemulihan otomatis (*auto-reclose*) terjadi apabila seluruh fasa telah kembali berada dalam batas aman ($> 115\text{ V}$ dan $< 276\text{ V}$).

---

## 2. File dan Komponen yang Terdampak
1. [`E3/app/fsm/inc/power_relay_manager.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/power_relay_manager.h):
   - Definisi konstanta ambang tegangan: `SPLN_VOLT_UNDER_CUTOFF_DV` (1100), `SPLN_VOLT_UNDER_RECLOSE_DV` (1150), `SPLN_VOLT_OVER_CUTOFF_DV` (2800), `SPLN_VOLT_OVER_RECLOSE_DV` (2760).
   - Penambahan enum alasan relai: `RELAY_REASON_UNDER_VOLTAGE` dan `RELAY_REASON_OVER_VOLTAGE`.
   - Penambahan flag status pada struct `power_relay_context_t`: `is_under_voltage_trip` dan `is_over_voltage_trip`.
   - Deklarasi fungsi `power_relay_process_voltage()` dan `power_relay_reason_to_string()`.
2. [`E3/app/fsm/src/power_relay_manager.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/power_relay_manager.c):
   - Implementasi logika pemutusan dan pemulihan tegangan serempak dengan histeresis SPLN Butir 10.3.
3. [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c):
   - Pemanggilan `power_relay_process_voltage()` setiap siklus pembacaan tegangan fasa R, S, T.
4. [`E3/tests/test_power_relay.c`](file:///g:/PPPP/ALLIN/E3/tests/test_power_relay.c):
   - Penambahan test suite `test_voltage_cutoff_protection()` yang menguji siklus normal, under-voltage trip, histeresis, recovery, over-voltage trip, histeresis, dan recovery.

---

## 3. Rincian Perubahan Kode

### A. [`power_relay_manager.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/power_relay_manager.h)
```c
#define SPLN_VOLT_UNDER_CUTOFF_DV    1100U /**< < 110.0 V: Relai Membuka Serempak Semua Fasa */
#define SPLN_VOLT_UNDER_RECLOSE_DV   1150U /**< > 115.0 V: Relai Menutup Kembali (Histeresis 5 V) */
#define SPLN_VOLT_OVER_CUTOFF_DV     2800U /**< > 280.0 V: Relai Membuka Serempak Semua Fasa */
#define SPLN_VOLT_OVER_RECLOSE_DV    2760U /**< < 276.0 V: Relai Menutup Kembali (Histeresis 4 V) */

typedef enum {
    ...
    RELAY_REASON_UNDER_VOLTAGE,     /**< Cut-Off Tegangan Rendah < 110V (SPLN Butir 10.3) */
    RELAY_REASON_OVER_VOLTAGE       /**< Cut-Off Tegangan Lebih > 280V (SPLN Butir 10.3) */
} relay_reason_t;

bool power_relay_process_voltage(power_relay_context_t *ctx, uint32_t v_r_dv, uint32_t v_s_dv, uint32_t v_t_dv, uint32_t timestamp);
```

### B. [`power_relay_manager.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/power_relay_manager.c)
```c
bool power_relay_process_voltage(power_relay_context_t *ctx, uint32_t v_r_dv, uint32_t v_s_dv, uint32_t v_t_dv, uint32_t timestamp) {
    if (ctx == NULL) return false;

    bool has_under_voltage = ((v_r_dv > 100U && v_r_dv < SPLN_VOLT_UNDER_CUTOFF_DV) ||
                              (v_s_dv > 100U && v_s_dv < SPLN_VOLT_UNDER_CUTOFF_DV) ||
                              (v_t_dv > 100U && v_t_dv < SPLN_VOLT_UNDER_CUTOFF_DV));

    bool has_over_voltage  = (v_r_dv > SPLN_VOLT_OVER_CUTOFF_DV ||
                              v_s_dv > SPLN_VOLT_OVER_CUTOFF_DV ||
                              v_t_dv > SPLN_VOLT_OVER_CUTOFF_DV);

    /* 1. Trip Under-Voltage (< 110 V) */
    if (has_under_voltage) {
        ctx->is_under_voltage_trip = true;
        if (ctx->relay_state == RELAY_STATE_CONNECTED) {
            power_relay_disconnect(ctx, RELAY_REASON_UNDER_VOLTAGE, timestamp);
            return true;
        }
        return false;
    }

    /* 2. Trip Over-Voltage (> 280 V) */
    if (has_over_voltage) {
        ctx->is_over_voltage_trip = true;
        if (ctx->relay_state == RELAY_STATE_CONNECTED) {
            power_relay_disconnect(ctx, RELAY_REASON_OVER_VOLTAGE, timestamp);
            return true;
        }
        return false;
    }

    /* 3. Pemulihan Otomatis Under-Voltage (> 115 V) */
    if (ctx->is_under_voltage_trip && ctx->last_reason == RELAY_REASON_UNDER_VOLTAGE) {
        bool all_above_115 = ((v_r_dv == 0 || v_r_dv > SPLN_VOLT_UNDER_RECLOSE_DV) &&
                              (v_s_dv == 0 || v_s_dv > SPLN_VOLT_UNDER_RECLOSE_DV) &&
                              (v_t_dv == 0 || v_t_dv > SPLN_VOLT_UNDER_RECLOSE_DV));
        if (all_above_115 && !has_over_voltage) {
            ctx->relay_state = RELAY_STATE_CONNECTED;
            ctx->last_reason = RELAY_REASON_NONE;
            ctx->is_under_voltage_trip = false;
            return false;
        }
    }

    /* 4. Pemulihan Otomatis Over-Voltage (< 276 V) */
    if (ctx->is_over_voltage_trip && ctx->last_reason == RELAY_REASON_OVER_VOLTAGE) {
        bool all_below_276 = (v_r_dv < SPLN_VOLT_OVER_RECLOSE_DV &&
                              v_s_dv < SPLN_VOLT_OVER_RECLOSE_DV &&
                              v_t_dv < SPLN_VOLT_OVER_RECLOSE_DV);
        if (all_below_276 && !has_under_voltage) {
            ctx->relay_state = RELAY_STATE_CONNECTED;
            ctx->last_reason = RELAY_REASON_NONE;
            ctx->is_over_voltage_trip = false;
            return false;
        }
    }

    return false;
}
```

---

## 4. Hasil Verifikasi & Pengujian
1. **Host CTest (x86_64 GCC / Ninja)**:
   - 7 dari 7 suite lulus (`100% tests passed, 0 tests failed`).
   - Eksekusi `test_voltage_cutoff_protection()` memverifikasi:
     - Fasa R = 105 V ($< 110\text{ V}$): Relai TRIP (alasan `UNDER_VOLTAGE`).
     - Fasa R = 113 V (zona histeresis): Relai tetap `DISCONNECTED`.
     - Fasa R = 118 V ($> 115\text{ V}$): Relai pulih otomatis `CONNECTED`.
     - Fasa T = 285 V ($> 280\text{ V}$): Relai TRIP (alasan `OVER_VOLTAGE`).
     - Fasa T = 278 V (zona histeresis): Relai tetap `DISCONNECTED`.
     - Fasa T = 272 V ($< 276\text{ V}$): Relai pulih otomatis `CONNECTED`.
2. **Kompilasi Target ARM Cortex-M33 NonSecure (`ALLIN_NS.elf`)**:
   - Kompiler: `arm-none-eabi-gcc 14.3.1`
   - Memory Region Footprint:
     - **RAM**: 62,440 B / 512 KB (11.91%)
     - **ROM**: 131,736 B / 1 MB (12.56%)
   - **0 Error, 0 Warning**.

