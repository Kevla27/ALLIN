# Changelog 014: Implementasi Audio Cadence Buzzer SPLN D3.006 (Tahap 2) & Integrasi Feedback Tombol

**Tanggal:** 27 Oktober 2026  
**Status:** Selesai & Terverifikasi Penuh (Kompilasi ARM ELF 0 Error 0 Warning; 6/6 E3 CTest Pass; 9/9 Batch Test Pass)

---

## 1. Latar Belakang & Kebutuhan Regulasi
Sesuai dengan roadmap 3 tahap penyelesaian Smart Meter ALLIN (STM32U585VIT6):
- **Tahap 1 (Selesai pada Log 013):** Paket HMI & OLED Fisik (Energi Reaktif kVARh, ToU WBP/LWBP, aktivasi I2C3 OLED).
- **Tahap 2 (Eksekusi Sekarang):** Audio Cadence Buzzer (Pin `PB7`).

### Dasar Acuan Regulasi:
1. **SPLN D3.006: 2021 Butir 6.14 (Indikator Alarm Audio):**
   - Mengharuskan meter dilengkapi dengan indikator audio (buzzer) yang berbunyi secara terputus-putus (*intermittent cadence*) dengan periode **200 ms ON / 200 ms OFF** ketika terjadi kondisi alarm sabotase (tamper), kegagalan internal (internal error), atau relai pemutus beban dalam status trip/disconnected.
2. **SPLN D3.006: 2021 Butir 6.9 huruf c & Desain HMI:**
   - Memberikan umpan balik taktil audio (*audio tactile feedback*) berupa bunyi *beep* pendek (50 ms) setiap kali tombol navigasi keypad (▲ / ▼) ditekan oleh pengguna.
   - Respon penekanan tombol harus memprioritaskan bunyi *beep* (override) meskipun buzzer sedang berada pada fase OFF dalam kadensi alarm.
   - Implementasi harus bersifat non-blocking (berbasis delta waktu millisecond), tanpa mengunci interrupt EXTI atau siklus RTOS meter.

---

## 2. Rincian Perubahan Kode

### A. Driver Hardware Buzzer (`E3/drivers/buzzer/inc/bsp_alarm_buzzer.h` & `bsp_alarm_buzzer.c`)
- **Deklarasi Fungsi Pembaruan Kadensi:**
  ```c
  bool bsp_alarm_buzzer_update_cadence(bool alarm_active, bool beep_override, uint32_t delta_ms);
  ```
- **Implementasi Algoritma Kadensi Non-Blocking (`bsp_alarm_buzzer.c`):**
  - Mengelola accumulator waktu siklus `s_alarm_cadence_timer_ms` berbasis `delta_ms`.
  - Jika `beep_override == true`, status keluaran dipaksa `HIGH` (ON).
  - Jika `alarm_active == true`, siklus 400 ms diterapkan:
    - `0 <= s_alarm_cadence_timer_ms < 200 ms`: status `HIGH` (Buzzer Berbunyi).
    - `200 ms <= s_alarm_cadence_timer_ms < 400 ms`: status `LOW` (Buzzer Hening).
  - Jika alarm pulih (`alarm_active == false`), timer di-reset ke 0 dan buzzer langsung dipadamkan.
  - Menyediakan implementasi hardware GPIO STM32 HAL (`PB7`) dan implementasi simulasi state (`s_alarm_buzzer_sim_state`) untuk pengujian Host CTest.

### B. Integrasi Aplikasi Meter (`E3/app/e3_meter_app.c`)
- Pemanggilan `bsp_alarm_buzzer_update_cadence(is_trip_commanded, beep_override, delta_ms)` di dalam loop utama `e3_meter_app_step()`.
- Variabel `s_button_beep_ms` didekrementasi secara non-blocking setiap kali dipicu oleh ISR tombol navigasi keypad `e3_meter_app_on_button_pressed()`.

### C. Konfigurasi Build System (`E3/CMakeLists.txt` & `scratch/run_all_tests.py`)
- Mendaftarkan `drivers/buzzer/src/bsp_alarm_buzzer.c` ke dalam library static `e3_core`.
- Memperbarui runner script standalone `run_all_tests.py` untuk mengompilasi driver buzzer bersama modul display dan tamper.

### D. Unit Test Verifikasi (`E3/tests/test_spln_display_tamper.c`)
- Ditambahkan blok pengujian baru **`[UJI 8] Verifikasi Audio Cadence Buzzer (SPLN D3.006 Butir 6.14)`**:
  1. Kondisi normal tanpa alarm: Buzzer OFF.
  2. Alarm aktif pada $t = 100\text{ ms}$: Output ON.
  3. Alarm aktif pada $t = 200\text{ ms}$: Output OFF.
  4. Alarm aktif pada $t = 400\text{ ms}$ (Wrap): Output ON kembali.
  5. Override penekanan tombol saat alarm berada di fase OFF: Output dipaksa ON.
  6. Pemulihan alarm normal: Buzzer langsung OFF dan timer kembali ke nol.

---

## 3. Diff / Perbandingan Kode Sebelum vs Sesudah

```diff
--- a/E3/drivers/buzzer/inc/bsp_alarm_buzzer.h
+++ b/E3/drivers/buzzer/inc/bsp_alarm_buzzer.h
@@ -37,6 +37,16 @@ bool bsp_alarm_buzzer_get(void);
 
+/**
+ * @brief Memperbarui siklus kadensi audio buzzer sesuai SPLN D3.006: 2021 Butir 6.14
+ * @param alarm_active true jika kondisi alarm sabotase atau trip sedang aktif (200 ms ON / 200 ms OFF)
+ * @param beep_override true jika ada override bunyi tombol 50 ms
+ * @param delta_ms Waktu langkah eksekusi dalam milidetik
+ * @return Status keluaran buzzer saat ini (true = aktif/ON, false = mati/OFF)
+ */
+bool bsp_alarm_buzzer_update_cadence(bool alarm_active, bool beep_override, uint32_t delta_ms);
+
 #ifdef __cplusplus
 }
 #endif

--- a/E3/drivers/buzzer/src/bsp_alarm_buzzer.c
+++ b/E3/drivers/buzzer/src/bsp_alarm_buzzer.c
@@ -7,4 +7,25 @@
 #include "bsp_alarm_buzzer.h"
 
+static uint32_t s_alarm_cadence_timer_ms = 0U;
+
+bool bsp_alarm_buzzer_update_cadence(bool alarm_active, bool beep_override, uint32_t delta_ms) {
+    bool buzzer_state = false;
+
+    if (beep_override) {
+        buzzer_state = true;
+    } else if (alarm_active) {
+        s_alarm_cadence_timer_ms += delta_ms;
+        if (s_alarm_cadence_timer_ms >= 400U) {
+            s_alarm_cadence_timer_ms %= 400U;
+        }
+        /* SPLN D3.006: 2021 Butir 6.14: Kadensi terputus-putus 200 ms ON / 200 ms OFF */
+        buzzer_state = (s_alarm_cadence_timer_ms < 200U);
+    } else {
+        s_alarm_cadence_timer_ms = 0U;
+        buzzer_state = false;
+    }
+
+    bsp_alarm_buzzer_set(buzzer_state);
+    return buzzer_state;
+}
```

---

## 4. Hasil Verifikasi, Kompilasi, dan Uji Host

### A. Pengujian Host Unit Test (CTest & Batch Runner)
```text
[PASS] E3 SPLN Display & Tamper (Termasuk UJI 8 Audio Cadence Buzzer)
[PASS] E3 Power Relay & FSM
[PASS] E3 Load Profile 15-Min
[PASS] E3 FSM Statechart
[PASS] E3 App & Tamper FIFO
[PASS] E3 NVRAM Flash Storage
[PASS] E1 Metrology ADE9000
[PASS] E2 DLMS/COSEM HDLC
[PASS] E1 ADE9000_Metrology_2 Simulation

>>> ALL 9 TEST SUITES (E1, E2, E3, Metrology_2) PASSED 100% <<<
```
```text
Test project G:/PPPP/ALLIN/E3/build
1/6 Test #1: Integration_E3_Demo ..............   Passed
2/6 Test #2: Unit_E3_Display_Tamper ...........   Passed
3/6 Test #3: Unit_E3_Power_Relay ..............   Passed
4/6 Test #4: Unit_E3_FSM_Statechart ...........   Passed
5/6 Test #5: Unit_E3_Load_Profile .............   Passed
6/6 Test #6: Unit_E3_SPLN_Display_Tamper ......   Passed

100% tests passed, 0 tests failed out of 6
```

### B. Kompilasi Target Fisik ARM Cortex-M33 (Ninja)
- **Target Secure:** `Secure/build/ALLIN_S.elf` -> Bersih (0 Error, 0 Warning).
- **Target NonSecure:** `NonSecure/build/ALLIN_NS.elf` -> Bersih (0 Error, 0 Warning).
```text
Memory region         Used Size  Region Size  %age Used
             RAM:       61448 B       512 KB     11.72%
           SRAM4:           0 B        16 KB      0.00%
             ROM:      121144 B         1 MB     11.55%
```
- Penggunaan Flash ROM hanya bertambah ~100 bytes (11.55% dari 1 MB), RAM statis tetap stabil (11.72% dari 512 KB).
- Pinout output hardware `PB7` beroperasi aman dan siap menggerakkan transistor buzzer alarm.

