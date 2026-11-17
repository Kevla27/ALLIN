# Pembuatan Flowchart Berstandar ANSI/ISO 5807 untuk Seluruh Task FreeRTOS

**Tanggal:** 17 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Technical Design & Multi-Tasking Architecture Documentation (ANSI/ISO 5807)  
**Komponen Terdampak:** Dokumentasi Sistem ([`docs/freertos_tasks_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/freertos_tasks_flowchart.drawio)), Pilar E3 (`E3/app/fsm/src/rtos_tasks.c`, `E3/app/display/src/display_task.c`), Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`, `NonSecure/Core/Src/app_freertos.c`)

---

## 1. Latar Belakang & Analisis Integrator

Sesuai dengan arsitektur *multitasking* FreeRTOS pada firmware Smart Meter 3-Fasa (STM32U585), eksekusi program terbagi ke dalam beberapa *task* independen dengan prioritas dan periode waktu yang dirancang untuk mencegah perebutan sumber daya (*race condition*) dan menjamin determinisme waktu nyata (*real-time determinism*).

Untuk melengkapi dokumentasi teknis sistem dan memudahkan audit aliran data antar-task, disusun berkas Draw.io multi-tab [**`docs/freertos_tasks_flowchart.drawio`**](file:///g:/PPPP/ALLIN/docs/freertos_tasks_flowchart.drawio) yang memuat bagan alir berstandar **ANSI/ISO 5807** untuk setiap task FreeRTOS yang berjalan di sistem.

---

## 2. Struktur Berkas `docs/freertos_tasks_flowchart.drawio` (6 Tab)

### Tab 1: `1. Arsitektur Task & IPC` (`tab0_rtos_overview`)
* **Tujuan:** Diagram gambaran makro yang memetakan seluruh task, prioritas thread, mekanisme komunikasi antar-task (IPC Queue `xTamperQueue`, Mutex `xMeasMutex`), periferal masukan/ISR (EXTI Tamper, ADE9000 IRQ, SysTick), partisi penyimpanan Flash NVM (`D1`–`D4`), serta supervisi *Independent Watchdog* (IWDG).

### Tab 2: `2. Task 1: TamperTask` (`task_tamper_emergency_entry`)
* **Fungsi:** Deteksi darurat sabotase fisik (Case Open `PC13`, Terminal Cover `PE6`, Medan Magnet Hall `PE4`) dan pemutusan relai kontaktor seketika.
* **Prioritas:** `PRIORITY_TASK_TAMPER` (Prioritas 3 - High).
* **Periode Siklus:** 200 ms (`vTaskDelay(pdMS_TO_TICKS(200))`).
* **Fitur Utama:**
  - Evaluasi transisi `display_task_is_alarm_active()`.
  - Pemutusan beban darurat: `power_relay_disconnect(..., RELAY_REASON_MANUAL, now_sec)`.
  - Pengepakan pesan `tamper_event_msg_t` dan pengiriman ke antrean IPC `xTamperQueue`.
  - Supervisi watchdog: `bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT)`.

### Tab 3: `3. Task 2: ProfileTask` (`task_metrology_profiling_entry`)
* **Fungsi:** Akuisisi sampel metrologi listrik ADE9000, evaluasi proteksi relai (asimetri arus fasa & batas daya kontrak VA), dan pembekuan deret berkala 23-kanal beban tiap 15 menit.
* **Prioritas:** `PRIORITY_TASK_PROFILING` (Prioritas 2 - Normal).
* **Periode Siklus:** 1000 ms (1 detik).
* **Fitur Utama:**
  - Panggilan `s_meter_sample_cb(&meas)` dan publikasi thread-safe via `rtos_meter_data_publish` (`xMeasMutex`).
  - Deteksi deviasi asimetri beban fasa 3-fasa (`power_relay_check_asymmetry`).
  - Pengawasan pembatas daya aktif (`power_relay_process_power` $\rightarrow$ Relai TRIP saat beban lebih).
  - Akumulasi interval 900 detik (15 menit) $\rightarrow$ pembekuan snapshot 23 kanal beban ke ring buffer `load_profile_add_entry`.
  - Supervisi watchdog: `bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT)`.

### Tab 4: `4. Task 3: DLMSTask` (`task_dlms_entry`)
* **Fungsi:** Pengolahan antrean pesan sabotase asinkron, pencatatan logbook Generic Profile DLMS Class 7, pemutakhiran counter OBIS, persistensi ke Flash NVRAM, dan penanganan server protokol HDLC.
* **Prioritas:** `PRIORITY_TASK_DLMS` (Prioritas 2 - Normal).
* **Periode Siklus:** 10 ms polling antrean.
* **Fitur Utama:**
  - Penerimaan non-blocking dari IPC Queue: `rtos_queue_receive_tamper_event(&event_msg, 0)`.
  - Pencatatan ke generic profile generic logbook (`0.0.99.98.0.255`, Class 7).
  - Inkremen OBIS Tamper Counter dan pemutakhiran `g_tamper_ctx`.
  - Penyimpanan snapshot logbook ke memori Flash: `nvram_save_tamper_log_snapshot`.
  - Layanan komunikasi server DLMS/COSEM HDLC (USART2/USART3).
  - Supervisi watchdog: `bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT)`.

### Tab 5: `5. Task 4: OLEDTask` (`task_oled128x32_carousel`)
* **Fungsi:** Manajemen antarmuka pengguna HMI OLED 128x32 SSD1306 SPLN D3.006 (Auto-scroll 5 detik Tabel E.1, interlok relai READY ON, dan pemicuan instan alarm sabotase).
* **Prioritas:** `PRIORITY_TASK_UI` (Prioritas 1 - Low/Normal).
* **Periode Siklus:** 100 ms refresh tick, 5000 ms pergeseran halaman carousel.
* **Fitur Utama:**
  - Inisialisasi driver display I2C3 agnostik perangkat keras.
  - Penanganan transisi instan sabotase (`s_tamper_trigger_instant` $\rightarrow$ `DISP_PAGE_TAMPER_ALARM`).
  - Penanganan status relai `READY ON` (`DISP_PAGE_RELAY_READY_ON`).
  - Auto-scroll carousel periodik 5 detik (`display_process_tick`).
  - Pengambilan data pengukuran secara thread-safe (`rtos_meter_data_get_snapshot`).
  - Komposisi frame SPLN D3.006 Gambar 4 (`display_render_spln_frame`) dan delegasi transfer grafis ke driver OLED.
  - Supervisi watchdog: `bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT)`.

### Tab 6: `6. Task 5: DefaultMeteringTask` (`app_metering_task_loop`)
* **Fungsi:** Thread CMSIS-RTOS2 utama (`StartDefaultTask`) yang mengoordinasikan eksekusi sinkron Tiga Pilar (E1 Metrologi, E2 DLMS, E3 Aplikasi) pada target perangkat keras STM32U5.
* **Prioritas:** `osPriorityNormal`.
* **Periode Siklus:** 100 ms (`osDelay(100)`).
* **Fitur Utama:**
  - Alokasi stack context ARM TrustZone Secure (`portALLOCATE_SECURE_CONTEXT`).
  - Eksekusi langkah Metrologi ADE9000: `e1_metrology_service_step`.
  - Sinkronisasi data model OBIS DLMS/COSEM: `e2_dlms_service_sync_snapshot`.
  - Eksekusi logika aplikasi meter: `e3_meter_app_step` (FSM relai, tamper elektrikal, telemetri baterai, profiling).

---

## 3. Hasil Verifikasi & Uji Sistem

1. **Integritas Bagan Alir Draw.io:**
   - Total Tab: **6 Tab**
   - Total Node / Vertex: **76 Node**
   - Total Konektor / Edge: **89 Edge**
   - Broken Edge / Orphan Port: **0**
   - Bounding Box Overlaps: **0 (100% Beban Bebas Tabrakan)**
2. **Kompilasi Firmware:**
   - `ninja -C NonSecure/build`: **0 Error, 0 Warning**.
   - Memori ROM: 194.488 B (18.55%), RAM: 74.176 B (14.15%).
3. **Unit Test CTest (Pilar E3):**
   - `ctest --test-dir E3/build`: **12/12 Passed (100%)**.
