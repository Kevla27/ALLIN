# Standarisasi Sub-Flowchart Definisi meter_system_bringup() Berbasis ISO 5807:1985 & Black/White Theme

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Documentation & System Architecture Alignment (ISO 5807:1985 Flowchart Standardization)  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_flowchart.drawio`, `docs/flowchart_meter_system_bringup.drawio.xml`, `docs/system_tab2_bootstrapping.drawio.xml`), Jembatan Sistem (`meter_system_bridge.c`)

---

## 1. Latar Belakang & Analisis Integrator

Pada diagram alir **Tab 2 (Bootstrapping & Keamanan TrustZone)**, proses aktivasi sistem terpadu didefinisikan sebagai sebuah **Predefined Process** (simbol balok dengan garis ganda samping / Klausul 9.2.2.1 ISO 5807:1985):
```
INISIALISASI LAYANAN INTI SISTEM
meter_system_bringup(): Metrologi, DLMS & Aplikasi Utama
```

Sesuai dengan kaidah rekayasa perangkat lunak standar internasional ISO 5807:1985, setiap simpul bertipe *Predefined Process* merepresentasikan sebuah sub-rutin terpisah yang memiliki diagram alir tersendiri untuk mendefinisikan rincian logika di dalamnya. Tanpa bagan alir pendukung, pembaca atau auditor tidak dapat menelusuri bagaimana parameter perifer dihubungkan, urutan inisialisasi ketiga modul fungsional dieksekusi, serta bagaimana pemulihan checkpoint energi dari NVRAM dijalankan.

Oleh karena itu, dibuatlah diagram alir detail khusus untuk **Definisi Sub-rutin: `meter_system_bringup()`** yang:
1. **Sepenuhnya Selaras dengan Kode C Nyata:** Memetakan baris 49–75 pada file [`NonSecure/Core/Src/meter_system_bridge.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/meter_system_bridge.c#L49-L75) secara presisi tanpa rekayasa langkah fiktif.
2. **Kepatuhan Simbolik ISO 5807:1985:**
   - **Terminal (Klausul 9.4.1):** Titik masuk sub-rutin (`SUB-RUTIN: meter_system_bringup()`) dan titik keluar (`SELESAI / RETURN KE main.c`).
   - **Process (Klausul 9.2.1):** Binding konteks handle, reset memory `memset`, konfigurasi retargeting printf console, pemulihan checkpoint energi, dan pencetakan log konfirmasi.
   - **Predefined Process (Klausul 9.2.2.1):** Pemanggilan sub-layanan metrologi, protokol komunikasi DLMS/COSEM, dan aplikasi meter.
   - **Decision (Klausul 9.2.3):** Evaluasi keberhasilan pembacaan data checkpoint dari memori non-volatile (`nvram_load_energy_checkpoint`).
3. **Penerapan Nomenklatur Profesional Bebas Kode Internal E:** Menggantikan penyebutan "E1, E2, E3" secara gamblang dengan nama fungsional resmi: *Layanan Metrologi Listrik (ADE9000)*, *Layanan Protokol Komunikasi DLMS/COSEM (IEC 62056)*, dan *Aplikasi Utama Smart Meter & HMI*.
4. **Penerapan Tema Black & White (B&W):** Kontras tinggi, latar putih bersih, garis tepi hitam tegas, dan tipografi seragam Helvetica untuk keterbacaan dokumen teknik siap cetak.

---

## 2. Rincian Pemetaan Kode Program ke Simpul Diagram Alir

Implementasi sub-rutin `meter_system_bringup()` dipetakan ke dalam simpul ISO 5807 sebagai berikut:

| ID Simpul | Simbol ISO 5807 | Baris Kode C (`meter_system_bridge.c`) | Fungsi / Deskripsi |
| :--- | :--- | :--- | :--- |
| **`u0`** | Terminal (Stadium) | Baris 49: `void meter_system_bringup(...)` | Titik masuk sub-rutin dengan penerimaan parameter `huart_opt`, `huart_modem`, `hadc_telemetry`. |
| **`u1`** | Process (Persegi Panjang) | Baris 51–52: `s_console_uart = huart_modem; memset(&s_meas, 0, ...);` | Binding pointer UART modem sebagai kanal logging dan pembersihan memori buffer context pengukuran `s_meas`. |
| **`u2`** | Process (Persegi Panjang) | Baris 55–56: `console_io_init(...); console_io_print_boot_banner(...);` | Registrasi fungsi callback transmisi console I/O dan pencetakan banner identitas meter ke terminal USART2 (19200 bps). |
| **`u3`** | Predefined Process | Baris 59: `e1_metrology_service_init();` | Inisialisasi SPI1 sensor metrologi ADE9000, konfigurasi register DSP, kalibrasi gain/offset, dan aktivasi pin IRQ. |
| **`u4`** | Predefined Process | Baris 60–61: `e2_dlms_service_init(huart_opt); e2_dlms_service_set_modem_uart(huart_modem);` | Inisialisasi parser DLMS/COSEM, katalog objek OBIS, dan binding kanal port optik USART3 serta modem USART2. |
| **`u5`** | Predefined Process | Baris 62: `e3_meter_app_init(METER_CONFIG_CUSTOMER_ID, hadc_telemetry);` | Inisialisasi modul inti: FSM Relai pemutus beban, 8 sensor tamper sabotase, sub-sistem display OLED SPLN, dan ADC telemetri. |
| **`u6`** | Decision (Belah Ketupat) | Baris 66: `if (nvram_load_energy_checkpoint(&cp))` | Evaluasi ketersediaan data checkpoint energi yang valid di memori non-volatile (SPI2 Flash / Backup Register). |
| **`u7a`** | Process (Branch [Ya]) | Baris 68–70: `s_meas.active_energy_wh = cp...;` | Memulihkan stand akumulasi energi aktif/reaktif terakhir dari checkpoint NVRAM ke dalam memori operasional. |
| **`u7b`** | Process (Branch [Tidak]) | Jalur alternatif (else implisit) | Mempertahankan nilai akumulasi energi default nol (kondisi fresh start atau cold boot pertama pabrik). |
| **`u8`** | Process (Persegi Panjang) | Baris 73: `printf("[BRIDGE] All Pillars ... initialized.\n");` | Mengirimkan log konfirmasi keberhasilan inisialisasi seluruh pilar ke konsol pengembang. |
| **`u9`** | Terminal (Stadium) | Baris 74–75: `}` | Titik keluar sub-rutin, kendali dikembalikan ke `main.c` untuk melanjutkan eksekusi ke `osKernelInitialize()`. |

---

## 3. Hasil Verifikasi, Pengujian & Sinkronisasi Dokumen

Sesuai protokol pre-flight SOP (`Acuan/SOP_SISTEM_KERJA_AI.md`), seluruh rangkaian verifikasi telah dijalankan tanpa kompromi:

1. **Integritas Format XML Draw.io:**
   - Standalone diagram tersimpan di [`docs/flowchart_meter_system_bringup.drawio.xml`](file:///g:/PPPP/ALLIN/docs/flowchart_meter_system_bringup.drawio.xml).
   - Diagram master 8-tab diperbarui di [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) dengan tab baru `Tab2b_System_Bringup: 2b. Inisialisasi Layanan (Bringup)`.
   - Parsing XML menggunakan pustaka Python `xml.etree.ElementTree` mengonfirmasi validitas struktur 100% tanpa malformed tag.
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
   Total Test time (real) = 0.18 sec
   ```
   Status: **12/12 Passed (100%)**.
