# Changelog 055: Sentralisasi Konfigurasi IDPEL dan Meter ID

**Tanggal:** 3 November 2026  
**Status:** Selesai & Terverifikasi  
**Terkait:** Penyederhanaan Konfigurasi Parameter Identitas Meter (IDPEL & Meter ID)

---

## 1. Latar Belakang & Tujuan
Sebelumnya, nilai ID Pelanggan (IDPEL) dan Nomor Seri Meter (Meter ID) tersebar di beberapa file kode sumber (misalnya `meter_system_bridge.c`, `e3_meter_app.c`, dan `display.c`), dengan nilai default fallback yang tidak seragam. Hal ini menyulitkan saat ingin mengganti nomor seri atau ID pelanggan untuk deployment meter baru.

Tujuan perubahan ini adalah menyatukan konfigurasi IDPEL dan Meter ID ke dalam **satu pusat pengaturan tunggal (Single Source of Truth)**.

---

## 2. Perubahan yang Dilakukan

### A. Definisi Sentral di `E3/app/board_profile_config.h`
Menambahkan konstanta konfigurasi di bagian awal header profil papan:
```c
/* ====================================================================
 * 0. IDENTITAS PELANGGAN & NOMOR SERI METER (IDPEL & METER ID)
 *    Pusat Pengaturan Tunggal (Single Source of Truth)
 * ==================================================================== */
#ifndef METER_CONFIG_CUSTOMER_ID
#define METER_CONFIG_CUSTOMER_ID        "54321678901"   /**< ID Pelanggan PLN (IDPEL, 11-12 digit) */
#endif

#ifndef METER_CONFIG_METER_ID
#define METER_CONFIG_METER_ID           "530000000001"  /**< ID Meter / Nomor Seri Meter (Meter ID, 12 digit) */
#endif
```

### B. Pembaruan Konsumen Konfigurasi
1. **`NonSecure/Core/Src/meter_system_bridge.c`**:
   - Pemanggilan `e3_meter_app_init("54321678901", hadc_telemetry)` diganti menjadi:
     ```c
     e3_meter_app_init(METER_CONFIG_CUSTOMER_ID, hadc_telemetry);
     ```
2. **`E3/app/e3_meter_app.c`**:
   - Pemanggilan `display_init(...)` diupdate menggunakan `METER_CONFIG_CUSTOMER_ID` dan `METER_CONFIG_METER_ID`.
3. **`E3/app/display/src/display.c`**:
   - Meng-include `"board_profile_config.h"`.
   - Mengganti fallback hardcoded `"12345678901"` dan `"MTR1234567890"` menjadi `METER_CONFIG_CUSTOMER_ID` dan `METER_CONFIG_METER_ID`.

---

## 3. Hasil Pengujian & Verifikasi
- **Target ARM Cortex-M33 (STM32U585):**
  - Kompilasi `cmake --build build/Debug -j8` sukses tanpa warning/error.
  - Alokasi Memori: RAM: `13.07%`, ROM/Flash: `16.78%`.
- **Unit & Integration Tests (CTest):**
  - 12/12 test suites Lulus 100% (0 Failed).
