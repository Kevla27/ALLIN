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

## 2. Struktur Multi-Tab Dekomposisi Flowchart E3

Berkas [`docs/e3_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/e3_flowchart_ansi.drawio) kini memuat 4 tab terstruktur yang mengurai logika subsistem modular E3 secara tuntas:

### Tab 1: 1. Flowchart Utama E3 (Terurai Penuh)
Mengurai siklus periodik master (100 ms) dengan mengintegrasikan langsung logika dari Tab 2, 3, dan 4 ke dalam satu alur poros vertikal lurus:
1. `Mulai Siklus E3 (100 ms)` (Oval).
2. Input data metrologi E1, status tamper, dan data RTC (Jajar Genjang).
3. Evaluasi tarif ToU aktif (WBP/LWBP) & akumulasi kWh.
4. Evaluasi 9 vektor sabotase fisik & 8 alarm hardware internal.
5. **Percabangan Gangguan/Sabotase:**
   - Ya $\rightarrow$ Buka kontak relai pemutus (Trip PC8) & catat logbook kejadian 100 FIFO.
   - Evaluasi Lockout selesai $\rightarrow$ Set status `READY ON` (LED PB7 kedip 1 Hz).
   - Evaluasi Tombol (▲ + ▼) ditekan $\ge 1$ s $\rightarrow$ Tutup kontak relai (Close PC9) & status `CONNECTED`.
   - Tidak $\rightarrow$ Pertahankan status relai normal (`CONNECTED`).
6. **Percabangan Profil Beban 15-Menit:**
   - Ya $\rightarrow$ Rekam deret profil beban & perbarui Maximum Demand (MD) bulanan ke Flash NVM SPI2.
7. **Percabangan Antarmuka HMI OLED & Tombol:**
   - Evaluasi tombol navigasi (▲/▼) ditekan $\rightarrow$ Navigasi manual vs Auto-scroll carousel 5 detik.
   - Kompilasi framebuffer SPLN & kirim ke display OLED via I2C3.
8. Sinkronisasi register OBIS & respons perintah remote DLMS.
9. **Percabangan Pemadaman Listrik (AC_FAIL / PVD):**
   - Ya $\rightarrow$ Eksekusi Last-Gasp (<15 ms ke Flash) $\rightarrow$ Standby layar 10 detik $\rightarrow$ Padamkan layar & masuk mode Stop 2 (4.5 µA).
10. Output status relai, data billing, logbook, dan visual OLED (Jajar Genjang).
11. Tunda siklus 100 ms (osDelay) & loop kembali ke siklus berikutnya.

### Tab 2: 2. Detail Sabotase & Relai FSM (Dekomposisi Penuh)
Membedah mesin status relai (*Power Relay Manager*) dan evaluasi 9 vektor sabotase serta 8 parameter alarm internal (SPLN D3.006:2021 Tabel 4, 5, dan 6).

### Tab 3: 3. Detail Tampilan Layar HMI (Dekomposisi Penuh)
Membedah logika mesin tampilan OLED SSD1306: transisi auto-scroll 5 detik, navigasi manual tombol ▲/▼, rotasi layar error 3 detik, dan banner peringatan tegangan ekstrem (Lo-V / Hi-V).

### Tab 4: 4. Detail Listrik Padam & Stop 2 (Dekomposisi Penuh)
Membedah alur mitigasi pemadaman listrik: penanganan interupsi `AC_FAIL_DET`, penyimpanan *emergency checkpoint* Last-Gasp (<15 ms), *off-power standby* layar 10 detik, transisi ke STM32U5 Stop 2 (4.5 µA), dan penanganan *wakeup* (listrik PLN pulih vs tombol inspeksi manual).

---

## 3. Hasil Verifikasi Sistem

1. **Berkas Draw.io XML:**
   - Tersimpan di [`docs/e3_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/e3_flowchart_ansi.drawio).
   - Struktur XML 100% valid dan lolos verifikasi parsing `xml.etree.ElementTree`.
2. **Kompilasi Firmware C:**
   - `ninja -C NonSecure/build`: **0 Error, 0 Warning**.
3. **Unit Test CTest:**
   - `ctest --test-dir E3/build`: **12/12 Passed (100%)**.

