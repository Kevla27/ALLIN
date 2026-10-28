# LOG-038: Penyempurnaan Parameter Halaman Tabel E.2, Stempel Magic NVRAM & Read SPI2, Event Log FIFO Tabel 18, dan Pembersihan RTOS Tasks E3

**Tanggal:** 28 Oktober 2026  
**Status:** Selesai & Terverifikasi (12/12 CTest Suites LULUS 100%, `ALLIN_S.elf` & `ALLIN_NS.elf` 0 Error / 0 Warning)

---

## 1. Latar Belakang Perubahan
Audit fokus khusus pada pekerjaan **Pilar E3** menemukan 5 celah penyempurnaan runtime:
1. **Preservasi & Perhitungan Parameter Halaman Manual-Scroll (SPLN Tabel E.2):**
   - Pemanggilan `display_update_measurements()` melakukan `memcpy` penuh yang menimpa field statis `power_limit_w`, `firmware_checksum`, `eob_day`, `eob_hour`, dan `pln_unit_code` menjadi `0` jika caller tidak mengisinya kembali.
   - `eob_hour` pada `display_init()` bernilai `0` (seharusnya `10` untuk Tanggal 01 pukul 10:00 WIB sesuai SPLN D3.006:2021 Butir 12.4).
   - `max_demand_tracker_t` belum menghitung *Maximum Demand* Daya Semu (`max_demand_s_va` kVA & `max_demand_s_timestamp`, Tabel E.2 No. 35–36).
   - Energi aktif impor/terima PLTS (`active_energy_import_wh`) belum diakumulasikan ke 3 zona tarif ToU (`DISP_PAGE_ENERGY_IMP_WBP`, `DISP_PAGE_ENERGY_IMP_LWBP1`, `DISP_PAGE_ENERGY_IMP_LWBP2`, Tabel E.2 No. 11–13).
2. **Stempel `magic` Checkpoint Energi NVRAM & Fungsi Baca Fisik `SPI2`:**
   - Menjamin `energy_cp.magic = NVRAM_ENERGY_CHECKPOINT_MAGIC` (`0x454E4759U`) disetel saat *Last-Gasp*, checkpoint berkala 15-menit, dan di dalam `nvram_save_energy_checkpoint()`, serta melengkapi fungsi baca fisik `spi2_flash_read_bytes()` (`0x03`) di `nvram_storage.c`.
3. **Kelengkapan Pencatatan 12 Kode *Event Log FIFO* (SPLN Tabel 18):**
   - Menambahkan pencatatan otomatis `SPLN_LOG_EVENT_TIME_CHANGE` saat jam diubah (`e3_meter_app_set_epoch()`), `SPLN_LOG_EVENT_IO_CHANGE` saat input kontrol bantu berubah (`e3_meter_app_on_aux_input()`), dan `SPLN_LOG_EVENT_RELAY_CHANGE` saat relai putus/sambung karena *cut-off* tegangan (`<110V`/`>280V`), CLI disconnect/reconnect, maupun konfirmasi tombol lokal simultan (`BUTTON_DIR_BOTH`).
4. **Akurasi Ikon Kontaktor OLED Saat Wake-Up Tombol 10 Detik di Kondisi Padam:**
   - Menambahkan evaluasi `spln_frame.is_relay_closed = evaluate_is_relay_closed()` pada blok render seketika saat bangun dari Stop 2 Mode maupun penekanan tombol saat listrik PLN padam.
5. **Sinkronisasi Konstanta `board_profile_config.h`, Pembersihan `rtos_tasks.c`, & Penguatan Integrasi TrustZone/CLI:**
   - Menyelaraskan `BOARD_CFG_ADC4_BATT_LOW_MV` menjadi `2700U` (`2,7 V` sesuai `PIN.docx` & SPLN `ERR03`) dan `BOARD_CFG_ADC4_SUPERCAP_LOW_MV` menjadi `2000U`.
   - Membersihkan ketergantungan header usang di `rtos_tasks.h`, `rtos_tasks.c`, `display_task.c`, dan `test_rtos_tasks.c`, serta mendaftarkannya ke dalam suite pengujian CTest (`Unit_E3_NVRAM_Storage` & `Unit_E3_RTOS_Tasks`).
   - Mengaktifkan bit deteksi tamper fisik `TAMP1E | TAMP3E | TAMP7E` pada `MX_TAMP_RTC_Init()` di `Secure/Core/Src/main.c`, membaca pin Secure GPIO (`PC13`, `PE6`, `PE4`) sekaligus membersihkan latch `TAMP7F` saat magnet dilepas di `SECURE_GetTamperStatusMask()` (`Secure/Core/Src/secure_nsc.c`), dan menambahkan auto-clear 10 detik (`BOARD_CFG_TAMPER_MAG_CLEAR_MS`) di `e3_meter_app.c`.
   - Melengkapi akumulasi energi impor PLTS (`active_power_w < 0`) dan reaktif (`reactive_power_var != 0`) pada mode beban tiruan di `meter_system_bridge.c`, serta menampilkan `ToU Terima (02.08.1..3)` dan `Peak Max Demand S (09.06)` pada perintah CLI `energy` dan `demand` di `console_io.c`.

---

## 2. Daftar Berkas yang Diubah
- [`E3/app/board_profile_config.h`](file:///g:/PPPP/ALLIN/E3/app/board_profile_config.h)
- [`E3/app/display/src/display.c`](file:///g:/PPPP/ALLIN/E3/app/display/src/display.c)
- [`E3/app/display/src/display_task.c`](file:///g:/PPPP/ALLIN/E3/app/display/src/display_task.c)
- [`E3/app/profiling/inc/load_profile.h`](file:///g:/PPPP/ALLIN/E3/app/profiling/inc/load_profile.h)
- [`E3/app/profiling/src/load_profile.c`](file:///g:/PPPP/ALLIN/E3/app/profiling/src/load_profile.c)
- [`E3/app/profiling/inc/nvram_storage.h`](file:///g:/PPPP/ALLIN/E3/app/profiling/inc/nvram_storage.h)
- [`E3/app/profiling/src/nvram_storage.c`](file:///g:/PPPP/ALLIN/E3/app/profiling/src/nvram_storage.c)
- [`E3/app/fsm/inc/rtos_tasks.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/rtos_tasks.h)
- [`E3/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/rtos_tasks.c)
- [`E3/app/console/src/console_io.c`](file:///g:/PPPP/ALLIN/E3/app/console/src/console_io.c)
- [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)
- [`Secure/Core/Src/main.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/main.c) & [`Secure/Core/Src/secure_nsc.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/secure_nsc.c)
- [`NonSecure/Core/Src/meter_system_bridge.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/meter_system_bridge.c)
- [`E3/CMakeLists.txt`](file:///g:/PPPP/ALLIN/E3/CMakeLists.txt)
- [`E3/tests/test_e3_app.c`](file:///g:/PPPP/ALLIN/E3/tests/test_e3_app.c)
- [`E3/tests/test_load_profile.c`](file:///g:/PPPP/ALLIN/E3/tests/test_load_profile.c)
- [`E3/tests/test_board_profile.c`](file:///g:/PPPP/ALLIN/E3/tests/test_board_profile.c)
- [`E3/tests/test_nvram_storage.c`](file:///g:/PPPP/ALLIN/E3/tests/test_nvram_storage.c)
- [`E3/tests/test_rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/tests/test_rtos_tasks.c)
- [`E3/tests/test_load_profile_dlms_adapter.c`](file:///g:/PPPP/ALLIN/E3/tests/test_load_profile_dlms_adapter.c)
- [`E3/README.md`](file:///g:/PPPP/ALLIN/E3/README.md) & [`E3/PANDUAN_ARSITEKTUR_DAN_FILE_E3.md`](file:///g:/PPPP/ALLIN/E3/PANDUAN_ARSITEKTUR_DAN_FILE_E3.md)

---

## 3. Verifikasi Build & Pengujian
- **E3 CTest (12/12 Test Suites):** 100% PASS (0 gagal, 0 warning).
- **Target Firmware STM32U585 (`Secure` & `NonSecure`):**
  - `ALLIN_S.elf`: RAM 12.056 B (4,60%), ROM 24.000 B (2,31%), ROM_NSC 160 B (1,95%).
  - `ALLIN_NS.elf`: RAM 65.336 B (12,46%), ROM 174.112 B (16,60%).

