# LOG-032: Integrasi Hardware RTC STM32U5 ke Jam/Tanggal Layar OLED & Otomasi Tarif ToU (WBP/LWBP)

**Tanggal**: 28 Oktober 2026  
**Status**: Selesai & Terverifikasi (Build ARM GCC 0 Error 0 Warning, CTest 9/9 Pass 100%)  
**File Terdampak**:
- `Secure/Core/Src/secure_nsc.c` *(Konversi kalender Howard Hinnant & RTC DateTime)*
- `Secure/Core/Src/main.c` *(Inisialisasi baseline kalender RTC jika uninitialized)*
- `E3/app/e3_meter_app.h` *(Deklarasi e3_meter_app_set_epoch)*
- `E3/app/e3_meter_app.c` *(Sinkronisasi periodik 1 detik dari Secure RTC & TOU evaluation)*
- `E3/app/console/inc/console_io.h` *(Hook get_time, set_time, get_active_tariff)*
- `E3/app/console/src/console_io.c` *(CLI Command `time` & `set_time`)*
- `E3/tests/test_console_cli.c` *(Unit test verifikasi RTC CLI)*
- `Changelog/README.md` *(Pembaruan indeks riwayat)*

---

## 1. Latar Belakang & Masalah

Pada firmware sebelumnya:
1. Layar OLED (SSD1306 via I2C3) telah memiliki halaman tampilan jam (`DISP_PAGE_TIME`, OBIS `01.00`, zz=22) dan tanggal (`DISP_PAGE_DATE`, OBIS `00.09.02`, zz=23). Namun nilai waktu hanya diakumulasikan secara virtual melalui soft-tick `s_ms_accumulator += delta_ms;`.
2. Akibatnya, jam mengalami drift jika terjadi beban task berat / interrupt, dan tidak tersinkronisasi dengan osilator TCXO 32.768 kHz hardware (`TCXO_32K_IN` pada PC14).
3. Di sisi Secure TrustZone, fungsi `SECURE_GetRTCDateTime()` sebelumnya hanya menambahkan offset detik harian ke konstanta tanggal statis (`s_sec_epoch_override`). Pergantian hari (midnight rollover), pergantian bulan, dan tahun kabisat belum dihitung secara dinamis dari register kalender hardware STM32U5 `RTC->DR` dan `RTC->TR`.
4. Belum tersedia perintah antarmuka serial (CLI) bagi teknisi di laboratorium untuk membaca status jam RTC nyata maupun menyinkronkan waktu meter dari PC.

---

## 2. Rincian Implementasi

### A. Secure World: Konversi Presisi Kalender & Unix Epoch (`secure_nsc.c`)
- Mengimplementasikan algoritma kalender sipil presisi Howard Hinnant:
  - `rtc_days_from_civil(year, month, day)`: Mengubah tanggal kalender hardware RTC menjadi hari sejak Unix Epoch (1 Januari 1970).
  - `rtc_civil_from_days(z, &y, &m, &d)`: Mengubah hari sejak epoch menjadi tanggal kalender (Tahun, Bulan, Hari, dan Hari dalam Minggu).
- **`SECURE_GetRTCDateTime(void)`**: Membaca register hardware `HAL_RTC_GetTime()` dan `HAL_RTC_GetDate()`. Jika tahun valid ($\ge 2026$), menghitung total detik Unix Epoch nyata dari hardware RTC.
- **`SECURE_SetRTCDateTime(epoch_sec)`**: Mengonversi detik epoch ke komponen Waktu (`sTime`) dan Tanggal (`sDate`) lalu menuliskan keduanya langsung ke register hardware STM32 RTC.
- **Inisialisasi Otomatis (`Secure/main.c`)**: Pada saat pertama kali menyala (fresh boot), jika tahun RTC masih $< 26$, sistem secara otomatis mengeset waktu baseline ke `28 Oktober 2026 10:00:00 WIB` (`1793181600` Unix Epoch).

### B. Pilar E3: Sinkronisasi Periodik & Distribusi Tarif ToU (`e3_meter_app.c`)
- Di dalam siklus task metering periodik (`e3_meter_app_step`), setiap akumulator 1 detik terpenuhi:
  ```c
  #if defined(EMBEDDED_HARDWARE_TARGET)
      uint32_t rtc_now = SECURE_GetRTCDateTime();
      if (rtc_now > 1700000000U) {
          s_now_epoch_sec = rtc_now;
      } else {
          s_now_epoch_sec += elapsed_sec;
      }
  #endif
  ```
- Waktu `s_now_epoch_sec` diteruskan ke `display_set_time(&s_disp_ctx, s_now_epoch_sec)` sehingga layar OLED menampilkan waktu yang berdetak nyata setiap detik.
- Logika Time-of-Use (`evaluate_tou_tariff_zone`) secara otomatis mengevaluasi jam:
  - **Pukul 17:00 s.d 22:00**: Akumulasi energi masuk ke **Tarif 1 (WBP)**.
  - **Pukul 22:00 s.d 06:00**: Akumulasi energi masuk ke **Tarif 2 (LWBP Malam)**.
  - **Pukul 06:00 s.d 17:00**: Akumulasi energi masuk ke **Tarif 3 (LWBP Siang)**.

### C. Serial Console CLI: Perintah `time` & `set_time` (`console_io.c`)
Menambahkan dua perintah interaktif baru pada serial console (USART2 @ 19200 bps):
- **`time`**: Menampilkan tanggal kalender (DD-MM-YYYY), jam (HH:MM:SS WIB), nilai detik Unix Epoch, dan zona tarif yang sedang aktif.
- **`set_time <epoch>`**: Mengatur waktu kalender hardware RTC dan langsung memperbarui tampilan OLED seketika.

---

## 3. Hasil Pengujian & Verifikasi

### 1. Kompilasi Target ARM Cortex-M33 TrustZone
- **Secure Target (`ALLIN_S.elf`)**: 0 Error, 0 Warning.
  - ROM: 23,592 Bytes (2.27% dari 1016 KB)
  - RAM: 12,048 Bytes (4.60% dari 256 KB)
- **NonSecure Target (`ALLIN_NS.elf`)**: 0 Error, 0 Warning.
  - ROM: 142,184 Bytes (13.56% dari 1 MB)
  - RAM: 64,880 Bytes (12.37% dari 512 KB)

### 2. Host CTest Suites
```text
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo
1/9 Test #1: Integration_E3_Demo ..............   Passed    0.03 sec
    Start 2: Unit_E3_Display_Tamper
2/9 Test #2: Unit_E3_Display_Tamper ...........   Passed    0.03 sec
    Start 3: Unit_E3_Power_Relay
3/9 Test #3: Unit_E3_Power_Relay ..............   Passed    0.03 sec
    Start 4: Unit_E3_FSM_Statechart
4/9 Test #4: Unit_E3_FSM_Statechart ...........   Passed    0.03 sec
    Start 5: Unit_E3_Load_Profile
5/9 Test #5: Unit_E3_Load_Profile .............   Passed    0.02 sec
    Start 6: Unit_E3_SPLN_Display_Tamper
6/9 Test #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.02 sec
    Start 7: Stress_E3_Final_System_Audit
7/9 Test #7: Stress_E3_Final_System_Audit .....   Passed    0.02 sec
    Start 8: Unit_E3_Console_CLI
8/9 Test #8: Unit_E3_Console_CLI ..............   Passed    0.12 sec
    Start 9: Unit_E3_Board_Profile
9/9 Test #9: Unit_E3_Board_Profile ............   Passed    0.02 sec

100% tests passed, 0 tests failed out of 9
```

