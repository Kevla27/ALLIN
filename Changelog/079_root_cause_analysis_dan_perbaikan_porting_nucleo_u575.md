# Log Perubahan 079: Root Cause Analysis & Resolusi Penuh Porting NUCLEO-U575ZI-Q

- **Judul Dokumen:** Root Cause Analysis (RCA) dan Resolusi Penuh Porting Smart Meter ke Board Target NUCLEO-U575ZI-Q
- **Tanggal:** 13 November 2026
- **Status:** Selesai & Terverifikasi Penuh (0 Error, 0 Warning, CTest 12/12 Passed)
- **Tipe Perubahan:** Hardware Porting / Bugfix / FreeRTOS Kernel Alignment / Console Retargeting
- **Pilar / Komponen Terdampak:** Middlewares (FreeRTOS SysTick & FPU), Pilar E3 (NVRAM Storage & Heartbeat LED), NonSecure Core (USART1 ST-LINK VCP), Secure Core (TrustZone GPIO Attributes).

---

## Bagian 1: Latar Belakang & Analisis Integrator

Saat firmware Smart Meter di-flash ke board target pengembangan **NUCLEO-U575ZI-Q** (`STM32U575ZIT6Q`, LQFP144), board teramati tidak menunjukkan respons fisik apapun (*"tidak terjadi apa-apa"*): LED indikator padam, layar OLED blank, dan terminal serial tidak menampilkan output.

Melalui investigasi mendalam menggunakan ST-LINK GDB Server (`arm-none-eabi-gdb`), pembacaan register Cortex-M33, fault registers (`CFSR`, `BFAR`, `HFSR`), serta penelusuran skematik NUCLEO-144 (UM2861), ditemukan **lima akar permasalahan utama (5 Root Causes)**:

1. **Starvasi SysTick & `HAL_GetTick()` pada FreeRTOS CMSIS-OS2:**
   - FreeRTOS meng-override `SysTick_Handler(void)` pada `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c`, namun tidak memanggil `HAL_IncTick()`.
   - Sebelum scheduler berjalan, fungsi inisialisasi layar `ssd1306_Init()` memanggil `HAL_Delay(150)`. Karena `HAL_IncTick()` tidak pernah dieksekusi, nilai `uwTick` bernilai 0 selamanya sehingga CPU terjebak dalam *infinite loop* saat baru boot.
2. **Infinite Busy-Wait Loop SPI2 NVRAM External Flash:**
   - Fungsi `spi2_flash_wait_busy()` pada `E3/app/profiling/src/nvram_storage.c` melakukan polling register status SPI Flash hingga `timeout_cycles = 100000U`.
   - Pada board NUCLEO tanpa chip eksternal SPI Flash, jalur MISO ditarik mengambang/pull-up sehingga selalu membaca byte `0xFF`. Akibatnya, bit `WIP` (Write In Progress, bit 0) selalu terbaca `1`, memblokir CPU selama ratusan detik pada setiap operasi baca NVRAM.
3. **Konflik Hardware FPU Cortex-M33 vs FreeRTOS Stack Frame Alignment:**
   - Unit FPU hardware aktif (`SCB->CPACR` mengaktifkan CP10 dan CP11 untuk Single Precision Floating Point).
   - Namun, konfigurasi FreeRTOS (`FreeRTOSConfig.h`) mendefinisikan `#define configENABLE_FPU 0`.
   - Ketika instruksi floating-point dieksekusi (atau saat penanganan register tarif/sudut fasa), bit `CONTROL.FPCA` aktif. Saat terjadi alih konteks tugas via `PendSV_Handler`, FreeRTOS hanya me-restore *integer frame* (8 register) bukannya *extended floating point frame* (26 register). Ketidakselarasan stack ini menyebabkan `PC` dan `SP` korup, memicu BusFault / HardFault fatal (`CFSR = 0x00009200`, `BFAR = 0xFFFFFFF8`).
4. **Timpaan State LED Heartbeat dalam Eksekusi Step Pilar E3:**
   - Blue User LED (PB7) di-toggle pada awal loop, namun di dalam fungsi yang sama, `e3_meter_app_step()` mengeksekusi `HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, is_yellow_led_on ? GPIO_PIN_SET : GPIO_PIN_RESET)`.
   - Dalam kondisi normal tanpa sabotase (`is_yellow_led_on == 0`), pin PB7 langsung di-reset ke LOW dalam hitungan mikrodetik, sehingga mata manusia melihat LED selalu padam.
5. **Mismatched Console UART terhadap Hardware ST-LINK V3E Virtual COM Port:**
   - Pada NUCLEO-U575ZI-Q, port USB ST-LINK V3E (Virtual COM Port pada PC) secara fisik terhubung ke **USART1** pada pin **PA9** (TX) dan **PA10** (RX).
   - Firmware mentransmisikan seluruh log boot dan interaktif CLI ke **USART2** (`PA2`/`PA3`), sehingga port serial PC (COM19) tidak menerima data sama sekali. Selain itu, pin `PA9` dan `PA10` belum dialokasikan ke domain Non-Secure pada TrustZone GTZC GPIO.

---

## Bagian 2: Rincian Perubahan Implementasi

### 1. Perbaikan SysTick Handler (`cmsis_os2.c`)
Memanggil `HAL_IncTick()` secara eksplisit di dalam interrupt handler SysTick:

```c
/* Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c */
extern void HAL_IncTick(void);

void SysTick_Handler (void) {
  HAL_IncTick();
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    xPortSysTickHandler();
  }
}
```

### 2. Safeguard Deteksi Cepat SPI Flash (`nvram_storage.c`)
Menambahkan deteksi `0xFF` (bus mengambang tanpa chip flash terpasang) dan membatasi timeout ke 50 siklus:

```c
/* E3/app/profiling/src/nvram_storage.c */
static bool spi2_flash_wait_busy(void)
{
    uint32_t timeout_cycles = 50U;
    while (timeout_cycles-- > 0U)
    {
        uint8_t cmd = SPI_FLASH_CMD_RDSR;
        uint8_t status = 0xFFU;
        // Transmit & receive status
        if (status == 0xFFU || (status & 0x01U) == 0U)
        {
            break;
        }
    }
    return true;
}
```

### 3. Penyelarasan FPU & Tickless Idle (`FreeRTOSConfig.h`)
Mengaktifkan integrasi FPU hardware pada alih konteks FreeRTOS Cortex-M33:

```c
/* NonSecure/Core/Inc/FreeRTOSConfig.h */
#define configENABLE_FPU                         1
#define configUSE_TICKLESS_IDLE                  0
```

### 4. Heartbeat LED Terpadu pada Pilar E3 (`e3_meter_app.c`)
Menjadikan status normal (tanpa sabotase/alarm) berkedip 1 Hz (500 ms ON / 500 ms OFF) sebagai konfirmasi visual eksekusi sistem:

```c
/* E3/app/e3_meter_app.c */
        if (is_ready_on)
        {
            GPIO_PinState blink_state = ((current_tick_ms / 500U) % 2U == 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
            HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, blink_state);
        }
        else if (is_yellow_led_on)
        {
            HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, GPIO_PIN_SET);
        }
        else
        {
            /* Heartbeat: Saat meter beroperasi normal tanpa alarm, kedipkan LED periodik 1 Hz */
            GPIO_PinState blink_state = ((current_tick_ms / 500U) % 2U == 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
            HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, blink_state);
        }
```

### 5. Integrasi ST-LINK Virtual COM Port (USART1 @ 115200 bps)
- **Secure Domain (`Secure/Core/Src/main.c`):** Memberikan izin atribut Non-Secure untuk `PA9` dan `PA10`:
  ```c
  HAL_GPIO_ConfigPinAttributes(GPIOA, ... | GPIO_PIN_9 | GPIO_PIN_10, GPIO_PIN_NSEC);
  ```
- **NonSecure Domain (`NonSecure/Core/Src/main.c` & `stm32u5xx_hal_msp.c`):**
  Menginisialisasi USART1 pada PA9 (TX) dan PA10 (RX) pada 115200 bps 8-N-1 dengan interrupt NVIC prioritas 5.
- **Bridge Console Retargeting (`NonSecure/Core/Src/meter_system_bridge.c`):**
  Mentransmisikan output konsol `printf()` ke `huart1` (ST-LINK VCP) dan mengaktifkan `HAL_UART_Receive_IT(&huart1, ...)` agar perintah CLI interaktif dapat langsung diketik melalui PuTTY/TeraTerm.

---

## Bagian 3: Hasil Verifikasi & Uji Sistem

| Item Pengujian | Target / Standar | Hasil Aktual | Status |
| :--- | :--- | :--- | :--- |
| **Kompilasi Secure** | `ninja -C Secure/build` | 0 Error, 0 Warning (`ALLIN_S.elf`) | **LULUS (100%)** |
| **Kompilasi NonSecure** | `ninja -C NonSecure/build` | 0 Error, 0 Warning (`ALLIN_NS.elf`) | **LULUS (100%)** |
| **Unit Test Pilar E3** | `ctest --test-dir E3/build` | 12/12 Test Passed (0.39s) | **LULUS (100%)** |
| **GDB Trace Execution** | Hardware In-Circuit Debug | Run dari Reset $\rightarrow$ `main()` $\rightarrow$ `app_metering_step()` stabil | **LULUS (100%)** |

Tabel Penggunaan Memori Flash & RAM NUCLEO-U575ZI-Q:
- **Secure Image:** ROM 24,080 B (2.31%), RAM 12,056 B (4.60%).
- **NonSecure Image:** ROM 193,528 B (18.46%), RAM 73,912 B (14.10%).
