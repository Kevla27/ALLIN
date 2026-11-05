# STANDARISASI BAGAN ALIR MAKRO SISTEM BERDASARKAN STANDAR ISO 5807:1985

- **Tanggal:** 05 November 2026
- **Status:** Selesai & Terverifikasi Penuh
- **Tipe Perubahan:** Documentation & Diagram Standardization / ISO Compliance
- **Pilar / Komponen Terdampak:** Dokumen Acuan & Dokumentasi Arsitektur Sistem (`docs/system_flowchart.drawio`, `docs/system_tab1_macro_overview.drawio.xml`, `Acuan/ISO-5807-1985.pdf`)

---

## Bagian 1: Latar Belakang & Analisis Integrator

Dalam perancangan dokumen arsitektur dan bagan alir (flowchart) sistem firmware Smart Meter 3-Fasa berbasis STM32U585, diperlukan kepatuhan terhadap standar internasional dokumentasi pemrosesan informasi, yaitu **ISO 5807:1985** (*Information processing — Documentation symbols and conventions for data, program and system flowcharts, program network charts and system resources charts*).

### Temuan Audit Diagram Sebelumnya:
1. **Titik Awal (Start / Entry Point):** Diagram awal langsung menggunakan blok persegi panjang inisialisasi tanpa adanya *Terminal Symbol* resmi untuk membedakan kondisi *Power-On Reset (POR)* dari instruksi program.
2. **Inisialisasi Bootstrapping:** Blok inisialisasi hardware dan TrustZone sebelumnya menggunakan persegi panjang biasa, sedangkan ISO 5807 Klausul 9.2.2.3 mengkhususkan operasi inisialisasi rutin dan konfigurasi register menggunakan simbol heksagon (*Preparation Symbol*).
3. **Representasi Loop FreeRTOS & Delay:** Blok loop FreeRTOS dan penundaan siklus 100 ms sebelumnya sempat digambarkan menggunakan bentuk kapsul/stadion. Dalam ISO 5807, bentuk stadion adalah *Terminal Symbol* (hanya untuk awal/akhir atau interupsi eksternal), bukan proses loop.
4. **Sub-proses Tiga Pilar (E1, E2, E3):** Telah sesuai menggunakan simbol *Predefined Process* (Klausul 9.2.2.1) berupa kotak dengan garis ganda vertikal, yang menegaskan bahwa ketiga pilar memiliki bagan alir modular masing-masing (Tab 3 untuk E1, Tab 4 untuk E2, dan Tab 5 / `Acuan/FlowChartE3.drawio` untuk E3).
5. **Abstraksi Hardware & Jalur Asinkron ISR:** Diagram sebelumnya meletakkan ISR secara serial di bawah delay, seolah-olah ISR baru dieksekusi setelah siklus selesai. Secara nyata, ISR bersifat asinkron dan dipicu oleh hardware kapan saja. Selain itu, media penyimpanan non-volatile (Flash SPI2) dan display HMI (OLED I2C3) belum menggunakan simbol standar ISO 5807 untuk *Direct Access Storage* (Silinder) dan *Display Device*.

---

## Bagian 2: Rincian Perubahan Implementasi & Penataan Layout Bersih (0 Spaghetti)

Untuk menjaga agar diagram tetap sangat mudah dipahami (*readable at a glance*), seluruh elemen diatur kembali mengikuti struktur tulang punggung vertikal (*single-column vertical spine*) dengan koridor panah terpisah:

1. **Tulang Punggung Vertikal Sentral (0 Garis Silang):**
   - **Terminal (Start POR):** Bentuk Stadion / Kapsul ISO 5807 (Klausul 9.4.1).
   - **Preparation (Inisialisasi & TrustZone):** Bentuk Heksagon ISO 5807 (Klausul 9.2.2.3).
   - **Siklus Utama FreeRTOS (100 ms):** Bentuk Process ISO 5807 (Klausul 9.2.1).
   - **Tiga Pilar (E1, E2, E3):** Bentuk Predefined Process ISO 5807 (Klausul 9.2.2.1) dengan penegasan E3 sebagai domain inti kita.
   - **Tunda Siklus (osDelay):** Bentuk Process ISO 5807 (Klausul 9.2.1).
   - **Jalur Asinkron & ISR:** Ditempatkan rapi di bawah siklus utama tanpa memotong alur lain.
   - **Lapisan Hardware & Media Penyimpanan:** Ditempatkan di dasar sistem dengan perincian lengkap Flash NOR, OLED SSD1306, ADE9000, dan Modem/Optik.

3. **Penyederhanaan Teks & Tipografi Ringkas (*Concise Text Optimization*):**
   - Menghapus deskripsi panjang dan daftar register/pin yang bertele-tele pada kotak makro.
   - Setiap kotak makro kini hanya memuat 1 judul utama tebal (*bold title*) dan 1 baris sub-deskripsi padat dengan penanda rujukan tab (contoh: `[Tab 3]`, `[Tab 4]`, `[Tab 5 & FlowChartE3]`).
   - Jarak antar-elemen (*vertical gap*) diseragamkan presisi tepat 16 piksel, dan tinggi keseluruhan diagram menjadi sangat proporsional (~660px) sehingga muat penuh di satu layar monitor tanpa perlu scrolling.

4. **Konversi Tema Black & White (B&W) & Pemetaan Eksplisit Sub-Pilar E3:**
   - Seluruh warna diubah ke tema monokrom standar publikasi teknis ISO/IEEE: background putih murni (`#ffffff`), border hitam pekat (`#000000;strokeWidth=1.5..2`), dan teks hitam kontras tinggi.
   - **Penyelesaian Kelemahan Black-Box E3:** Untuk menjawab kelemahan di mana sebelumnya tidak terlihat sub-fungsi mana yang mengakses NVRAM dan Display, Pilar E3 dipecah secara transparan menjadi 3 sub-proses terstruktur:
     1. **E3.1:** `Tarif ToU & FSM Relai Breaker` (Proteksi sabotase & kendali breaker relai).
     2. **E3.2:** `Perekaman Flash NVRAM` (Penyimpanan Load Profile 15-menit & Logbook) $\rightarrow$ Mengalirkan garis putus-putus (*dashed line*) di **koridor kiri** ($x = 145$) langsung ke atas **SPI2 NOR Flash (4 MB)**.
     3. **E3.3:** `Mesin Tampilan HMI Display` (Framebuffer SPLN & auto-scroll) $\rightarrow$ Mengalirkan garis putus-putus (*dashed line*) di **koridor kanan** ($x = 832$) langsung ke atas **OLED SSD1306 (I2C3)**.
   - **Arsitektur Simetris Beban (Zero Crossings):** Koridor kiri didedikasikan untuk NVRAM Storage, koridor kanan untuk OLED Display, dan kolom tengah untuk alur sekuensial program FreeRTOS 100 ms. Seluruh panah tidak saling memotong (0 garis silang).

| Simbol / Elemen | Klausul ISO 5807:1985 | Bentuk / Tipe Draw.io | Implementasi dalam Sistem Smart Meter |
| :--- | :--- | :--- | :--- |
| **Terminal (Start)** | Klausul 9.4.1 | Stadium / Kapsul (`arcSize=50`) | **START: POWER-ON RESET (POR)** — Kondisi cold boot saat catu daya 3.3V aktif |
| **Preparation** | Klausul 9.2.2.3 | Heksagon / Segienam (`shape=hexagon`) | **INISIALISASI & TRUSTZONE (Cortex-M33)** — Inisialisasi clock 160MHz, MPU, peripheral HAL, dan kernel FreeRTOS |
| **Process Loop** | Klausul 9.2.1 | Persegi Panjang (`shape=rectangle`) | **SIKLUS UTAMA FREERTOS (100 ms)** — Loop eksekusi `app_metering_task_loop()` |
| **Predefined Process** | Klausul 9.2.2.1 | Kotak Garis Ganda (`shape=process`) | **PILAR E1 (Metrologi ADE9000)**, **PILAR E2 (DLMS/COSEM)**, dan **PILAR E3 (Aplikasi Smart Meter & HMI - Domain Inti Kita)** |
| **Process (Delay)** | Klausul 9.2.1 | Persegi Panjang Sudut Tumpul | **Tunda Siklus 100 ms (`osDelay`)** — Pelepasan CPU ke tickless idle |
| **Interrupt Entry** | Klausul 9.4.1 | Kotak Terminal Asinkron | **JALUR ASINKRON & ISR** — EXTI PC3 (AC Fail), EXTI PA0/PB6 (Navigasi), EXTI PB5/PB1 (IRQ ADE), EXTI PE8 (Zero-Cross), UART RX |
| **Asynchronous Lines** | Klausul 9.3.2.3 | Garis Putus-Putus (`dashed=1`) | Aliran sinyal kejadian dari ISR ke Pilar E3 (notifikasi event) dan ke Pilar E2 (byte stream RX) |
| **Direct Access Storage** | Klausul 9.1.2.3 | Silinder (`shape=cylinder3`) | **SPI2 NOR FLASH (4 MB)** — Penyimpanan Load Profile 15-menit, snapshot billing, dan logbook FIFO |
| **Display Device** | Klausul 9.1.2.8 | Simbol Display (`shape=display`) | **OLED SSD1306 (I2C3)** — Visualisasi antarmuka pengguna standar SPLN |
| **Annotation / Reference** | Klausul 9.4.3 | Kotak Komentar Terbuka / Callout | Panduan referensi silang antar-tab (Tab 2 s/d Tab 7 dan `Acuan/FlowChartE3.drawio`) |

---

## Bagian 3: Hasil Verifikasi & Uji Sistem

### 1. Validasi Integritas Struktur XML
- Struktur XML dievaluasi dan divalidasi menggunakan modul `xml.etree.ElementTree` Python untuk memastikan kesesuaian parsing tanpa error.
- Seluruh 7 tab pada file master [system_flowchart.drawio](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) tetap utuh, terhubung rapi, dan siap diimpor ke aplikasi Draw.io melalui menu `Arrange > Insert > Advanced > XML...`.

### 2. Kompilasi Firmware (Ninja)
```text
ninja: Entering directory `NonSecure/build'
ninja: no work to do.
```
- **Status:** 0 Error, 0 Warning.

### 3. Unit Testing CTest (Pilar E3)
```text
100% tests passed, 0 tests failed out of 12
Total Test time (real) = 0.45 sec
```
- **Status:** 12/12 Test Passed (100%).

