# Evaluasi & Sinkronisasi Penuh DFD Sistem (Level 0, Level 1, Level 2) terhadap Dokumen Acuan

**Tanggal:** 06 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Technical Design & Data Architecture Documentation (DFD Ford & Coulston / Gane & Sarson / Yourdon & DeMarco)  
**Komponen Terdampak:** Dokumentasi Sistem ([`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml))

---

## 1. Latar Belakang & Alasan Perubahan

Menindaklanjuti evaluasi komprehensif terhadap berkas [`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) menggunakan seluruh dokumen acuan di direktori `Acuan/` ([`Design For ECE.pdf`](file:///g:/PPPP/ALLIN/Acuan/Design%20For%20ECE.pdf) Bab 5 & Bab 6.4, [`Desain Sistem.pdf`](file:///g:/PPPP/ALLIN/Acuan/Desain%20Sistem.pdf), [`PIN.docx`](file:///g:/PPPP/ALLIN/Acuan/PIN.docx), [`PRD.md`](file:///g:/PPPP/ALLIN/Acuan/PRD.md), dan [`SPLN D3.006 2021 CSV.pdf`](file:///g:/PPPP/ALLIN/Acuan/SPLN%20D3.006%202021%20CSV.pdf)), dilakukan pemutakhiran menyeluruh pada ketiga diagram (**Tab 1: DFD Level 0**, **Tab 2: DFD Level 1**, dan **Tab 3: DFD Level 2**) untuk menuntaskan seluruh temuan metodologis, pemetaan pinout fisik, dan kapasitas fungsional.

---

## 2. Daftar Perbaikan yang Diterapkan

### A. Kepatuhan Kaidah Metodologi DFD & *Parent-Child Balancing* (`Design For ECE.pdf` Bab 6.4 Hal. 101–104)
1. **Sinkronisasi Aliran Sinyal Grid & Daya (`AC_FAIL_DET PC3`, `ADE_CF3_ZX PE8`, `VBAT PD11`, `VCAP PC2`):**
   - Ditambahkan aliran eksplisit `df_grid_e3` dari `ent_grid` ke `Proses 3.0` pada **DFD Level 1** untuk menyeimbangkan entitas `bnd_grid` pada **DFD Level 2**.
   - Ditambahkan aliran `f_grid_33` (`Sinyal Zero-Crossing ADE_CF3_ZX PE8`) dari `bnd_grid` ke `Proses 3.3` pada **DFD Level 2** untuk *Zero-Current Switching* relai.
2. **Penyeimbangan Antarmuka Konsol Diagnostik CLI (`USART2`) & Pulsa Kalibrasi Optik (`PE12/PE13`):**
   - Ditambahkan aliran `df_opt_e3_cli` dan `df_e3_opt_cli` antara `ent_opt` dan `Proses 3.0` pada **DFD Level 1** sehingga entitas `bnd_cli` (`f_cli_37` & `f_37_cli`) pada **DFD Level 2** seimbang 100% (*zero orphan interface*).
   - Ditambahkan aliran `df_e1_opt_cf` (`Pulsa Kalibrasi Optik CF1/CF2 PE12/PE13`) dari `Proses 1.0 (Pilar E1)` ke `ent_opt` sesuai `IF-09` pada `Desain Sistem.pdf`.
3. **Penyeimbangan Pasangan Baca/Tulis (*Read/Write*) pada Seluruh 4 *Data Store* (`D1`–`D4`) di DFD Level 2:**
   - **`D1` (`ds_d1` Checkpoint Energi & Status Relai):** Dilengkapi aliran tulis `f_36_d1` (beserta suplai `f_31_36` & `f_33_36` ke `3.6`) dan aliran baca restorasi saat boot `f_d1_31` ke `3.1`.
   - **`D2` (`ds_d2` 3 Register Logbook Kejadian FIFO):** Dilengkapi aliran tulis `f_34_d2` dan aliran baca riwayat `f_d2_34`.
   - **`D3` (`ds_d3` Buffer Profil Beban 90-Hari & Billing 12-Bln):** Dilengkapi aliran tulis `f_32_d3` dan aliran baca historis `f_d3_32`.
   - **`D4` (`ds_d4` Tabel Tarif ToU, Daya Kontrak & Identitas Meter):** Dilengkapi aliran baca `f_d4_31` dan aliran tulis pembaruan konfigurasi `f_31_d4`.
4. **Penyeimbangan Aliran OBIS & Konfigurasi antara Pilar E3 dan Pilar E2 (`bnd_e2`):**
   - Ditambahkan aliran `f_33_e2` (`Status Relai & State Class 70`) dari `Proses 3.3` ke `bnd_e2`.
   - Ditambahkan aliran `f_e2_31` (`Set Tarif ToU, Kontrak, IDPEL & Sync RTC Cls 8`) dari `bnd_e2` ke `Proses 3.1`.
5. **Pemurnian Frasa Benda Aliran Data (*Pure Data Flow Labels*):**
   - Mengganti label bernuansa pemicu kontrol menjadi paket data/status (`f_34_33`: `"Status & Vektor Sabotase Aktif (9 Tamper)"`, `f_36_35`: `"Mode Daya, Timer 10s & Power LCD (PD2/PD3)"`).

### B. Koreksi Pemetaan Pinout STM32U585 & ADE9000 Sesuai `PIN.docx` & `Desain Sistem.pdf`
- **Tamper Fisik:** Diperbarui dari `PE2/PE3/PB5` menjadi **`PC13` (`TAMP_CASE_OPEN` / `TAMP_IN1`)**, **`PE6` (`TAMP_TERM_COVER` / `TAMP_IN3` `ERR20`)**, dan **`PE4` (`SENSOR_MAGNET` / `EXTI4` `ERR25`)**.
- **Interupsi AFE ADE9000 & Evaluasi Tamper Elektrikal (`ERR21`–`ERR27`):** Dipetakan secara akurat ke **`PB5` (`ADE_IRQ0`)** dan **`PB1` (`ADE_IRQ1`)** dari `Pilar E1` (`bnd_e1`) menuju `Proses 3.4` (`f_e1_34`).
- **Aliran Proteksi Relai & Tampilan Layar dari `Proses 3.1`:** Ditambahkan aliran `f_31_33` (`V_rms, I_rms, S_total & Batas Kontrak`) ke `Proses 3.3` dan aliran `f_31_35` (`Data Energi ToU, V/I/P/PF & IDPEL Layar`) ke `Proses 3.5`.
- **Kontrol Bantu & Power Gating:** Dicantumkan pin **`PD10/PD14` (`AUX_CTRL_IN1/2`)**, **`PD8` (`AUX_CTRL_OUT1`)**, **`PD2/PD3` (`LCD_PWR_EN` / `LCD_BL_CTRL`)**, **`PD4/PD5` (`RS232_PWR_EN` / `MODEM_12V_EN`)**, serta **`PD11/PC2` (`VBAT_SENSE` / `VCAP_SENSE`)**.

### C. Koreksi Kapasitas Sesuai `SPLN D3.006:2021` & `PRD.md`
- **Vektor Tamper & Alarm:** Diperbarui dari 8 menjadi **9 Vektor Tamper (Tabel 6)**, **8 Alarm Perangkat Keras (`ERR00`–`ERR07`, Tabel 4)**, dan **12 Kejadian Event (Tabel 5)**.
- **Kapasitas Logbook (`D2`):** Diperbarui menjadi **3 Register Logbook FIFO Terpisah (Log Alarm, Log Event, Log Tamper) masing-masing >= 30 entri** (Tabel 18 SPLN D3.006:2021).
- **Kapasitas Profil Beban (`D3`):** Diperbarui dari 60 hari menjadi **Profil Beban 15-Menit (23-Kanal, 90 Hari / 3 Bulan) & Histori Penagihan Bulanan (12 Bulan)** pada NOR Flash SPI2 4 MByte (`PB12..PB15`).

---

## 3. Hasil Verifikasi Geometri, Kompilasi & Unit Test

1. **Verifikasi Geometri & Perutean Ortogonal Draw.io XML (Tata Letak Kompak Terlokalisasi):**
   - **Tab 1 (DFD Level 0):** Kanvas `1600 x 1060 px`, 8 vertices, 10 edges — `0` vertex overlaps, `0` edge-vertex hits, `0` collinear overlaps, `0` label collisions.
   - **Tab 2 (DFD Level 1):** Kanvas `1880 x 1080 px`, 11 vertices, 19 edges — `0` vertex overlaps, `0` edge-vertex hits, `0` collinear overlaps, `0` label collisions.
   - **Tab 3 (DFD Level 2 — Kompak Opsi 1):** Kanvas diperkecil dari `2020 x 1260 px` menjadi **`1640 x 1000 px`** (setara ukuran Level 0) dengan melokalisasi keempat *Data Store* (`D1`–`D4`) tepat di dekat proses utamanya (`D4` di atas `3.1`, `D3` di atas `3.2`, `D2` di samping `3.4`, `D1` di bawah `3.6`) dan menggabungkan pasangan panah bolak-balik (*Read/Write* & *Request/Response*) menjadi panah dua-arah (`↔`, 19 vertices, 26 edges pendek) — `0` vertex overlaps, `0` edge-vertex hits, `0` collinear overlaps, `0` label collisions.
2. **Kompilasi Firmware C (`ninja -C NonSecure/build`):** **0 Error, 0 Warning**.
3. **Unit Test CTest (`ctest --test-dir E3/build`):** **12/12 Passed (100%)**.

