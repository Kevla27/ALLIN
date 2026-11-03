# Changelog 056: Implementasi Penerima String IDPEL dan Meter ID di Pilar E3 & Persistensi NVRAM

**Tanggal:** 3 November 2026  
**Status:** Selesai & Terverifikasi  
**Terkait:** Dukungan Pemrograman ID Pelanggan (IDPEL) dan Nomor Seri Meter via Port Optik (Pilar E2 -> E3)

---

## 1. Latar Belakang & Tujuan
Saat teknisi PLN memprogram ID Pelanggan (OBIS `0.0.96.1.2.255`) atau Nomor Seri Meter (OBIS `0.0.96.1.0.255`) melalui Port Optik, Pilar E2 (DLMS/COSEM) memerlukan API penerima (*receiver API*) di Pilar E3 untuk:
1. Memvalidasi string (panjang 11–12 digit, karakter numerik).
2. Memperbarui konteks tampilan layar LCD/OLED (*real-time*).
3. Menyimpan nilai baru secara permanen ke memori non-volatile (SPI2 Flash / NVRAM) dengan perlindungan *magic stamp*.
4. Merekam kejadian perubahan parameter konfigurasi ke **Event Log FIFO** (`SPLN_LOG_EVENT_NON_TOU_CHANGE`, SPLN D3.006:2021 Tabel 5 No. 4).

---

## 2. Rincian Implementasi

### A. Penyimpanan Non-Volatile (`E3/app/profiling/inc/nvram_storage.h` & `nvram_storage.c`)
- Ditambahkan struktur `nvram_device_config_t` dengan stempel magic `0x43464944` (`"CFID"`):
  ```c
  typedef struct {
      uint32_t magic;                    /**< 0x43464944 ("CFID") */
      char customer_id[16];              /**< ID Pelanggan PLN (IDPEL, 11-12 digit) */
      char meter_id[16];                 /**< Nomor Seri Meter (Meter ID, 12 digit) */
  } nvram_device_config_t;
  ```
- Implementasi fungsi simpan dan muat:
  - `nvram_save_device_config(const nvram_device_config_t *config)`
  - `nvram_load_device_config(nvram_device_config_t *out_config)`
  - Alokasi sektor konfigurasi perangkat pada offset SPI2 Flash fisik `0x000800U` (2 KB).

### B. Pembaruan Layar HMI (`E3/app/display/inc/display.h` & `display.c`)
- Implementasi fungsi:
  - `display_update_customer_id(display_context_t *ctx, const char *customer_id)`
  - `display_update_meter_id(display_context_t *ctx, const char *meter_id)`
- Memungkinkan teks IDPEL dan Meter ID langsung ter-refresh seketika di Carousel No. 1 tanpa memerlukan reset mikroprosesor.

### C. API Aplikasi Meter (`E3/app/e3_meter_app.h` & `e3_meter_app.c`)
- Implementasi fungsi publik:
  - `bool e3_meter_app_set_customer_id(const char *customer_id)`
  - `const char* e3_meter_app_get_customer_id(void)`
  - `bool e3_meter_app_set_meter_id(const char *meter_id)`
  - `const char* e3_meter_app_get_meter_id(void)`
- Pada saat booting (`e3_meter_app_init`), firmware memuat `nvram_load_device_config(&dev_cfg)`: jika ada konfigurasi yang pernah diprogram oleh PLN, nilai tersebut langsung digunakan menggantikan nilai default bawaan pabrik.

### D. Jembatan Sistem (`NonSecure/Core/Inc/meter_system_bridge.h` & `meter_system_bridge.c`)
- Menambahkan fungsi jembatan:
  - `bool meter_system_bridge_set_customer_id(const char *customer_id)`
  - `const char* meter_system_bridge_get_customer_id(void)`

---

## 3. Hasil Pengujian & Verifikasi
- **Target ARM Cortex-M33 (STM32U585):**
  - Kompilasi `cmake --build build/Debug -j8` sukses:
    - RAM: `68,552 B` / `512 KB` (`13.08%`)
    - ROM/Flash: `176,368 B` / `1 MB` (`16.82%`)
- **Unit & Integration Tests (CTest):**
  - 12/12 test suites Lulus 100% (0 Failed), termasuk unit test baru pada `test_nvram_storage.c` (`test_nvram_device_config`).
