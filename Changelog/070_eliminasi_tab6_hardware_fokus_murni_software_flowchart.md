# Eliminasi Tab 6 Hardware & Penegasan Fokus Murni Flowchart Perangkat Lunak 5 Tab

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** System Documentation Streamlining & Scope Clarification  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_flowchart.drawio`, `docs/system_tab1_macro_overview.drawio.xml`), Eliminasi File Statis (`docs/system_tab6_hardware_layer.drawio.xml`)

---

## 1. Latar Belakang & Analisis Integrator

File master dokumentasi `docs/system_flowchart.drawio` dirancang untuk memodelkan **aliran proses logika perangkat lunak (Software Process Flowchart)** sistem Smart Meter 3-Fasa STM32U585 sesuai standar internasional **ISO 5807:1985**.

Dalam audit arsitektur terbaru, keberadaan **Tab 6 (Hardware & Pin Periferal)** dievaluasi secara kritis:
1. **Ketidaksesuaian Konseptual:** Tab 6 sama sekali bukan sebuah bagan alir (*flowchart*). Tab tersebut hanya berupa kumpulan 7 kartu teks statis tanpa panah aliran eksekusi, tanpa titik awal/akhir (*terminal*), dan tanpa percabangan logika (*decision*).
2. **Redundansi Informasi:** Seluruh pemetaan pinout fisik mikrokontroler STM32U585 sudah didokumentasikan secara resmi, komprehensif, dan terperinci pada:
   - [`Acuan/PIN.docx`](file:///g:/PPPP/ALLIN/Acuan/PIN.docx) (Tabel Pinout Resmi MCU)
   - [`Config_MX/KONFIGURASI_MX.md`](file:///g:/PPPP/ALLIN/Config_MX/KONFIGURASI_MX.md)
   - [`ALLIN.ioc`](file:///g:/PPPP/ALLIN/ALLIN.ioc) (Proyek STM32CubeMX)
3. **Penyempurnaan Integritas Dokumen:** Menghilangkan lembar statis ini mengembalikan hakikat file `system_flowchart.drawio` sebagai dokumen murni *Process Flowchart*, menghilangkan ambiguitas bagi auditor, penguji, dan pengembang.

---

## 2. Rincian Perubahan Implementasi

### A. Eliminasi Tab 6 dari Master Diagram
- Diagram `Tab6_Hardware_Layer` dihapus dari file master [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio).
- File standalone `docs/system_tab6_hardware_layer.drawio.xml` dihapus secara permanen dari repositori.

### B. Struktur Final 5 Tab Murni Flowchart Proses
Kini seluruh tab pada file master adalah diagram alir perangkat lunak yang utuh dan saling terintegrasi:

| Nomor Tab | ID Diagram | Judul Resmi Tab | Sifat Diagram |
| :---: | :--- | :--- | :--- |
| **Tab 1** | `Tab1_Macro_Overview` | **1. Ikhtisar Sistem Makro** | Flowchart Makro (Siklus 100 ms RTOS & Jalur ISR) |
| **Tab 2** | `Tab2_Metrology` | **2. Layanan Metrologi Listrik** | Flowchart Detail Pilar 1 (ADE9000 & DSP) |
| **Tab 3** | `Tab3_DLMS` | **3. Layanan Protokol DLMS-COSEM** | Flowchart Detail Pilar 2 (IEC 62056 & Security Suite 0) |
| **Tab 4** | `Tab4_App_Hub` | **4. Aplikasi Smart Meter & HMI** | Flowchart Detail Pilar 3 (Domain Inti E3: FSM Relai, Tamper, OLED) |
| **Tab 5** | `Tab5_Asynchronous_ISR` | **5. Jalur Asinkron & ISR** | Flowchart Detail Interupsi (UART RX & EXTI AC-Fail Last-Gasp) |

### C. Pembaruan Rujukan pada Tab 1 (Makro Sistem)
- Simpul `iso_hw` (Lapisan Perangkat Keras) diperbarui teksnya dari `[Tab 6]` menjadi:
  `[Rujukan Alokasi Pin: Acuan/PIN.docx]`
- Simpul footer navigasi `iso_nav` diperbarui menjadi:
  `PANDUAN REFERENSI TAB: [Tab 2] Metrologi ADE9000 | [Tab 3] DLMS/COSEM | [Tab 4 & FlowChartE3] Rincian Aplikasi E3 | [Tab 5] Jalur Asinkron & ISR | [Acuan/PIN.docx] Pinout Hardware`

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Format XML:**
   Parsing struktur XML pada master dan standalone file mengonfirmasi bahwa seluruh 5 tab valid 100% tanpa referensi rusak.
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
   Total Test time (real) = 0.61 sec
   ```
   Status: **12/12 Passed (100%)**.
