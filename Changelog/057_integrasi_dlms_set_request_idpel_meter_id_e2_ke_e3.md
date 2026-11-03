# Integrasi DLMS SET-Request untuk IDPEL dan Meter ID (E2 -> Bridge -> E3)

## Ringkasan Perubahan
Mengintegrasikan penanganan paket DLMS **SET-Request-Normal** (`0xC1 0x01`) dari Port Optik / Port Komunikasi (Pilar E2) ke Aplikasi Smart Meter (Pilar E3) melalui lapisan abstraksi `meter_system_bridge`. Integrasi ini memungkinkan pihak PLN atau commissioning tool menuliskan ID Pelanggan (IDPEL) dan Nomor Seri Meter (Meter ID) via DLMS/COSEM secara standar, yang kemudian langsung memperbarui tampilan HMI OLED, tersimpan permanen ke SPI2 Flash NVRAM, dan mencatat event pergantian konfigurasi setting sesuai SPLN D3.006: 2021 Tabel 5 No. 4.

---

## Detail Implementasi Teknis

### 1. Pilar E2 (`E2/Src/ami_dlms_apdu.c`, `E2/Inc/ami_dlms_apdu.h`, `E2/Src/ami_dlms_port.c`, `E2/Src/e2_dlms_service.c`)
- **Implementasi Handler APDU `ami_dlms_process_set_request()`**:
  - Memeriksa validitas PDU SET-Request: Tag `0xC1 0x01`, Invoke ID, Class ID (`0x0001` - Data), Attr ID (`0x02` - Value), dan pencocokan OBIS.
  - Mendukung ekstraksi string format `VisibleString` (`0x0A`) maupun `OctetString` (`0x09`).
  - Menangani OBIS ID Pelanggan `0.0.96.1.2.255` -> memanggil `meter_system_bridge_set_customer_id()`.
  - Menangani OBIS Nomor Seri Meter `0.0.96.1.0.255` -> memanggil `meter_system_bridge_set_meter_id()`.
  - Menghasilkan respon `SetResponseNormal` (`0xC5 0x01 <invoke_id> <result_code>`):
    - `0x00` (Success): Jika data valid dan berhasil disimpan.
    - `0x02` (Type-unmatched / Temporary failure): Jika panjang atau tipe data tidak sesuai.
    - `0x03` (Read-write-denied): Jika mencoba menulis ke objek read-only lain.
- **Penyambungan ke Kanal Komunikasi Port Optik**:
  - `E2/Src/ami_dlms_port.c`: Pada fungsi `dispatch_i_frame()`, tag `0xC1` didelegasikan ke `ami_dlms_process_set_request()`.
  - `E2/Src/e2_dlms_service.c`: Menambahkan parser cabang tag `0xC1` pada I-Frame HDLC channel UART.

### 2. Lapisan Jembatan Sistem (`NonSecure/Core/Inc/meter_system_bridge.h`, `NonSecure/Core/Src/meter_system_bridge.c`)
- Menambahkan fungsi jembatan:
  - `bool meter_system_bridge_set_meter_id(const char *meter_id)` -> meneruskan ke `e3_meter_app_set_meter_id()`.
  - `const char* meter_system_bridge_get_meter_id(void)` -> meneruskan ke `e3_meter_app_get_meter_id()`.

### 3. Pilar E3 (`E3/app/e3_meter_app.c`, `E3/tests/test_spln_display_tamper.c`)
- Memvalidasi string masukan (IDPEL: 11-12 digit numerik; Meter ID: 6-15 karakter alfanumerik).
- Memperbarui konteks display HMI (`display_update_customer_id`, `display_update_meter_id`).
- Melakukan persistensi ke NVRAM SPI2 Flash (`nvram_save_device_config`).
- Mencatat event log perubahan pengaturan:
  - Event IDPEL: `0x960102` (OBIS 0.0.96.1.2.255).
  - Event Meter ID: `0x960100` (OBIS 0.0.96.1.0.255).
- Menambahkan uji dinamis pembaruan string pada CTest `test_spln_display_tamper.c`.

---

## Verifikasi dan Pengujian
- **Target ARM Cortex-M33 Non-Secure (`ALLIN_NS.elf`)**:
  - Kompilasi `cmake --build build/Debug -j8` berhasil **0 Warning, 0 Error**.
  - Flash Usage: `177,552 B` / `1 MB` (`16.93%`).
  - RAM Usage: `68,552 B` / `512 KB` (`13.08%`).
- **CTest Suite E3**:
  - 12/12 Unit & Integration Tests lulus 100% (`0.21s`).
