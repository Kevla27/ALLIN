# Log Perubahan 036: Manajemen Power Saving Ekstrem STM32U5 (Stop 2 Mode saat Listrik Padam)

**ID Log:** LOG-036  
**Tanggal:** 28 Oktober 2026  
**Topik Utama:** Manajemen Power Saving Ekstrem STM32U5 (Stop 2 Mode saat Listrik Padam, Wakeup EXTI/RTC & Restorasi Jam Adaptif)  
**Penanggung Jawab:** Power & Firmware Architecture Lead (Pilar E3)

---

## 1. Latar Belakang & Urgensi Rekayasa
Pada smart meter listrik 3-fasa standar SPLN D3.006:2021, meter wajib mampu mempertahankan jam kalender RTC, mendeteksi sabotase (tamper), dan melayani inspeksi lokal melalui tombol navigasi layar LCD/OLED selama bertahun-tahun dalam kondisi tanpa tegangan jala-jala PLN (*off-power / blackout*).

Apabila mikrokontroler STM32U585 terus berada pada *Run Mode* (160 MHz), konsumsi arus mencapai $\approx 19\text{ mA}$, yang akan menguras baterai litium internal dalam hitungan hari. Oleh karena itu, diimplementasikan arsitektur **Manajemen Power Saving Ekstrem berbasis Stop 2 Mode**:
- **Konsumsi Arus Normal (Run 160 MHz):** $\approx 19\text{ mA}$ (suplai PLN aktif).
- **Konsumsi Arus Baterai Siaga Display (MSIS 4 MHz):** $\approx 1.5\text{ mA}$ (selama 10 detik saat tombol ditekan).
- **Konsumsi Arus Ekstrem Stop 2 Mode:** $\approx 4.5\ \mu\text{A}$ (CPU deep sleep, clock mati, retensi data SRAM1..SRAM4 penuh).

---

## 2. Arsitektur State Machine Manajemen Daya (`bsp_power_mode.h` & `bsp_power_mode.c`)

Dibangun modul abstraksi terpadu `bsp_power_mode` dengan tiga status daya operasional:
1. `METER_POWER_MODE_RUN` (Mode Normal PLN):
   - Beroperasi pada kecepatan tinggi penuh (PLL 160 MHz, Voltage Scale 1).
   - Seluruh periferal aktif: ADE9000 Run, OLED aktif, RS-232 aktif, Modem 12V aktif, FreeRTOS 100ms task loop.
2. `METER_POWER_MODE_BATTERY_ACTIVE` (Mode Darurat Layar Aktif):
   - Dipicu oleh penekanan tombol navigasi display (PA0 UP / PB6 DOWN) saat listrik padam.
   - Jam diposisikan pada **MSIS 4 MHz (Scale 4)** tanpa menyalakan PLL yang boros daya.
   - Rail daya OLED diaktifkan (`LCD_PWR_EN`, `LCD_BL_CTRL`), SSD1306 dibangunkan (`0xAF`), dan timer 10 detik dijalankan.
3. `METER_POWER_MODE_STOP2` (Mode Deep Sleep Ekstrem):
   - Dipicu otomatis saat listrik padam dan timer layar 10 detik habis (`s_lcd_wakeup_timer_ms == 0`).
   - Periferal analog (ADC4) dihentikan, relai daya display diputus, OLED dikirim perintah sleep (`0xAE`).
   - SysTick interrupt dinonaktifkan sementara dan scheduler FreeRTOS ditangguhkan.
   - CPU memasuki `HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI)` dengan retensi penuh register dan SRAM.

---

## 3. Sumber Wake-up Terpadu (Interrupt EXTI & RTC)

Mikrokontroler dapat bangun dari Stop 2 secara seketika (< 5 $\mu\text{s}$) melalui sumber pemicu:
- **EXTI Line 3 (`AC_FAIL_DET`, PC3):** *Rising edge* saat suplai listrik PLN menyala kembali.
- **EXTI Line 0 (`BTN_SCROLL_UP`, PA0):** *Falling edge* saat tombol navigasi atas ditekan.
- **EXTI Line 6 (`BTN_SCROLL_DN`, PB6):** *Falling edge* saat tombol navigasi bawah ditekan.
- **EXTI Line 13 (`TAMP_CASE_OPEN`, PC13) & TrustZone TAMP:** Sakelar pembukaan tutup meter.
- **EXTI Line 6 (`TAMP_TERM_COVER`, PE6) & Line 4 (`SENSOR_MAGNET`, PE4):** Sabotase terminal dan medan magnet.
- **RTC Wakeup Timer Periodik:** Timer interval kalender (misal: penutupan tagihan bulanan atau *health check*).

---

## 4. Logika Restorasi Jam Adaptif (*Adaptive Clock Restoration*)

Saat bangun dari Stop 2, STM32U5 secara perangkat keras selalu memulai eksekusi pada clock internal MSIS (4 MHz):
1. **Jika terbangun karena Listrik PLN Pulih (`AC_RESTORED`):**
   - Fungsi `bsp_power_restore_run_clock()` dipanggil.
   - Voltage regulator dinaikkan ke Scale 1, PLL1 diinisialisasi ulang hingga frekuensi 160 MHz (`FLASH_LATENCY_4`), dan `SystemCoreClockUpdate()` dieksekusi.
   - Periferal ADE9000, RS-232, Modem 12V, dan display dikembalikan ke status RUN penuh.
2. **Jika terbangun karena Tombol Navigasi Layar (`BUTTON_PRESS`):**
   - Jam **tetap dipertahankan pada MSIS 4 MHz (Scale 4)** tanpa menyalakan PLL 160 MHz.
   - Layar OLED dinyalakan selama 10 detik untuk melayani pembacaan stand meter kWh dan status tamper.
   - Konsumsi energi baterai ditekan hingga 92% lebih hemat dibandingkan menyalakan PLL.
3. **Jika terbangun karena Tamper Sabotase (`TAMPER`):**
   - Event dicatat ke log FIFO Secure & Non-Secure, lalu mikrokontroler segera kembali tidur ke Stop 2.

---

## 5. Integrasi Perintah Serial CLI (`console_io.c`)

Menambahkan fitur inspeksi daya pada konsol serial UART:
- Menambahkan perintah `power` (dan alias `pwr`) untuk menampilkan diagnostik detail daya:
  ```text
  === MANAJEMEN POWER SAVING EKSTREM STM32U5 ===
    Status Suplai AC : NORMAL (AC MAINS AKTIF)
    Moda Konsumsi    : RUN_160MHZ_AC_MAINS
    Target Arus      : ~19 mA (Run 160MHz PLL)
    Total Siklus Wk  : 0 kali
    Pemicu Terakhir  : NONE (Initial boot)
    Profil Perangkat : Stop 2 ENABLED (Aktif), Wakeup Clock MSI 4MHz
  ```
- Menambahkan status moda konsumsi daya dan riwayat siklus bangun pada perintah `status`.

---

## 6. Hasil Pengujian & Kompilasi

### A. Pengujian Host Otomatis (CTest Suite)
Menambahkan Test Suite ke-10 `test_power_mode.c` untuk menguji siklus inisialisasi, transisi status daya, simulasi 4 pemicu bangun (Tombol, Tamper, RTC, AC Restored), dan restorasi clock.
```text
Test project G:/PPPP/ALLIN/E3/build
      Start  1: Integration_E3_Demo ...............   Passed    0.02 sec
      Start  2: Unit_E3_Display_Tamper ...........   Passed    0.02 sec
      Start  3: Unit_E3_Power_Relay ..............   Passed    0.02 sec
      Start  4: Unit_E3_FSM_Statechart ...........   Passed    0.02 sec
      Start  5: Unit_E3_Load_Profile .............   Passed    0.02 sec
      Start  6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.02 sec
      Start  7: Stress_E3_Final_System_Audit .....   Passed    0.02 sec
      Start  8: Unit_E3_Console_CLI ..............   Passed    0.02 sec
      Start  9: Unit_E3_Board_Profile ............   Passed    0.02 sec
      Start 10: Unit_E3_Power_Mode_Stop2 .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 10
Total Test time (real) = 0.25 sec
```

### B. Kompilasi Target Hardware ARM Cortex-M33 (GCC STM32U585)
- **Secure Domain (`Secure/build`):** ROM 23.592 B (2.27%), RAM 12.048 B (4.60%) — *0 Error, 0 Warning*.
- **Non-Secure Domain (`NonSecure/build`):** ROM 157.064 B (14.98%), RAM 65.088 B (12.41%) — *0 Error, 0 Warning*.
