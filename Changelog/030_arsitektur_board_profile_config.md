# Log Perubahan 030 (LOG-030): Arsitektur Fleksibilitas Perangkat Keras via Board Profile Config

## 1. Ringkasan Eksekutif
- **ID Log:** `LOG-030`
- **Tanggal:** 28 Oktober 2026
- **Kategori:** Hardware Abstraction Layer (HAL) & Modular Architecture (Pilar E3)
- **Dasar Acuan Standar:**
  - **SPLN D3.006: 2021** (Spesifikasi Meter Statip Fase Tiga Prabayar & Pascabayar)
  - **Prinsip Arsitektur Perangkat Lunak:** *Separation of Concerns* & *Dependency Injection*
- **Status Verifikasi:**
  - **Host Unit Test:** Lulus 100% (9/9 suites CTest, termasuk `Unit_E3_Board_Profile`).
  - **Target ARM Cortex-M33 (NonSecure ELF):** 0 Error, 0 Warning (`ROM: 13.45%`, `RAM: 12.37%`).
  - **Target ARM Cortex-M33 (Secure ELF):** 0 Error, 0 Warning.

---

## 2. Latar Belakang & Masalah Teknis

Dalam proses siklus hidup produk meteran listrik (terutama transisi dari purwarupa R&D ke produksi massal pabrikasi), sering kali terjadi perubahan komponen perangkat keras (*hardware revisions*):
1. **Layar Tampilan:** Papan evaluasi/purwarupa menggunakan OLED I2C 128x32 (SSD1306) untuk kemudahan diagnostik visual, sedangkan papan produksi massal beralih ke Kaca Segment LCD khusus (driver Holtek HT1621 / PCF8576) demi efisiensi konsumsi daya ultra rendah (<10 µA) dan ketahanan terhadap sinar matahari langsung selama 10–15 tahun.
2. **Koil Relai Latching:** Dari tipe Dual-Coil (pin SET dan RESET terpisah) menjadi Single-Coil H-Bridge dengan polaritas pulsa berbeda.
3. **Polaritas Sensor Sabotase:** Sakelar mekanik tutup meter atau tutup terminal yang berubah dari tipe Normally Open (NO) menjadi Normally Closed (NC), atau Hall effect sensor aktif rendah (*active-low*).
4. **Penyimpanan Non-Volatile:** Dari chip SPI Flash eksternal (W25Qxx) menjadi I2C EEPROM (AT24C512) atau Ferroelectric RAM (FRAM).

Sebelum pembaruan ini, konstanta perangkat keras dan tingkat logika GPIO tersebar secara langsung di dalam kode aplikasi `e3_meter_app.c`, sehingga jika ada komponen yang diganti, insinyur firmware harus mencari dan mengubah baris-baris kode di berbagai tempat, meningkatkan risiko regresi.

---

## 3. Rincian Solusi & Perubahan Teknis

1. **Pembuatan Header Konfigurasi Profil Papan Terpusat ([board_profile_config.h](file:///g:/PPPP/ALLIN/E3/app/board_profile_config.h)):**
   - Menyediakan 3 profil papan standar:
     - `BOARD_PROFILE_PROTOTYPE_OLED`: Profil purwarupa aktif (OLED 128x32, Dual-Coil Latching, SPI Flash, NO switches).
     - `BOARD_PROFILE_PRODUCTION_SEGLCD`: Profil produksi massal (Segment LCD HT1621, Single-Coil H-Bridge, I2C EEPROM, NC switches).
     - `BOARD_PROFILE_HEADLESS_BENCH`: Profil automated bench/CI tanpa display fisik.
   - Mengelompokkan seluruh konstanta konfigurasi perangkat keras:
     - **Display:** I2C address, resolusi, timeout wake-up (10s), interval carousel (5s), rotasi error (3s).
     - **Relay:** Tipe koil, lebar pulsa (100ms / 80ms), polaritas feedback kontak bantu, ambang cut-off undervoltage (110V) & overvoltage (280V), durasi stabil reconnect (5s), dan durasi tombol simultan (1s).
     - **Tamper:** Polaritas sakelar tutup meter, tutup terminal, medan magnet, debounce (100ms), dan waktu persistensi magnet (3s / 10s).
     - **Storage NVRAM:** Backend Flash vs EEPROM, kapasitas ring buffer profil beban (100 entri), FIFO log (30 entri), billing snapshot (12 bulan).
     - **Komunikasi:** Baudrate port optik (9600 bps), port modem (19200 bps), port CLI (115200 bps).
     - **Metrologi & Telemetri:** Ambang anti-creep (3W), tegangan nominal (220V), frekuensi nominal (50Hz), ambang alarm baterai (<3.0V) dan superkapasitor (<2.5V).

2. **Penyelarasan Kode Aplikasi ([e3_meter_app.c](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)):**
   - Memodifikasi pembacaan pin GPIO tamper:
     ```c
     bool tamp_case = (HAL_GPIO_ReadPin(TAMP_CASE_OPEN_GPIO_Port, TAMP_CASE_OPEN_Pin) == 
                       (BOARD_CFG_TAMPER_CASE_OPEN_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET));
     ```
   - Memodifikasi pembacaan feedback relai:
     ```c
     bool relay_closed_fb = (HAL_GPIO_ReadPin(RELAY_STATUS_GPIO_Port, RELAY_STATUS_Pin) == 
                             (BOARD_CFG_RELAY_FB_CLOSED_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET));
     ```
   - Menghubungkan timeout layar mati:
     ```c
     s_lcd_wakeup_timer_ms = BOARD_CFG_DISPLAY_WAKE_MS;
     ```
   - Melengkapi field dinamis entri profil beban 15-menit:
     ```c
     lp_entry.active_energy_export_wh = meas->active_energy_import_wh; /* PLTS Export */
     lp_entry.reactive_energy_import_varh = meas->reactive_energy_varh;
     lp_entry.status_mask = tamper_get_active_mask(&s_tamper_ctx);
     ```

3. **Penambahan Unit Test Profil Papan ([test_board_profile.c](file:///g:/PPPP/ALLIN/E3/tests/test_board_profile.c)):**
   - Memverifikasi secara otomatis seluruh makro profil papan, nama revisi hardware, dan fungsi pembantu inline `board_cfg_is_oled_active()`, `board_cfg_is_segment_lcd_active()`, dan `board_cfg_get_board_name()`.
   - Ditambahkan sebagai test suite ke-9 di CTest (`Unit_E3_Board_Profile`).

---

## 4. Hasil Verifikasi & Kepatuhan

- **Kompilasi Host CTest:** 9 dari 9 test suites lolos 100% (durasi: 0.31 detik).
- **Target ARM NonSecure:** Berhasil dikompilasi dengan ROM 141.008 B (13.45%), RAM 64.864 B (12.37%), 0 warning, 0 error.
- **Target ARM Secure:** 0 warning, 0 error.

