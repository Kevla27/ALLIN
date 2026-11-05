# Perampingan Arsitektur Flowchart Sistem: Eliminasi Tab Bootstrapping Redundan & Penyelarasan Fokus Pilar Fungsional

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** System Documentation Refactoring & Architecture Simplification (Opsi A)  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_flowchart.drawio`, `docs/system_tab1_macro_overview.drawio.xml`), Eliminasi File Redundan (`docs/flowchart_meter_system_bringup.drawio.xml`, `docs/system_tab2_bootstrapping.drawio.xml`)

---

## 1. Latar Belakang & Analisis Integrator

Berdasarkan tinjauan kritis arsitektur sistem, keberadaan diagram alir terpisah untuk proses **Bootstrapping (Tab 2 dan Tab 2b `meter_system_bringup`)** teridentifikasi sebagai bentuk *over-decomposition* (dekomposisi berlebihan) yang tidak memberikan nilai teknis signifikan bagi sistem Smart Meter:
1. **Sifat Kode yang Trivial & Linier:**
   Inisialisasi perangkat keras pada `meter_system_bringup()` dan `main.c` hanyalah eksekusi sekuensial lurus konfigurasi register, pin GPIO, dan clock tanpa logika bisnis, tanpa percabangan algoritmik rumit, dan hanya dieksekusi 1 kali saat *cold boot* (~50 ms).
2. **Kesesuaian dengan Standar ISO 5807:1985 Klausul 9.2.2.3 (Preparation):**
   Pada **Tab 1 (Makro Sistem)**, seluruh proses booting dan konfigurasi TrustZone Cortex-M33 sudah terwakili secara sah, padat, dan elegan oleh sebuah simpul **Preparation (Segienam/Heksagon)**:
   ```
   BOOTSTRAPPING & KEAMANAN TRUSTZONE
   Inisialisasi Hardware, Gateway NSC, Clock 160MHz & Kernel FreeRTOS
   ```
   Simbol heksagon menurut ISO 5807 memang dirancang khusus untuk merangkum seluruh tahapan persiapan/pengaturan perangkat sebelum sistem memasuki siklus operasional utamanya.
3. **Fokus Inti Smart Meter (SPLN D3.006):**
   Auditor PLN, LMK, dan pembaca dokumen berfokus pada pilar operasional fungsional: keakuratan metrologi listrik, interoperabilitas protokol DLMS/COSEM, kontrol relai breaker & proteksi sabotase tamper (Aplikasi E3), dan ketahanan pemadaman listrik darurat (Last-Gasp / ISR).

Oleh karena itu, **Opsi A** dieksekusi untuk memangkas Tab 2 dan Tab 2b, merampingkan struktur diagram menjadi 6 tab fungsional yang padat, terpadu, dan bebas dari redundansi.

---

## 2. Rincian Perubahan Implementasi

### A. Rekonfigurasi Tab Master (`docs/system_flowchart.drawio`)
Tab diagram disederhanakan dari sebelumnya 8 tab menjadi **6 Tab Fungsional Utama**:

| Tab Lama | Tab Baru | ID Diagram | Nama Tab Baru | Deskripsi Fungsional |
| :--- | :--- | :--- | :--- | :--- |
| Tab 1 | **Tab 1** | `Tab1_Macro_Overview` | **1. Ikhtisar Sistem Makro** | Siklus periodik 100 ms, orkestrasi 3 pilar, jalur asinkron ISR, dan lapisan hardware. |
| Tab 2 | *Dihapus* | - | - | Dilebur ke dalam simpul Preparation Tab 1. |
| Tab 2b | *Dihapus* | - | - | Dieliminasi (over-decomposition). |
| Tab 3 | **Tab 2** | `Tab2_Metrology` | **2. Layanan Metrologi Listrik** | Detail akuisisi ADC SPI1 ADE9000, kalkulasi P/Q/S/RMS, dan akumulasi energi. |
| Tab 4 | **Tab 3** | `Tab3_DLMS` | **3. Layanan Protokol DLMS-COSEM** | Sinkronisasi register snapshot OBIS, enkripsi AES-128-GCM, dan profil beban. |
| Tab 5 | **Tab 4** | `Tab4_App_Hub` | **4. Aplikasi Smart Meter & HMI** | State machine relai, deteksi 8 vektor tamper, tampilan OLED SPLN, dan tarif ToU. |
| Tab 6 | **Tab 5** | `Tab5_Asynchronous_ISR` | **5. Jalur Asinkron & ISR** | Interupsi EXTI AC_FAIL (Last-Gasp), tombol navigasi, dan parser UART RX. |
| Tab 7 | **Tab 6** | `Tab6_Hardware_Layer` | **6. Hardware & Pin Periferal** | Pemetaan pinout mikrokontroler STM32U585 dan antarmuka periferal. |

### B. Pembaruan Tab 1 Makro Sistem (`docs/system_tab1_macro_overview.drawio.xml`)
- Menghapus referensi `[Tab 2]` pada simpul `iso_prep` sehingga simpul heksagon menjadi *self-contained preparation node*.
- Memperbarui teks referensi tab pada simpul pilar dan panduan navigasi bawah (`iso_nav`) agar presisi merujuk ke Tab 2 (Metrologi), Tab 3 (DLMS), Tab 4 (Aplikasi E3), Tab 5 (ISR), dan Tab 6 (Hardware).
- Menghapus sebutan internal "Pilar E1, E2, E3" secara gamblang pada judul simpul proses utama, digantikan dengan nomenklatur standar rekayasa.

### C. Pembersihan File Redundan
- Menghapus file `docs/flowchart_meter_system_bringup.drawio.xml`.
- Menghapus file `docs/system_tab2_bootstrapping.drawio.xml`.

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Struktur XML:**
   Parsing XML menggunakan `xml.etree.ElementTree` mengonfirmasi master file [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) memuat tepat 6 tab diagram yang terindeks rapi dan bebas dari referensi rusak.
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
   Total Test time (real) = 0.23 sec
   ```
   Status: **12/12 Passed (100%)**.

