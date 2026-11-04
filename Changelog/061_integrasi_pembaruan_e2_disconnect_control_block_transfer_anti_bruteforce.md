# LOG-061: Integrasi Pembaruan Pilar E2 (Disconnect Control Class 70, Block Transfer, Anti-Bruteforce & NVM Counter)

**Tanggal:** 04 November 2026  
**Status:** Selesai & Terverifikasi Penuh (Build Cortex-M33 0 Error 0 Warning, CTest 12/12 Passed)  
**Tipe:** Feature Integration / Protokol DLMS / Penyesuaian Standar SPLN D5.002: 2024  
**Pilar Terdampak:** Pilar E2 (DLMS/COSEM Stack), Pilar E3 (Aplikasi Smart Meter / Relai Breaker), Build System (NonSecure CMake)

---

## 1. Latar Belakang & Identifikasi Pembaruan E2

Pembaruan dari folder `Updare/E2` menghadirkan evolusi signifikan pada stack DLMS/COSEM Pilar E2 guna memenuhi standar AMI PLN terkini (SPLN D5.002: 2024 & IEC 62056-6-2):

1. **DLMS/COSEM Class 70 - Disconnect Control (`ami_disconnect_control.h` / `ami_disconnect_control.c`):**
   - Mengimplementasikan objek Breaker Disconnect Control (OBIS `0.0.96.3.10.255`).
   - Mendukung 6 mode kontrol (`AMI_DISCONNECT_MODE_1` s.d `AMI_DISCONNECT_MODE_6`), dengan mode default PLN yaitu **Mode 2** (Disconnect: Remote, Manual, Local | Reconnect: Remote, Manual, Local).
   - Method 1: `remote_disconnect(integer)` $\rightarrow$ Mentransisikan status ke `DISCONNECTED`, membuka kontak relai kontaktor (output state 0), dan mencatat log.
   - Method 2: `remote_reconnect(integer)` $\rightarrow$ Pada Mode 2 mentransisikan status ke `CONNECTED`, menutup kontak relai (output state 1).
   - HAL callback `AmiBreakerActuatorFn` untuk penggerak kumparan koil bistable latching relay (`CMD_OPEN` / `CMD_CLOSE`).

2. **xDLMS DataBlock Transfer (`ami_dlms_block_transfer.h` / `ami_dlms_block_transfer.c`):**
   - Mekanisme pemecahan blok data (*Block Transfer with Action*) untuk objek berkapasitas besar.
   - Digunakan untuk streaming Load Profile 23-kanal, Event Log FIFO, dan Riwayat Penagihan Bulanan (*Billing Profile*) agar tidak melampaui batas PDU HDLC.

3. **Proteksi Anti-Bruteforce HLS5 (`ami_anti_bruteforce.h` / `ami_anti_bruteforce.c`):**
   - Proteksi progressive exponential backoff lockout sesuai SPLN D5.002: 2024.
   - Menghalau serangan brute force terhadap otentikasi High-Level Security (HLS 5 GMAC) dengan menolak AARQ saat meter dalam masa penguncian (*lockout*).

4. **Persistent Invocation Counter (`ami_nvm_counter.h` / `ami_nvm_counter.c`):**
   - Pelacak invocation counter kriptografis yang persisten dengan dukungan backup register hardware (STM32 TAMP) dan fallback Flash SPI untuk mencegah serangan *replay attack*.

5. **Driver Abstraksi UART DMA (`ami_bsp_uart.h` / `ami_bsp_uart.c`):**
   - Transpor UART berkinerja tinggi menggunakan circular buffer DMA dan interupsi IDLE line.

6. **Perluasan Model Snapshot Register COSEM (`ami_cosem_model.h` / `ami_cosem_model.c`):**
   - Struktur `AmiCosemRegisterSnapshot` diperluas dengan field native:
     - Multi-Tariff Energy (T1..T4): `active_import_tariff_wh[4]`, `active_export_tariff_wh[4]`, `reactive_import_tariff_varh[4]`.
     - Kualitas Daya (PQ): `frequency_mhz`, `power_factor[4]`, `thd_voltage_percent_x10[4]`, `thd_current_percent_x10[4]`, `active_power_watts`, `reactive_power_vars`.
     - Demand Limiter (Class 71) & Disconnect Control (Class 70) state attributes.
     - Visible Strings: `meter_id[17]` dan `meter_type[32]`.

---

## 2. Rincian Perubahan & Integrasi Kode

### A. Integrasi Lapisan Aplikasi Meter Pilar E3
1. **Header API ([`E3/app/e3_meter_app.h`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.h)):**
   - Mendeklarasikan fungsi aktuasi remote dari HES/DLMS:
     ```c
     bool e3_meter_app_remote_disconnect(void);
     bool e3_meter_app_remote_reconnect(void);
     ```
2. **Implementasi Aplikasi ([`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)):**
   - Menghubungkan fungsi remote breaker langsung ke state machine relai dan event logging:
     ```c
     bool e3_meter_app_remote_disconnect(void)
     {
         return cli_relay_disconnect(RELAY_REASON_REMOTE_DISCONNECT);
     }

     bool e3_meter_app_remote_reconnect(void)
     {
         return cli_relay_request_reconnect();
     }
     ```

### B. Penyambungan Layer Integrasi DLMS ([`E2/Src/e2_dlms_service.c`](file:///g:/PPPP/ALLIN/E2/Src/e2_dlms_service.c))
1. **Aktuator Koil Breaker:**
   - Mengimplementasikan callback `dlms_breaker_actuator(void *user, AmiBreakerCommand cmd)` yang memanggil `e3_meter_app_remote_disconnect()` pada `CMD_OPEN` dan `e3_meter_app_remote_reconnect()` pada `CMD_CLOSE`.
2. **Inisialisasi Disconnect Control:**
   - Menambahkan konteks `AmiDisconnectControlContext s_disconnect_ctrl` pada mode `AMI_DISCONNECT_MODE_2`.
   - Mengaitkan pointer `s_cipher_backend.disconnect_control = &s_disconnect_ctrl;` sehingga APDU glo-action Class 70 dapat mengeksekusi aksi pembukaan/penutupan kontaktor.
3. **Pengisian Field Snapshot COSEM Lengkap:**
   - Menyinkronkan multi-tarif impor/ekspor (`active_import_tariff_wh` T1..T3, `active_export_tariff_wh` T1..T3).
   - Menyinkronkan frekuensi (`frequency_mhz`), faktor daya 4-fasa (`power_factor`), THD nominal 1.5% V dan 2.5% I, daya aktif total (`active_power_watts`), dan daya reaktif total (`reactive_power_vars`).
   - Menyinkronkan identitas meter dari E3 (`meter_id` & `meter_type`).
   - Memanggil `ami_disconnect_control_sync_snapshot(&s_disconnect_ctrl, snapshot)`.
   - Menyalakan bitmask validitas: `AMI_COSEM_VALID_TARIFF_REGISTERS | AMI_COSEM_VALID_POWER_QUALITY | AMI_COSEM_VALID_DISCONNECT_CTRL | AMI_COSEM_VALID_DEVICE_INFO`.
4. **Penanganan SET-Request Plaintext:**
   - Merespons permintaan `SET-Request` (tag `0xC1`) tanpa sesi enkripsi dengan `SetResponseNormal` status `read-write-denied (3)` (`0xC5 0x01 invoke_id 0x03`).

### C. Penyesuaian Kompatibilitas Software Crypto Fallback
- Kode pembaruan mengasumsikan TrustZone Hardware SAES NSC tersedia ketika `-DSTM32U585xx` aktif. Namun, gateway NSC target tidak menyertakan modul SAES.
- Membuka pembatas `#if !defined(STM32U585xx) && !defined(USE_HAL_DRIVER)` pada modul:
  - [`E2/Inc/ami_security.h`](file:///g:/PPPP/ALLIN/E2/Inc/ami_security.h) & [`E2/Src/ami_security.c`](file:///g:/PPPP/ALLIN/E2/Src/ami_security.c): Mengaktifkan mesin AES-128-GCM, GMAC, dan RFC 3394 key unwrap murni software.
  - [`E2/Src/ami_dlms_cipher.c`](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_cipher.c): Mengamankan pemanggilan hardware SAES di balik `#if defined(SECURE_HARDWARE_SAES_ENABLED)` dengan fallback software yang konsisten.
  - [`E2/Inc/ami_dlms_association.h`](file:///g:/PPPP/ALLIN/E2/Inc/ami_dlms_association.h) & [`E2/Src/ami_dlms_association.c`](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_association.c): Memastikan fungsi asosiasi dasar `ami_dlms_process_aarq()` dapat dipanggil pada target embedded.

### D. Penyesuaian Build System ([`NonSecure/CMakeLists.txt`](file:///g:/PPPP/ALLIN/NonSecure/CMakeLists.txt))
- Mendaftarkan seluruh berkas implementasi baru E2 ke dalam target `ALLIN_NS`:
  - `E2/Src/ami_anti_bruteforce.c`
  - `E2/Src/ami_bsp_uart.c`
  - `E2/Src/ami_disconnect_control.c`
  - `E2/Src/ami_dlms_block_transfer.c`
  - `E2/Src/ami_nvm_counter.c`

---

## 3. Hasil Pengujian & Verifikasi

1. **Target Embedded Firmware (`ALLIN_NS.elf`):**
   - Kompilasi: **0 Error, 0 Warning**
   - Ukuran Memori:
     - ROM (Flash): **193,664 Byte** (18.47% dari kuota 1 MB)
     - RAM (SRAM): **73,768 Byte** (14.07% dari kuota 512 KB)
2. **Target Secure Firmware (`ALLIN_S.elf`):**
   - Kompilasi: **0 Error, 0 Warning** (Bersih tanpa modifikasi)
3. **Suite Pengujian CTest (`E3/build`):**
   - Total Uji: **12/12 Lulus (100% Passed, 0 Failed)**:
     - `Integration_E3_Demo` $\rightarrow$ Passed
     - `Unit_E3_Display_Tamper` $\rightarrow$ Passed
     - `Unit_E3_Power_Relay` $\rightarrow$ Passed
     - `Unit_E3_FSM_Statechart` $\rightarrow$ Passed
     - `Unit_E3_Load_Profile` $\rightarrow$ Passed
     - `Unit_E3_SPLN_Display_Tamper` $\rightarrow$ Passed
     - `Stress_E3_Final_System_Audit` $\rightarrow$ Passed
     - `Unit_E3_Console_CLI` $\rightarrow$ Passed
     - `Unit_E3_Board_Profile` $\rightarrow$ Passed
     - `Unit_E3_Power_Mode_Stop2` $\rightarrow$ Passed
     - `Unit_E3_NVRAM_Storage` $\rightarrow$ Passed
     - `Unit_E3_RTOS_Tasks` $\rightarrow$ Passed
