# Pembuatan Salinan Flowchart Sistem Berstandar ANSI/ISO 5807 (Presisi E3 ANSI)

**Tanggal:** 06 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Documentation & System Architecture (ANSI/ISO 5807 Standard Flowchart)  
**Komponen Terdampak:** [`docs/system_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart_ansi.drawio)

---

## 1. Latar Belakang & Tujuan

Menindaklanjuti kebutuhan standarisasi visual formal untuk pelaporan teknis, pengujian kepatuhan (*compliance audit*), dan publikasi akademik, dibuatlah salinan diagram alir sistem makro dan subsistem tiga pilar meter dari berkas induk [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) menjadi format baku **ANSI/ISO 5807** yang 100% mengikuti standar presisi, perutean non-tabrakan, dan proporsi geometris dari [`docs/e3_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/e3_flowchart_ansi.drawio).

Berkas hasil konversi disimpan pada:
📁 **[`docs/system_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart_ansi.drawio)**

---

## 2. Struktur Multi-Tab Diagram ANSI

Berkas `docs/system_flowchart_ansi.drawio` memuat 5 tab terstruktur penuh:

1. **Tab 1: `1. Ikhtisar Sistem Makro`** (A4 Portrait, Poros Tengah $X = 413$)  
   - **Terminator:** `Mulai` (Elips, $120 \times 32$).
   - **Data Masukan:** `Catu daya utama, parameter kalibrasi, dan konfigurasi TrustZone` (Jajar Genjang, $340 \times 40$).
   - **Persiapan:** `Bootstrapping TrustZone & Inisialisasi Periferal` (Heksagon ANSI, $340 \times 40$).
   - **Inisialisasi RTOS:** `Inisialisasi Penjadwal FreeRTOS & Antrean Antar-Tugas` (Persegi, $340 \times 38$).
   - **Tiga Pilar:** Sub-proses E1 Metrologi ($340 \times 38$), E2 DLMS/COSEM ($340 \times 38$), dan E3 Aplikasi/HMI ($340 \times 38$).
   - **Sinkronisasi NVRAM:** Sinkronisasi status global & buffer NVRAM.
   - **Keputusan Catu Daya:** `Indikasi Listrik PLN Padam (AC_FAIL)` (Belah Ketupat, $180 \times 60$, tanpa tanda tanya) $\rightarrow$ Ya ke kanan ($200 \times 38$), Tidak ke bawah.
   - **Data Keluaran:** `Status relai, pulsa kalibrasi metrologi, respons DLMS, dan display HMI` (Jajar Genjang, $340 \times 40$).
   - **Keputusan Loop RTOS:** `Sistem Beroperasi Normal` $\rightarrow$ Ya ke kiri melalui `Tunda Siklus 100 ms (osDelay)` loopback ke E1; Tidak ke `Selesai`.
   - **Terminator:** `Selesai` (Elips, $120 \times 32$).

2. **Tab 2: `2. Layanan Metrologi Listrik`** (A4 Portrait, Poros Tengah $X = 413$)  
   - **Terminator:** `Mulai` $\rightarrow$ **Data Masukan:** Register ADE9000 & koefisien kalibrasi (Jajar Genjang).
   - **Proses:** Baca register V/I/P via SPI1.
   - **Keputusan:** `Mode Beban Sintetis Aktif` $\rightarrow$ Ya ke injeksi sintetis konsol CLI; Tidak ke konversi nilai riil ADC ADE9000.
   - **Kalkulasi:** Hitung daya aktif, reaktif, semu, & akumulasi energi 3-fasa.
   - **DSP & Pulsa:** Filter DSP, kompensasi drift, & pembangkit pulsa kalibrasi CF1/CF2.
   - **Data Keluaran:** Struktur pengukuran global `s_meas` & pulsa optik LED (Jajar Genjang) $\rightarrow$ **Terminator:** `Selesai`.

3. **Tab 3: `3. Layanan Protokol DLMS-COSEM`** (A4 Portrait, Poros Tengah $X = 413$)  
   - **Terminator:** `Mulai` $\rightarrow$ **Data Masukan:** Snapshot metrologi `s_meas` & frame request DLMS (Jajar Genjang).
   - **Pemetaan OBIS:** Pemetaan objek OBIS standar SPLN D3.002.
   - **Pembaruan Atribut:** Update register COSEM (Class ID 3 & 4) & profil beban Class ID 7.
   - **Keputusan Kriptografi:** `Enkripsi & Otentikasi Diperlukan` $\rightarrow$ Ya ke enkripsi AES-128-GCM & GMAC tag; Tidak langsung ke HDLC frame.
   - **Kompilasi HDLC:** Kompilasi respons APDU & pembentukan frame HDLC.
   - **Data Keluaran:** Frame respons HDLC/APDU & status sesi asosiasi (Jajar Genjang) $\rightarrow$ **Terminator:** `Selesai`.

4. **Tab 4: `4. Aplikasi Smart Meter & HMI`** (A4 Portrait, Poros Tengah $X = 413$)  
   - Mengadopsi 1:1 blueprint visual dan tata letak dari [`docs/e3_flowchart_ansi.drawio`](file:///g:/PPPP/ALLIN/docs/e3_flowchart_ansi.drawio):
   - `Mulai` $\rightarrow$ Data Masukan (Metrologi E1, RTC, telemetri baterai) $\rightarrow$ Baca Snapshot $\rightarrow$ Tarif ToU $\rightarrow$ Telemetri ADC4 $\rightarrow$ Proteksi Relai & Sabotase FSM $\rightarrow$ Irama Kedip LED PB7 $\rightarrow$ Integrasi Maximum Demand (15 Menit) $\rightarrow$ Keputusan `Interval 15 Menit Tercapai` (Ya ke Flash NVRAM SPI2) $\rightarrow$ Mesin Tampilan HMI OLED $\rightarrow$ Keputusan `Indikasi Listrik PLN Padam (AC_FAIL)` (Ya ke Mitigasi Stop 2) $\rightarrow$ Data Keluaran $\rightarrow$ `Selesai`.

5. **Tab 5: `5. Jalur Asinkron & ISR`** (A4 Landscape, Kolom Ganda $X = 270$ dan $X = 850$)  
   - **Kolom Kiri (UART RX ISR):** `Mulai` $\rightarrow$ Input byte UART RX $\rightarrow$ Keputusan `Format Karakter ASCII CLI` (Ya ke Konsol CLI; Tidak ke Ring Buffer) $\rightarrow$ Keputusan `Frame Lengkap & FCS Valid` $\rightarrow$ Dekode APDU DLMS $\rightarrow$ Eksekusi & Respons $\rightarrow$ Output frame TX $\rightarrow$ Re-arm interrupt (`HAL_UART_Receive_IT`) $\rightarrow$ `Selesai`.
   - **Kolom Kanan (GPIO EXTI ISR):** `Mulai` $\rightarrow$ Input sinyal GPIO EXTI $\rightarrow$ Cascade Keputusan: `Pin Terpicu AC_FAIL (PC3)`, `Pin Terpicu Tombol HMI (PA0)`, `Pin Terpicu ADE9000 IRQ (PB0)`, `Pin Terpicu Sinyal AUX (PB1)` $\rightarrow$ Bus penyatu $\rightarrow$ Output notifikasi event ke RTOS $\rightarrow$ Bersihkan pending flag EXTI (PR Register) $\rightarrow$ `Selesai`.

---

## 3. Matriks Keselarasan dengan Standar ANSI E3

| Elemen Diagram | Format Sebelumnya (Salinan Lama) | Format Standar ANSI E3 (Terkini) |
| :--- | :--- | :--- |
| **Simbol Terminal** | Berisi teks fungsi C panjang & tag `<b>` | Murni bertuliskan `Mulai` dan `Selesai` (Elips $120 \times 32$) |
| **Simbol Data I/O** | Hanya ada di titik tertentu | Jajar Genjang Input setelah `Mulai`, Jajar Genjang Output sebelum `Selesai` |
| **Simbol Keputusan** | Berisi tanda tanya `?` | Belah Ketupat dengan kalimat kondisi murni tanpa tanda tanya |
| **Perutean Percabangan** | Garis berpotongan dan menyamping | Pola baku E3: `Ya` ke kanan ($200 \times 38$), `Tidak` lurus ke bawah |
| **Dekorasi / Banner** | Banner judul abu-abu besar & catatan kaki navigasi | Dieliminasi total; halaman bersih dan fokus pada diagram baku |
| **Tipografi** | Campuran tag `<b>`, `<font color="...">` | Teks bersih font *Helvetica*, ukuran 10.5 pt (Terminal), 9.5 pt (Proses/IO), 9 pt (Decision), 8.5 pt (Label) |
| **Sumbu Utama** | Bergeser-geser | Terpusat presisi pada $X = 413$ (A4 Portrait) |
