# Log Perubahan 043: Eliminasi Overflow Stack ISR DLMS, Persistensi Energi Saat Boot, dan Koreksi Latch Tamper Sabotase

## 1. Latar Belakang & Analisis Temuan Audit
Pemeriksaan mendalam baris demi baris pada seluruh pilar firmware (E1 Metrologi, E2 DLMS/COSEM, E3 HMI/Aplikasi, Secure World, Non-Secure World, dan Linker Script) mengungkap sejumlah cacat fatal pada alokasi memori, interupsi hardware, dan persistensi register energi:

1. **Overflow Stack MSP pada Konteks Interrupt ISR UART DLMS (`E2/Src/e2_dlms_service.c` & `E2/Src/ami_dlms_hdlc.c`)**:
   - Fungsi `process_channel_rx(DlmsChannelState *ch)` dipanggil secara langsung dari `HAL_UART_RxCpltCallback(huart)` yang berjalan pada konteks **Main Stack Pointer (MSP)** di dalam ISR.
   - Di dalam fungsi tersebut, dialokasikan array lokal di stack:
     ```c
     uint8_t tx_buf[AMI_HDLC_MAX_FRAME_BYTES];    /* 544 bytes */
     uint8_t apdu_resp[AMI_COSEM_AXDR_ENCODED_MAX]; /* 512 bytes */
     ```
     Total alokasi stack lokal = **1.056 bytes** hanya pada fungsi tersebut.
   - Selain itu, `ami_dlms_hdlc_build_frame()` yang dipanggil di dalamnya mengalokasikan lagi:
     ```c
     uint8_t payload[AMI_HDLC_MAX_INFO_LENGTH];   /* 512 bytes */
     ```
     Total kedalaman stack gabungan mencapai **> 1.600 bytes** di dalam sebuah ISR!
   - Padahal, linker script `NonSecure/STM32U585xx_FLASH_ns.ld` dan `RAM_ns.ld` mengalokasikan `_Min_Stack_Size = 0x400` (1.024 bytes) untuk MSP. Akibatnya, pemrosesan frame DLMS pada interrupt UART seketika meluap (*stack overflow*) ke variabel RAM di sekitarnya, berpotensi memicu HardFault / MemManage Fault acak di lapangan.

2. **Hilangnya Stand Akumulasi Energi Saat Boot / Reboot (`meter_system_bridge.c`)**:
   - Saat boot, `e3_meter_app_init()` memuat stand energi dari NVRAM Checkpoint (`nvram_load_energy_checkpoint()`), mengisi `s_last_active_energy_wh` dan buffer tampilan `s_disp_ctx`.
   - Namun, variabel context pengukuran sistem `s_meas` di `meter_system_bridge.c` tidak pernah dipulihkan dari checkpoint dan tetap bernilai `0`.
   - Pada tick pertama `app_metering_step()`, `s_meas` (bernilai 0 Wh) disalin ke `disp_meas`, seketika menimpa buffer tampilan kembali ke `0`.
   - Selain itu, `s_meas` (0 Wh) disinkronkan ke DLMS model melalui `e2_dlms_service_sync_snapshot()`, membuat pembacaan DLMS OBIS 01.08 reset ke 0.
   - Lebih parah lagi, karena `s_last_active_energy_wh` bernilai stand lama (misal 50.000 Wh) sedangkan `s_meas.active_energy_wh` mulai dari 0, kondisi `meas->active_energy_wh > s_last_active_energy_wh` tidak pernah terpenuhi, sehingga pembagian tarif Time-of-Use (ToU WBP/LWBP) macet total sampai energi baru melampaui stand lama.

3. **Sinkronisasi Stand Ekspor Energi Aktif & Reaktif DLMS Snapshot (`E2/Src/e2_dlms_service.c`)**:
   - `snapshot->active_export_wh` sebelumnya di-hardcode ke `0ULL`, mengabaikan energi yang diterima dari PLTS atap (`meas->active_energy_import_wh`).
   - Fallback energi reaktif ekspor (`snapshot->reactive_export_varh`) juga di-hardcode `0ULL` saat `quads == NULL`.

4. **Kelemahan Latch Permanen `alarm_relay_trigger` pada Sabotase (`E3/app/tamper/src/tamper_manager.c`)**:
   - Saat terdeteksi sabotase yang berhak memutus relai kontaktor (misal tutup terminal dibuka), `ctx->alarm_relay_trigger` disetel ke `true`.
   - Namun, saat sabotase tersebut telah dinetralkan (tutup terminal dipasang kembali) dan `active_tamper_mask` telah bersih, `ctx->alarm_relay_trigger` tidak pernah di-reset ke `false`.
   - Akibatnya, `tamper_should_trip_relay()` terus menerus mengembalikan `true`, mengunci relai secara permanen dan mencegah relai menutup kembali (*reconnect*) meskipun operator/konsumen telah menekan tombol konfirmasi simultan.

5. **Pencegahan Kehilangan Pulsa Sabotase Magnet Sesaat pada TrustZone (`Secure/Core/Src/secure_nsc.c`)**:
   - Pada `SECURE_GetTamperStatusMask()`, bit `(sr & TAMP_SR_TAMP7F)` sebelumnya langsung dibersihkan oleh `TAMP->SCR = TAMP_SCR_CTAMP7F` jika pin fisik `PE4` sedang bernilai LOW pada saat pembacaan, tanpa menyetel `mask |= (1U << 2)`.
   - Jika magnet ditempelkan sesaat (< 100 ms) saat MCU dalam Stop 2 atau di sela polling, event sabotase magnetik hilang total.

6. **Pencegahan Auto-Reclose Relai Saat Padam Total / Blackout (`E3/app/fsm/src/power_relay_manager.c`)**:
   - Pada pemulihan *under-voltage* (< 110 V), kondisi `v_r_dv == 0 || v_r_dv > 1150` menyebabkan kondisi padam total (semua fasa = 0 V) dianggap pulih secara keliru.

---

## 2. Rincian Perbaikan

### A. Eliminasi Alokasi Stack ISR & Perluasan MSP Linker Script
- **[`E2/Src/e2_dlms_service.c`](file:///g:/PPPP/ALLIN/E2/Src/e2_dlms_service.c)**:
  - Memindahkan array `tx_buf[AMI_HDLC_MAX_FRAME_BYTES]` dan `apdu_resp[AMI_COSEM_AXDR_ENCODED_MAX]` ke dalam struct statis `DlmsChannelState` (`s_optical_channel` dan `s_modem_channel` di segmen `.bss`).
  - `process_channel_rx()` kini menggunakan pointer ke buffer kanal tersebut (`ch->tx_buf` dan `ch->apdu_resp`), mereduksi penggunaan stack fungsi dari 1.056 byte menjadi ~16 byte.
  - Memetakan `snapshot->active_export_wh = meas->active_energy_import_wh` dan menyelaraskan ekspor reaktif.
- **[`E2/Src/ami_dlms_hdlc.c`](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_hdlc.c)**:
  - Mengubah buffer lokal `payload[AMI_HDLC_MAX_INFO_LENGTH]` pada `ami_dlms_hdlc_build_frame()` menjadi `static uint8_t s_build_payload[]`, menghemat 512 byte stack tambahan saat ISR berjalan.
- **[`NonSecure/STM32U585xx_FLASH_ns.ld`](file:///g:/PPPP/ALLIN/NonSecure/STM32U585xx_FLASH_ns.ld) & [`NonSecure/STM32U585xx_RAM_ns.ld`](file:///g:/PPPP/ALLIN/NonSecure/STM32U585xx_RAM_ns.ld)**:
  - Menaikkan `_Min_Stack_Size` dari `0x400` (1.024 byte) ke `0x800` (2.048 byte), memberikan ruang aman 2x lipat untuk nesting interrupt HAL dan FreeRTOS.

### B. Pemulihan Persistensi Energi Saat Boot
- **[`NonSecure/Core/Src/meter_system_bridge.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/meter_system_bridge.c)**:
  - Menambahkan pemulihan stand energi dari checkpoint Flash NVRAM langsung ke `s_meas` di dalam `meter_system_bringup()`:
    ```c
    nvram_energy_checkpoint_t cp = {0};
    if (nvram_load_energy_checkpoint(&cp))
    {
        s_meas.active_energy_wh        = cp.active_energy_import_wh;
        s_meas.active_energy_import_wh = cp.active_energy_export_wh;
        s_meas.reactive_energy_varh    = cp.reactive_energy_varh;
    }
    ```

### C. Koreksi Latch Sabotase & Pemulihan Reconnect
- **[`E3/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/ALLIN/E3/app/tamper/src/tamper_manager.c)**:
  - Menyelaraskan status `ctx->alarm_relay_trigger` dan `tamper_should_trip_relay()` terhadap kondisi aktif aktual `((ctx->active_tamper_mask & TAMPER_MASK_RELAY_TRIP) != 0)`.
  - Menambahkan kasus uji regresi di `E3/tests/test_e3_app.c` yang memverifikasi bahwa saat sabotase dinetralkan, relai dibebaskan dari pemblokiran sabotase dan dapat melakukan *reconnect*.

### D. Keamanan Hardware Tamper di Secure World
- **[`Secure/Core/Src/secure_nsc.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/secure_nsc.c)**:
  - Menjamin bahwa jika latch hardware `TAMP_SR_TAMP7F` terpicu, `mask |= (1U << 2)` selalu dilaporkan ke Non-Secure, dan latch baru dibersihkan setelah dilaporkan jika medan magnet fisik telah menjauh.
  - Memperbarui kalkulasi `century_base` agar generik melintasi abad selanjutnya (`century_base += 100U;` dan `(y / 100U) * 100U`).

### E. Penjagaan Proteksi Tegangan
- **[`E3/app/fsm/src/power_relay_manager.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/power_relay_manager.c)**:
  - Menambahkan pengecekan `has_active_phase` pada pemulihan under-voltage untuk mencegah auto-reclose saat kondisi padam total (blackout 0 V pada seluruh fasa).

---

## 3. Hasil Verifikasi & Uji Kompilasi
- **CTest Suite (`E3/build`)**: 12/12 test suites PASSED (100%) dalam 0.80 detik.
- **ARM GCC Bare-metal Toolchain (`build/Debug`)**:
  - `ALLIN_S.elf`: ROM 24.072 B (2,31%), RAM 12.056 B (4,60%). 0 Errors, 0 Warnings.
  - `ALLIN_NS.elf`: ROM 173.704 B (16,57%), RAM 68.992 B (13,16%). 0 Errors, 0 Warnings.
