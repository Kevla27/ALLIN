# Log Perubahan 033: Prosedur Listrik Padam (Last Gasp) Khusus Layar OLED, Emergency Flash Checkpoint & CLI Simulasi

**ID Log:** LOG-033  
**Tanggal:** 28 Oktober 2026  
**Topik Utama:** Prosedur Listrik Padam (Last Gasp) Layar OLED, Emergency NVRAM Flash Checkpoint, Off-Power Standby 10 Detik, dan Perintah CLI Simulasi (`acfail` & `button`)  
**Penanggung Jawab:** Pilar E3 (Display, HMI, Tamper & Storage Lead)

---

## 1. Latar Belakang & Urgensi Rekayasa
Pada fase pra-fabrikasi PCB massal saat modul layar prototipe yang digunakan adalah **OLED SSD1306 (128x64 I2C3)**, terdapat perbedaan karakteristik konsumsi daya yang sangat kritis dibandingkan Segment LCD produksi:
1. **Konsumsi Arus:** OLED SSD1306 mengonsumsi daya dinamis sekitar **20 mA s.d. 40 mA** (karena memiliki charge pump step-up DC-DC internal dan matriks LED mandiri), sedangkan Segment LCD mikrokontroler hanya mengonsumsi daya dalam rentang **mikroampere (µA)**.
2. **Keterbatasan Energi Superkapasitor (VCAP):** Ketika listrik PLN padam (`AC_FAIL_DET` / PC3 EXTI3 mendeteksi *falling edge*), meter harus beroperasi mengandalkan energi sisa pada superkapasitor 3.3V (dimonitor melalui pin `VCAP_SENSE` / PC2 ADC4 Channel 3). Jika OLED dibiarkan menyala saat proses *Last Gasp*, muatan superkapasitor akan drop sangat cepat sebelum operasi tulis sektor SPI Flash (W25Q / NVRAM Flash Bank 2) selesai, yang berisiko merusak integritas stand kWh penagihan.
3. **Bahaya Phantom Powering I2C:** Jika power rail `LCD_PWR_EN` (PD2) langsung diputus tanpa mematikan kontroler display terlebih dahulu, modul OLED dapat menarik arus parasitis (*phantom powering*) melalui jalur pull-up I2C (`PC0` SDA / `PC1` SCL), yang mempercepat pengosongan VCAP.

Oleh karena itu, diimplementasikan prosedur darurat terpadu bertingkat (*graceful shutdown*) khusus OLED dengan dukungan pembacaan stand stand-by darurat 10 detik sesuai standar SPLN D3.006: 2021 Butir 10.3 dan Desain Sistem Hal. 37.

---

## 2. Rincian Implementasi & Perubahan Kode

### A. Abstraksi Power Mode Driver Layar (`display_task.h` & `bsp_display_oled.c`)
- Menambahkan pointer antarmuka fungsi `set_power_mode(bool sleep_mode)` pada struktur `display_driver_interface_t`.
- Mengimplementasikan `bsp_oled_set_power_mode(bool sleep_mode)` di `bsp_display_oled.c`:
  - Saat `sleep_mode = true`: Memanggil `ssd1306_SetDisplayOn(0)` yang mengeksekusi opcode `0xAE` (Display OFF & Charge Pump OFF) dalam waktu < 1 ms via I2C3.
  - Saat `sleep_mode = false`: Memanggil `ssd1306_SetDisplayOn(1)` yang mengeksekusi opcode `0xAF` (Display ON).

### B. Modul Checkpoint Energi Flash NVRAM (`nvram_storage.h` & `nvram_storage.c`)
- Mendefinisikan struktur `nvram_energy_checkpoint_t`:
  ```c
  typedef struct {
      uint32_t magic;                    /**< 0x454E4759 ("ENGY") */
      uint32_t timestamp;                /**< Epoch timestamp saat pemadaman */
      uint64_t active_energy_import_wh;  /**< Stand energi aktif impor total (Wh) */
      uint64_t active_energy_export_wh;  /**< Stand energi aktif ekspor total (Wh) */
      uint64_t reactive_energy_varh;     /**< Stand energi reaktif (varh) */
      uint64_t tou_tariff_wh[8];         /**< Stand per zona tarif ToU (WBP, LWBP, dll) */
      uint32_t vcap_mv_at_fail;          /**< Tegangan superkapasitor VCAP saat AC Fail (mV) */
      uint32_t active_tamper_mask;       /**< Bitmask vektor sabotase yang sedang aktif */
  } nvram_energy_checkpoint_t;
  ```
- Mengimplementasikan `nvram_save_energy_checkpoint()` dan `nvram_load_energy_checkpoint()`. Pada target STM32, data ditulis ke sektor dedicated SPI2 Flash (`NVRAM_ENERGY_CHECKPOINT_START_ADDR`). Pada mode host simulation, data diamankan dalam RAM mirror berbasis magic byte.

### C. Alur Prosedur Pemadaman Listrik & Standby (`e3_meter_app.c`)
1. **Boot Uji Restorasi Stand (`e3_meter_app_init`):**
   - Menjalankan `nvram_load_energy_checkpoint()`. Jika validasi magic byte `0x454E4759` lolos, stand akumulasi `s_last_active_energy_wh` dan array ToU 8-zona langsung dipulihkan secara otomatis.
2. **Saat Terjadi Pemadaman (`e3_meter_app_on_ac_fail(true)`):**
   - Mencatat log event `SPLN_LOG_EVENT_POWER_FAILURE` ke FIFO Event Log.
   - Menyimpan seluruh stand energi aktif/reaktif/ToU ke Flash NVRAM via `nvram_save_energy_checkpoint()`.
   - Mengamankan snapshot 3 log FIFO (Alarm, Event, Tamper) via `nvram_save_tamper_log_snapshot()`.
   - Mengirimkan paket push darurat DLMS Last-Gasp via `e2_dlms_send_last_gasp_push()`.
   - Mengirim perintah sleep OLED `0xAE` melalui antarmuka driver I2C3.
   - Melakukan *power gating* GPIO: Mematikan `LCD_PWR_EN` (PD2), `LCD_BL_CTRL` (PD3), `RS232_PWR_EN` (PD4), `MODEM_12V_EN` (PD1), dan membisukan Buzzer (PB7).
3. **Saat Tombol Fisik Ditekan dalam Kondisi Padam (`e3_meter_app_on_button`):**
   - Menghidupkan power rail `LCD_PWR_EN` dan `LCD_BL_CTRL`.
   - Mengirimkan perintah wake-up `0xAF` ke OLED.
   - Mengatur timer hitung mundur `s_lcd_wakeup_timer_ms = 10000U` (10 detik).
   - Merender halaman UI SPLN seketika ke OLED agar petugas lapangan/konsumen dapat membaca stand meter tanpa suplai PLN.
4. **Saat Timer 10 Detik Habis (`e3_meter_app_step`):**
   - Mengirimkan kembali perintah sleep `0xAE` ke OLED.
   - Mematikan `LCD_PWR_EN` dan `LCD_BL_CTRL` untuk menghemat muatan superkapasitor/baterai.
5. **Saat Listrik PLN Pulih (`e3_meter_app_on_ac_fail(false)`):**
   - Mencatat log event `SPLN_LOG_EVENT_POWER_UP`.
   - Mengaktifkan kembali seluruh power rail dan membangunkan layar OLED secara permanen.

### D. Penambahan Perintah CLI Serial Interaktif (`console_io.c` & `console_io.h`)
- Menambahkan hook `set_ac_fail`, `is_ac_failed`, dan `button_press` pada `console_meter_hooks_t`.
- Mengimplementasikan perintah konsol:
  - `acfail` : Menampilkan status suplai listrik PLN.
  - `acfail 1` / `acfail cut` : Memicu simulasi listrik padam (Last-Gasp, checkpoint Flash, OLED sleep, power gating).
  - `acfail 0` / `acfail restore` : Memicu simulasi listrik pulih normal.
  - `button <up|down|both>` : Mensimulasikan penekanan tombol navigasi fisik, termasuk verifikasi menyalanya OLED selama 10 detik saat kondisi AC Fail.

---

## 3. Hasil Pengujian & Verifikasi

### A. Pengujian Unit Test Konsol CLI & Last Gasp Host (CTest)
Dieksekusi menggunakan CMake & CTest pada host w64devkit:
```text
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo
1/9 Test #1: Integration_E3_Demo ..............   Passed    0.03 sec
    Start 2: Unit_E3_Display_Tamper
2/9 Test #2: Unit_E3_Display_Tamper ...........   Passed    0.03 sec
    Start 3: Unit_E3_Power_Relay
3/9 Test #3: Unit_E3_Power_Relay ..............   Passed    0.03 sec
    Start 4: Unit_E3_FSM_Statechart
4/9 Test #4: Unit_E3_FSM_Statechart ...........   Passed    0.02 sec
    Start 5: Unit_E3_Load_Profile
5/9 Test #5: Unit_E3_Load_Profile .............   Passed    0.03 sec
    Start 6: Unit_E3_SPLN_Display_Tamper
6/9 Test #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.03 sec
    Start 7: Stress_E3_Final_System_Audit
7/9 Test #7: Stress_E3_Final_System_Audit .....   Passed    0.03 sec
    Start 8: Unit_E3_Console_CLI
8/9 Test #8: Unit_E3_Console_CLI ..............   Passed    0.03 sec
    Start 9: Unit_E3_Board_Profile
9/9 Test #9: Unit_E3_Board_Profile ............   Passed    0.03 sec

100% tests passed, 0 tests failed out of 9
```

### B. Kompilasi Target ARM Cortex-M33 (GCC STM32U585)
- **Secure Domain (`Secure/build`):**
  - ROM: 23.592 B (2.27%)
  - RAM: 12.048 B (4.60%)
  - **Hasil:** 0 Error, 0 Warning.
- **Non-Secure Domain (`NonSecure/build`):**
  - ROM: 144.536 B (13.78%)
  - RAM: 65.000 B (12.40%)
  - **Hasil:** 0 Error, 0 Warning.
