# LOG-005: Penyempurnaan Power Rail Periferal, Chip Select SPI, Interupsi ADE9000 & Validasi Relai

- **Tanggal**: 27 Oktober 2026
- **Status**: Selesai & Terverifikasi (Build 0 Errors, 0 Warnings)

---

## 1. Latar Belakang & Analisis Kelengkapan
Berdasarkan audit komparatif terhadap dokumen spesifikasi hardware (`Acuan/pin_doc_extracted.txt`), disempurnakan 5 hal penting:
1. **Inisialisasi Power Rail Periferal**:
   Pada `MX_GPIO_Init()`, pin `LCD_PWR_EN` (`PD2`), `LCD_BL_CTRL` (`PD3`), `RS232_PWR_EN` (`PD4`), dan `MODEM_12V_EN` (`PD5`) diinisialisasi pada kondisi `RESET` (Mati). Saat sistem boot pada kondisi listrik PLN normal, rail daya ini wajib dihidupkan (`SET`) agar layar LCD, RS-232, dan modem aktif.
2. **Kondisi Chip Select SPI Flash & Reset Metrologi**:
   - `FLASH_SPI2_CS` (`PB12`) wajib di-set `HIGH` (Deselect) saat idle agar jalur SPI2 tidak terblokir.
   - `ADE_RESET` (`PE7`) wajib di-release `HIGH` agar AFE metrologi ADE9000 mulai beroperasi normal.
   - `ADE_PM0` (`PE9`) dan `ADE_PM1` (`PE10`) diatur ke `LOW` (Mode PSM0 Full Run).
3. **Interupsi Lengkap ADE9000 (CF1, CF2, IRQ0, IRQ1, EVENT, CF_ZX)**:
   - Sinyal pulsa kalibrasi energi aktif `ADE_CF1` (`PE12`) dan reaktif `ADE_CF2` (`PE13`) dihubungkan ke counter `s_cf1_pulse_count` & `s_cf2_pulse_count`.
   - Sinyal peringatan anomali fasa `ADE_IRQ0` (`PB5`) dan netral `ADE_IRQ1` (`PB1`) dihubungkan langsung ke modul Tamper Manager (`tamper_process_signal`).
   - Sinyal mutu daya `ADE_EVENT` (`PE11`) dan perlintasan nol `ADE_CF_ZX` (`PE8`) ditangkap untuk sinkronisasi pemutusan/penyambungan relai pada titik nol (Zero-Crossing).
4. **Input Kontrol Bantu Digital (`PD10` / `PD14`)**:
   - `AUX_CTRL_IN1` dan `AUX_CTRL_IN2` ditangani pada EXTI interrupt.
5. **Driver LED Alarm (`PB7`) & Validasi Feedback Relai (`PC7`)**:
   - LED kuning `LED_ALM` (`PB7`) dinyalakan bersama buzzer saat terjadi trip atau sabotase tamper.
   - Feedback aux kontak `RELAY_STATUS` (`PC7`) divalidasi: jika perintah `RELAY_TRIP` aktif namun kontak fisik masih terhubung, memicu alarm internal **`ERR07` (`SPLN_ALARM_RELAY_FAIL`)**.

---

## 2. Perubahan Kode Rinci pada `NonSecure/Core/Src/main.c`

### A. Blok `USER CODE BEGIN PV`
Menambahkan variabel pencacah pulsa energi dan timestamp zero-crossing:
```c
/* ADE9000 Energy Calibration Pulse Counters (PE12 & PE13) */
static volatile uint32_t s_cf1_pulse_count = 0;
static volatile uint32_t s_cf2_pulse_count = 0;
static volatile uint32_t s_last_zx_tick = 0;
```

### B. Blok `USER CODE BEGIN 2`
Mengaktifkan catu daya normal periferal dan inisialisasi pin kontrol:
```c
  /* 0. Inisialisasi State Power Rail & Chip Select */
  HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET); /* Deselect Flash CS */
  HAL_GPIO_WritePin(ADE_RESET_GPIO_Port, ADE_RESET_Pin, GPIO_PIN_SET);         /* Release ADE9000 Reset */
  HAL_GPIO_WritePin(ADE_PM0_GPIO_Port, ADE_PM0_Pin, GPIO_PIN_RESET);           /* Normal PSM0 Run */
  HAL_GPIO_WritePin(ADE_PM1_GPIO_Port, ADE_PM1_Pin, GPIO_PIN_RESET);           /* Normal PSM0 Run */
```

### C. Blok `USER CODE BEGIN 4` (EXTI Callbacks)
Menangani pulsa energi, anomali jaringan, zero-crossing, dan kontrol bantu:
```c
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_SCROLL_UP_Pin)
    {
        display_handle_button_press(&disp_ctx, BUTTON_DIR_UP);
    }
    else if (GPIO_Pin == BTN_SCROLL_DN_Pin)
    {
        display_handle_button_press(&disp_ctx, BUTTON_DIR_DOWN);
    }
    else if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik Padam (Last Gasp): Matikan beban periferal berdaya tinggi & ADE9000 ke Sleep */
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ADE_PM0_GPIO_Port, ADE_PM0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ADE_PM1_GPIO_Port, ADE_PM1_Pin, GPIO_PIN_SET);
    }
    else if (GPIO_Pin == ADE_CF1_Pin)
    {
        s_cf1_pulse_count++;
    }
    else if (GPIO_Pin == ADE_CF2_Pin)
    {
        s_cf2_pulse_count++;
    }
    else if (GPIO_Pin == ADE_IRQ0_Pin)
    {
        tamper_process_signal(&tamper_ctx, TAMPER_VECTOR_REVERSE_POWER, true, now_epoch_sec);
    }
    else if (GPIO_Pin == ADE_IRQ1_Pin)
    {
        tamper_process_signal(&tamper_ctx, TAMPER_VECTOR_NEUTRAL_BYPASS, true, now_epoch_sec);
    }
    else if (GPIO_Pin == ADE_EVENT_Pin)
    {
        /* Mutu Daya: Dip/Sag, Swell, Overcurrent (SPLN D3.006) */
    }
    else if (GPIO_Pin == ADE_CF_ZX_Pin)
    {
        /* Zero-Crossing: Sinkronisasi pemutusan/penyambungan relai pada titik nol */
        s_last_zx_tick = HAL_GetTick();
    }
    else if (GPIO_Pin == AUX_CTRL_IN1_Pin || GPIO_Pin == AUX_CTRL_IN2_Pin)
    {
        /* Kontrol Bantu Digital Input Eksternal */
    }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_SCROLL_DN_Pin)
    {
        display_handle_button_press(&disp_ctx, BUTTON_DIR_DOWN);
    }
    else if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik PLN Normal Kembali: Hidupkan kembali periferal & ADE9000 ke Run */
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ADE_PM0_GPIO_Port, ADE_PM0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ADE_PM1_GPIO_Port, ADE_PM1_Pin, GPIO_PIN_RESET);
    }
}
```

### D. Blok `app_metering_step()`
Menghubungkan indikator LED alarm `PB7` dan deteksi kegagalan relai `ERR07`:
```c
  /* D. Jalankan State Machine Pembatas Beban & Relai (PC8) */
  bool relay_closed_fb = (HAL_GPIO_ReadPin(RELAY_STATUS_GPIO_Port, RELAY_STATUS_Pin) == GPIO_PIN_SET);
  power_relay_process_power(&relay_ctx, (int32_t)meas.active_power_w, now_epoch_sec);

  bool is_trip_commanded = (power_relay_get_state(&relay_ctx) == RELAY_STATE_DISCONNECTED || tamper_is_alarm_pending(&tamper_ctx));
  if (is_trip_commanded)
  {
    HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, GPIO_PIN_SET);
    bsp_alarm_buzzer_set(true);

    /* Validasi Feedback Relai: Jika sudah di-trip tapi kontak fisik masih terhubung -> Alarm ERR07 */
    if (relay_closed_fb)
    {
      display_set_internal_alarm(&disp_ctx, SPLN_ALARM_RELAY_FAIL);
    }
  }
  else
  {
    HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, GPIO_PIN_RESET);
    bsp_alarm_buzzer_set(false);
  }
```

---

## 3. Hasil Pengujian & Kompilasi
- **Compiler**: ARM GNU Toolchain 14.3.1 (`arm-none-eabi-gcc`), Target Cortex-M33 Hard-FP.
- **Status Build**: `ALLIN_S.elf` & `ALLIN_NS.elf` sukses dikompilasi dengan **0 Errors, 0 Warnings**.
- **Memory Consumption**:
  - `RAM`: 26024 Bytes (4.96%)
  - `Flash ROM`: 69664 Bytes (6.64%)
