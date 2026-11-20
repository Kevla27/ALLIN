# Log Perubahan 084: Arsitektur Preemptive FreeRTOS Multi-Tasking & Supervisor Watchdog (WCET/PD12)

- **Judul Dokumen:** Implementasi 4 Task Preemptive FreeRTOS Terpisah, IPC Mutex/Queue, Alokasi TrustZone Cortex-M33, dan Pengawasan Supervisor Watchdog Pin PD12 Sesuai Bab 3.3 Desain Perangkat Lunak & Firmware E3 (Prioritas 3)
- **Tanggal:** 20 November 2026
- **Status:** Selesai & Terverifikasi Penuh (0 Error, 0 Warning, CTest 13/13 Passed, 100%)
- **Tipe Perubahan:** FreeRTOS Multi-Tasking Architecture / Preemptive Scheduling / Rate-Monotonic Realignment / WCET Watchdog
- **Pilar / Komponen Terdampak:** Pilar E3 (`E3/app/fsm/inc/rtos_tasks.h`, `E3/app/fsm/src/rtos_tasks.c`, `E3/app/display/src/display_task.c`, `E3/tests/test_rtos_tasks.c`), Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`, `NonSecure/Core/Src/app_freertos.c`, `NonSecure/CMakeLists.txt`).

---

## Bagian 1: Latar Belakang & Analisis Integrator

Berdasarkan audit arsitektur terhadap **Bab 3.3 Desain Perangkat Lunak & Firmware E3 (Tabel 3.3.2 Hirarki Task dan Anggaran Waktu)** serta **SPLN D3.006: 2021**, firmware sebelumnya masih menjalankan loop metrologi dan seluruh fungsi sistem di dalam satu thread *superloop* monolithic (`app_metering_step`), sementara task-task FreeRTOS terpisah belum aktif sepenuhnya. Hal ini berisiko menimbulkan *starvation* pada tugas darurat sabotase atau kegagalan *timing* pada protokol komunikasi DLMS.

Sesuai rencana pembenahan **Prioritas 3**, sistem direfaktor menjadi arsitektur **Preemptive Multi-Tasking murni** dengan 4 Task FreeRTOS independen + 1 Supervisor Task pengawas:

1. **Pemisahan 4 Task Preemptive Berdasarkan Prioritas CMSIS-RTOS V2:**
   - **`Task_Tamper_Emergency` (Prioritas 32 - `osPriorityRealtime`):** Siklus 100 ms / event-driven. Bertanggung jawab memonitor sakelar sabotase fisik melalui antarmuka TrustZone (`SECURE_GetTamperStatusMask`), memutus relai kontaktor seketika (*instant trip*), memicu *instant refresh* display OLED, dan mengirimkan pesan ke antrean IPC `xTamperQueue`.
   - **`Task_Metrology_Profiling` (Prioritas 28 - `osPriorityHigh`):** Siklus 1000 ms. Mengakuisisi register metrologi ADE9000 & perhitungan beban sintetis, mempublikasikan snapshot ke `xMeasMutex`, melakukan *dual-slot Ping-Pong checkpoint* ke NVRAM tiap 60 detik, dan membekukan rekaman 23 kanal profil beban tiap 15 menit.
   - **`Task_DLMS_Server` (Prioritas 24 - `osPriorityNormal`):** Siklus 50 ms. Mengonsumsi antrean sabotase `xTamperQueue`, mencatat ke logbook Generic Profile DLMS Class 7, menaikkan counter OBIS, dan melayani komunikasi HDLC port optik & modem seluler.
   - **`Task_UI_Display` (Prioritas 16 - `osPriorityBelowNormal`):** Siklus 100 ms (dengan rotasi *auto-scroll* frame 5 detik sesuai SPLN D3.006 Tabel E.1). Mengambil snapshot pengukuran secara aman melalui Mutex dan merender frame ke layar OLED 128x32 SSD1306.
   - **`defaultTask` (Supervisor & Watchdog, Prioritas 8 - `osPriorityLow`):** Berfungsi sebagai pengawas sistem. Memproses byte CLI interaktif dan mengevaluasi kepatuhan batas waktu WCET seluruh tugas melalui `wcet_watchdog_supervise_and_feed()`. Pulsa penyegaran pin hardware **PD12** (`EXT_WDI_PULSE`) hanya dihasilkan jika seluruh task berjalan normal dan tidak ada task yang melanggar *deadline* atau mengalami *starvation*.

2. **Kepatuhan TrustZone ARM Cortex-M33:**
   - Memastikan task Non-Secure yang mengakses fungsi Secure NSC (`SECURE_GetTamperStatusMask`) mengalokasikan konteks keamanan dengan memanggil `portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE)`.

3. **Sinkronisasi Data Bebas Race-Condition (Thread-Safe Snapshot):**
   - Mengimplementasikan `rtos_meter_data_publish()` dan `rtos_meter_data_get_snapshot()` yang diproteksi `xMeasMutex`. Dengan demikian, proses rendering grafis OLED atau serialisasi DLMS tidak akan pernah membaca data pengukuran yang sedang diperbarui di tengah jalan oleh task metrologi.

---

## Bagian 2: Rincian Modifikasi Kode Program

### 1. Definisi Prioritas & Stack Task (`E3/app/fsm/inc/rtos_tasks.h`)
- Menyesuaikan hirarki prioritas task ke standar numerik CMSIS-RTOS V2:
  ```c
  #define PRIORITY_TASK_TAMPER    32  /* Task 1: Realtime Emergency Tamper */
  #define PRIORITY_TASK_PROFILING 28  /* Task 2: High Metrology & Profiling */
  #define PRIORITY_TASK_DLMS      24  /* Task 3: Normal DLMS/COSEM HDLC Server */
  #define PRIORITY_TASK_UI        16  /* Task 4: BelowNormal UI Display HMI */
  ```
- Menambahkan fungsi pemutakhiran dan pengambilan snapshot:
  ```c
  void rtos_meter_data_publish(const meter_measurements_t *meas);
  bool rtos_meter_data_get_snapshot(meter_measurements_t *out_meas);
  ```

### 2. Implementasi Multi-Tasking & Entry Points (`E3/app/fsm/src/rtos_tasks.c`)
- Pada `task_tamper_emergency_entry`:
  - Menambahkan alokasi `portALLOCATE_SECURE_CONTEXT`.
  - Mengintegrasikan pembacaan status tamper hardware `SECURE_GetTamperStatusMask()`.
  - Memicu pemutusan relai instan dan `display_task_trigger_instant_refresh()`.
- Pada `task_metrology_profiling_entry`:
  - Menambahkan eksekusi *callback* metrologi `bridge_metrology_sample_callback`.
  - Menerapkan interval 60 detik untuk penyimpanan *checkpoint* energi Ping-Pong dual-buffering ke NVRAM.
  - Menerapkan interval 15 menit (900 detik) untuk pembekuan profil beban ke NVRAM.
- Pada `task_dlms_entry`:
  - Mengonsumsi antrean sabotase dari `xTamperQueue`.
  - Memanggil `rtos_meter_data_get_snapshot()` dan `e2_dlms_service_sync_snapshot()` agar data instan DLMS selalu tersinkronisasi thread-safe.
- Pada `task_oled128x32_carousel` (`E3/app/display/src/display_task.c`):
  - Mengambil data melalui `rtos_meter_data_get_snapshot(&meas)`.
  - Menambahkan instrumentasi waktu eksekusi: `wcet_record_start(WCET_TASK_UI)` dan `wcet_record_stop(WCET_TASK_UI)`.

### 3. Jembatan Sistem & Supervisor Loop (`NonSecure/Core/Src/meter_system_bridge.c`)
- Menghubungkan driver display OLED fisik ke display task: `display_task_set_driver(bsp_display_oled_get_driver())`.
- Mengimplementasikan `bridge_metrology_sample_callback(meter_measurements_t *out_meas)` yang menjembatani akuisisi ADE9000 dan kalkulasi beban tiruan (*synthetic load*).
- Pada `app_metering_task_loop()`:
  - Meluncurkan ke-4 task via `rtos_start_all_tasks()`.
  - Mengubah thread `defaultTask` menjadi *Supervisor*: memproses FIFO antrean CLI serial non-blocking serta memanggil `wcet_watchdog_supervise_and_feed()` setiap 100 ms.

### 4. Konfigurasi Target CMake (`NonSecure/CMakeLists.txt`)
- Menambahkan file sumber yang diperlukan ke `target_sources`:
  - `${CMAKE_SOURCE_DIR}/../E3/app/display/src/display_task.c`
  - `${CMAKE_SOURCE_DIR}/../E3/app/fsm/src/rtos_tasks.c`

### 5. Pengujian Unit Test Suite (`E3/tests/test_rtos_tasks.c`)
- Memvalidasi siklus penuh:
  - Inisialisasi antrean dan mutex.
  - Pengiriman pesan event sabotase oleh task Tamper (Producer).
  - Penerimaan pesan event oleh task DLMS (Consumer) dan penambahan counter OBIS.
  - Publikasi dan pengambilan thread-safe measurement snapshot.

---

## Bagian 3: Verifikasi & Hasil Pengujian

- **Unit Test CTest Host:**
  ```text
  Test #12: Unit_E3_RTOS_Tasks ............... Passed    0.08 sec
  Test #13: Unit_E3_WCET_Profiler ............ Passed    0.02 sec
  100% tests passed, 0 tests failed out of 13
  Total Test time (real) = 0.40 sec
  ```
- **Kompilasi Firmware Target ARM (`ALLIN_NS.elf`):**
  ```text
  Memory region         Used Size  Region Size  %age Used
               RAM:       85384 B       512 KB     16.29%
             SRAM4:           0 B        16 KB      0.00%
               ROM:      198528 B         1 MB     18.93%
  ```
  *Status: 0 Errors, 0 Warnings pada target mikrokontroler STM32U585VIT6 Non-Secure.*
- **Integritas Git Commit:**
  - Commit ID: `64f25da`
  - Push status: Sukses ke `origin/main`.
