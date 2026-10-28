# Changelog 015: Finalisasi Fitur Penagihan Lanjutan (Tahap 3), Audit Ketahanan Ekstrem & Pemantapan Versi Final Firmware

**Tanggal:** 27 Oktober 2026  
**Status:** Selesai & Terverifikasi Penuh 100% (Kompilasi ARM ELF 0 Error 0 Warning; 7/7 E3 CTest Pass; 10/10 Batch Test Pass; Uji Rendam 10.000 Siklus Pass)

---

## 1. Latar Belakang & Kebutuhan Regulasi
Untuk memantapkan versi final firmware Smart Meter 3-Fasa **ALLIN (STM32U585VIT6)** sesuai standar **SPLN D3.006: 2021 + SUP-1: 2022 + SUP-2: 2023**, seluruh rangkaian tiga tahap roadmap telah dituntaskan:
- **Tahap 1 (Log 013):** HMI Layar OLED I2C3, Energi ToU WBP & LWBP, dan Energi Reaktif kVARh.
- **Tahap 2 (Log 014):** Audio Cadence Buzzer intermiten 200 ms ON / 200 ms OFF dan feedback tombol 50 ms.
- **Tahap 3 (Log 015 - Finalisasi Sekarang):** Fitur Penagihan Lanjutan, Proteksi Sabotase Netral & Reverse Power, serta Uji Ketahanan Jangka Panjang.

### Dasar Acuan Regulasi:
1. **SPLN D3.006: 2021 Tabel E.1 No. 4–7 (Auto-Scroll Maximum Demand):**
   - No 4: Maximum demand aktif kirim (`P_MAX`), satuan **kW (3 desimal)**, OBIS `01.06`, periode integrasi 15 menit (900 detik).
   - No 5: Waktu & Tanggal pencatatan Maximum Demand.
   - No 6: Kumulatif Maximum Demand (`CMD`).
   - No 7: Jumlah kejadian reset Maximum Demand (*reset counter*).
2. **SPLN D3.006: 2021 Butir 6.13 & Lampiran E (Penyimpanan Stand Penagihan Bulanan):**
   - Meter wajib menyimpan data historis penagihan (*monthly billing snapshot*) sekurang-kurangnya **12 bulan terakhir** dalam ring buffer non-volatile dengan checksum CRC-16.
   - Setiap entri menyimpan stand: Energi Aktif Total, WBP, LWBP, Energi Reaktif, serta nilai $MD$ dan timestamp penutupan billing.
3. **SPLN D3.006: 2021 Tabel 6 No. 5 & No. 8 (Deteksi Sabotase Otomatis):**
   - **Tabel 6 No. 5 (Kawat Netral Putus / Bypass):** Teks `PERIKSA`, Kode `ERR23`. Terdeteksi otomatis saat beban fasa aktif namun arus netral lenyap ($I_N < 50\text{ mA}$) atau sebaliknya arus netral melonjak drastis akibat injeksi grounding.
   - **Tabel 6 No. 8 (Reverse Power):** Teks `REVERSE`, Kode `ERR26`. Terdeteksi otomatis saat daya aktif terbalik ($P < -50\text{ W}$) pada instalasi konsumen.
4. **Proteksi Brownout & Hemat Daya saat Pemadaman:**
   - Feedback audio buzzer dimatikan (*mute*) saat pemadaman listrik PLN (`s_is_ac_failed == true`) agar tidak membebani baterai dan superkapasitor meter.

---

## 2. Rincian Perubahan Kode

### A. Modul Profiling & Penagihan (`E3/app/profiling/inc/load_profile.h` & `load_profile.c`)
- **Implementasi Tracker Maximum Demand (MD 15-Menit):**
  - Struktur data `max_demand_tracker_t` mengintegrasikan daya aktif per detik selama interval 900 detik.
  - Daya demand dihitung: $P_{dem} = \frac{\sum P}{900\text{ s}}$ (Watt).
  - Puncak tertinggi disimpan pada `max_demand_w` beserta `max_demand_timestamp`.
  - Fungsi `max_demand_reset()` memindahkan rekor MD berjalan ke `cumulative_max_demand_w` dan menaikkan `reset_count`.
- **Implementasi Basis Data Stand Penagihan 12 Bulan:**
  - Struktur data `billing_snapshot_entry_t` dan ring buffer `billing_snapshot_ctx_t` dengan kapasitas `BILLING_MAX_SNAPSHOTS = 12`.
  - Fungsi perhitungan dan validasi checksum `billing_snapshot_calc_crc16()` dan `billing_snapshot_verify_integrity()`.

### B. Modul Tampilan HMI (`E3/app/display/inc/display.h` & `display.c`)
- **Penambahan Enum ID Halaman:**
  - `DISP_PAGE_MAX_DEMAND` (zz: `"19"`, OBIS: `"01.06"`, satuan `"kW"` 3 desimal, font 11x18).
- **Penambahan Field Struct `meter_measurements_t`:**
  - `max_demand_w`, `max_demand_timestamp`, `cumulative_max_demand_w`, `max_demand_reset_count`.
- **Integrasi Tampilan Frame SPLN Gambar 4 & ASCII CLI:**
  - Render frame SPLN: `obis_code = "01.06"`, `scroll_index_zz = "19"`, `unit = "kW"`.
  - Siklus auto-carousel berputar dari `DISP_PAGE_IDPEL` s.d. `DISP_PAGE_MAX_DEMAND`, lalu kembali ke awal atau menampilkan darurat alarm.

### C. Modul Layanan Aplikasi Meter (`E3/app/e3_meter_app.h` & `e3_meter_app.c`)
- Mengintegrasikan instance `s_md_tracker` dan `s_billing_ctx`.
- Deteksi otomatis arus netral abnormal (ERR23) dan daya aktif terbalik (ERR26).
- Penyediaan API publik:
  - `e3_meter_app_trigger_billing_snapshot()`
  - `e3_meter_app_get_billing_snapshots()`
  - `e3_meter_app_get_max_demand()`
- Penambahan pengaman `!s_is_ac_failed` pada penekanan tombol agar buzzer hening saat kondisi darurat pemadaman.

### D. Penambahan Uji Ketahanan & Audit Final (`E3/tests/test_final_system_stress.c`)
- Menjalankan simulasi rendam **10.000 siklus eksekusi** dengan variasi beban dinamis (1 kW hingga 11 kW).
- Menguji rollover billing bulanan sebanyak 15 kali (memastikan tepat 12 entri tertua yang dipertahankan dan lolos uji integritas CRC-16).
- Menguji render seluruh 24 halaman tampilan HMI SPLN Gambar 4.
- Menguji ketahanan kadensi buzzer 1.000 langkah tanpa penyimpangan fasa waktu.

---

## 3. Diff / Cuplikan Kode Kunci

```diff
--- a/E3/app/profiling/inc/load_profile.h
+++ b/E3/app/profiling/inc/load_profile.h
@@ -52,6 +52,38 @@ typedef struct {
     uint32_t total_written;                 /**< Total entri yang pernah ditulis (kumulatif) */
 } load_profile_ctx_t;
 
+/* --- Snapshot Billing Bulanan (12 Bulan) Sesuai SPLN D3.006:2021 Butir 6.13 --- */
+#define BILLING_MAX_SNAPSHOTS 12
+
+typedef struct {
+    uint32_t billing_epoch_sec;          /**< Timestamp snapshot billing */
+    uint64_t active_energy_import_wh;    /**< Stand Energi Aktif Total (Wh) */
+    uint64_t active_energy_wbp_wh;       /**< Stand Energi WBP (Wh) */
+    uint64_t active_energy_lwbp_wh;      /**< Stand Energi LWBP (Wh) */
+    uint64_t reactive_energy_varh;       /**< Stand Energi Reaktif Total (varh) */
+    uint32_t max_demand_w;               /**< Maximum Demand bulan tersebut (Watt) */
+    uint32_t max_demand_timestamp;       /**< Timestamp kejadian MD */
+    uint16_t crc16;                      /**< Checksum CRC-16 */
+} billing_snapshot_entry_t;
+
+/* --- Maximum Demand (MD) Tracker 15-Menit Sesuai SPLN Tabel E.1 No 4-7 --- */
+typedef struct {
+    uint32_t interval_sec;               /**< Periode integrasi MD (900 detik) */
+    uint32_t timer_sec;                  /**< Timer detik integrasi berjalan */
+    int64_t  power_accum_w_sec;          /**< Akumulasi Watt-detik */
+    uint32_t current_demand_w;           /**< Daya demand rata-rata interval terakhir */
+    uint32_t max_demand_w;               /**< Rekor MD tertinggi periode berjalan */
+    uint32_t max_demand_timestamp;       /**< Waktu RTC saat MD tercatat */
+    uint32_t cumulative_max_demand_w;    /**< Akumulasi MD kumulatif (CMD) */
+    uint32_t reset_count;                /**< Jumlah counter reset MD */
+} max_demand_tracker_t;

--- a/E3/app/display/inc/display.h
+++ b/E3/app/display/inc/display.h
@@ -46,6 +46,7 @@ typedef enum {
     DISP_PAGE_REACTIVE_ENERGY,
     DISP_PAGE_TIME,
+    DISP_PAGE_MAX_DEMAND,     /* zz: 19, OBIS: 01.06 (P_MAX, kW 3 desimal) */
     DISP_PAGE_POWER_LIMITER,  /* Tabel E.2 No 34: P_LIM kW */
```

---

## 4. Hasil Verifikasi, Kompilasi, dan Uji Host

### A. Pengujian Host Unit Test (CTest 7/7 Pass & Batch Runner 10/10 Pass)
```text
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo
1/7 Test #1: Integration_E3_Demo ..............   Passed    0.03 sec
    Start 2: Unit_E3_Display_Tamper
2/7 Test #2: Unit_E3_Display_Tamper ...........   Passed    0.03 sec
    Start 3: Unit_E3_Power_Relay
3/7 Test #3: Unit_E3_Power_Relay ..............   Passed    0.02 sec
    Start 4: Unit_E3_FSM_Statechart
4/7 Test #4: Unit_E3_FSM_Statechart ...........   Passed    0.02 sec
    Start 5: Unit_E3_Load_Profile
5/7 Test #5: Unit_E3_Load_Profile .............   Passed    0.02 sec
    Start 6: Unit_E3_SPLN_Display_Tamper
6/7 Test #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.03 sec
    Start 7: Stress_E3_Final_System_Audit
7/7 Test #7: Stress_E3_Final_System_Audit .....   Passed    0.18 sec

100% tests passed, 0 tests failed out of 7
```

```text
[PASS] E3 SPLN Display & Tamper
[PASS] E3 Power Relay & FSM
[PASS] E3 Load Profile 15-Min
[PASS] E3 FSM Statechart
[PASS] E3 App & Tamper FIFO
[PASS] E3 NVRAM Flash Storage
[PASS] E1 Metrology ADE9000
[PASS] E2 DLMS/COSEM HDLC
[PASS] E1 ADE9000_Metrology_2 Simulation
[PASS] E3 Stress & Final System Audit

>>> ALL 10 TEST SUITES (E1, E2, E3, Metrology_2) PASSED 100% <<<
```

### B. Kompilasi Target Fisik ARM Cortex-M33 (Ninja)
- **Target Secure:** `Secure/build/ALLIN_S.elf` -> **0 Error, 0 Warning**.
- **Target NonSecure:** `NonSecure/build/ALLIN_NS.elf` -> **0 Error, 0 Warning**.
```text
Memory region         Used Size  Region Size  %age Used
             RAM:       62208 B       512 KB     11.87%
           SRAM4:           0 B        16 KB      0.00%
             ROM:      122312 B         1 MB     11.66%
```
- Firmware beroperasi sangat efisien: hanya menggunakan **11.66% Flash ROM** dan **11.87% RAM**, meninggalkan margin headroom yang sangat besar (>88%) untuk kestabilan RTOS jangka panjang.

