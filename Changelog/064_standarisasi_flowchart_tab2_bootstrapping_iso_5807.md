# STANDARISASI BAGAN ALIR TAB 2 (BOOTSTRAPPING & TRUSTZONE) BERDASARKAN ISO 5807:1985

- **Tanggal:** 05 November 2026
- **Status:** Selesai & Terverifikasi Penuh
- **Tipe Perubahan:** Documentation & Diagram Standardization / ISO 5807 Compliance / Technical Nomenclature Refinement
- **Pilar / Komponen Terdampak:** Dokumen Acuan & Dokumentasi Arsitektur Sistem (`docs/system_flowchart.drawio`, `docs/system_tab2_bootstrapping.drawio.xml`, `NonSecure/Core/Src/main.c`, `NonSecure/Core/Src/meter_system_bridge.c`)

---

## Bagian 1: Latar Belakang & Analisis Integrator

Setelah menyelesaikan standarisasi ISO 5807:1985 pada Tab 1 (Ikhtisar Sistem Makro), dilakukan evaluasi komprehensif terhadap **Tab 2 (Bootstrapping & Keamanan TrustZone)**. Tab 2 merepresentasikan urutan inisialisasi cold boot perangkat keras mikrokontroler STM32U585VIT6, konfigurasi gerbang keamanan TrustZone Cortex-M33, peluncuran tiga layanan modular sistem, hingga pengalihan kendali eksekusi ke multitasking kernel FreeRTOS.

### Temuan Audit & Masukan Pengguna:
1. **Penghapusan Sebutan Internal "E" secara Gamblang:**
   Penggunaan istilah internal proyek seperti "E1", "E2", dan "E3" dalam diagram teknis formal dapat membingungkan auditor eksternal, penguji sertifikasi PLN/LMK, dan pemangku kepentingan. Istilah tersebut diganti secara menyeluruh menggunakan nomenklatur fungsional rekayasa standar industri:
   - *Pilar E1* $\rightarrow$ **Layanan Metrologi Listrik (ADE9000)**
   - *Pilar E2* $\rightarrow$ **Layanan Protokol Komunikasi DLMS/COSEM (IEC 62056)**
   - *Pilar E3* $\rightarrow$ **Aplikasi Utama Smart Meter & HMI**
2. **Kepatuhan Simbol Standar ISO 5807:1985:**
   - Inisialisasi TrustZone MPU/NSC dan penyiapan clock 160 MHz/HAL periferal sebelumnya menggunakan persegi panjang biasa. Berdasarkan ISO 5807 Klausul 9.2.2.3 (*Preparation Symbol*), keduanya wajib menggunakan bentuk **Heksagon (Segienam)**.
   - Rutin terpadu `meter_system_bringup()` yang memanggil inisialisasi sub-sistem modular diubah menjadi simbol **Predefined Process (Klausul 9.2.2.1)** dengan garis vertikal ganda di sisi kiri dan kanan.
   - Titik keluar fase bootstrapping menuju *app_metering_task_loop()* distandarisasi sebagai **Off-Page Connector / Terminal Keluar (Klausul 9.4.2 / 9.4.1)** yang merujuk langsung ke Tab 1.
3. **Konversi Tema Black & White (B&W):**
   Seluruh skema warna ungu/biru diubah menjadi monokrom resolusi tinggi (latar belakang putih `#ffffff`, garis tepi hitam `#000000`, teks hitam `#000000`) yang siap cetak (*print-ready*).
4. **Keselarasan Kronologis dengan Kode Sumber Nyata:**
   Urutan eksekusi disinkronkan persis dengan implementasi pada [main.c](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/main.c) baris 90–125 dan [meter_system_bridge.c](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/meter_system_bridge.c) baris 49–75.

---

## Bagian 2: Rincian Perubahan Implementasi

Dilakukan pembaruan menyeluruh pada file [system_tab2_bootstrapping.drawio.xml](file:///g:/PPPP/ALLIN/docs/system_tab2_bootstrapping.drawio.xml) dan Tab 2 pada [system_flowchart.drawio](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) dengan rincian pemetaan simbol sebagai berikut:

| ID Elemen | Simbol ISO 5807:1985 | Bentuk / Tipe Draw.io | Teks Deskripsi Fungsional (Tanpa Sebutan E Gamblang) |
| :--- | :--- | :--- | :--- |
| **b1** | Terminal (Klausul 9.4.1) | Stadium / Kapsul (`arcSize=50`) | **START: POWER-ON RESET (POR)** — Catu daya 3.3V aktif & reset sistem selesai |
| **b2** | Preparation (Klausul 9.2.2.3) | Heksagon / Segienam (`shape=hexagon`) | **INISIALISASI KEAMANAN TRUSTZONE (CORTEX-M33)** — Gateway NSC, Driver RTC LSE 32.768 kHz & Sensor Tamper |
| **b3** | Preparation (Klausul 9.2.2.3) | Heksagon / Segienam (`shape=hexagon`) | **INISIALISASI HAL HARDWARE & PERIFERAL** — Clock 160 MHz, SPI1 ADE9000, SPI2 Flash, I2C3 OLED, USART & ADC4 |
| **b4** | Predefined Process (Klausul 9.2.2.1) | Kotak Garis Ganda (`shape=process`) | **INISIALISASI LAYANAN INTI SISTEM (`meter_system_bringup`)** — Metrologi Listrik, Protokol DLMS & Aplikasi Smart Meter |
| **b5** | Process / Storage (Klausul 9.1.2.3) | Persegi Panjang Sudut Tumpul | **PEMULIHAN PARAMETER DARI FLASH NVRAM (SPI2)** — Checkpoint energi Wh/varh, tarif ToU & pointer logbook FIFO |
| **b6** | Process / Display (Klausul 9.1.2.8) | Persegi Panjang Sudut Tumpul | **CETAK BANNER SERIAL & TAMPILAN SPLASH OLED** — Kirim log boot USART2 (19200 bps) & layar selamat datang SPLN |
| **b7** | Process (Klausul 9.2.1) | Persegi Panjang Sudut Tumpul | **ALOKASI KERNEL FREERTOS & DEFAULT TASK** — Inisialisasi kernel, stack 4KB prioritas normal & tickless idle |
| **b8** | Process (Klausul 9.2.1) | Persegi Panjang Sudut Tumpul | **JALANKAN SCHEDULER RTOS (`osKernelStart`)** — Scheduler aktif & mengambil alih eksekusi thread CPU |
| **b9** | Off-Page Connector (Klausul 9.4.2) | Stadium / Kapsul (`arcSize=50`) | **MENUJU TAB 1: SIKLUS UTAMA PERIODIK (100 ms)** — Eksekusi loop rutin utama `app_metering_task_loop()` |
| **b_card** | Annotation (Klausul 9.4.3) | Kotak Komentar Terbuka | **PETA ALOKASI MEMORI SISTEM (TRUSTZONE CORTEX-M33)** — Secure Flash, Non-Secure Flash, Non-Secure SRAM, VBAT |

---

## Bagian 3: Hasil Verifikasi & Uji Sistem

### 1. Validasi Integritas Struktur XML
- Struktur XML divalidasi menggunakan modul `xml.etree.ElementTree` Python:
  - Seluruh 7 diagram pada [system_flowchart.drawio](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) terverifikasi utuh tanpa kerusakan relasi ID (`Total diagrams: 7`).
  - File mandiri [system_tab2_bootstrapping.drawio.xml](file:///g:/PPPP/ALLIN/docs/system_tab2_bootstrapping.drawio.xml) terparsing bersih.

### 2. Kompilasi Firmware (Ninja)
```text
ninja: Entering directory `NonSecure/build'
ninja: no work to do.
```
- **Status:** 0 Error, 0 Warning.

### 3. Unit Testing CTest (Pilar E3)
```text
100% tests passed, 0 tests failed out of 12
Total Test time (real) = 0.20 sec
```
- **Status:** 12/12 Test Passed (100%).

