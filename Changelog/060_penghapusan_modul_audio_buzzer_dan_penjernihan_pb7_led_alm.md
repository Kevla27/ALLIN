# LOG-060: Penghapusan Modul Audio Buzzer dan Penjernihan Peran Pin PB7 sebagai LED_ALM (Kuning)

**Tanggal:** 04 November 2026  
**Status:** Selesai & Terverifikasi Penuh (Build Cortex-M33 0 Error 0 Warning, CTest 12/12 Passed)  
**Tipe:** Refactoring / Pembersihan Arsitektur / Penyesuaian Standar Hardware & SPLN  
**Pilar Terdampak:** Pilar E3 (Aplikasi Smart Meter / HMI / Driver Hardware)

---

## 1. Latar Belakang & Alasan Perubahan

Berdasarkan audit komparatif antara kode program dengan dokumen acuan resmi:
1. **Dokumen Alokasi Pin MCU ([`PIN.docx`](file:///g:/PPPP/ALLIN/Acuan/pin_doc_extracted.txt)):**
   - Tidak ada pin dedicated yang dialokasikan untuk aktuator suara (piezo buzzer).
   - Pin `PB7` yang sebelumnya ditumpangi oleh driver buzzer secara resmi berlabel:
     > `PB7 | LED_ALM | Output Push-Pull, Low | Driver Indikator LED Kuning (Super Bright) untuk peringatan penyalahgunaan (tamper) atau ketidaknormalan sistem (alarm).`
2. **Standar SPLN D3.006: 2021 Butir 6.9 ("Indikator LED"):**
   - Meter fase 3 AMR/AMI PLN hanya mensyaratkan 4 buah indikator LED visual:
     1. Merah 1: Keluaran pulsa energi aktif (`Imp/kWh`) $\rightarrow$ Pin `PE12` (`ADE_CF1`)
     2. Merah 2: Keluaran pulsa energi reaktif (`Imp/kVARh`) $\rightarrow$ Pin `PE13` (`ADE_CF2`)
     3. Kuning: Indikator penyalahgunaan (*tamper*) / alarm sistem $\rightarrow$ Pin `PB7` (`LED_ALM`)
     4. Hijau: Ketersediaan tegangan catu daya utama $\rightarrow$ Rail daya DC
   - Standar SPLN meter 3-fasa pascabayar **tidak mewajibkan alarm audio / buzzer**.
3. **Penyelesaian Konflik Kendali GPIO PB7**:
   - Sebelumnya, fungsi `bsp_alarm_buzzer_update_cadence()` menimpa kendali pin `PB7` dengan denyut 200 ms ON / 200 ms OFF atau pulsa beep 50 ms saat tombol ditekan.
   - Penghapusan buzzer menjernihkan peruntukan pin `PB7` agar 100% dikendalikan oleh modul alarm/tamper LED Kuning:
     - **Mati (OFF)** saat kondisi normal.
     - **Menyala terus (SOLID ON)** saat terjadi tamper atau kegagalan internal (*error register*).
     - **Berkedip 1 Hz** saat status relai *READY ON* (menunggu konfirmasi rekoneksi tombol ▲+▼ sesuai Desain Sistem Hal. 10 & 36).

---

## 2. Rincian Perubahan Kode

### A. Penghapusan Driver Hardware Buzzer
Folder driver `E3/drivers/buzzer/` dihapus secara total dari repositori:
- `E3/drivers/buzzer/inc/bsp_alarm_buzzer.h` (Dihapus)
- `E3/drivers/buzzer/inc/bsp_pin_mapping.h` (Dihapus)
- `E3/drivers/buzzer/src/bsp_alarm_buzzer.c` (Dihapus)

### B. Penonaktifan pada Konfigurasi Profil Papan ([`board_profile_config.h`](file:///g:/PPPP/ALLIN/E3/app/board_profile_config.h))
Konstanta waktu kadensi buzzer pada Profil 1, 2, dan 3 digantikan dengan makro eksplisit:
```c
/* --- E. Audio Buzzer (Ditiadakan) --- */
#define BOARD_CFG_ENABLE_BUZZER         0          /**< 0 = Ditiadakan: PB7 murni untuk LED_ALM (Kuning) per PIN.docx & SPLN D3.006 */
```

### C. Pembersihan Aplikasi Meter ([`e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c))
1. Menghapus `#include "bsp_alarm_buzzer.h"` dan variabel `s_button_beep_ms`.
2. Menghapus `bsp_alarm_buzzer_init()` pada inisialisasi aplikasi.
3. Menghapus perhitungan timer non-blocking dan pemanggilan `bsp_alarm_buzzer_update_cadence()` di dalam `e3_meter_app_step()`.
4. Menghapus pemicu beep tombol di `e3_meter_app_on_button_pressed()`.
5. Menghapus pemadaman buzzer di `e3_meter_app_on_ac_fail()`.

### D. Pembersihan Task RTOS & Statechart
- [`rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/rtos_tasks.c): Menghapus include buzzer, pemanggilan `bsp_alarm_buzzer_init()`, dan pemanggilan toggle/set buzzer.
- [`fsm_statechart.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/fsm_statechart.h): Memperbarui dokumentasi State 3 menjadi "LED Kuning Alarm PB7".

### E. Penyesuaian Build System
- [`NonSecure/CMakeLists.txt`](file:///g:/PPPP/ALLIN/NonSecure/CMakeLists.txt): Menghapus direktori `E3/drivers/buzzer/inc` dan file `bsp_alarm_buzzer.c`.
- [`E3/CMakeLists.txt`](file:///g:/PPPP/ALLIN/E3/CMakeLists.txt): Menghapus target kompilasi driver buzzer dari pustaka `e3_core`.

### F. Penyesuaian Test Suite CTest E3 (`E3/tests/`)
- [`test_board_profile.c`](file:///g:/PPPP/ALLIN/E3/tests/test_board_profile.c): Memverifikasi `assert(BOARD_CFG_ENABLE_BUZZER == 0)`.
- [`test_final_system_stress.c`](file:///g:/PPPP/ALLIN/E3/tests/test_final_system_stress.c): Menghapus inisialisasi dan uji kadensi buzzer tahap 6.
- [`test_rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/tests/test_rtos_tasks.c): Menghapus pengujian aktuasi buzzer.
- [`test_spln_display_tamper.c`](file:///g:/PPPP/ALLIN/E3/tests/test_spln_display_tamper.c): Menghapus blok pengujian UJI 8 (Audio Cadence Buzzer) dan merapikan penomoran uji.

---

## 3. Hasil Verifikasi & Build

1. **Kompilasi Firmware Target STM32U585 (ARM Cortex-M33 Non-Secure)**:
   - Perintah: `cmake --build build/Debug -j8`
   - Hasil: **SUKSES 100% (0 Error, 0 Warning)**
   - Memory Footprint:
     - **RAM**: `69,144 B` / `512 KB` (`13.19%`) $\rightarrow$ Berkurang 16 Byte.
     - **ROM (Flash)**: `186,552 B` / `1 MB` (`17.79%`) $\rightarrow$ Berkurang 440 Byte (*dead code removal*).
2. **Suite Pengujian Otomatis CTest E3**:
   - Perintah: `ctest --test-dir E3/build --output-on-failure`
   - Hasil: **12/12 Test Passed (100% lulus dalam 0.35 detik)**

---

## 4. Kesimpulan

Modul buzzer telah dihilangkan secara tuntas dari codebase. Pin `PB7` kini sepenuhnya berfungsi sebagai pengendali **LED Kuning Alarm** (`LED_ALM`) yang mematuhi standar SPLN D3.006: 2021 Butir 6.9 huruf c tanpa ada gangguan perebutan sinyal GPIO dari modul lain.
