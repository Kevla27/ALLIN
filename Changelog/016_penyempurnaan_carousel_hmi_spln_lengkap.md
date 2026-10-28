# Changelog 016: Penyempurnaan 100% Carousel HMI SPLN D3.006: 2021 (Auto-Scroll Tabel E.1 & Manual-Scroll Tabel E.2)

**Tanggal:** 27 Oktober 2026  
**Status:** Selesai & Terverifikasi Penuh (Kompilasi ARM ELF 0 Error 0 Warning; 7/7 E3 CTest Pass; 10/10 Batch Test Pass)

---

## 1. Latar Belakang & Analisis Kelengkapan
Berdasarkan audit komparatif mendalam terhadap dokumen acuan **SPLN D3.006: 2021 Lampiran E**, dilakukan penyempurnaan menyeluruh pada carousel tampilan HMI smart meter untuk memastikan tidak ada satupun parameter tampilan standar yang tertinggal:

1. **Kelengkapan 100% Tabel E.1 (Auto-Scroll Display):**
   - **No 1:** Energi aktif total kirim (`DISP_PAGE_ACTIVE_ENERGY`, OBIS `01.08`, kWh)
   - **No 2:** Energi aktif kirim Tarif 1 WBP (`DISP_PAGE_ENERGY_WBP`, OBIS `01.08.1`, kWh)
   - **No 3:** Energi aktif kirim Tarif 2 LWBP (`DISP_PAGE_ENERGY_LWBP`, OBIS `01.08.2`, kWh)
   - **No 4:** Maximum demand aktif kirim (`DISP_PAGE_MAX_DEMAND`, OBIS `01.06`, kW 3 desimal, zz: `17`)
   - **No 5:** Tanggal & waktu kejadian demand maksimum (**BARU:** `DISP_PAGE_MD_TIME`, OBIS `01.06.0`, zz: `18`, format `DD/MM hh:mm`)
   - **No 6:** Kumulatif Maximum Demand (**BARU:** `DISP_PAGE_CMD`, OBIS `01.02`, zz: `19`, format `kW` 3 desimal)
   - **No 7:** Jumlah kejadian reset Maximum Demand (**BARU:** `DISP_PAGE_MD_RESET_COUNT`, OBIS `01.01`, zz: `20`, format counter `RST`)
   - **No 8:** Energi reaktif kirim (`DISP_PAGE_REACTIVE_ENERGY`, OBIS `03.08`, kvarh 3 desimal, zz: `21`)
   - **No 9:** Waktu saat ini Jam & Menit WIB (`DISP_PAGE_TIME`, OBIS `01.00`, zz: `22`)
   - **No 10:** Tanggal saat ini Hari, Bulan, Tahun (**BARU:** `DISP_PAGE_DATE`, OBIS `00.09.02`, zz: `23`, format `DD-MM-YYYY`)

2. **Penyempurnaan Tabel E.2 (Manual-Scroll Display):**
   - **No 1:** Uji segmen layar LCD (**BARU:** `DISP_PAGE_TEST_DISPLAY`, zz: `00`, teks `"88888888"` dan seluruh indikator menyala)
   - **No 17:** Stand energi aktif penagihan bulan lalu (**BARU:** `DISP_PAGE_HISTORICAL_BILLING`, OBIS `01.08*01`, zz: `24`, format `kWh`)

---

## 2. Rincian Perubahan Kode

### A. Modul Header Display (`E3/app/display/inc/display.h`)
- Penambahan halaman enum pada `display_page_id_t`:
  - `DISP_PAGE_MD_TIME`
  - `DISP_PAGE_CMD`
  - `DISP_PAGE_MD_RESET_COUNT`
  - `DISP_PAGE_DATE`
  - `DISP_PAGE_TEST_DISPLAY`
  - `DISP_PAGE_HISTORICAL_BILLING`
- Penambahan field `billing_last_month_wh` pada `meter_measurements_t`.

### B. Implementasi Engine Display (`E3/app/display/src/display.c`)
- **Fungsi Konversi Kalender Sipil Euclidean:**
  Implementasi `display_epoch_to_date()` yang ringan, presisi, dan deterministik untuk mengonversi UNIX epoch ke Hari, Bulan, Tahun, Jam, Menit, dan Detik tanpa overhead library eksternal.
- **Siklus Auto-Carousel:**
  Auto-carousel secara mulus berputar dari `DISP_PAGE_IDPEL` (zz: `01`) s.d. `DISP_PAGE_DATE` (zz: `23`), lalu kembali ke awal atau menampilkan halaman darurat sabotase/relai.
- **Render Frame SPLN Gambar 4 & ASCII CLI:**
  Implementasi tata letak OBIS, nilai numerik besar (Font 11x18) / teks Font 7x10, dan indeks urutan `zz` untuk seluruh 30 halaman sistem.

### C. Integrasi Aplikasi (`E3/app/e3_meter_app.c`)
- Pengambilan data otomatis dari `s_billing_ctx` (stand bulan lalu) ke `disp_meas.billing_last_month_wh`.

---

## 3. Hasil Kompilasi & Pengujian

### A. Pengujian Host (CTest & Batch Runner)
```text
Test project G:/PPPP/ALLIN/E3/build
1/7 Test #1: Integration_E3_Demo ..............   Passed
2/7 Test #2: Unit_E3_Display_Tamper ...........   Passed
3/7 Test #3: Unit_E3_Power_Relay ..............   Passed
4/7 Test #4: Unit_E3_FSM_Statechart ...........   Passed
5/7 Test #5: Unit_E3_Load_Profile .............   Passed
6/7 Test #6: Unit_E3_SPLN_Display_Tamper ......   Passed
7/7 Test #7: Stress_E3_Final_System_Audit .....   Passed

100% tests passed, 0 tests failed out of 7
```
```text
>>> ALL 10 TEST SUITES (E1, E2, E3, Metrology_2) PASSED 100% <<<
```

### B. Target ARM Cortex-M33 (Ninja)
- `ALLIN_S.elf`: **0 Error, 0 Warning**.
- `ALLIN_NS.elf`: **0 Error, 0 Warning** (Flash ROM: 11.87%, RAM: 11.87%).

