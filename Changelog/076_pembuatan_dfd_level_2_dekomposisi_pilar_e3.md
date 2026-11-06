# Pembuatan DFD Level 2: Dekomposisi Fungsional Pilar E3 (Aplikasi Smart Meter)

**Tanggal:** 06 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Technical Design & Data Architecture Documentation (DFD Gane & Sarson / Yourdon & DeMarco)  
**Komponen Terdampak:** Dokumentasi Sistem ([`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml)), Modul Aplikasi E3 (`E3/app/`)

---

## 1. Latar Belakang & Analisis Integrator

Sebagai pemilik dan pengembang domain **Pilar E3 (Aplikasi Smart Meter)** sebagaimana digariskan dalam [`Acuan/SOP_SISTEM_KERJA_AI.md`](file:///g:/PPPP/ALLIN/Acuan/SOP_SISTEM_KERJA_AI.md), dekomposisi sistem tidak boleh berhenti hanya pada level makro Tiga Pilar (Level 1). Diperlukan visualisasi mendalam tingkat **DFD Level 2** yang membedah arsitektur internal **Proses 3.0 (Sub-Sistem Logika Aplikasi & Sistem)** menjadi sub-proses modular yang merefleksikan kebenaran implementasi *firmware* C nyata di folder `E3/app/`.

Dokumen ini melengkapi berkas Draw.io multi-tab [**`docs/system_dfd_diagram.drawio.xml`**](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) dengan menambahkan **Tab 3: `DFD Level 2: Dekomposisi Pilar E3`**.

---

## 2. Struktur Dekomposisi DFD Level 2

### A. Tujuh Sub-Proses Fungsional Pilar E3
1. **Proses 3.1: `BILLING ENGINE & TARIF ToU (8-ZONA)`** (`billing_tariff.c`)  
   Menerima struktur data `s_meas` dari Pilar E1 dan parameter skedul dari D4; menghitung energi aktif per zona tarif (WBP/LWBP), energi reaktif, serta menyiapkan snapshot billing bulanan untuk dikirim ke Pilar E2.
2. **Proses 3.2: `LOAD PROFILING & MAX DEMAND (15-M)`** (`load_profile.c`)  
   Melakukan integrasi daya aktif/reaktif tiap 15 menit, mencatat nilai puncak Maximum Demand (MD) bulanan, dan merekam data berkala ke memori profil beban `D3`.
3. **Proses 3.3: `PENGELOLA RELAI & PROTEKSI (SPLN D3.006)`** (`power_relay_manager.c`, `fsm_statechart.c`)  
   Menjalankan FSM relai kontaktor pembatas beban (`CONNECTED`, `READY ON`, `LOCKED OUT`), mengeksekusi pulsa koil Trip (`PC8`) dan Close (`PC9`), membaca feedback kontak aux (`PC7`), menerima perintah remote Class 70 dari E2, serta mengonfirmasi rekoneksi manual via tombol (▲+▼).
4. **Proses 3.4: `DETEKSI SABOTASE & MANAJEMEN ALARM`** (`tamper_manager.c`)  
   Mengevaluasi 8 vektor sabotase fisik (saklar tutup meter, terminal, sensor Hall magnet, bypass netral), memicu interupsi trip relai ke 3.3, mengatur pola irama kedip LED Alarm PB7 ke 3.5, mencatat riwayat 100 event ke `D2`, dan menyalurkan trap notifikasi ke E2.
5. **Proses 3.5: `ANTARMUKA HMI OLED & TOMBOL NAVIGASI`** (`display.c`, `oled.c`)  
   Mengatur auto-scroll carousel 5 detik (Tabel E.1), navigasi tombol manual (Tabel E.2), rotasi error 3 detik, kompilasi framebuffer OLED SSD1306 via I2C3, dan mendeteksi penekanan tombol simultan (▲+▼ $\ge 1\text{ s}$) untuk rekoneksi relai.
6. **Proses 3.6: `MANAJEMEN DAYA & LAST-GASP (STOP 2)`** (`bsp_power_mode.c`)  
   Mendeteksi sinyal interupsi pemadaman listrik `AC_FAIL_DET` (`PC3`), mengeksekusi checkpoint darurat ($<15\text{ ms}$) ke Flash NVRAM `D1`, mengelola standby layar 10 detik, dan mematikan periferal untuk transisi ke mode Stop 2 ($4.5\text{ }\mu\text{A}$).
7. **Proses 3.7: `KONSOL SERIAL DIAGNOSTIK (CLI)`** (`console_io.c`)  
   Mem-parsing perintah teks ASCII CLI dari teknisi lapangan melalui port serial, mengeksekusi injeksi beban sintetis diagnostik (`load`, `volt`, `curr`), dan mencetak status audit sistem.

### B. Empat Data Store Persisten NVM Terdedikasi
- **`D1 | FLASH CHECKPOINT ENERGI & STATUS RELAI`:** Buffer darurat Last-Gasp penahan data pemadaman listrik.
- **`D2 | RING BUFFER LOGBOOK KEJADIAN (FIFO)`:** 100 rekaman audit kejadian sabotase, perubahan status relai, dan alarm hardware internal.
- **`D3 | BUFFER PROFIL BEBAN 15-MENIT (60 HARI)`:** Deret berkala profil beban konsumsi energi 60 hari sesuai SPLN D3.006.
- **`D4 | TABEL TARIF ToU & KONFIGURASI KONTRAK`:** Skedul jam 8-zona tarif, daya kontrak VA, IDPEL, dan parameter meter.

### C. Batas Antarmuka Subsistem & Entitas
- **Batas Kiri:** Subsistem Metrologi E1, Sensor Sabotase Fisik, dan Deteksi Catu Daya (AC_FAIL).
- **Batas Bawah:** Pelanggan Listrik (Tombol & Layar) dan Instalasi Beban Pelanggan (Relai Breaker).
- **Batas Kanan:** Protokol DLMS/COSEM E2 (Modem/Optik) dan Port Uji Diagnostik CLI.

---

## 3. Hasil Verifikasi Sistem

1. **Integritas Parsing XML:**
   - Berkas [`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) memuat 3 diagram lengkap:
     - Tab 1: DFD Level 0 (8 vertices, 10 edges)
     - Tab 2: DFD Level 1 (11 vertices, 15 edges)
     - Tab 3: DFD Level 2 (19 vertices, 24 edges)
   - Seluruh 49 panah terhubung 100% valid tanpa *orphan edges* dan bebas tabrakan (*zero bounding box overlap*).
2. **Kompilasi Firmware C:**
   - `ninja -C NonSecure/build`: **0 Error, 0 Warning**.
3. **Unit Test CTest (Pilar E3):**
   - `ctest --test-dir E3/build`: **12/12 Passed (100%)**.
