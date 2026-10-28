# LOG-037: Sinkronisasi Kalender & Kekebalan Y2K38 (Hingga 2106) pada RTC/DLMS/A-XDR, Antrean RX CLI Non-HDLC, dan Akurasi Ikon Kontaktor OLED

**Tanggal:** 28-10-2026  
**Status:** Selesai & Terverifikasi 100%

---

## 1. Latar Belakang / Alasan Perubahan
Dalam audit lanjutan terhadap penanganan waktu kalender UNIX Epoch (`uint32_t` hingga **7 Februari 2106**), protokol DLMS/COSEM, dan integrasi perangkat keras terhadap dokumen `Acuan/`, dilakukan penyempurnaan pada 6 titik berikut:
1. **Konversi Kalender Gregorian Dinamis pada `dlms_encode_datetime()` (`load_profile_dlms_adapter.c`)**: Menggantikan nilai tahun/bulan/tanggal statis (`2024-09-29`) pada encoder 12-byte DLMS `date-time` untuk Load Profile, Tamper Log, dan SPLN Log dengan algoritma kalender Gregorian penuh berbasis `uint32_t` (termasuk `day_of_week` 1..7 dan deviasi zona waktu WIB `+420 menit` / `0x01A4`), bebas masalah Y2K38 hingga tahun 2106.
2. **Dukungan Lintas Abad (`2000`–`2106`) pada Hardware RTC (`secure_nsc.c`)**: Menambahkan pelacakan `s_sec_century_base` (`2000` vs `2100`) pada `SECURE_SetRTCDateTime()` dan `SECURE_GetRTCDateTime()` agar register 2-digit BCD tahun pada STM32U5 (`RTC_DR.YT/YU`) tetap akurat melampaui tahun 2038 dan 2099 hingga batas `uint32_t` (2106).
3. **Pemicu Otomatis End-of-Billing (`EoB`) Tepat Tanggal 1 Kalender (`e3_meter_app.c`)**: Mengganti pendekatan modulo 30 hari (`days_since_epoch % 30`) dengan perhitungan tanggal kalender Gregorian (`day_of_month == 1 && sec_of_day == 36000`) sehingga penutupan tagihan bulanan (`e3_meter_app_trigger_billing_snapshot()`) selalu jatuh tepat setiap tanggal 1 pukul 10:00:00 sesuai **SPLN D3.006: 2021 Butir 12.4**.
4. **Sinkronisasi Objek Clock DLMS Class 8 (`0-0:1.0.0.255`) dengan Hardware RTC (`e2_dlms_service.c`)**: Menyinkronkan `clock_date_time` pada snapshot COSEM langsung dari `e3_meter_app_get_epoch()`.
5. **Coexistence Protokol Biner HDLC & Konsol CLI Interaktif pada `USART2`**: Menambahkan ring-buffer lock-free 64-byte (`e2_dlms_service_pop_cli_rx_byte()`) untuk meneruskan karakter ASCII non-HDLC ke `console_cli_process_rx_byte()` di `meter_system_bridge.c`.
6. **Akurasi Ikon Kontaktor Relai pada Layar OLED (`[o---o]` vs `[o/  o]`)**: Memperbarui evaluasi `frame->is_relay_closed` di `display.c` dan `e3_meter_app.c`.

---

## 2. Daftar File yang Berubah
- `E3/app/profiling/src/load_profile_dlms_adapter.c` (baris 10–42)
- `Secure/Core/Src/secure_nsc.c` (baris 41–47, 180–235)
- `E3/app/e3_meter_app.c` (baris 440–456, 680–688)
- `E2/Inc/e2_dlms_service.h` (baris 55–65)
- `E2/Src/e2_dlms_service.c` (baris 6–34, 117–133, 165–177, 308–322)
- `NonSecure/Core/Src/meter_system_bridge.c` (baris 103–110)
- `E3/app/display/src/display.c` (baris 1144–1149)
- `E3/tests/test_load_profile.c` (baris 224–240)

---

## 3. Dampak terhadap Kompilasi & Pengujian
- **Target Firmware (`ALLIN_S.elf` + `ALLIN_NS.elf`)**: Sukses dikompilasi (`ALLIN_S`: `ROM 23.656 B`, `ALLIN_NS`: `RAM 65.160 B / 12.43%`, `ROM 171.264 B / 16.33%`), 0 error, 0 warning.
- **Host Test Suite (`CTest` pada `E3/build`)**: 10/10 test suite (termasuk uji kekebalan Y2K38 tahun 2050 & 2100) lulus 100% tanpa warning.
