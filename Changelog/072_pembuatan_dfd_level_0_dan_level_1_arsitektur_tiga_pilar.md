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
- **Konsistensi Frasa Kata Benda (Noun Phrases):** Seluruh label aliran data diselaraskan menjadi murni kata benda (*data/signal packet*):
  - `Sinyal Tegangan/Arus Analog, ZX & Status AC_FAIL` (dari Jaringan Listrik PLN).
  - `Sinyal Pemicu Sabotase Fisik (EXTI)` (dari Vektor Sabotase).
  - `Sinyal Tombol Navigasi & Konfirmasi Rekoneksi` (dari Pelanggan).
  - `Data Tampilan Layar OLED & Indikator LED PB7` (ke Pelanggan).
  - `Sinyal Pulsa Koil Relai Trip/Close (PC8/PC9)` (ke Instalasi Beban).
  - `Sinyal Status Kontak Umpan Balik Relai (PC7)` (dari Instalasi Beban).
  - `Paket Perintah Audit OBIS & Konfigurasi IDPEL (Optik)` (dari Petugas HHU).
  - `Frame Respons HDLC Data Meter (Optik)` (ke Petugas HHU).
  - `Paket Perintah Remote Disconnect & Waktu RTC (Modem)` (dari Sistem HES).
  - `Paket APDU DLMS (Profil Beban, Trap & Last-Gasp)` (ke Sistem HES).
- **Kerapian Visual & Latar Belakang Solid:** Seluruh label panah dikonfigurasi dengan latar putih solid (`labelBackgroundColor=#ffffff;fillColor=#ffffff`) sehingga garis panah tidak menembus teks saat diekspor ke PDF/dokumen laporan, serta orientasi teks vertikal (`horizontal=0`) pada panah vertikal agar tata letak tetap estetis dan terbaca sempurna.
- **Entitas Eksternal:**
  1. **Sensor CT/VT & Jaringan Listrik PLN:** Memasok sinyal analog tegangan 3-fasa, arus, zero-crossing, dan sinyal deteksi pemadaman `AC_FAIL_DET`.
  2. **Vektor Sabotase & Anomali Fisik:** Memasok sinyal pemicu interupsi EXTI (tutup meter, tutup terminal, medan magnet kuat, netral bypass).
  3. **Pelanggan Listrik:** Mengirim sinyal masukan tombol navigasi (PA0/PB6) dan konfirmasi penekanan simultan ($\blacktriangle + \blacktriangledown$) 1 detik; menerima data tayangan OLED 6 baris dan indikator alarm LED PB7.
  4. **Instalasi Beban Pelanggan:** Menerima penyaluran daya (via pulsa koil trip PC8 / close PC9) dan mengirim sinyal umpan balik status kontak aux PC7.
  5. **Petugas Lapangan PLN (HHU):** Berinteraksi via Port Optik depan (USART3 / IEC 62056-21) untuk pembacaan audit billing/logbook lokal dan konfigurasi IDPEL/No Seri.
  6. **Sistem Pusat PLN (HES / AMI System):** Berinteraksi via Modem Seluler (USART2) untuk polling profil beban 15-menitan, sinkronisasi waktu RTC, perintah remote disconnect/reconnect (Class 70), serta menerima notifikasi seketika (*Push Trap* sabotase & *Last-Gasp*).

### B. DFD Level 1: Dekomposisi Fungsional Tiga Pilar (Fokus Pilar E3)
Mendekomposisi proses internal menjadi 3 pilar utama dan 4 *data store* permanen:
- **Proses 1.0 (Pilar E1 - Metrologi Listrik):** Akuisisi ADC SPI1, kalkulasi RMS, daya P/Q/S, power factor, dan pulsa energi. Disalurkan via bridge buffer `meter_measurements_t s_meas`.
- **Proses 2.0 (Pilar E2 - Protokol DLMS/COSEM):** Framing HDLC, dekripsi AES-128-GCM, manajemen objek OBIS, dan pengiriman APDU.
- **Proses 3.0 (Pilar E3 - Logika Bisnis Aplikasi):**
  - `3.1 Engine Billing & Tarif ToU (8-Zona)`
  - `3.2 Integrasi Profil Beban & Maximum Demand (15-Menit)`
  - `3.3 Pengelola Relai & Proteksi Beban (SPLN D3.006)`
  - `3.4 Deteksi Sabotase (8 Vektor Tamper) & Alarm`
  - `3.5 Antarmuka HMI (OLED Display SPLN & Tombol)`
  - `3.6 Manajemen Daya & Penyelamatan Last-Gasp (Stop 2)`
  - `3.7 Interactive Serial Console CLI (USART2 / Port Uji)`
- **Data Stores (Penyimpanan Persisten):**
  - `D1 | NVRAM FLASH CHECKPOINT & STATUS RELAI`: Menyimpan stand energi Wh/varh darurat dan status relai.
  - `D2 | RING BUFFER LOGBOOK KEJADIAN (FIFO)`: Menyimpan 100 catatan rekaman sabotase dan pemadaman.
  - `D3 | BUFFER PROFIL BEBAN 15-MENIT (60 HARI)`: Menyimpan riwayat kanal profil beban dan MD bulanan.
  - `D4 | TABEL TARIF ToU & KONFIGURASI KONTRAK`: Menyimpan jadwal WBP/LWBP, IDPEL, dan nomor seri meter.

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

