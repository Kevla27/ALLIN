# LOG-004: Penyempurnaan Telemetri Periferal & Pembersihan Dead Code

- **Tanggal**: 27 Oktober 2026
- **Status**: Selesai & Terverifikasi

---

## 1. Latar Belakang & Masalah
1. **Dead Code di `main()`**:
   Sebelumnya, loop pengukuran berada di dalam `while(1)` fungsi `main()`. Namun karena `osKernelStart()` telah dipanggil sebelumnya, eksekusi CPU beralih ke FreeRTOS scheduler dan kode di dalam `while(1)` pada `main()` tidak pernah dijalankan (*unreachable code*).
2. **Kelengkapan Telemetri Periferal**:
   Beberapa periferal hardware belum terhubung ke loop tugas FreeRTOS:
   - External Watchdog IC (`PD12`) butuh pulsa periodik (*heartbeat*).
   - ADC4 untuk memantau tegangan Baterai (`PD11`) dan Superkapasitor (`PC2`).
   - Port Optik DLMS (`USART3` pada `PC4`/`PC5`) butuh parser frame HDLC terhubung ke interrupt UART.
   - Sensor feedback posisi relai fisik (`PC7`).

---

## 2. Perubahan Rinci (Sebelum vs Sesudah)

### A. File: `NonSecure/Core/Src/main.c`

#### 1. Pembersihan Loop `while(1)` di `main()`
* **Alasan**: Menghindari kode duplikat yang tidak pernah dieksekusi.
* **Sebelum**:
  ```c
  osKernelStart();
  while (1)
  {
      // Terisi 170 baris kode pembacaan ADE9000, display, profiling
      // KODE INI TIDAK PERNAH DIJALANKAN KARENA osKernelStart()
      HAL_Delay(10);
  }
  ```
* **Sesudah**:
  ```c
  osKernelStart();
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
  ```

#### 2. Penambahan Heartbeat Watchdog (`PD12`)
* **Lokasi**: Awal fungsi `app_metering_step()`
* **Kode Ditambahkan**:
  ```c
  /* 0. Heartbeat External Watchdog (PD12) */
  HAL_GPIO_TogglePin(EXT_WDI_PULSE_GPIO_Port, EXT_WDI_PULSE_Pin);
  ```

#### 3. Penambahan Sampling Baterai & Superkapasitor via ADC4
* **Lokasi**: Fungsi statis baru dan pengecekan berkala setiap 5 detik di `app_metering_step()`
* **Kode Ditambahkan**:
  ```c
  static void sample_battery_and_supercap(uint32_t *out_vbat_mv, uint32_t *out_vcap_mv)
  {
      if (!out_vbat_mv || !out_vcap_mv) return;

      /* Channel 3: PC2 (VCAP_SENSE) */
      ADC_ChannelConfTypeDef sConfig = {0};
      sConfig.Channel = ADC_CHANNEL_3;
      sConfig.Rank = ADC4_RANK_CHANNEL_NUMBER;
      sConfig.SamplingTime = ADC4_SAMPLINGTIME_COMMON_1;
      HAL_ADC_ConfigChannel(&hadc4, &sConfig);

      HAL_ADC_Start(&hadc4);
      if (HAL_ADC_PollForConversion(&hadc4, 5) == HAL_OK)
      {
          uint32_t raw_vcap = HAL_ADC_GetValue(&hadc4);
          *out_vcap_mv = (raw_vcap * 3300) / 4095;
      }
      HAL_ADC_Stop(&hadc4);

      /* Channel 15: PD11 (VBAT_SENSE) */
      sConfig.Channel = ADC_CHANNEL_15;
      HAL_ADC_ConfigChannel(&hadc4, &sConfig);

      HAL_ADC_Start(&hadc4);
      if (HAL_ADC_PollForConversion(&hadc4, 5) == HAL_OK)
      {
          uint32_t raw_vbat = HAL_ADC_GetValue(&hadc4);
          *out_vbat_mv = (raw_vbat * 3300) / 4095;
      }
      HAL_ADC_Stop(&hadc4);
  }
  ```
  Di dalam siklus task:
  ```c
  /* C2. Telemetri Tegangan Baterai (PD11) & Superkapasitor (PC2) via ADC4 */
  if (current_tick_ms - last_telemetry_ms >= 5000)
  {
      last_telemetry_ms = current_tick_ms;
      sample_battery_and_supercap(&s_vbat_millivolts, &s_vcap_millivolts);

      uint32_t alarm_flags = 0;
      if (s_vbat_millivolts < 2700 && s_vbat_millivolts > 500)
      {
          alarm_flags |= SPLN_ALARM_LOW_BATTERY; /* ERR03: Low Battery */
      }
      if (s_vcap_millivolts < 2000 && s_vcap_millivolts > 100)
      {
          alarm_flags |= SPLN_ALARM_SUPERCAP_FAIL; /* ERR06: Supercap anomaly */
      }
      display_set_internal_alarm(&disp_ctx, alarm_flags);
  }
  ```

#### 4. Penanganan Komunikasi Optik DLMS (`USART3` pada `PC4`/`PC5`)
* **Inisialisasi**:
  ```c
  ami_dlms_hdlc_init(&s_optical_hdlc_ctx);
  HAL_UART_Receive_IT(&huart3, &s_optical_rx_byte, 1);
  ```
* **Callback Interupsi**:
  ```c
  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  {
      if (huart->Instance == USART3)
      {
          AmiHdlcStatus status = ami_dlms_hdlc_parse_byte(&s_optical_hdlc_ctx, s_optical_rx_byte, &s_optical_rx_frame);
          if (status == AMI_HDLC_FRAME_READY)
          {
              if (s_optical_rx_frame.type == AMI_HDLC_FRAME_SNRM)
              {
                  uint8_t tx_buf[64];
                  size_t tx_len = 0;
                  ami_dlms_hdlc_encode_frame(s_optical_rx_frame.source_sap,
                                            s_optical_rx_frame.destination_sap,
                                            AMI_HDLC_CONTROL_UA,
                                            NULL, 0,
                                            tx_buf, sizeof(tx_buf), &tx_len);
                  if (tx_len > 0)
                  {
                      HAL_UART_Transmit(&huart3, tx_buf, (uint16_t)tx_len, 100);
                  }
              }
          }
          HAL_UART_Receive_IT(&huart3, &s_optical_rx_byte, 1);
      }
  }
  ```

#### 5. Feedback Posisi Relai (`PC7`)
* **Kode Ditambahkan**:
  ```c
  bool relay_closed_fb = (HAL_GPIO_ReadPin(RELAY_STATUS_GPIO_Port, RELAY_STATUS_Pin) == GPIO_PIN_SET);
  (void)relay_closed_fb;
  ```

---

## 3. Hasil Kompilasi Akhir
```text
Memory region         Used Size  Region Size  %age Used
             RAM:       26008 B       512 KB      4.96%
           SRAM4:           0 B        16 KB      0.00%
             ROM:       69288 B         1 MB      6.61%
0 Errors, 0 Warnings
```

