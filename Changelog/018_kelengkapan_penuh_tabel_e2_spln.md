# Log 018: Kelengkapan Penuh 100% Parameter Tampilan Scrolling Tombol (Tabel E.2) SPLN D3.006: 2021

## 1. Konteks & Tujuan
Memenuhi secara 100% persyaratan **Tabel E.2 (Informasi Layar Tampilan Scrolling Tombol / Manual)** standar acuan **SPLN D3.006: 2021** (halaman 72–73), yang mencakup parameter operasional teknisi dan pengujian tera di gardu PLN.

---

## 2. Modifikasi Teknis

### A. Ekstensi Modul Display (`E3/app/display/inc/display.h` & `display.c`)
1. **Penambahan 17 Halaman Baru ke `display_page_id_t`:**
   - **Aliran Balik & Impor PLTS:**
     - `DISP_PAGE_ACTIVE_POWER_IMP` (zz: 27, OBIS `02.07`, $P_{IMP}$ kW 3 desimal) - Tabel E.2 No 4
     - `DISP_PAGE_REACTIVE_POWER_IMP` (zz: 28, OBIS `04.07`, $Q_{IMP}$ kVAR 3 desimal) - Tabel E.2 No 6
     - `DISP_PAGE_ENERGY_IMP_WBP` (zz: 29, OBIS `02.08.1`, $EA_{IMP1}$ kWh 3 desimal) - Tabel E.2 No 11
     - `DISP_PAGE_ENERGY_IMP_LWBP1` (zz: 30, OBIS `02.08.2`, $EA_{IMP2}$ kWh 3 desimal) - Tabel E.2 No 12
     - `DISP_PAGE_ENERGY_IMP_LWBP2` (zz: 31, OBIS `02.08.3`, $EA_{IMP3}$ kWh 3 desimal) - Tabel E.2 No 13
     - `DISP_PAGE_REACTIVE_ENERGY_EXP` (zz: 32, OBIS `03.08.0`, $ER_{EXP}$ kVARh 3 desimal) - Tabel E.2 No 15
     - `DISP_PAGE_REACTIVE_ENERGY_IMP` (zz: 33, OBIS `04.08.0`, $ER_{IMP}$ kVARh 3 desimal) - Tabel E.2 No 16
   - **Diagram Fasor & Sudut Fasa:**
     - `DISP_PAGE_ANGLE_UR` (zz: 35, OBIS `14.81.0`, $Sdt\_UR$ derajat) - Tabel E.2 No 22
     - `DISP_PAGE_ANGLE_US` (zz: 36, OBIS `14.81.1`, $Sdt\_US$ derajat) - Tabel E.2 No 23
     - `DISP_PAGE_ANGLE_UT` (zz: 37, OBIS `14.81.2`, $Sdt\_UT$ derajat) - Tabel E.2 No 24
     - `DISP_PAGE_ANGLE_IR` (zz: 39, OBIS `11.81.0`, $Sdt\_IR$ derajat) - Tabel E.2 No 29
     - `DISP_PAGE_ANGLE_IS` (zz: 40, OBIS `11.81.1`, $Sdt\_IS$ derajat) - Tabel E.2 No 30
     - `DISP_PAGE_ANGLE_IT` (zz: 41, OBIS `11.81.2`, $Sdt\_IT$ derajat) - Tabel E.2 No 31
   - **Maksimum Demand Daya Semu & Utilitas:**
     - `DISP_PAGE_MAX_DEMAND_S` (zz: 42, OBIS `09.06.0`, $S_{MAX}$ kVA 3 desimal) - Tabel E.2 No 35
     - `DISP_PAGE_MD_TIME_S` (zz: 43, OBIS `09.06.0*0`, $TS_{MAX}$ timestamp) - Tabel E.2 No 36
     - `DISP_PAGE_EOB` (zz: 44, OBIS `00.09.01`, $EoB$ tanggal-jam `01-00`) - Tabel E.2 No 37
     - `DISP_PAGE_PLN_UNIT` (zz: 45, OBIS `96.01.0`, Unit PLN UP3 `"PLN UID"`) - Tabel E.2 No 39
   - Total halaman `DISP_PAGE_COUNT` kini menjadi **49 Halaman**.

2. **Penyempurnaan Arsitektur Navigasi Dual-Mode:**
   - **Mode Auto-Carousel:** Tetap membatasi perputaran otomatis pada parameter primer konsumen (17 halaman dari `DISP_PAGE_IDPEL` hingga `DISP_PAGE_DATE` lalu membungkus kembali ke `DISP_PAGE_IDPEL`), sehingga perputaran tetap gesit dan cepat.
   - **Mode Manual Push-Button (▲ / ▼):** Menampilkan seluruh 49 halaman lengkap untuk inspeksi teknisi gardu, dan otomatis kembali ke Auto-Carousel setelah 10 detik tanpa penekanan tombol (`manual_timeout_ms = 10000`).

3. **Struktur Data Metrologi (`meter_measurements_t`):**
   - Dilengkapi variabel sudut fasa `angle_ur_deg`, `angle_us_deg`, `angle_ut_deg`, `angle_ir_deg`, `angle_is_deg`, `angle_it_deg`.
   - Dilengkapi variabel daya semu `max_demand_s_va`, `max_demand_s_timestamp`.
   - Dilengkapi variabel utilitas `eob_day`, `eob_hour`, dan `pln_unit_code`.

---

## 3. Hasil Pengujian & Verifikasi

1. **Uji Unit Test (`test_spln_display_tamper.c`):**
   - Seluruh indeks `zz`, nilai numerik, satuan, dan OBIS DLMS terverifikasi akurat [PASS].
2. **Uji Audit Rendam (`test_final_system_stress.c`):**
   - Seluruh **49 Halaman Tampilan** render tanpa cacat dalam 10.000 siklus pengujian [PASS].
3. **Kompilasi Firmware Target ARM Cortex-M33 TrustZone:**
   - `Secure/build/ALLIN_S.elf` & `NonSecure/build/ALLIN_NS.elf`: **0 Error, 0 Warning**.
   - **ROM:** 129.000 B / 1 MB (12.30%).
   - **RAM:** 62.432 B / 512 KB (11.91%).

