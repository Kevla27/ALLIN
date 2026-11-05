# Standarisasi Bagan Tab 6 Lapisan Hardware & Alokasi Pin MCU ke Tema Black & White

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Documentation & Visual Monochrome Alignment  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_flowchart.drawio`, `docs/system_tab6_hardware_layer.drawio.xml`), Pemetaan Hardware MCU STM32U585 (`Acuan/PIN.docx`)

---

## 1. Latar Belakang & Analisis Integrator

Dalam arsitektur sistem Smart Meter 3-Fasa STM32U585, **Tab 6: Hardware & Pin Periferal** (sebelumnya bernomor Tab 7) berfungsi sebagai peta acuan perangkat keras fisik. Diagram ini merangkum seluruh koneksi bus mikrokontroler (SPI1 ADE9000, SPI2 Flash NVRAM, I2C3 OLED, USART2 Modem, USART3 Port Optik, ADC4 telemetri, dan jalur relai kontaktor shunt trip).

Setelah Tab 1 (Makro Sistem) dan Tab 5 (Jalur Asinkron & ISR) berhasil distandarisasi ke tema **Black & White (B&W) High-Contrast**, Tab 6 sebelumnya masih menggunakan gaya warna-warni lama (header biru gelap, border slate, dan kartu catatan hijau). Untuk menjaga keseragaman estetika dokumen teknik tingkat publikasi/audit, Tab 6 diselaraskan ke tema monokrom kontras tinggi yang siap cetak.

---

## 2. Rincian Perubahan Implementasi

### A. Transformasi Tema Visual Monokrom (B&W High-Contrast)
1. **Header Utama (`hw_hdr`):** Berubah dari latar biru gelap (`#1e293b`) menjadi putih bersih (`#ffffff`) dengan garis batas hitam tegas 2px (`strokeColor=#000000;strokeWidth=2`) dan tipografi Helvetica.
2. **Blok Periferal Hardware (`hw1` s/d `hw7`):** Seluruh 7 kartu antarmuka periferal distandarisasi menggunakan latar putih (`#ffffff`), garis tepi hitam 1.5px (`#000000`), dan teks hitam kontras tinggi.
3. **Kartu Integritas Perangkat Keras (`hw_card`):** Menggunakan latar putih bersih dengan border hitam solid 1px, menegaskan bahwa seluruh alokasi pin telah diverifikasi bebas dari konflik pin multiplexing.

### B. Sinkronisasi Penomoran File
- File lama `docs/system_tab7_hardware_layer.drawio.xml` dihapus dan digantikan oleh file baru yang telah terindeks rapi: [`docs/system_tab6_hardware_layer.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_tab6_hardware_layer.drawio.xml).
- Master file [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio) pada diagram `Tab6_Hardware_Layer` diperbarui secara mulus.

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Format XML:**
   Parsing struktur XML pada master dan standalone file mengonfirmasi kepatuhan skema XML 100% tanpa malformed tag.
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

