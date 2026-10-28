# Log 017: Penambahan Halaman Carousel EA_EXP3 (Tarif 3) & EA_IMP (Energi Impor) SPLN D3.006

## 1. Konteks & Tujuan
Memenuhi secara 100% persyaratan **Tabel E.1 (Informasi Layar Tampilan Scrolling Otomatis)** standar acuan **SPLN D3.006: 2021** (halaman 72):
- **Tabel E.1 No. 4:** `EA_EXP3` (Energi Aktif Kirim Tarif 3 LWBP2, 3 desimal kWh, OBIS `01.08.3`, zz: `25`).
- **Tabel E.1 No. 6:** `EA_IMP` (Energi Aktif Terima / Impor Net-Metering PLTS Atap, 3 desimal kWh, OBIS `02.08`, zz: `26`).

---

## 2. Modifikasi Teknis

### A. Modul Tampilan (`E3/app/display/inc/display.h` & `display.c`)
1. **Penambahan ID Halaman Baru pada `display_page_id_t`:**
   - `DISP_PAGE_ENERGY_LWBP2`: Enum halaman untuk Tarif 3 LWBP2.
   - `DISP_PAGE_ENERGY_IMPORT`: Enum halaman untuk Energi Terima Impor PLTS.
   - Total halaman `DISP_PAGE_COUNT` bertambah dari 30 menjadi **32 Halaman**.
2. **Ekstensi Struktur Data `meter_measurements_t`:**
   - Ditambahkan field `uint64_t active_energy_lwbp2_wh;`
   - Ditambahkan field `uint64_t active_energy_import_wh;`
3. **Renderer SPLN Frame (`display_render_spln_frame`):**
   - Kasus `DISP_PAGE_ENERGY_LWBP2`: OBIS `"01.08.3"`, zz: `"25"`, Satuan `"kWh"` (3 desimal).
   - Kasus `DISP_PAGE_ENERGY_IMPORT`: OBIS `"02.08"`, zz: `"26"`, Satuan `"kWh"` (3 desimal).
4. **Renderer CLI (`display_render_frame`):**
   - Header L2: `"ENERGY LWBP2 (T3)"` dan `"ENERGY IMPORT (EA_IMP)"`.

### B. Integrasi Aplikasi Meter (`E3/app/e3_meter_app.c`)
- Memetakan akumulasi energi Time-of-Use zona ke-3 (`s_tou_tariff_wh[2]`) ke `disp_meas.active_energy_lwbp2_wh`.
- Memetakan `meas->active_energy_import_wh` ke `disp_meas.active_energy_import_wh`.

### C. Pengujian Unit Test & Stress Audit
- Diperbarui [`test_spln_display_tamper.c`](file:///g:/PPPP/ALLIN/E3/tests/test_spln_display_tamper.c):
  - Uji render SPLN Gambar 4 untuk `DISP_PAGE_ENERGY_LWBP2` (zz: 25, OBIS 01.08.3, 14.750 kWh) [PASS].
  - Uji render SPLN Gambar 4 untuk `DISP_PAGE_ENERGY_IMPORT` (zz: 26, OBIS 02.08, 5.200 kWh) [PASS].
  - Uji render CLI Frame baris 2 dan 3 [PASS].
- Pengujian [`test_final_system_stress.c`](file:///g:/PPPP/ALLIN/E3/tests/test_final_system_stress.c):
  - Audit seluruh **32 Halaman Tampilan** render tanpa cacat [PASS].

---

## 3. Hasil Pengujian
- **CTest E3/build:** 7/7 passed (100%).
- **Target ARM Cortex-M33 Target Build (`Secure` & `NonSecure`):**
  - `ALLIN_S.elf` & `ALLIN_NS.elf`: **0 Error, 0 Warning**.
  - NonSecure RAM: 11.87% (62.256 / 512.000 B).
  - NonSecure ROM: 11.91% (124.880 / 1.048.576 B).

