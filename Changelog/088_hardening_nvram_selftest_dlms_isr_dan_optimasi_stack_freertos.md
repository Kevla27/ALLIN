# LOG-088: Hardening NVRAM SPI Flash, Self-Test ERR00/01/05, Dekopling DLMS ISR & Optimasi Stack/Heap STM32U585VIT6

- **Tanggal:** 25 November 2026
- **Status:** Selesai & Terverifikasi Penuh (CTest 13/13 Passed, `ALLIN_S.elf` & `ALLIN_NS.elf` 0 Error/0 Warning)
- **Tipe Perubahan:** Reliability / NVRAM Durability / RTOS Real-Time Determinism / Memory Safety Optimization
- **Pilar / File Terdampak:** `E1/SPI/*`, `E1/Metrology/e1_metrology_service.c`, `E2/Inc/e2_dlms_service.h`, `E2/Src/e2_dlms_service.c`, `E3/app/board_profile_config.h`, `E3/app/display/inc/display.h`, `E3/app/display/src/display_task.c`, `E3/app/e3_meter_app.*`, `E3/app/fsm/*`, `E3/app/profiling/*`, `E3/tests/test_nvram_storage.c`, `E3/tests/test_rtos_tasks.c`, `NonSecure/Core/Inc/FreeRTOSConfig.h`, `NonSecure/Core/Src/app_freertos.c`, `NonSecure/Core/Src/meter_system_bridge.c`, `docs/*`

---

## 1. Latar Belakang & Tujuan

Berdasarkan temuan tinjauan desain FreeRTOS dan NVRAM pada [`docs/EVALUASI_FREERTOS_DAN_NVRAM_DESIGN_REVIEW.md`](file:///g:/PPPP/ALLIN/docs/EVALUASI_FREERTOS_DAN_NVRAM_DESIGN_REVIEW.md) serta hasil inspeksi keluaran `-fstack-usage` kompilasi ARM Cortex-M33 (`ALLIN_NS.elf`), dituntaskan empat area celah firmware inti agar siap diuji di target perangkat keras **STM32U585VIT6**.

---

## 2. Rincian Modifikasi Teknis

### A. Keamanan NVRAM, Retensi 92-Hari & Wear-Leveling (`ERR00`)
- **[`nvram_storage.h`](file:///g:/PPPP/ALLIN/E3/app/profiling/inc/nvram_storage.h), [`nvram_storage.c`](file:///g:/PPPP/ALLIN/E3/app/profiling/src/nvram_storage.c):**
  - Mengimplementasikan penyimpanan **dual-bank atomik** bervalidasi `magic`, nomor urut `seq` monotonik, dan `nvram_calc_crc32()` untuk `Tamper Snapshot` (`NVRAM_TAMPER_SNAP_MAGIC`), `Device Config`, dan `Calibration`, sehingga Flash kosong (`0xFF`) maupun sektor korup tidak lagi memuat data sampah ke konteks meter.
  - Mengimplementasikan **Checkpoint Ring 8 Sektor × 16 Page (128 Slot)** dengan interval penulisan RTOS **300 detik** (5 menit) serta pelompatan halaman kotor saat pemulihan boot.
  - Mengimplementasikan antarmuka **Load Profile 92-Hari (`8.832` slot, stride `128 B` sejajar sektor 4 KB)** melalui `nvram_write_load_profile_flash_slot()` dan `nvram_read_load_profile_flash_slot()`.
  - Mendukung verifikasi JEDEC ID **W25Q64 (`0x17`)** maupun **W25Q128JVS (`0x18`)** di `nvram_flash_verify()` / `nvram_w25q64_verify()`, dan melacak kegagalan SPI2 melalui `nvram_has_hardware_fault()`.
  - Mengisolasi array `simulated_*` hanya untuk `#if !defined(EMBEDDED_HARDWARE_TARGET)` sehingga `ALLIN_NS.elf` tidak mengalokasikan buffer simulasi Flash ~733 KB di SRAM MCU.

### B. Self-Test (`ERR00`, `ERR01`, `ERR05`), Zona Waktu WIB & Cleanup WCET
- **[`SPI.h`](file:///g:/PPPP/ALLIN/E1/SPI/SPI.h), [`SPI.c`](file:///g:/PPPP/ALLIN/E1/SPI/SPI.c):** Menambahkan `ADE9000_SPI_IsHardwareActive()`.
- **[`e3_meter_app.h`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.h), [`e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c), [`board_profile_config.h`](file:///g:/PPPP/ALLIN/E3/app/board_profile_config.h):**
  - Menambahkan `e3_meter_app_run_ram_selftest()` (pola checkerboard + walking-1 non-destruktif untuk `ERR01`).
  - Mengaktifkan pemeriksaan Flash (`ERR00`), RAM (`ERR01`), dan kesehatan ADE9000 (`ADE9000_PART_ID`, `ADE9000_RUN`, serta deteksi `e1_metrology_get_dready_pulses()` macet untuk `ERR05`) dalam `e3_meter_app_run_health_check()`.
  - Menambahkan `BOARD_CFG_LOCAL_TIMEZONE_SEC` (`0U`) untuk ToU/EoB, memperbarui penulisan Load Profile ke `s_lp_ctx.total_written - 1U`, serta membersihkan sisa `wcet_record_start`/`stop` di `e3_meter_app_step()` dan `app_metering_step()`.

### C. Determinisme RTOS (`vTaskDelayUntil`) & Sinkronisasi RTC Epoch
- **[`rtos_tasks.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/rtos_tasks.h), [`rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/rtos_tasks.c), [`display_task.c`](file:///g:/PPPP/ALLIN/E3/app/display/src/display_task.c):**
  - Menambahkan `rtos_get_rtc_epoch_sec()` dan mengganti seluruh timestamp berbasis tick boot ke epoch RTC nyata.
  - Mengganti seluruh `vTaskDelay()` periodik (`TamperTask`, `ProfileTask`, `DLMSTask`, `ModemTask`, `HealthTask`, `OLEDTask`) ke `vTaskDelayUntil()`, menyinkronkan waktu OLED (`display_set_time()`), dan menyelaraskan pemicu Load Profile 15 menit terhadap batas `900 s` RTC.

### D. Dekopling DLMS UART ISR, Mode E Optik, THD ADE9000 & FSM Modem 4G
- **[`e2_dlms_service.h`](file:///g:/PPPP/ALLIN/E2/Inc/e2_dlms_service.h), [`e2_dlms_service.c`](file:///g:/PPPP/ALLIN/E2/Src/e2_dlms_service.c), [`meter_system_bridge.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/meter_system_bridge.c):**
  - `e2_dlms_service_uart_rx_callback()` kini hanya memasukkan byte ke SPSC ring buffer (`rx_ring[256]`) di ISR; seluruh parsing HDLC, APDU/cipher, jabat tangan optik **IEC 62056-21 Mode E** (`handle_optical_mode_e_byte()`), dan transmisi TX berjalan di `DLMSTask` via `e2_dlms_service_poll()`.
  - Menambahkan `e2_dlms_service_uart_error_callback()` untuk pemulihan `ORE`/`NE`/`FE`, serta pemisahan FIFO AT modem (`s_modem_at_fifo`) dengan FSM AT non-blocking (`AT`, `AT+CPIN?`, `AT+CREG?`, `AT+CGATT=1`) di `bridge_modem_step_callback()`.
- **[`display.h`](file:///g:/PPPP/ALLIN/E3/app/display/inc/display.h#L152-L155), [`e1_metrology_service.c`](file:///g:/PPPP/ALLIN/E1/Metrology/e1_metrology_service.c#L248-L276):** Membaca register THD `ADE9000_AVTHD..CITHD` ke snapshot DLMS saat AFE fisik aktif.

### E. Optimasi Stack & Heap 8-Task terhadap Memori STM32U585VIT6
- Memindahkan struct lokal besar pada singleton task ke `static`, memangkas stack frame `-fstack-usage` (`-O0 -g3`) secara drastis:
  - `task_storage_logger_entry`: `1.824 B` -> **`24 B`**
  - `task_oled128x32_carousel`: `816 B` -> **`136 B`**
  - `task_metrology_profiling_entry`: `552 B` -> **`56 B`**
  - `task_tamper_emergency_entry`: `512 B` -> **`112 B`**
  - `task_dlms_entry`: `264 B` -> **`32 B`**
- Menetapkan konfigurasi baru di [`rtos_tasks.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/rtos_tasks.h#L36-L43), [`app_freertos.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/app_freertos.c#L52), dan [`FreeRTOSConfig.h`](file:///g:/PPPP/ALLIN/NonSecure/Core/Inc/FreeRTOSConfig.h#L74-L190):
  - `STACK_SIZE_*` (words): Tamper `512` (`2.048 B`), Profiling `768` (`3.072 B`), DLMS `1536` (`6.144 B`), Modem `768` (`3.072 B`), UI `768` (`3.072 B`), Health `512` (`2.048 B`), Storage `768` (`3.072 B`), defaultTask `768` (`3.072 B`).
  - `configTOTAL_HEAP_SIZE`: **`49.152 B` (`48 KB`)**, kebutuhan kernel + 8 task `~30 KB` (sisa margin ~`19 KB` / ~`39%`).
  - `configMINIMAL_SECURE_STACK_SIZE`: **`256` words (`1.024 B`)** agar Secure Heap `10 KB` aman hingga 8 task pemanggil NSC (termasuk `OLEDTask` & `DLMSTask`).

---

## 3. Verifikasi Hasil Build & Pengujian

- **CTest Host Suite (`E3/build`):** **`100% tests passed, 0 tests failed out of 13`**, termasuk penambahan test di [`test_nvram_storage.c`](file:///g:/PPPP/ALLIN/E3/tests/test_nvram_storage.c#L198-L280) (virgin `0xFF`, 140 checkpoint = 9 sector erase, dan 92-hari 8.832-slot wrap) dan [`test_rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/tests/test_rtos_tasks.c#L272-L282).
- **STM32U585VIT6 Build (`0 Error`, `0 Warning`):**
  - `ALLIN_S.elf`: **`RAM 12.064 B / 256 KB (4,60%)`**, `ROM 25.072 B / 1016 KB (2,41%)`, `ROM_NSC 192 B / 8 KB (2,34%)`.
  - `ALLIN_NS.elf`: **`RAM 98.512 B / 512 KB (18,79%)`**, `ROM 219.072 B / 1 MB (20,89%)`, `SRAM4 0 B`.
