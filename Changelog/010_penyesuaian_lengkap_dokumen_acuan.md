# LOG-010: Audit & Penyesuaian Lengkap Firmware terhadap Dokumen Acuan (`Acuan/`)

**Tanggal:** 27 Oktober 2026  
**Status:** Selesai & Terverifikasi Kompilasi (`ALLIN_S.elf`, `ALLIN_NS.elf`, dan 5/5 CTest Suite)

---

## 1. Latar Belakang / Alasan Perubahan
Berdasarkan hasil audit menyeluruh antara kode program (`ALLIN.ioc`, `Secure/`, `Secure_nsclib/`, `NonSecure/`, `E1/`, `E2/`, `E3/`) dengan seluruh dokumen di dalam folder `g:\PPPP\ALLIN\Acuan`:
- **`PIN.docx` / `pin_doc_all_text.txt`** (Tabel Alokasi 53 Pin LQFP100 STM32U585VIT6)
- **`Desain Sistem.pdf`** (103 Halaman: Arsitektur Level 0–3, Tabel Register ADE9000 Hal. 67–75, Tabel Register STM32U585 Hal. 76–100)
- **`Spesifikasi .pdf`** (52 Halaman: 24 Spesifikasi Kuantitatif B200 SP-01 s.d SP-24)
- **`26-week-plan-revD-no-evalboard.pdf`** (Deliverables Pilar E1, E2, E3 & Gate G0–G5)
- **`SPLN D3.006 2021 CSV.pdf`** & **`Config_MX/KONFIGURASI_MX.md`**

Ditemukan beberapa ketidaklengkapan yang perlu disempurnakan:
1. **TrustZone Secure World & NSC Gateway (`Secure/`, `Secure_nsclib/`)**:
   - Belum tersedianya fungsi Non-Secure Callable (`CMSE_NS_ENTRY`) sesuai `KONFIGURASI_MX.md` (`SECURE_GetActiveEnergyWh`, `SECURE_GetReactiveEnergyVarh`, `SECURE_RecordPulseCF1/CF2`, `SECURE_GetTamperStatusMask`, `SECURE_GetRTCDateTime`, `SECURE_IsLseClockHealthy`).
   - Belum diaktifkannya `ICACHE`, `PWR_BDCR1_MONEN` (Backup Voltage Monitor), `PWR_BDCR2_VBE` (Supercap Trickle Charger), dan `TAMP_ERCFGR0` (Auto-Erase Backup Registers saat Case Open `PC13`) sesuai `Desain Sistem.pdf` Hal. 83, 95, 98–100.
2. **Pilar E1 Metrologi (`E1/`)**:
   - `RegisterMap.h` dan `Mock.c` belum mencakup register kontrol & energi ADE9000 (`PGA_GAIN`, `ACCMODE`, `CFMODE`, `EP_CFG`, `EGY_TIME`, `WR_LOCK`, `ANGL_*`, `xVARHR`, `xVAHR`).
   - `SPI.c` hanya membaca memori mock tanpa melakukan transaksi fisik `HAL_SPI_Transmit`/`HAL_SPI_Receive` pada `hspi1` (`PA4`–`PA7`) dengan auto-fallback ke mock jika chip fisik belum terpasang.
   - `e1_metrology_service.c` belum menjalankan sekuens inisialisasi register ADE9000 (`e1_metrology_bringup_init`), kalibrasi gain (`e1_metrology_calibrate_channel`), serta masih menghardcode `current_n_mamps`, `reactive_power_var`, `apparent_power_va`, `power_factor_ppm`, dan `frequency_mhz`.
3. **Pilar E2 Protokol DLMS/COSEM (`E2/`)**:
   - `e2_dlms_service.c` hanya membalas frame `SNRM` dengan `UA` dan belum menghubungkan frame `DISC`, `RR`, dan `I` (`AARQ`, `GET-Request`, `RLRQ`) ke modul `ami_dlms_association.c`, `ami_dlms_apdu.c`, dan `ami_cosem_model.c`.
   - Belum mendukung kanal komunikasi ganda (`USART3` Port Optik 9600 bps & `USART2` Port Modem 19200 bps) maupun pengiriman paket darurat Last-Gasp (`e2_dlms_send_last_gasp_push` Event Notification `0xC2`) saat `AC_FAIL_DET` (`PC3`) aktif.
4. **Pilar E3 Aplikasi, NVRAM, OLED & Pinout (`E3/`, `NonSecure/`, `ALLIN.ioc`)**:
   - Konfigurasi EXTI/Pull pada `PB6` (`BTN_SCROLL_DN`), `PE8` (`ADE_CF_ZX`), `PE13` (`ADE_CF2`), `PD10`/`PD14` (`AUX_CTRL_IN1/2`), serta kecepatan GPIO `PA4` (`ADE_NSS`) dan `PB12` (`FLASH_SPI2_CS`) belum sesuai tabel register di `Desain Sistem.pdf` Hal. 81–94 & `PIN.docx`.
   - Driver `ssd1306.h` masih mengarah ke `hi2c1` (`PB8`/`PB9`), padahal dokumen acuan menetapkan `hi2c3` (`PC0` SCL, `PC1` SDA).
   - `nvram_storage.c` belum melakukan transaksi `SPI2` (`hspi2` `PB12`–`PB15`) pada target hardware.
   - `e3_meter_app.c` belum memiliki fitur nyala layar sementara 10 detik saat PLN padam (`s_lcd_wakeup_timer_ms`), kedipan `LED_ALM` (`PB7`) 1 Hz saat status relai `READY ON`, serta akumulator energi Time-of-Use (ToU) 8-Zona Tarif (`SP-13`).

---

## 2. Daftar File yang Berubah
1. `Secure_nsclib/secure_nsc.h` & `Secure/Core/Src/secure_nsc.c`
2. `Secure/Core/Src/main.c`
3. `E1/RegisterMap/RegisterMap.h`, `E1/Mock/Mock.h`, `E1/Mock/Mock.c`, `E1/SPI/SPI.c`
4. `E1/Metrology/Metrology.h`, `E1/Metrology/Metrology.c`, `E1/Metrology/e1_metrology_service.h`, `E1/Metrology/e1_metrology_service.c`
5. `E2/Inc/e2_dlms_service.h` & `E2/Src/e2_dlms_service.c`
6. `E3/drivers/oled/inc/ssd1306.h` & `E3/drivers/oled/src/ssd1306.c`
7. `E3/app/profiling/src/nvram_storage.c` & `E3/CMakeLists.txt`
8. `E3/app/e3_meter_app.h` & `E3/app/e3_meter_app.c`
9. `NonSecure/Core/Src/main.c` & `ALLIN.ioc`

---

## 3. Perbandingan Kode Penting (Sebelum vs Sesudah)

### A. Pipeline Penuh HDLC -> AARQ -> GET-Request & Last-Gasp Push (`E2/Src/e2_dlms_service.c`)
**Sebelum:**
```c
if (s_optical_rx_frame.type == AMI_HDLC_FRAME_SNRM)
{
    ami_dlms_hdlc_encode_frame(..., AMI_HDLC_CONTROL_UA, ...);
    HAL_UART_Transmit(s_optical_huart, tx_buf, (uint16_t)tx_len, 100);
}
```
**Sesudah:**
```c
if (ch->rx_frame.type == AMI_HDLC_FRAME_SNRM || ch->rx_frame.type == AMI_HDLC_FRAME_DISC)
{
    ami_dlms_association_reset(&ch->assoc);
    if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame, NULL, 0,
                                  tx_buf, sizeof(tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
        HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
}
else if (ch->rx_frame.type == AMI_HDLC_FRAME_I && ch->rx_frame.payload_length > 0)
{
    uint8_t tag = ch->rx_frame.payload[0];
    if (tag == 0x60U) /* AARQ */
        ami_dlms_process_aarq(&ch->assoc, ...);
    else if (tag == 0xC0U) /* GET-Request */
        ami_dlms_process_get_request(&ch->assoc, &s_global_dlms_snapshot, ...);
}
```

### B. Pembacaan 7-Kanal ADE9000, Sekuens Bring-Up & Pulsa TrustZone (`E1/Metrology/e1_metrology_service.c`)
**Sebelum:**
```c
meas->current_n_mamps = 50; /* Arus netral nominal standby */
meas->reactive_power_var = (uint32_t)(meas->active_power_w * 0.15f);
meas->apparent_power_va  = (uint32_t)(meas->active_power_w * 1.01f);
meas->frequency_mhz      = 50000; /* 50.000 Hz */
```
**Sesudah:**
```c
ADE9000_SPI_TransactionRead(ADE9000_NIRMS,   ADE9000_SPI_DATA_32BIT, &raw_nirms,   NULL);
ADE9000_SPI_TransactionRead(ADE9000_AVAR,    ADE9000_SPI_DATA_32BIT, &raw_avar,    NULL);
ADE9000_SPI_TransactionRead(ADE9000_AVA,     ADE9000_SPI_DATA_32BIT, &raw_ava,     NULL);
ADE9000_SPI_TransactionRead(ADE9000_APF,     ADE9000_SPI_DATA_32BIT, &raw_apf,     NULL);
ADE9000_SPI_TransactionRead(ADE9000_APERIOD, ADE9000_SPI_DATA_32BIT, &raw_aperiod, NULL);
...
meas->current_n_mamps    = (i_n > 0.0f) ? (uint32_t)(i_n * 1000.0f) : 0U;
meas->reactive_power_var = (int32_t)(q_r + q_s + q_t);
meas->apparent_power_va  = (total_s > 0.0f) ? (uint32_t)total_s : 0U;
meas->power_factor_ppm   = Metrology_PowerFactorPermilleFromRaw((int32_t)raw_apf);
meas->frequency_mhz      = (freq_hz > 0.0f) ? (uint16_t)(freq_hz * 1000.0f) : 50000U;
```

---

## 4. Dampak terhadap Hardware & Kompilasi
- **Target Secure (`ALLIN_S.elf`)**:
  - `RAM`: `12048 B / 256 KB (4.60%)`
  - `ROM`: `22504 B / 1016 KB (2.16%)`
  - `ROM_NSC`: `160 B / 8 KB (1.95%)`
- **Target Non-Secure (`ALLIN_NS.elf`)**:
  - `RAM`: `60832 B / 512 KB (11.60%)`
  - `ROM`: `99176 B / 1 MB (9.46%)`
- **Host Unit Test (`E3/build` CTest)**:
  - `5/5 Tests Passed (100%)` dalam `0.45 sec`.

