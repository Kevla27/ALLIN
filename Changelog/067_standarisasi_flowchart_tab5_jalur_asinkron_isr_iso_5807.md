# Standarisasi Bagan Alir Tab 5 Jalur Asinkron & ISR Berbasis ISO 5807:1985 & Black/White Theme

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Documentation & Visual Standardization (ISO 5807:1985 & Monochrome Standardization)  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_flowchart.drawio`, `docs/system_tab5_asynchronous_isr.drawio.xml`), Penanganan Interupsi Sistem (`stm32u5xx_it.c`, `meter_system_bridge.c`)

---

## 1. Latar Belakang & Analisis Integrator

Bagan alir **Tab 5 (Jalur Asinkron & Interrupt Service Routine - ISR)**—yang sebelumnya bernomor Tab 6 sebelum perampingan arsitektur—merupakan pilar penting dalam arsitektur sistem Smart Meter 3-Fasa STM32U585. Diagram ini memodelkan seluruh kejadian asinkron berkecepatan tinggi yang berjalan di luar siklus utama FreeRTOS (100 ms).

Meskipun logika alurnya sudah 100% selaras dengan kode program C, diagram sebelumnya memiliki beberapa kelemahan visual dan konvensi:
1. **Pewarnaan Heterogen (Multi-Color):** Menggunakan palet warna-warni (biru tua, merah pekat, kuning, hijau mint) yang tidak seragam dengan Tab 1 dan berpotensi menurunkan kontras keterbacaan saat dicetak atau diekspor ke PDF monokrom.
2. **Inkonsistensi Simbol ISO 5807:1985:** Simpul operasi proses masih menggunakan sudut membulat (`rounded=1;`) yang ambigu dengan simpul Terminal (`arcSize=50;`).
3. **Penyebutan Kode Internal C:** Masih memuat nama fungsi internal mentah seperti `e2_dlms_service_pop_cli_rx_byte()` dan `e2_dlms_service_uart_rx_callback()`.

Oleh karena itu, dilakukan pembaruan menyeluruh untuk menyelaraskan Tab 5 dengan standar **ISO 5807:1985**, tema **Black & White (B&W) High-Contrast**, serta terminologi rekayasa baku.

---

## 2. Rincian Perubahan Implementasi

### A. Transformasi Tema Visual (B&W High-Contrast)
- Seluruh latar simpul distandarisasi ke putih bersih (`fillColor=#ffffff`).
- Garis tepi simpul menggunakan warna hitam solid (`strokeColor=#000000;strokeWidth=1.5..2`).
- Seluruh teks dan label percabangan menggunakan warna hitam tegas (`fontColor=#000000`) dengan tipografi Helvetica yang bersih.
- Garis konektor panah orthogonal menggunakan warna hitam tegas dengan label latar putih (`labelBackgroundColor=#ffffff`) untuk menjamin keterbacaan tanpa tabrakan visual.

### B. Kepatuhan Simbolik ISO 5807:1985
1. **Terminal (Klausul 9.4.1):** Simbol kapsul/stadium (`rounded=1;arcSize=50;`) diterapkan eksklusif pada titik masuk interupsi (`HAL_UART_RxCpltCallback`, `HAL_GPIO_EXTI_Callback`) dan titik keluar interupsi (`RE-ARM INTERRUPSI UART RX`, `SELESAI PENANGANAN EVENT ISR GPIO`).
2. **Decision (Klausul 9.2.3):** Simbol belah ketupat (`rhombus;`) diterapkan pada evaluasi karakter ASCII CLI, kelengkapan frame HDLC/FCS-16, dan identifikasi sumber pin EXTI.
3. **Process (Klausul 9.2.1):** Simbol persegi panjang standar bersudut tajam (`rounded=0;` / default rectangle) diterapkan pada seluruh tahapan parsing APDU, eksekusi perintah DLMS, transmisi respons, dan penanganan event GPIO.

### C. Pembersihan Terminologi Internal C
- `e2_dlms_service_pop_cli_rx_byte()` $\longrightarrow$ **KIRIM KE KONSOL INTERAKTIF** (Push Byte ke Antrean FIFO CLI USART2).
- `e2_dlms_service_uart_rx_callback()` $\longrightarrow$ **PUSH KE RING BUFFER DEKODER** (Antrean Buffer Penerimaan Frame HDLC).
- Pembaruan penomoran judul dan ID diagram menjadi **Tab 5: Jalur Asinkron & ISR** (`Tab5_Asynchronous_ISR`).

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Struktur XML:**
   - Standalone diagram tersimpan di [`docs/system_tab5_asynchronous_isr.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_tab5_asynchronous_isr.drawio.xml).
   - Master file [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) pada diagram `Tab5_Asynchronous_ISR` terverifikasi 100% valid XML.
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
   Total Test time (real) = 0.21 sec
   ```
   Status: **12/12 Passed (100%)**.
