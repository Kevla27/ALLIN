# Pembuatan Data Flow Diagram (DFD) Level 0 & Level 1: Arsitektur Tiga Pilar (Fokus Pilar E3)

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Technical Design & Data Architecture Documentation (DFD Gane & Sarson / Yourdon & DeMarco)  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_dfd_diagram.drawio.xml`), Logika Bisnis Aplikasi E3 (`E3/app/`), Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`), Layanan Metrologi E1 (`E1/`), Layanan Protokol DLMS E2 (`E2/`)

---

## 1. Latar Belakang & Analisis Integrator (Kacamata Pilar E3)

Sebagai pengembang dan pemilik **Pilar E3 (Aplikasi Smart Meter)**, salah satu tantangan terbesar dalam arsitektur perangkat lunak tertanam (*embedded firmware*) adalah menjaga **batas wilayah yang bersih (*clean boundaries*)** agar tidak terjadi ketergantungan silang yang semrawut (*spaghetti dependency*).

Sistem Smart Meter 3-Fasa pada hakikatnya bertumpu pada **Pilar E3 sebagai Jantung Pemrosesan Data (Central Data Processing Engine)**:
- **Pilar E1 (Metrologi ADE9000):** Bertindak sebagai *Data Ingest* (pemasok murni) data tegangan, arus, daya, energi, dan frekuensi.
- **Pilar E2 (DLMS/COSEM):** Bertindak sebagai *Data Egress / Consumer* yang mengemas data objek OBIS ke dalam format tumpukan protokol untuk dibaca oleh HES PLN atau Petugas Lapangan.
- **Pilar E3 (Aplikasi Smart Meter):** Pusat gravitasi logika yang mengelola akumulasi billing/tarif ToU 8-zona, perekaman profil beban 15-menit, proteksi keselamatan pemutus beban relai (SPLN D3.006:2021), deteksi 8 vektor sabotase/tamper, tampilan visual OLED SSD1306, serta penyelamatan *checkpoint* energi darurat (*Last-Gasp Stop 2*) ke Flash NVM SPI2.

Untuk mengunci kontrak antarmuka data tersebut secara resmi dan memudahkan pengujian tipe/sertifikasi PLN, dibuatlah dokumen **Data Flow Diagram (DFD)** mandiri dalam format Draw.io XML dengan 2 tab: **Level 0 (Diagram Konteks)** dan **Level 1 (Dekomposisi Tiga Pilar - Fokus E3)** bertema *Black & White High-Contrast*.

---

## 2. Rincian Pemetaan Data Flow Diagram

### A. DFD Level 0: Diagram Konteks Sistem Smart Meter 3-Fasa
Memetakan entitas eksternal (*terminator*) yang berinteraksi langsung dengan batas sistem meter, dengan proses utama `0.0` dirancang murni sebagai **Kotak Hitam (*Black Box*)** tanpa mencantumkan rincian sub-pilar internal:
- **Proses 0.0:** `SISTEM SMART METER 3-FASA TERTANAM (Host MCU STM32U585 Cortex-M33)`.
- **Tata Letak Simetris Bebas Tumpang Tindih:**
  - 3 Entitas Atas (`y=100`): Sensor/Jaringan Listrik (Kiri), Pelanggan Listrik (Tengah), Petugas Lapangan PLN (Kanan).
  - 3 Entitas Bawah (`y=700`): Vektor Sabotase (Kiri), Instalasi Beban Pelanggan (Tengah), Sistem Pusat HES PLN (Kanan).
  - Proses 0.0 tepat di tengah (`x=540, y=350, w=340, h=220`) dengan jarak bebas vertikal/horizontal $\ge 130$ px.
- **Standarisasi Tata Bahasa Aliran Data (100% Frasa Benda):**
  Seluruh panah aliran data distandarisasikan menjadi frasa benda (*noun phrases*) murni tanpa kata kerja imperatif, dilengkapi `labelBackgroundColor=#ffffff` solid dan `spacing=3` agar garis tidak menembus teks saat diekspor ke dokumen laporan:
  1. `Sinyal Tegangan/Arus 3-Fasa, ZX & Status AC_FAIL` (Grid $\rightarrow$ Sistem)
  2. `Sinyal Pemicu Sabotase & Anomali Fisik (EXTI)` (Tamper $\rightarrow$ Sistem)
  3. `Sinyal Tombol Navigasi & Konfirmasi Rekoneksi (▲+▼)` (Pelanggan $\rightarrow$ Sistem)
  4. `Data Tampilan Layar OLED & Status LED PB7` (Sistem $\rightarrow$ Pelanggan)
  5. `Sinyal Pulsa Koil Relai Trip/Close (PC8/PC9)` (Sistem $\rightarrow$ Beban)
  6. `Sinyal Umpan Balik Status Kontak Aux (PC7)` (Beban $\rightarrow$ Sistem)
  7. `Paket Perintah Audit OBIS & Konfigurasi IDPEL (Optik)` (Petugas $\rightarrow$ Sistem)
  8. `Frame Respons HDLC Data Billing & Logbook (Optik)` (Sistem $\rightarrow$ Petugas)
  9. `Paket APDU Perintah Remote Disconnect & Sync RTC` (HES $\rightarrow$ Sistem)
  10. `Paket APDU DLMS, Profil Beban & Notifikasi Trap/Last-Gasp` (Sistem $\rightarrow$ HES)

### B. DFD Level 1: Dekomposisi Fungsional Tiga Pilar (Fokus Pilar E3)
Mendekomposisi proses internal menjadi 3 pilar utama dengan arsitektur tata letak terstruktur (*grid pipeline* 5-kolom) bebas tabrakan garis (*zero-collision*), tanpa offset parent group yang merusak perutean garis (*all parent="1"*), dan standarisasi aliran data frasa benda:
- **Arsitektur Tata Letak 5-Kolom Terstruktur:**
  - **Kolom 1 (Kiri Atas):** Entitas Masukan Fisik Grid (`ent_grid_l1`).
  - **Kolom 2 (Kiri):** Pilar E1 (`p1_0` Metrologi ADE9000 & `bridge_e1_e3`) di bagian atas; di bawahnya berbaris Entitas Eksternal Lokal (`ent_load_l1` Beban, `ent_tamper_l1` Sabotase, `ent_user_l1` Pelanggan) yang posisinya tepat sejajar horizontal dengan sub-proses E3 pasangannya.
  - **Kolom 3 (Tengah):** Pilar E3 (Jantung Logika Bisnis & Aplikasi Smart Meter):
    - Baris 1: `3.1 Billing ToU` & `3.2 Load Profile/MD`
    - Baris 2: `3.3 Pengelola Relai SPLN` (sejajar lurus horizontal dengan `ent_load_l1`)
    - Baris 3: `3.4 Deteksi Sabotase 8-Vektor` (sejajar lurus horizontal dengan `ent_tamper_l1`)
    - Baris 4: `3.5 Antarmuka HMI OLED & Tombol` (sejajar lurus horizontal dengan `ent_user_l1`)
    - Baris 5: `3.6 Manajemen Daya Last-Gasp` & `3.7 Console CLI`
    - Baris 6: 4 Data Stores NVM (`D1 Checkpoint`, `D2 Logbook FIFO`, `D3 Load Profile 60-hari`, `D4 Tabel Tarif ToU`)
  - **Kolom 4 (Tengah-Kanan):** Pilar E2 (`2.0 Tumpukan Protokol DLMS/COSEM`) melayani objek OBIS dan APDU remote disconnect.
  - **Kolom 5 (Kanan):** Entitas Eksternal Komunikasi AMI & Lapangan (`ent_hes_l1` HES 4G/NB-IoT dan `ent_opt_l1` Petugas Lapangan Port Optik).
- **Eliminasi Total Cacat Visual (Zero Out-of-Bounds & Zero Spaghetti):**
  - Mengubah seluruh elemen sel dan garis menjadi `parent="1"` agar terhindar dari *coordinate offset bug* Draw.io yang sebelumnya melempar panah keluar kanvas hingga x > 2300.
  - Menetapkan titik belok (*waypoint coordinates*) eksplisit pada celah kanal antar-kolom sehingga tidak ada satu pun garis panah yang menabrak teks maupun menembus badan kotak sub-proses lain.
  - Menggunakan latar belakang label solid putih (`labelBackgroundColor=#ffffff`) dengan padding yang pas agar garis tidak memotong teks.
- **Proses 1.0 (Pilar E1 - Metrologi Listrik ADE9000):** Menerima sinyal fisik 3-fasa & interrupt DREADY PE11; melakukan sampling ADC SPI1 8kSPS & konversi DSP; menyalurkan data pengukuran mentah ke jembatan sistem `meter_system_bridge.c` (`meter_measurements_t s_meas`).
- **Proses 3.0 (Pilar E3 - Logika Bisnis Aplikasi Smart Meter / Jantung Sistem):**
  - Menerima `Data Pengukuran Listrik Terfilter (meter_measurements_t)` dari bridge untuk diolah oleh seluruh sub-proses:
    - `3.1 Engine Billing & Tarif ToU (8-Zona)`
    - `3.2 Integrasi Profil Beban & Maximum Demand (15-Menit)`
    - `3.3 Pengelola Relai & Proteksi Beban (SPLN D3.006)`
    - `3.4 Deteksi Sabotase (8 Vektor Tamper) & Alarm`
    - `3.5 Antarmuka HMI (OLED Display SPLN & Tombol Navigasi)`
    - `3.6 Manajemen Daya & Penyelamatan Last-Gasp (Stop 2)`
    - `3.7 Interactive Serial Console CLI (USART2 / Port Uji)`
- **Proses 2.0 (Pilar E2 - Tumpukan Protokol DLMS/COSEM SPLN D5.002):**
  - Menerima `Snapshot Data Kamus Objek OBIS` dari E3 (`load_profile.c`, `billing_tariff.c`, relai).
  - Mengirim `Paket APDU Perintah Disconnect/Reconnect Class 70` ke E3 (`power_relay_manager.c`).
  - Menangani komunikasi framing HDLC & enkripsi AES-128-GCM dengan entitas eksternal:
    - `Sistem Pusat PLN (HES / AMI)` via Modem USART2.
    - `Petugas Lapangan PLN / HHU` via Port Optik USART3.
- **Data Stores Persisten NVM (Pilar E3):**
  - `D1 | NVRAM FLASH CHECKPOINT ENERGI & RELAI`: Ditulis oleh `3.6 Last-Gasp` saat listrik padam (Stop 2).
  - `D2 | RING BUFFER LOGBOOK KEJADIAN (FIFO)`: Ditulis oleh `3.4 Tamper` untuk mencatat 100 rekaman sabotase.
  - `D3 | BUFFER PROFIL BEBAN 15-MENIT (60 HARI)`: Ditulis oleh `3.2 Load Profile` untuk riwayat konsumsi daya.
  - `D4 | TABEL TARIF ToU & KONFIGURASI KONTRAK`: Dibaca oleh `3.1 Billing Engine` untuk jadwal tarif & daya kontrak.

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Dokumen Draw.io XML:**
   - Tersimpan di [`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) dengan 2 diagram terpisah (*tabs*).
   - Validasi struktur XML 100% lolos parsing `xml.etree.ElementTree`.
2. **Kompilasi Firmware C (STM32U585 GCC/Ninja):**
   ```
   ninja -C NonSecure/build
   ninja: Entering directory `NonSecure/build'
   ninja: no work to do.
   ```
   Status: **0 Error, 0 Warning**.
3. **Unit Testing CTest (12 Test Suites):**
   ```
   ctest --test-dir E3/build
   100% tests passed, 0 tests failed out of 12
   Total Test time (real) = 0.37 sec
   ```
   Status: **12/12 Passed (100%)**.

