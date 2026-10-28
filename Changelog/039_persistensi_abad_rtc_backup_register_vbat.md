# LOG-039: Persistensi Abad RTC (s_sec_century_base) di Backup Register VBAT (`secure_nsc.c`)

**Tanggal:** 28 Oktober 2026  
**Status:** Selesai & Terverifikasi Kompilasi (ARM GCC Secure + NonSecure, 11/11 CTest Passed 100%)

---

## 1. Latar Belakang & Masalah Teknis

1. **Kehilangan Basis Abad Pasca-Reset (*Century Reset Problem*):**
   - Peripheral RTC perangkat keras internal STM32U5 hanya menyimpan 2 digit tahun (format BCD `00` s.d. `99`).
   - Untuk mendukung imunitas kalender multi-abad Y2K38 hingga 7 Februari 2106, Secure World mengandalkan variabel `s_sec_century_base` (`2000U` atau `2100U`).
   - Sebelumnya, variabel `s_sec_century_base` hanya disimpan sebagai variabel statis biasa di Secure SRAM (`static volatile uint16_t s_sec_century_base = 2000U;`).
   - Apabila meter disinkronkan ke abad 22 (misalnya tahun 2102) lalu mengalami siklus daya mati/reboot (*power-cycle*, pemadaman total listrik, atau reset software), variabel SRAM tersebut kembali ter-reset ke `2000U`.
   - Akibatnya, saat fungsi `SECURE_GetRTCDateTime()` membaca register tahun RTC (`sDate.Year = 2`), sistem menjumlahkannya dengan `2000U`, menghasilkan tahun `2002` bukannya `2102`.
2. **Ketersediaan Domain Cadangan VBAT (`VSW`):**
   - Mikrokontroler STM32U585 memiliki 32 register cadangan (*Backup Registers* `RTC_BKP_DR0` s.d. `RTC_BKP_DR31`) yang berada di domain daya `VSW` dan disuplai langsung oleh baterai Li-SOCl2 3.6V dan superkapasitor.
   - `RTC_BKP_DR0` telah digunakan untuk menyimpan akumulasi energi aktif Wh (`s_sec_active_wh`).
   - `RTC_BKP_DR1` telah digunakan untuk menyimpan akumulasi energi reaktif VARh (`s_sec_reactive_varh`).
   - Register `RTC_BKP_DR2` masih bebas dan ideal untuk mengamankan persistensi basis abad.

---

## 2. Solusi Rekayasa & Implementasi

1. **Alokasi Register Cadangan `RTC_BKP_DR2` dengan Stempel Magic:**
   - Dialokasikan `SECURE_RTC_BKP_CENTURY_REG = RTC_BKP_DR2`.
   - Ditambahkan stempel integritas bita tinggi `SECURE_RTC_CENTURY_MAGIC = 0xCE000000U` (`"CE"` = Century Signature). Nilai yang disimpan berformat:
     $$\text{RegVal} = 0x\text{CE000000} \mid (\text{uint32\_t})s\_sec\_century\_base$$
2. **Fungsi Helper Akses Domain VBAT (`secure_rtc_get_century_base` & `secure_rtc_save_century_base`):**
   - Membaca `RTC_BKP_DR2` melalui `HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2)`.
   - Memvalidasi stempel magic `0xCE000000`. Jika valid dan bernilai `2000` atau `2100`, basis abad dipulihkan.
   - Jika register belum diinisialisasi (misal baterai baru pertama kali dipasang), sistem otomatis mengisinya dengan default `2000U`.
3. **Pembaruan Sinkronisasi Waktu (`SECURE_SetRTCDateTime`):**
   - Saat sinkronisasi waktu kalender dilakukan via DLMS Clock atau konsol CLI, basis abad yang baru (`y >= 2100 ? 2100 : 2000`) langsung ditulis ke `RTC_BKP_DR2` menggunakan `HAL_RTCEx_BKUPWrite`.
4. **Deteksi Transisi Abad Otonom (*Autonomous Century Rollover Detection*):**
   - Di dalam `SECURE_GetRTCDateTime()`, jika RTC hardware beroperasi melewati 31 Desember 2099 23:59:59 menuju 01 Januari 2100 00:00:00 (`sDate.Year` bergulir dari 99 ke 00):
   - Karena meter diproduksi tahun 2026 ke atas, nilai `sDate.Year < 20U` saat `century_base == 2000U` secara matematis menandakan pergantian abad ke-22.
   - Sistem secara otomatis mendeteksi pergantian ini, menaikkan `century_base = 2100U`, dan memperbarui `RTC_BKP_DR2` tanpa perlu menunggu perintah sinkronisasi eksternal.

---

## 3. Hasil Verifikasi & Kompilasi

- **Target Secure ARM Cortex-M33 (`ALLIN_S.elf`):**
  - `RAM`: `12,056 B / 256 KB (4.60%)`
  - `ROM`: `23,880 B / 1016 KB (2.30%)`
  - `ROM_NSC`: `160 B / 8 KB (1.95%)`
- **Target Non-Secure ARM Cortex-M33 (`ALLIN_NS.elf`):**
  - `RAM`: `65,160 B / 512 KB (12.43%)`
  - `ROM`: `171,856 B / 1 MB (16.39%)`
- **Host CTest Unit Tests:**
  - `11/11 Test Suites Passed 100%` (0 fail).
