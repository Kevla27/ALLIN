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
3. **Penyeimbangan Pasangan Baca/Tulis (*Read/Write*) dengan 2 Panah Searah Terpisah pada Seluruh 3 *Data Store* Terlokalisasi (`D1`–`D3`) di DFD Level 2 (Opsi 2):**
   - Sesuai kaidah DFD formal (Yourdon/DeMarco, Gane/Sarson, dan `Design For ECE.pdf` Bab 6.4), setiap panah aliran data wajib **100% searah murni (`endArrow=classic`, tanpa `startArrow` / `BiDir=0`)** karena setiap arah membawa paket data yang berbeda; interaksi dua arah wajib digambar sebagai **2 panah searah terpisah**.
   - **`D1` (`ds_d1` Memori Tarif ToU, Profil Beban 90-Hari & Billing 12-Bln):** Dilengkapi panah tulis `f_31_d1_w` (`3.1 -> D1`) dan panah baca `f_d1_31_r` (`D1 -> 3.1`).
   - **`D2` (`ds_d2` 3 Register Logbook Kejadian FIFO >= 30 Entri/Log):** Dilengkapi panah tulis `f_32_d2_w` (`3.2 -> D2`) dan panah baca `f_d2_32_r` (`D2 -> 3.2`).
   - **`D3` (`ds_d3` Flash Checkpoint Energi & Status Relai):** Dilengkapi panah tulis darurat `f_35_d3_w` (`3.5 -> D3`, beserta suplai `f_31_35_snap` & `f_33_35` ke `3.5`) dan panah baca restorasi saat boot `f_d3_35_r` (`D3 -> 3.5`).
4. **Konsolidasi Kompak 5 Sub-Proses Inti Pilar E3 (`3.1`–`3.5`, Opsi 2):**
   - **`3.1` (`p3_1`):** *Engine Billing, Tarif ToU & Profil Beban (15-M)* — menyatukan kalkulasi energi WBP/LWBP, *Billing* 12-bulan, integrasi 23-kanal 15-menit (90-hari), *Peak MD*, dan konfigurasi kontrak/RTC.
   - **`3.2` (`p3_2`):** *Deteksi Sabotase (9 Tamper), Alarm & Logbook FIFO* — evaluasi 9 vektor tamper, 8 alarm HW (`ERR00`..`ERR27`), 12 event, dan pencatatan 3 antrean Logbook FIFO.
   - **`3.3` (`p3_3`):** *Pengelola Relai & Proteksi (SPLN D3.006)* — FSM Relai (`CONNECTED`, `READY_FOR_RECONNECTION`, `LOCKED OUT`), *Zero-Crossing Switching*, proteksi beban/tegangan, dan AUX.
   - **`3.4` (`p3_4`):** *Antarmuka HMI OLED, Tombol & Konsol CLI* — 25 layar SPLN, rotasi `ERR` 3s, kedip LED `PB7`, buzzer `PB8`, dan parser diagnostik CLI (`USART2`).
   - **`3.5` (`p3_5`):** *Manajemen Daya & Last-Gasp (Stop 2)* — *Emergency Save* (`<15ms`) ke `D3`, *wakeup* layar 10s saat padam, *power gating*, dan Stop 2 (`4.5µA`).
5. **Penyeimbangan Aliran OBIS & Konfigurasi antara Pilar E3 dan Pilar E2 (`bnd_e2`) dengan 5 Panah Searah Terpisah:**
   - `f_31_e2` (`3.1 -> E2`: OBIS Billing, Tarif ToU & Profil Beban Cls 4 & 7) dan `f_e2_31` (`E2 -> 3.1`: Set Tarif ToU, Kontrak, IDPEL & Sync RTC Cls 8).
   - `f_32_e2` (`3.2 -> E2`: Trap Alarm Sabotase & Logbook OBIS).
   - `f_e2_33` (`E2 -> 3.3`: Perintah Disconnect/Reconnect Class 70) dan `f_33_e2` (`3.3 -> E2`: Status Relai & State Class 70).
6. **Pemurnian Frasa Benda Aliran Data (*Pure Data Flow Labels*):**
   - Mengganti label bernuansa pemicu kontrol menjadi paket data/status (`f_32_33`: `"Status & Vektor Sabotase Aktif (9 Tamper)"`, `f_35_34`: `"Mode Daya, Timer 10s & Power LCD (PD2/3)"`).

### B. Koreksi Pemetaan Pinout STM32U585 & ADE9000 Sesuai `PIN.docx` & `Desain Sistem.pdf`
- **Tamper Fisik:** Diperbarui dari `PE2/PE3/PB5` menjadi **`PC13` (`TAMP_CASE_OPEN` / `TAMP_IN1`)**, **`PE6` (`TAMP_TERM_COVER` / `TAMP_IN3` `ERR20`)**, dan **`PE4` (`SENSOR_MAGNET` / `EXTI4` `ERR25`)**.
- **Interupsi AFE ADE9000 & Evaluasi Tamper Elektrikal (`ERR21`–`ERR27`):** Dipetakan secara akurat ke **`PB5` (`ADE_IRQ0`)** dan **`PB1` (`ADE_IRQ1`)** dari `Pilar E1` (`bnd_e1`) menuju `Proses 3.2` (`f_e1_32`).
- **Aliran Proteksi Relai & Tampilan Layar dari `Proses 3.1`:** Ditambahkan aliran `f_31_33` (`V_rms, I_rms, S_total & Batas Kontrak`) ke `Proses 3.3` dan aliran `f_31_34` (`Data Energi ToU, V/I/P/PF & IDPEL`) ke `Proses 3.4`.
- **Kontrol Bantu & Power Gating:** Dicantumkan pin **`PD10/PD14` (`AUX_CTRL_IN1/2`)**, **`PD8` (`AUX_CTRL_OUT1`)**, **`PD2/PD3` (`LCD_PWR_EN` / `LCD_BL_CTRL`)**, **`PD4/PD5` (`RS232_PWR_EN` / `MODEM_12V_EN`)**, serta **`PD11/PC2` (`VBAT_SENSE` / `VCAP_SENSE`)**.

### C. Koreksi Kapasitas Sesuai `SPLN D3.006:2021` & `PRD.md`
- **Vektor Tamper & Alarm:** Diperbarui dari 8 menjadi **9 Vektor Tamper (Tabel 6)**, **8 Alarm Perangkat Keras (`ERR00`–`ERR07`, Tabel 4)**, dan **12 Kejadian Event (Tabel 5)**.
- **Kapasitas Logbook (`D2`):** Diperbarui menjadi **3 Register Logbook FIFO Terpisah (Log Alarm, Log Event, Log Tamper) masing-masing >= 30 entri** (Tabel 18 SPLN D3.006:2021).
- **Kapasitas Profil Beban (`D1`):** Diperbarui dari 60 hari menjadi **Profil Beban 15-Menit (23-Kanal, 90 Hari / 3 Bulan) & Histori Penagihan Bulanan (12 Bulan)** pada NOR Flash SPI2 4 MByte (`PB12..PB15`).

---

## 3. Hasil Verifikasi Geometri, Kompilasi & Unit Test

1. **Verifikasi Geometri, Perutean Ortogonal & Kepatuhan Panah Searah Murni (`BiDir=0`):**
   - **Tab 1 (DFD Level 0):** Kanvas `1600 x 1060 px`, 8 vertices, 10 edges — `BiDir=0`, `EV=0`, `Collinear=0`, `LL=0`, `LV=0`.
   - **Tab 2 (DFD Level 1):** Kanvas `1880 x 1080 px`, 11 vertices, 19 edges — `BiDir=0`, `EV=0`, `Collinear=0`, `LL=0`, `LV=0`.
   - **Tab 3 (DFD Level 2 — Kompak Opsi 2 dengan Panah 100% Searah Murni):** Kanvas **`1680 x 1040 px`** (setara ukuran Level 0) dengan 5 Sub-Proses Inti (`3.1`–`3.5`), 3 *Data Store* Terlokalisasi (`D1`–`D3`), 7 Entitas Batas (total 16 vertices), dan **32 panah searah murni (`BiDir=0`, 2 panah terpisah untuk setiap pasangan Baca/Tulis dan Perintah/Respons)** — `BiDir=0`, `EV=0`, `Collinear=0`, `LL=0`, `LV=0`.
2. **Kompilasi Firmware C (`ninja -C NonSecure/build`):** **0 Error, 0 Warning**.
3. **Unit Test CTest (`ctest --test-dir E3/build`):** **12/12 Passed (100%)**.

