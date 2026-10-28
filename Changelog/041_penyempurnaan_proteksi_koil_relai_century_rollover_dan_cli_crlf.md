# LOG-041: Penyempurnaan Proteksi Koil Relai (ERR07 Debounce & Pulse), Century Rollover RTC, dan CLI CRLF Filter

**Tanggal:** 28 Oktober 2026  
**Status:** SELESAI & TERVERIFIKASI  
**Target:** STM32U585VIT6, FreeRTOS, Secure World, Non-Secure World, Konsol CLI  

---

## 1. Latar Belakang & Analisis Mendalam Line-by-Line

Dalam audit teliti baris demi baris terhadap seluruh modul interaktif dan periferal, ditemukan **3 titik perbaikan kritis**:

### A. Eliminasi Alarm Palsu ERR07 & Proteksi Termal Koil Relai Pembatas Beban
* **Berkas:** [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c#L676-L720)
* **Masalah:**
  1. Sebelumnya, saat `is_trip_commanded` bernilai `true`, `HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_SET)` aktif terus-menerus dan pin feedback `relay_closed_fb` (`PC7`) langsung diperiksa pada siklus eksekusi pertama.
  2. Karena relai mekanik membutuhkan waktu transit sekitar 20 s.d 100 ms untuk memisahkan kontak fisiknya, pin status masih mendeteksi kontak tertutup pada siklus pertama, sehingga secara instan dan keliru memicu alarm internal **`ERR07` (`SPLN_ALARM_RELAY_FAIL`)** pada setiap pemutusan relai normal.
  3. Menahan pin koil relai (`PC8`) bernilai HIGH terus-menerus berisiko membakar koil aktuator relai latching pada perangkat keras meter fisik.
* **Perbaikan:**
  1. Menambahkan timer pulsa `s_relay_trip_ms`. Pin `RELAY_TRIP_Pin` diberi pulsa HIGH selama tepat `BOARD_CFG_RELAY_PULSE_MS` (100 ms) lalu otomatis di-RESET ke LOW untuk menghemat daya dan melindungi koil relai dari *thermal burnout*.
  2. Evaluasi kontak feedback `relay_closed_fb` diberikan jeda toleransi mekanik selama **500 ms**. Hanya jika setelah 500 ms kontak fisik masih terhubung, sistem baru menetapkan alarm `ERR07`.

### B. Proteksi Transisi Abad Otonom (Rollover 99->00) & Safeguard Reboot Abad 2100
* **Berkas:** [`Secure/Core/Src/secure_nsc.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/secure_nsc.c#L251-L265) & [`Secure/Core/Src/main.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/main.c#L356-L372)
* **Masalah:**
  1. Pada `secure_nsc.c`, pengecekan sebelumnya `if (century_base == 2000U && sDate.Year < 20U)` menyebabkan waktu tahun 2000..2019 (misalnya saat pengujian unit test) otomatis dinaikkan ke abad 2100 (menjadi 2100..2119).
  2. Pada `MX_RTC_Init()` di `main.c`, pengecekan `if (sDateCheck.Year < 26U)` akan menyebabkan meter pada tahun 2100..2125 selalu di-reset kembali ke tanggal 28 Oktober 2026 setiap kali sistem me-reboot.
* **Perbaikan:**
  1. Pada `secure_nsc.c`, deteksi transisi otonom 2099 -> 2100 kini melacak transisi fisik RTC dari tahun 99 menuju 00 (`s_prev_rtc_year == 99U && sDate.Year == 0U`).
  2. Pada `main.c`, inisialisasi default hanya dilakukan jika domain VBAT belum memiliki stempel magic `0xCE000000` atau abad masih 2000 dan tahun < 26.

### C. Filter Redundan `\n` (CRLF) pada Interactive Serial CLI
* **Berkas:** [`E3/app/console/src/console_io.c`](file:///g:/PPPP/ALLIN/E3/app/console/src/console_io.c#L1060-L1075)
* **Masalah:**
  Terminal serial standar (seperti PuTTY, TeraTerm, atau minicom) secara default mengirimkan sekuens `\r\n` (CRLF) saat tombol Enter ditekan. Byte `\r` mengeksekusi perintah dan mencetak baris prompt `SMART-METER> `. Selanjutnya, byte `\n` yang masuk dengan indeks buffer kosong mengeksekusi blok `else`, mencetak baris kosong tambahan dan menghasilkan **prompt ganda (*double prompt*)** pada layar terminal.
* **Perbaikan:**
  Menambahkan pelacak byte sebelumnya `s_prev_rx_byte`. Jika byte saat ini adalah `\n` dan byte sebelumnya adalah `\r`, karakter diabaikan sehingga tampilan prompt CLI tetap bersih dan rapi.

---

## 2. Hasil Verifikasi & Pengujian

### A. Pengujian Host CTest (12 Test Suite)
```
Test project G:/PPPP/ALLIN/E3/build
 1/12 Test  #1: Integration_E3_Demo ..............   Passed    0.10 sec
 2/12 Test  #2: Unit_E3_Display_Tamper ...........   Passed    0.07 sec
 3/12 Test  #3: Unit_E3_Power_Relay ..............   Passed    0.07 sec
 4/12 Test  #4: Unit_E3_FSM_Statechart ...........   Passed    0.07 sec
 5/12 Test  #5: Unit_E3_Load_Profile .............   Passed    0.07 sec
 6/12 Test  #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.11 sec
 7/12 Test  #7: Stress_E3_Final_System_Audit .....   Passed    0.16 sec
 8/12 Test  #8: Unit_E3_Console_CLI ..............   Passed    0.12 sec
 9/12 Test  #9: Unit_E3_Board_Profile ............   Passed    0.06 sec
10/12 Test #10: Unit_E3_Power_Mode_Stop2 .........   Passed    0.06 sec
11/12 Test #11: Unit_E3_NVRAM_Storage ............   Passed    0.06 sec
12/12 Test #12: Unit_E3_RTOS_Tasks ...............   Passed    0.08 sec

100% tests passed, 0 tests failed out of 12 (1.04 sec)
```

### B. Kompilasi Silang ARM Cortex-M33 (GCC Bare-metal)
* **Secure World (`ALLIN_S.elf`):**
  * ROM: 24,056 B / 1016 KB (2.31%)
  * RAM: 12,056 B / 256 KB (4.60%)
  * ROM_NSC: 160 B / 8 KB (1.95%)
  * Status: 0 Errors, 0 Warnings
* **Non-Secure World (`ALLIN_NS.elf`):**
  * ROM: 174,328 B / 1 MB (16.63%)
  * RAM: 65,336 B / 512 KB (12.46%)
  * Status: 0 Errors, 0 Warnings

