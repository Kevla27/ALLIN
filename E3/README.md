# Paket Pekerjaan Pilar E3 &ndash; Application, HMI Display, Tamper Manager & RTOS Orchestrator

Folder ini berisi seluruh kode sumber, pengujian unit, driver pendukung, dan dokumentasi arsitektur untuk **Pilar E3 (Engineer 3 / Application & System Integration Lead)** pada proyek Smart Meter 3-Fasa berbasis mikrokontroler STM32U585/STM32U575 dan standar **SPLN D3.006: 2021**.

> [!IMPORTANT]
> **Dokumentasi Fundamental & Detail Fungsi Tiap File:**
> Untuk penjelasan mendalam mengenai prinsip arsitektur, diagram aliran data, dan fungsi dari **setiap berkas** di dalam Pilar E3, silakan baca dokumen master:
> 📖 [**PANDUAN_ARSITEKTUR_DAN_FILE_E3.md**](file:///g:/PPPP/ALLIN/E3/PANDUAN_ARSITEKTUR_DAN_FILE_E3.md)

---

## 1. Identitas Peran & Tanggung Jawab E3

Pilar E3 memegang posisi sentral sebagai **System Integrator dan Application Lead**:
- **Display & HMI**: Format tampilan layar OLED SSD1306 128x32 yang 100% patuh pada layout **SPLN D3.006-1 Gambar 4** (Mode Otomatis & Manual Scroll).
- **Tamper Event Manager**: Manajemen deteksi 9 vektor sabotase fisik dan kelistrikan sesuai **SPLN D3.006 Tabel 6** (Tutup Meter, Tutup Terminal, Medan Magnet, Arus Balik, Netral Putus, dll).
- **Relay & Power Limiter FSM**: Manajemen kondisi relai pemutus beban/Shunt Trip MCCB (`PC8`), evaluasi asimetri beban fasa ($>30\%$), pembatas daya (*Power Limiter*), status `READY ON`, dan mekanisme *Tripping Release* paska konfirmasi tombol lokal.
- **Profiling & Storage**: Perekam profil beban periodik 15-menitan (*Load Profile*) dan penyimpanan log kejadian ke memori non-volatile (Flash/FRAM).
- **RTOS Orchestration**: Arsitektur thread-safe FreeRTOS (`xMeasMutex`, `xTamperQueue`), manajemen konsumsi data dari E1 (Metrologi) dan distribusi data ke E2 (DLMS/COSEM).

---

## 2. Struktur Direktori E3

```
E3/
├── app/
│   ├── console/              # Driver konsol I/O serial CLI
│   │   ├── inc/console_io.h
│   │   └── src/console_io.c
│   ├── display/              # Logika HMI Carousel SPLN Gambar 4 & Task UI
│   │   ├── inc/display.h, display_task.h
│   │   └── src/display.c, display_task.c
│   ├── fsm/                  # Mesin Keadaan (FSM), Power Relay Manager, RTOS Tasks
│   │   ├── inc/fsm_statechart.h, power_relay_manager.h, rtos_tasks.h
│   │   └── src/fsm_statechart.c, power_relay_manager.c, rtos_tasks.c
│   ├── profiling/            # Load Profile 15-Menit, Adapter DLMS, & NVRAM Storage
│   │   ├── inc/load_profile.h, load_profile_dlms_adapter.h, nvram_storage.h
│   │   └── src/load_profile.c, load_profile_dlms_adapter.c, nvram_storage.c
│   └── tamper/               # Penanganan Sabotase Fisik/Listrik & Antrean Alarm
│       ├── inc/tamper_manager.h
│       └── src/tamper_manager.c
├── drivers/
│   ├── oled/                 # Driver perangkat keras OLED SSD1306 via I2C1
│   │   ├── inc/ssd1306.h, ssd1306_fonts.h, bsp_display_oled.h
│   │   └── src/ssd1306.c, ssd1306_fonts.c, bsp_display_oled.c
│   └── buzzer/               # Driver aktuator Buzzer & LED Alarm PB7
│       ├── inc/bsp_alarm_buzzer.h
│       └── src/bsp_alarm_buzzer.c
├── tests/                    # Pengujian unit & integrasi mandiri domain E3
│   ├── test_e3_app.c
│   ├── test_power_relay.c
│   ├── test_spln_display_tamper.c
│   ├── test_fsm_statechart.c
│   ├── test_load_profile.c
│   ├── test_load_profile_dlms_adapter.c
│   ├── test_nvram_storage.c
│   ├── test_rtos_tasks.c
│   ├── test_stress_soak.c
│   ├── test_e1_to_e2_integration.c
│   └── e3_mock_stubs.c
├── docs/                     # Dokumentasi arsitektur dan integrasi sistem E3
│   ├── PANDUAN_DAN_RANGKUMAN_SISTEM_E3.md
│   ├── INTEGRASI_METROLOGI_E1_KE_DISPLAY_E3.md
│   └── RANGKUMAN_INTEGRASI_E1_E2_E3_DLMS.md
├── CMakeLists.txt            # Sistem build CMake mandiri untuk E3
├── main.c                    # Program demonstrasi mandiri paket E3
└── README.md                 # Berkas petunjuk ini
```

---

## 3. Cara Membangun & Menjalankan Pengujian Mandiri E3

Paket E3 dilengkapi dengan berkas `CMakeLists.txt` mandiri sehingga dapat dikompilasi dan diuji secara independen tanpa bergantung pada seluruh target mikrokontroler:

```bash
# Konfigurasi dan bangun paket E3
cmake -B E3/build -S E3 -G Ninja
ninja -C E3/build

# Jalankan pengujian CTest mandiri (100% PASS)
ctest --test-dir E3/build --output-on-failure

# Jalankan program demonstrasi mandiri
./E3/build/e3_demo.exe
```

---

## 4. Antarmuka Integrasi dengan Pilar Lain

- **Integrasi dengan E1 (Metrologi ADE9000)**:
  - Mengambil data pengukuran tegangan, arus, daya, energi, dan frekuensi via fungsi thread-safe `rtos_meter_data_publish()` & `rtos_meter_data_get_snapshot()`.
  - Mengonsumsi sinyal interupsi perangkat keras `IRQ0`, `IRQ1`, dan `EVENT` (Sag/Swell/OI) untuk diteruskan ke log sabotase/kualitas daya.
- **Integrasi dengan E2 (DLMS/COSEM)**:
  - Menyediakan data Profil Beban via objek DLMS Profile Generic Class 7 (`0.0.99.98.0.255`).
  - Menerima perintah *Remote Disconnect* dan *Remote Reconnect* (`READY ON`) dari objek Disconnect Control DLMS.

