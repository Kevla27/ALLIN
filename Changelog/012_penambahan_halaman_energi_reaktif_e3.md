# Changelog 012: Penambahan Halaman Tampilan Energi Reaktif (ER_BIL) pada Pilar E3

**Tanggal:** 27 Oktober 2026  
**Status:** Selesai & Terverifikasi (Target ARM ELF 0 Error, 0 Warning; 6/6 E3 CTest Pass; 9/9 Batch Test Pass)

---

## 1. Latar Belakang & Kebutuhan Regulasi
Berdasarkan dokumen acuan **SPLN D3.006: 2021 Lampiran E** (Halaman 48–49):
- **Tabel E.1 No. 8 (Scrolling Otomatis / Auto Carousel):**  
  Mengharuskan parameter **Energi reaktif billing (kompensasi) - `ER_BIL`** dengan satuan **kVARh (3 desimal)** ditampilkan pada urutan layar sesaat.
- **Tabel E.2 No. 18 (Scrolling Tombol / Manual Scroll):**  
  Mengharuskan parameter energi reaktif billing sesaat (`ER_BIL`, 3 desimal) dapat diakses melalui navigasi tombol.
- **DLMS/COSEM OBIS Code:**  
  Sesuai standar DLMS Blue Book & SPLN D3.006, kode register untuk Energi Reaktif Billing kumulatif adalah `03.08` (`1.0.3.8.0.255`).

Pilar E1 telah mengakumulasi energi reaktif 4-kuadran dan memetakan total energi reaktif ke dalam field `meas->reactive_energy_varh`. Pembaruan ini menghubungkan data metrologi tersebut secara penuh ke antarmuka pengguna (HMI/Display) Pilar E3.

---

## 2. Rincian Perubahan Kode

### A. Modul Tampilan HMI (`E3/app/display/inc/display.h`)
- Menambahkan ID halaman baru `DISP_PAGE_REACTIVE_ENERGY` pada enum `display_page_id_t` tepat setelah `DISP_PAGE_ACTIVE_ENERGY` dan sebelum `DISP_PAGE_TIME`:
  ```c
  typedef enum {
      ...
      DISP_PAGE_FREQUENCY,
      DISP_PAGE_ACTIVE_ENERGY,
      DISP_PAGE_REACTIVE_ENERGY,  /* SPLN D3.006: 2021 Lampiran E Tabel E.1 No 8 (ER_BIL) */
      DISP_PAGE_TIME,
      DISP_PAGE_TAMPER_ALARM,
      DISP_PAGE_RELAY_READY_ON,
      DISP_PAGE_COUNT
  } display_page_id_t;
  ```

### B. Implementasi Tampilan LCD / SPLN Frame (`E3/app/display/src/display.c`)
1. **Render Frame Mode Teks/CLI (`display_render_frame`):**
   - Menambahkan case untuk `DISP_PAGE_REACTIVE_ENERGY`:
     - Baris 2: `"REACTIVE ENERGY (ER_BIL)"`
     - Baris 3: `"%lu.%03lu kvarh"`
2. **Render Frame Standar SPLN Gambar 4 (`display_render_spln_frame`):**
   - Menambahkan handler `case DISP_PAGE_REACTIVE_ENERGY:`
     - `obis_code`: `"03.08"`
     - `scroll_index_zz`: `"15"`
     - Format Nilai: `"%lu.%03lu"` (3 desimal presisi)
     - `unit`: `"kvarh"`
     - `is_large_font`: `true` (Font 11x18)
   - Menggeser nomor urut scroll untuk `DISP_PAGE_TIME` menjadi `zz = "16"` (Font 7x10).
3. **Logika Navigasi & Carousel:**
   - Navigasi tombol (UP / DOWN) dan auto-carousel tetap aman dan mulus tanpa jeda karena `DISP_PAGE_TIME` tetap berada tepat sebelum `DISP_PAGE_TAMPER_ALARM`.

### C. Build System CTest (`E3/CMakeLists.txt`)
- Mendaftarkan target executable dan unit test CTest ke-6:
  ```cmake
  # 6. Unit Test: SPLN Display & Tamper Formats
  add_executable(test_spln_display_tamper tests/test_spln_display_tamper.c)
  target_link_libraries(test_spln_display_tamper PRIVATE e3_core)
  add_test(NAME Unit_E3_SPLN_Display_Tamper COMMAND test_spln_display_tamper)
  ```

### D. Unit Test (`E3/tests/test_spln_display_tamper.c` & `test_e3_app.c`)
- Menambahkan pengujian verifikasi untuk `DISP_PAGE_REACTIVE_ENERGY`:
  - Validasi OBIS `03.08`, zz `15`, satuan `kvarh`, format 3 desimal.
- Memperbarui nomor urut zz halaman waktu menjadi `16`.
- Menambahkan pengujian verifikasi format ASCII CLI untuk `DISP_PAGE_REACTIVE_ENERGY`.
- Memperbaiki warning format specifier `printf` di `test_e3_app.c`.

---

## 3. Hasil Pengujian & Verifikasi

### A. Host Unit & Integration Tests (CTest)
```
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo ..............   Passed    0.11 sec
    Start 2: Unit_E3_Display_Tamper ...........   Passed    0.08 sec
    Start 3: Unit_E3_Power_Relay ..............   Passed    0.08 sec
    Start 4: Unit_E3_FSM_Statechart ...........   Passed    0.08 sec
    Start 5: Unit_E3_Load_Profile .............   Passed    0.08 sec
    Start 6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.09 sec

100% tests passed, 0 tests failed out of 6
```

### B. Batch Test Runner (Semua Pilar E1, E2, E3, dan Simulasi ADE9000_Metrology_2)
```
[PASS] E3 SPLN Display & Tamper
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

### C. Kompilasi Target ARM (STM32U585 TrustZone)
- `Secure/build/ALLIN_S.elf`: **0 Errors, 0 Warnings**
- `NonSecure/build/ALLIN_NS.elf`: **0 Errors, 0 Warnings**
  - Penggunaan RAM: 60.880 B (11.61%)
  - Penggunaan Flash ROM: 103.488 B (9.87%)

