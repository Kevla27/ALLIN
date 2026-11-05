# Pembuatan Flowchart Standar ANSI/ISO untuk Pilar E3 (Logika Aplikasi Smart Meter)

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Documentation & System Architecture (ANSI/ISO 5807 Standard Flowchart)  
**Komponen Terdampak:** Dokumentasi Sistem (`docs/e3_flowchart_ansi.drawio`), DFD Ringkas (`docs/system_dfd_diagram.drawio.xml`), Logika Aplikasi E3 (`E3/app/`)

---

## 1. Latar Belakang & Tujuan

Menindaklanjuti kebutuhan standarisasi format visual diagram sistem untuk laporan teknis / akademik, dibuatlah diagram **Flowchart Utama Pilar E3** yang mengadopsi format dan gaya visual ANSI/ISO 5807 (gaya vertikal terstruktur, notasi baku proses, keputusan, dan subrutin bergaris ganda) yang selaras dengan diagram alir Pilar E1.

Diagram ini menyajikan siklus hidup eksekusi *firmware* Pilar E3 (Aplikasi Smart Meter) secara sekuensial dan terstruktur, mulai dari inisialisasi, penanganan proteksi & sabotase, kalkulasi billing ToU, integrasi profil beban 15-menit, antarmuka HMI OLED/tombol, hingga mitigasi darurat pemadaman listrik (*Last-Gasp Stop 2*).

---

## 2. Rincian Simbol & Tahapan Alur Flowchart E3

1. **Terminator (Oval):**
   - `Mulai`: Titik awal saat sistem dinyalakan (*power-on reset*).
   - `Selesai`: Titik akhir siklus sebelum kembali ke pemantauan periodik.
2. **Data Masukan (Jajar Genjang):**
   - Menerima sinyal pengukuran metrologi ($V, I, P, Q, S$), status sensor sabotase (*tamper*), dan data *checkpoint* NVM SPI Flash.
3. **Inisialisasi Sistem Aplikasi (Persegi Panjang):**
   - Pemulihan *state machine* relai, verifikasi zona tarif ToU, dan konfigurasi periferal.
4. **Ingest Data Metrologi (Persegi Panjang):**
   - Penerimaan berkala data besaran teknik dari jembatan sistem (`meter_system_bridge.c`).
5. **Evaluasi Sabotase / Tamper (Belah Ketupat & Subrutin):**
   - Pengecekan interupsi EXTI saklar tutup/terminal meter & sensor magnetik.
   - Jika terdeteksi $\rightarrow$ Subrutin: Rekam logbook FIFO, aktivasi alarm PB7/buzzer, dan kunci relai beban (SPLN D3.006).
6. **Evaluasi Proteksi Beban (Belah Ketupat & Subrutin):**
   - Pengecekan beban lebih (*overpower* > batas kontrak) & tegangan abnormal (*OV/UV*).
   - Jika terdeteksi $\rightarrow$ Subrutin: Timer pemutus beban & pulsa koil trip relai (PC8).
7. **Perhitungan Billing & Tarif ToU (Subrutin Bergaris Ganda):**
   - Akumulasi energi aktif/reaktif zona WBP/LWBP dan kalkulasi kredit/kVAh.
8. **Integrasi Profil Beban 15-Menit (Belah Ketupat & Persegi Panjang):**
   - Pengecekan interval 15 menit $\rightarrow$ simpan deret rata-rata daya & Maximum Demand (MD) bulanan ke Flash NVM SPI2.
9. **Antarmuka HMI Display OLED & Tombol (Subrutin Bergaris Ganda):**
   - Pembaruan tampilan OLED 0.96" SSD1306 (OBIS Code SPLN) dan pemindaian tombol navigasi (PA0 ▲ / PB6 ▼).
10. **Komunikasi Protokol DLMS E2 (Belah Ketupat & Persegi Panjang):**
    - Pengecekan permintaan remote HES/HHU $\rightarrow$ eksekusi mutasi relai Class 70 dan sinkronisasi jam RTC.
11. **Penyelamatan Darurat Last-Gasp (Belah Ketupat & Subrutin Bergaris Ganda):**
    - Deteksi tegangan jatuh PVD VDD_MCU < 2.9V $\rightarrow$ simpan *checkpoint* energi & status relai (<15ms ke Flash SPI2) lalu transisi ke mode hemat daya STM32U5 Stop 2.
12. **Data Keluaran (Jajar Genjang):**
    - Status relai beban, tagihan energi kVAh/kWh, rekaman logbook, data profil beban, dan visualisasi OLED.

---

## 3. Hasil Verifikasi Sistem

1. **Berkas Draw.io XML:**
   - Tersimpan di [`docs/e3_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/e3_flowchart_ansi.drawio).
   - Struktur XML 100% valid dan lolos verifikasi parsing `xml.etree.ElementTree`.
2. **Kompilasi Firmware C:**
   - `ninja -C NonSecure/build`: **0 Error, 0 Warning**.
3. **Unit Test CTest:**
   - `ctest --test-dir E3/build`: **12/12 Passed (100%)**.
