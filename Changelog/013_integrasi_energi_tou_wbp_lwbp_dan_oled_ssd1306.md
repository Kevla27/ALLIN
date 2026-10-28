# Changelog 013: Integrasi Energi Aktif per Tarif (WBP & LWBP), Koneksi Driver Layar OLED SSD1306 (I2C3), dan Feedback Audio Tombol

**Tanggal:** 27 Oktober 2026  
**Status:** Selesai & Terverifikasi (Kompilasi ARM ELF 0 Error 0 Warning; 6/6 E3 CTest Pass; 9/9 Batch Test Pass)

---

## 1. Latar Belakang & Kebutuhan Regulasi
Melanjutkan pemenuhan standar **SPLN D3.006: 2021 Lampiran E** (Halaman 48–49) dan implementasi antarmuka perangkat keras fisik:
1. **Tabel E.1 No. 2 & No. 3 (Auto-Scroll Display):**
   - Mengharuskan parameter **Energi aktif kirim Tarif 1 (WBP) - `EA_EXP1`** dan **Energi aktif kirim Tarif 2 (LWBP1) - `EA_EXP2`** ditampilkan sesaat dengan satuan **kWh (3 desimal)**.
2. **Tabel E.2 No. 7, 8, 11, 12 (Manual Scroll Display):**
   - Mengharuskan navigasi tombol dapat menampilkan stand energi aktif per zona tarif (WBP dan LWBP).
3. **Koneksi Hardware Display OLED 128x32 (I2C3 PC0=SCL, PC1=SDA):**
   - Menghubungkan modul rendering layout SPLN Gambar 4 ([`bsp_display_oled.c`](file:///g:/PPPP/ALLIN/E3/drivers/oled/src/bsp_display_oled.c)) ke siklus eksekusi aplikasi meter [`e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c).
4. **Indikasi Audio / Feedback Tombol (Pin PB7):**
   - Memberikan respons getaran suara / beep non-blocking (50 ms) saat tombol navigasi ditekan.

---

## 2. Rincian Perubahan Kode

### A. Modul Tampilan HMI (`E3/app/display/inc/display.h` & `display.c`)
- **Penambahan Enum ID Halaman Display:**
  ```c
  DISP_PAGE_ACTIVE_ENERGY,    /* zz: 14, OBIS: 01.08   (Total kWh) */
  DISP_PAGE_ENERGY_WBP,       /* zz: 15, OBIS: 01.08.1 (Tarif 1 WBP, kWh 3 desimal) */
  DISP_PAGE_ENERGY_LWBP,      /* zz: 16, OBIS: 01.08.2 (Tarif 2 LWBP, kWh 3 desimal) */
  DISP_PAGE_REACTIVE_ENERGY,  /* zz: 17, OBIS: 03.08   (ER_BIL, kvarh 3 desimal) */
  DISP_PAGE_TIME,             /* zz: 18, OBIS: 01.00   (Jam & Tanggal WIB) */
  ```
- **Penambahan Field pada Struct `meter_measurements_t`:**
  - `uint64_t active_energy_wbp_wh;` (Tarif 1: WBP)
  - `uint64_t active_energy_lwbp_wh;` (Tarif 2: LWBP)
- **Implementasi Rendering Frame SPLN Gambar 4 (`display.c`):**
  - Case `DISP_PAGE_ENERGY_WBP`: OBIS `"01.08.1"`, zz `"15"`, format `"%lu.%03lu"`, unit `"kWh"`, font 11x18.
  - Case `DISP_PAGE_ENERGY_LWBP`: OBIS `"01.08.2"`, zz `"16"`, format `"%lu.%03lu"`, unit `"kWh"`, font 11x18.
  - Geser indeks zz untuk `DISP_PAGE_REACTIVE_ENERGY` menjadi `"17"` dan `DISP_PAGE_TIME` menjadi `"18"`.

### B. Konfigurasi Driver OLED SSD1306 (`E3/drivers/oled/inc/ssd1306_conf.h` & `ssd1306.c`)
- Dibuat file konfigurasi resmi `ssd1306_conf.h` dengan aktivasi mode I2C3, resolusi 128x32, serta include font `Font_6x8`, `Font_7x10`, dan `Font_11x18`.
- Diperbarui log diagnostik pada `ssd1306.c` menjadi `I2C3 (PC0=SCL, PC1=SDA)`.

### C. Build System Target ARM (`NonSecure/CMakeLists.txt`)
- Didaftarkan direktori include `${CMAKE_SOURCE_DIR}/../E3/drivers/oled/inc`.
- Didaftarkan source driver OLED ke target `ALLIN_NS`:
  - `bsp_display_oled.c`
  - `ssd1306.c`
  - `ssd1306_fonts.c`

### D. Integrasi Aplikasi Meter (`E3/app/e3_meter_app.c`)
- **Penyaluran Energi ToU ke HMI:**
  Data `s_tou_tariff_wh[0]` (WBP) dan `s_tou_tariff_wh[1] + s_tou_tariff_wh[2]` (LWBP) disalurkan ke `disp_meas.active_energy_wbp_wh` dan `disp_meas.active_energy_lwbp_wh`.
- **Inisialisasi & Boot Splash Screen Layar OLED Fisik:**
  Memanggil `bsp_display_oled_get_driver()->init()` dan menampilkan splash screen `"ALLIN SMART METER - SPLN D3.006:2021"`.
- **Rendering Layar Fisik Berkala:**
  Setiap interval 3000 ms, `spln_frame` dikirim langsung ke driver OLED `bsp_display_oled_get_driver()->render_frame(&spln_frame)`.
- **Feedback Audio Non-Blocking Tombol (PB7):**
  Saat tombol ditekan, timer `s_button_beep_ms = 50U` diaktifkan dan dipadamkan secara otomatis di `e3_meter_app_step()` tanpa memblokir interrupt EXTI.

### E. Pembaruan Suite Pengujian (`E3/tests/test_spln_display_tamper.c`)
- Ditambahkan pengujian verifikasi untuk `DISP_PAGE_ENERGY_WBP` (zz 15, OBIS 01.08.1, kWh).
- Ditambahkan pengujian verifikasi untuk `DISP_PAGE_ENERGY_LWBP` (zz 16, OBIS 01.08.2, kWh).
- Diperbarui validasi untuk `DISP_PAGE_REACTIVE_ENERGY` (zz 17) dan `DISP_PAGE_TIME` (zz 18).
- Ditambahkan pengujian verifikasi format ASCII CLI untuk WBP dan LWBP.

---

## 3. Hasil Pengujian & Verifikasi

### A. Host Unit & Integration Tests (CTest)
```
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo ..............   Passed    0.08 sec
    Start 2: Unit_E3_Display_Tamper ...........   Passed    0.08 sec
    Start 3: Unit_E3_Power_Relay ..............   Passed    0.08 sec
    Start 4: Unit_E3_FSM_Statechart ...........   Passed    0.07 sec
    Start 5: Unit_E3_Load_Profile .............   Passed    0.08 sec
    Start 6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.10 sec

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
- `Secure/build/ALLIN_S.elf`: **0 Error, 0 Warning**
- `NonSecure/build/ALLIN_NS.elf`: **0 Error, 0 Warning**
  - Clean rebuild 67/67 files sukses 100%
  - Penggunaan RAM: 61.432 B (11.72%)
  - Penggunaan Flash ROM: 119.360 B (11.38%)

