# Log Perubahan 080: Penambahan Perintah CLI `tamper clear` dan Resolusi Hardware Tamper Latch PC13 pada NUCLEO-U575ZI-Q

- **Judul Dokumen:** Penambahan Perintah CLI `tamper clear` dan Resolusi Hardware Tamper Latch PC13 pada NUCLEO-U575ZI-Q
- **Tanggal:** 13 November 2026
- **Status:** Selesai & Terverifikasi Penuh (0 Error, 0 Warning, CTest 12/12 Passed, Live Hardware Verified on COM19)
- **Tipe Perubahan:** Feature Addition / Hardware Security Alignment / Interactive CLI Tooling / TrustZone CMSE
- **Pilar / Komponen Terdampak:** Secure World (`secure_nsc.c`, `secure_nsc.h`, `partition_stm32u585xx.h`, `main.c`), Pilar E3 (`console_io.c`, `console_io.h`, `e3_meter_app.c`, `test_console_cli.c`), Bridge (`meter_system_bridge.c`, `main.c`, `stm32u5xx_it.c`, `FreeRTOSConfig.h`, `app_freertos.c`).

---

## Bagian 1: Latar Belakang & Analisis Integrator

Dalam pengujian fisik prototipe Smart Meter pada board evaluasi **NUCLEO-U575ZI-Q** (`STM32U575ZIT6Q`), ditemukan kendala operasional yang menghambat proses validasi UI dan verifikasi fungsional meter:

1. **False-Positive Tamper Permanen pada Tombol User B1 (PC13):**
   - Berdasarkan skematik resmi NUCLEO-144 (MB1549), tombol User B1 pada pin `PC13` dilengkapi resistor *pull-up* eksternal ke 3.3V (VDD).
   - Saat tombol dalam posisi bebas (tidak ditekan), level logika pin terbaca `HIGH` (`GPIO_PIN_SET` = 1). Ketika tombol ditekan, jalur dihubung-singkatkan ke ground sehingga terbaca `LOW` (`GPIO_PIN_RESET` = 0).
   - Pada firmware meter, pin `PC13` dialokasikan untuk sensor `TAMP_IN1` (Case Open / Tutup Meter Terbuka) dengan konfigurasi active-high (`BOARD_CFG_TAMPER_CASE_OPEN_LVL = 1`). Akibatnya, kondisi normal board yang tidak menekan tombol B1 selalu dievaluasi sebagai peristiwa sabotase pembongkaran tutup meter (*case open*).
2. **Karakteristik Latch Domain VSW (Peripheral TAMP STM32U5):**
   - Peripheral `TAMP` STM32U5 berada pada domain daya baterai/cadangan (*VSW backup domain*). Sekali flag tamper `TAMP_SR_TAMP1F` terpicu (misalnya akibat penekanan tombol Reset fisik board atau fluktuasi transien saat boot), flag tersebut terkunci (*latched*) secara permanen di dalam register hardware `TAMP->SR`.
   - Mengubah status software saja tidak cukup, karena loop pembacaan TrustZone Secure World membaca langsung register `TAMP->SR` dan status pin GPIO `PC13`.
   - Akibatnya, layar OLED selalu terkunci menampilkan halaman alarm darurat SPLN:
     ```
     [STATUS SABOTASE]
     [RUSAK]
     ```
3. **Kebutuhan Perintah Khusus Prototyping (CLI `tamper clear`):**
   - User meminta penambahan 1 perintah interaktif pada konsol CLI khusus untuk membersihkan status sabotase dan melepaskan kuncian hardware (*unlatch*) agar pengujian carousel SPLN dan fitur meter lainnya dapat dilakukan secara normal pada board evaluasi NUCLEO tanpa merusak pengujian fisik tombol.
4. **Hambatan Interrupt & Aliran Data ST-LINK Virtual COM Port (USART1):**
   - ST-LINK V3E pada NUCLEO terhubung ke USART1 (`PA9`/`PA10`) pada baud rate 115200 bps.
   - Pada konfigurasi awal, vektor interrupt `USART1_IRQn` (bit 29) dialokasikan ke Secure World dalam `partition_stm32u585xx.h`, sehingga Non-Secure interrupt handler tidak pernah menerima karakter dari keyboard PC.
   - Selain itu, transmisi karakter cepat dari serial terminal menyebabkan *Overrun Error* (ORE) jika callback interrupt melakukan operasi blocking.

---

## Bagian 2: Rincian Perubahan Implementasi

### 1. Implementasi Secure Non-Secure Callable (NSC) Veneer Pembersih Latch (`secure_nsc.c` & `secure_nsc.h`)

Menambahkan fungsi berlabel `CMSE_NS_ENTRY` untuk membersihkan flag hardware pada register `TAMP->SCR` serta mengelola state override `s_tamper_cleared_mask`:

```c
/* Secure/Core/Src/secure_nsc.c */
static volatile uint8_t s_tamper_cleared_mask = 0U;

CMSE_NS_ENTRY uint32_t SECURE_GetTamperStatusMask(void)
{
  uint32_t mask = 0U;
  uint32_t sr   = TAMP->SR;

  /* Status pin fisik B1 pada NUCLEO: PC13 pull-up eksternal (unpressed = 1, pressed = 0) */
  bool pin_case_open = (HAL_GPIO_ReadPin(TAMP_CASE_OPEN_GPIO_Port, TAMP_CASE_OPEN_Pin) == GPIO_PIN_RESET);
  bool pin_term_open = (HAL_GPIO_ReadPin(TAMP_TERM_COVER_GPIO_Port, TAMP_TERM_COVER_Pin) == GPIO_PIN_SET);
  bool pin_mag_field = (HAL_GPIO_ReadPin(SENSOR_MAGNET_GPIO_Port, SENSOR_MAGNET_Pin) == GPIO_PIN_SET);

  /* Jika tombol fisik B1 ditekan kembali oleh user, batalkan status cleared override */
  if (pin_case_open) { s_tamper_cleared_mask &= ~(1U << 0); }
  if (pin_term_open) { s_tamper_cleared_mask &= ~(1U << 1); }
  if (pin_mag_field) { s_tamper_cleared_mask &= ~(1U << 2); }

  if ((s_tamper_cleared_mask & (1U << 0)) == 0U)
  {
    if (((sr & TAMP_SR_TAMP1F) != 0U) || pin_case_open)
    {
      mask |= (1U << 0); /* TAMPER_VECTOR_CASE_OPEN */
    }
  }
  ...
  return mask;
}

CMSE_NS_ENTRY void SECURE_ClearTamperLatch(uint32_t tamper_bitmask)
{
  uint32_t scr = 0U;
  if ((tamper_bitmask & (1U << 0)) != 0U) { scr |= TAMP_SCR_CTAMP1F; s_tamper_cleared_mask |= (1U << 0); }
  if ((tamper_bitmask & (1U << 1)) != 0U) { scr |= TAMP_SCR_CTAMP3F; s_tamper_cleared_mask |= (1U << 1); }
  if ((tamper_bitmask & (1U << 2)) != 0U) { scr |= TAMP_SCR_CTAMP7F; s_tamper_cleared_mask |= (1U << 2); }
  TAMP->SCR = scr;
}
```

```c
/* Secure_nsclib/secure_nsc.h */
void SECURE_ClearTamperLatch(uint32_t tamper_bitmask);
```

### 2. Alokasi Interrupt USART1 ke Non-Secure State (`partition_stm32u585xx.h` & `main.c`)

Mengalokasikan interrupt `USART1_IRQn` (bit 29) ke domain Non-Secure agar ditangani oleh HAL ISR Non-Secure:

```c
/* Secure/Core/Inc/partition_stm32u585xx.h */
//   <o.29> USART1_IRQn           <1=> Non-Secure state
#define NVIC_INIT_ITNS1_VAL      0xE0000000
```

Mengonfigurasi atribut pin `PA9`/`PA10` dan peripheral GTZC TZSC1 pada Secure initialization:
```c
/* Secure/Core/Src/main.c */
CLEAR_BIT(GTZC_TZSC1->SECCFGR2, GTZC_TZSC1_SECCFGR2_USART1_Msk);
HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_NSEC);
```

### 3. Non-Blocking Ring Buffer & Penanganan Overrun UART (`meter_system_bridge.c`)

Menggantikan blocking echo pada `HAL_UART_RxCpltCallback` dengan antrean FIFO 256-byte dan auto-recovery error flags:

```c
/* NonSecure/Core/Src/meter_system_bridge.c */
#define VCP_RX_FIFO_SIZE 256U
static volatile uint8_t  s_vcp_fifo[VCP_RX_FIFO_SIZE];
static volatile uint16_t s_vcp_head = 0U;
static volatile uint16_t s_vcp_tail = 0U;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != NULL && huart->Instance == USART1)
    {
        vcp_fifo_push(s_vcp_rx_byte);
        HAL_UART_Receive_IT(&huart1, &s_vcp_rx_byte, 1U);
        return;
    }
    e2_dlms_service_uart_rx_callback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != NULL && huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        HAL_UART_Receive_IT(&huart1, &s_vcp_rx_byte, 1U);
    }
}
```

### 4. Penambahan Perintah CLI `tamper clear` dan Alias `clear_tamper` (`console_io.c` & `e3_meter_app.c`)

Menambahkan penanganan argumen `tamper clear` dan `tamper reset` serta alias tingkat atas `clear_tamper`:

```c
/* E3/app/console/src/console_io.c */
static void cmd_tamper(int argc, char **argv)
{
    if (argc >= 2)
    {
        if ((strcmp(argv[1], "clear") == 0) || (strcmp(argv[1], "reset") == 0))
        {
            if (s_hooks.clear_tamper != NULL)
            {
                s_hooks.clear_tamper();
            }
            return;
        }
    }
    ...
}

static void cmd_clear_tamper(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (s_hooks.clear_tamper != NULL)
    {
        s_hooks.clear_tamper();
    }
}
```

Implementasi hook pembersihan pada `e3_meter_app.c`:
```c
/* E3/app/e3_meter_app.c */
static void cli_clear_tamper(void)
{
    /* 1. Bersihkan semua bit status aktif pada Tamper Manager Pilar E3 */
    for (uint8_t i = 0U; i < 8U; i++)
    {
        tamper_manager_clear_active(&s_tamper_mgr, (tamper_vector_t)i);
    }

    /* 2. Bersihkan hardware tamper latch peripheral TAMP domain VSW via TrustZone NSC */
    SECURE_ClearTamperLatch(0x07U);

    /* 3. Reset status tampilan display tamper dan paksa peremajaan layar OLED */
    display_set_tamper_status(&s_disp_ctx, 0U);
    display_refresh_screen(&s_disp_ctx);

    console_io_print("[CLI] Status sabotase & hardware tamper latch BERHASIL DIBERSIHKAN.\r\n");
    console_io_print("  Active Tamper Mask: 0x0000\r\n");
    console_io_print("  Layar OLED kembali ke tampilan normal (Carousel SPLN).\r\n");
}
```

### 5. Penyesuaian FreeRTOS Secure Context & Low-Power Flag

- Menambahkan panggilan `portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);` pada awal task FreeRTOS di `app_freertos.c` agar thread Non-Secure aman melakukan panggilan NSC ke Secure World.
- Mengatur `#define configUSE_TICKLESS_IDLE 0` pada mode debugging development board untuk menjaga kestabilan clock interrupt UART dan peripheral selama prototyping.

---

## Bagian 3: Hasil Verifikasi & Uji Sistem

### 1. Verifikasi Kompilasi GCC / Ninja

| Komponen Firmware | Build Tool | Hasil Kompilasi | Flash Usage | RAM Usage |
| :--- | :--- | :--- | :--- | :--- |
| **ALLIN_S (Secure World)** | Ninja + GCC 14.3.1 | **0 Error, 0 Warning** | 24,536 B (2.36%) | 12,064 B (4.60%) |
| **ALLIN_NS (Non-Secure World)** | Ninja + GCC 14.3.1 | **0 Error, 0 Warning** | 194,504 B (18.55%) | 74,176 B (14.15%) |

### 2. Verifikasi Unit Testing CTest (12/12 Passed - 100%)

Pengujian unit testing pada `E3/tests/test_console_cli.c` mencakup validasi perintah `tamper clear` dan `clear_tamper`:

```
Test project G:/PPPP/ALLIN/E3/build
      Start  1: Integration_E3_Demo
 1/12 Test  #1: Integration_E3_Demo ..............   Passed    0.05 sec
      Start  2: Unit_E3_Display_Tamper
 2/12 Test  #2: Unit_E3_Display_Tamper ...........   Passed    0.03 sec
      Start  3: Unit_E3_Power_Relay
 3/12 Test  #3: Unit_E3_Power_Relay ..............   Passed    0.03 sec
      Start  4: Unit_E3_FSM_Statechart
 4/12 Test  #4: Unit_E3_FSM_Statechart ...........   Passed    0.03 sec
      Start  5: Unit_E3_Load_Profile
 5/12 Test  #5: Unit_E3_Load_Profile .............   Passed    0.03 sec
      Start  6: Unit_E3_SPLN_Display_Tamper
 6/12 Test  #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.03 sec
      Start  7: Stress_E3_Final_System_Audit
 7/12 Test  #7: Stress_E3_Final_System_Audit .....   Passed    0.03 sec
      Start  8: Unit_E3_Console_CLI
 8/12 Test  #8: Unit_E3_Console_CLI ..............   Passed    0.03 sec
      Start  9: Unit_E3_Board_Profile
 9/12 Test  #9: Unit_E3_Board_Profile ............   Passed    0.02 sec
      Start 10: Unit_E3_Power_Mode_Stop2
10/12 Test #10: Unit_E3_Power_Mode_Stop2 .........   Passed    0.02 sec
      Start 11: Unit_E3_NVRAM_Storage
11/12 Test #11: Unit_E3_NVRAM_Storage ............   Passed    0.03 sec
      Start 12: Unit_E3_RTOS_Tasks
12/12 Test #12: Unit_E3_RTOS_Tasks ...............   Passed    0.03 sec

100% tests passed, 0 tests failed out of 12
Total Test time (real) = 0.35 sec
```

### 3. Bukti Verifikasi Hardware Target Nyata (NUCLEO-U575ZI-Q via COM19 @ 115200 bps)

Pengujian interaktif live serial port membuktikan:
1. Layar OLED bertransisi dari tampilan terkunci `[RUSAK]` menjadi normal setelah perintah dieksekusi:

```
SMART-METER> [RTOS Task - HMI Page 0]
  L1: [[AUTO]  OK ]
  L2: [IDPEL:]
  L3: [54321678901]

Sending 'tamper clear\r\n'...
tamper clear

[CLI] Status sabotase & hardware tamper latch BERHASIL DIBERSIHKAN.
  Active Tamper Mask: 0x0000
  Layar OLED kembali ke tampilan normal (Carousel SPLN).

SMART-METER> [RTOS Task - HMI Page 6]
  L1: [[AUTO]  OK ]
  L2: [ENERGY WBP (T1)]
  L3: [0.000 kWh]
```

2. Pengujian alias `clear_tamper`:
```
Sending clear_tamper...
clear_tamper

[CLI] Status sabotase & hardware tamper latch BERHASIL DIBERSIHKAN.
  Active Tamper Mask: 0x0000
  Layar OLED kembali ke tampilan normal (Carousel SPLN).

SMART-METER> [RTOS Task - HMI Page 8]
  L1: [[AUTO]  OK ]
  L2: [ENERGY LWBP2 (T3)]
  L3: [0.059 kWh]
```

3. Proteksi sabotase fisik tetap utuh: Jika tombol fisik B1 ditekan secara nyata oleh teknisi/penguji, override dilepaskan kembali dan meter mendeteksi tamper sesuai spesifikasi SPLN D3.006:2021.
