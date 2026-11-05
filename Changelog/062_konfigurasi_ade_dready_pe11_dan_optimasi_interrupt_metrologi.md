# LOG-062: Konfigurasi ADE_DREADY (PE11), Penataan Boundary Interupsi Metrologi & Update Dokumen Acuan

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh (Build Cortex-M33 0 Error 0 Warning, CTest 12/12 Passed)  
**Tipe:** Architecture Alignment / Hardware Pin Configuration / Power & Interrupt Optimization  
**Pilar Terdampak:** Pilar E1 (Metrologi ADE9000), Jembatan Sistem (`meter_system_bridge`), Konfigurasi Pin Hardware (`main.c`, `main.h`, `ALLIN.ioc`), Dokumen Acuan (`Acuan/PIN.docx`, `pin_doc_extracted.txt`, `pin_doc_all_text.txt`, `KONFIGURASI_MX.md`, `system_flowchart.drawio`)

---

## 1. Latar Belakang & Analisis Integrator

Pin 23 pada IC AFE Metrologi ADE9000 (kemasan LFCSP-40) merupakan pin multi-fungsi yang terhubung ke pin mikrokontroler STM32U585 **`PE11`** (`EXTI11`). Pada konfigurasi awal, pin ini dinamai `ADE_EVENT` dan dihubungkan secara keliru ke fungsi deteksi tamper fasa hilang pada Pilar E3 (`e3_meter_app_on_tamper(TAMPER_VECTOR_PHASE_LOSS)`).

Setelah peninjauan arsitektur sistem dan keselarasan dengan dokumen acuan:
1. **Pergeseran Domain:** Pin 23 ADE9000 difungsikan sebagai **`DREADY` (Data Ready)** untuk sinkronisasi akuisisi sampel waveform instan 7-kanal ADC (8 kSPS / 125 µs) pada Pilar E1 Metrologi, bukan sinyal alarm kejadian mutu daya.
2. **Eliminasi Interrupt Storm (8.000 Hz):** Jika pin ini memicu interupsi EXTI pada 8 kSPS secara terus-menerus ke Pilar E3:
   - Terjadi banjir interupsi (*interrupt storm*) yang menghabiskan siklus CPU STM32U585.
   - Buffer FIFO kejadian tamper E3 dibanjiri puluhan ribu log "Phase Loss" palsu per menit.
3. **Pemberdayaan Algoritma Mandiri E3:** Deteksi fasa hilang (ERR24 - SPLN D3.006:2021) sudah dihitung secara otonom di dalam loop periodik `e3_meter_app_step()` berdasarkan tegangan RMS fisik ($V_R, V_S, V_T < 110\text{ V}$), sehingga tidak memerlukan interupsi pin eksternal.
4. **Implementasi Opsi A (Polling Periodik Hemat Daya):**
   - Interupsi pin PE11 dialihkan secara eksklusif ke hook metrologi `e1_metrology_on_data_ready()`.
   - Gerbang interupsi NVIC `EXTI11_IRQn` dinonaktifkan secara default saat meter beroperasi dalam mode pembacaan register RMS periodik (100 ms).
   - Jalur hardware tetap `EXTI11` sehingga siap diaktifkan sewaktu-waktu jika dibutuhkan streaming gelombang instan.

---

## 2. Rincian Perubahan Implementasi

### A. Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`)
- Melepaskan pemanggilan `e3_meter_app_on_tamper(TAMPER_VECTOR_PHASE_LOSS)` dari callback `ADE_DREADY_Pin`.
- Mengarahkan penanganan falling-edge interupsi ke fungsi pilar E1:
  ```c
  else if (GPIO_Pin == ADE_DREADY_Pin)
  {
      /* ADE9000 DREADY (Data Ready ADC 8 kSPS) - Sinkronisasi Metrologi Pilar E1 */
      e1_metrology_on_data_ready();
  }
  ```

### B. Pilar E1 Metrologi (`E1/Metrology/e1_metrology_service.h` & `e1_metrology_service.c`)
- Menambahkan fungsi handler sinyal Data Ready ADC:
  ```c
  void e1_metrology_on_data_ready(void);
  uint32_t e1_metrology_get_dready_pulses(void);
  ```
- Mengimplementasikan pencacah pulsa aman `s_dready_pulse_count` untuk pelacakan performa ADC.

### C. Konfigurasi GPIO & NVIC (`NonSecure/Core/Src/main.c` & `NonSecure/Core/Inc/main.h`)
- Mendefinisikan label `ADE_DREADY_Pin` (`GPIO_PIN_11`, Port `GPIOE`, IRQ `EXTI11_IRQn`) dengan alias backward-compatible `ADE_EVENT_Pin`.
- Memisahkan konfigurasi GPIO `ADE_DREADY_Pin` dari pulsa kalibrasi energi (`ADE_CF1_Pin` & `ADE_CF2_Pin`).
- Menahan aktivasi interupsi NVIC (`/* HAL_NVIC_EnableIRQ(EXTI11_IRQn); */`) untuk mencegah *CPU overload* selama mode operasional polling standar.

### D. File Proyek CubeMX (`ALLIN.ioc`)
- Memperbarui label pin: `PE11.GPIO_Label=ADE_DREADY`.

### E. Dokumen Acuan & Spesifikasi Perangkat Keras
1. **`Acuan/PIN.docx`:**
   - Memperbarui Baris 40 (Tabel Pin MCU): Mengubah label dari `ADE_EVENT` menjadi `ADE_DREADY`.
   - Memperbarui deskripsi fungsi: Sinyal DREADY periodik 8 kSPS ADE9000 untuk sinkronisasi akuisisi ADC ke E1, dengan proteksi interrupt storm pada Opsi A.
2. **`Acuan/pin_doc_extracted.txt` & `Acuan/pin_doc_all_text.txt`:**
   - Menyinkronkan entri `PE11` dengan nama `ADE_DREADY` dan penjelasan arsitektur Opsi A.
3. **`Config_MX/KONFIGURASI_MX.md`:**
   - Memperbarui node diagram Mermaid `PE11 (DREADY)` dan baris 40 tabel pinout.
4. **`docs/system_flowchart.drawio`:**
   - Tab 6 (Asynchronous ISR) dan Tab 7 (Hardware Layer) telah diselaraskan dengan label `DREADY`.

---

## 3. Hasil Verifikasi & Pengujian Sistem

1. **Kompilasi Firmware Cortex-M33 (GCC ARM / Ninja):**
   - Target: `ALLIN_NS.elf`
   - Hasil: **0 Error, 0 Warning**
   - Penggunaan Memori: ROM: 193.712 Byte (18.47%), RAM: 73.776 Byte (14.07%).
2. **CTest Unit Testing Pilar E3:**
   - 12/12 Unit Tests Passed (100% lulus dalam 0.49 detik).
   - Pengujian mencakup: Integration Demo, Display Tamper, Power Relay, FSM Statechart, Load Profile, SPLN Display Tamper, Stress Final System Audit, Console CLI, Board Profile, Power Mode Stop2, NVRAM Storage, dan RTOS Tasks.
3. **Integritas Flowchart:**
   - `Acuan/FlowChartE3.drawio` & `docs/e3_modular_flowcharts.drawio` 100% konsisten tanpa memerlukan perubahan karena deteksi fasa hilang berjalan secara mandiri di software E3.
