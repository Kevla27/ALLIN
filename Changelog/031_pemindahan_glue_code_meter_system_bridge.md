# LOG-031: Pemisahan Glue Code dari main.c ke Dedicated System Bridge Module (`meter_system_bridge`)

**Tanggal**: 28 Oktober 2026  
**Status**: Selesai & Terverifikasi (Build ARM GCC 0 Error 0 Warning, CTest 9/9 Pass 100%)  
**File Terdampak**:
- `NonSecure/Core/Inc/meter_system_bridge.h` *(File Baru)*
- `NonSecure/Core/Src/meter_system_bridge.c` *(File Baru)*
- `NonSecure/Core/Src/main.c` *(Dibersihkan total dari glue code)*
- `NonSecure/Core/Src/app_freertos.c` *(Koneksi loop metering)*
- `NonSecure/CMakeLists.txt` *(Penambahan sumber kompilasi target)*
- `Changelog/README.md` *(Pembaruan indeks)*

---

## 1. Latar Belakang & Alasan Pemindahan

Sebelum refaktorisasi ini, seluruh "glue code" (kode lem/jembatan perekat) yang menghubungkan periferal perangkat keras STM32 HAL dengan Tiga Pilar Firmware diletakkan langsung di dalam `NonSecure/Core/Src/main.c`:
1. **Inisialisasi Multi-Pilar manual di `USER CODE BEGIN 2`**: Memanggil berurutan `console_io_init()`, `e1_metrology_service_init()`, `e2_dlms_service_init()`, `e2_dlms_service_set_modem_uart()`, dan `e3_meter_app_init()`.
2. **Implementasi Callback HAL STM32 di `USER CODE BEGIN 4`**:
   - `HAL_UART_RxCpltCallback()`: Delegasi parsing UART optik & modem ke Pilar E2.
   - `HAL_GPIO_EXTI_Falling_Callback()`: Routing sinyal tombol, AC Fail (Last Gasp), deteksi pulsa CF1/CF2 ADE9000, serta IRQ0/IRQ1/EVENT tamper ke Pilar E1 dan E3.
   - `HAL_GPIO_EXTI_Rising_Callback()`: Routing pulsa normalisasi AC Fail, Zero-Crossing, dan AUX input.
   - `app_metering_step()`: Pipeline sinkronisasi data periodik E1 (akuisisi metrologi) -> E2 (sinkronisasi snapshot OBIS DLMS) -> E3 (evaluasi proteksi/display).
   - `app_metering_task_loop()`: Loop tak berhingga untuk task FreeRTOS.
3. **Penyimpanan state global `static meter_measurements_t g_meas` dan `console_uart_tx` di `main.c`**.

### Permasalahan yang Muncul:
- **Rentan Tertimpa STM32CubeMX**: Walaupun berada di dalam blok `/* USER CODE BEGIN ... */`, penumpukan ratusan baris kode logika sistem di file autogenerasi CubeMX membuat `main.c` menjadi gemuk (bloated) dan sulit diaudit.
- **Ketergantungan Kuat (Tight Coupling)**: `main.c` terikat erat pada seluruh header pilar E1, E2, E3, display, console, dan tamper.
- **Portabilitas Sulit**: Ketika mikrokontroler atau RTOS diganti (misal berpindah ke RTOS native, baremetal super-loop, atau MCU vendor lain), `main.c` harus dirombak ulang secara signifikan.

---

## 2. Arsitektur Jembatan Terpadu (`meter_system_bridge`)

Dengan memindahkan seluruh glue code ke modul independen `meter_system_bridge.c` dan `meter_system_bridge.h`:

```
+-----------------------------------------------------------------------------+
|                                    main.c                                   |
| - Inisialisasi HAL Peripheral (GPIO, I2C, SPI, UART, RTC, ADC)              |
| - USER CODE BEGIN 2: meter_system_bringup(&huart3, &huart2, &hadc4);        |
| - osKernelStart();                                                          |
| - USER CODE BEGIN 4: BERSIH / KOSONG (0 lines)                              |
+-----------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------+
|                          meter_system_bridge (.h/.c)                         |
|                                                                             |
| [1] Inisialisasi Terpadu:                                                   |
|     meter_system_bringup(...) -> Console IO, E1, E2, E3                     |
|                                                                             |
| [2] Callback Interrupt Hardware (Override __weak HAL Symbols):               |
|     - HAL_UART_RxCpltCallback()                                             |
|     - HAL_GPIO_EXTI_Falling_Callback()                                      |
|     - HAL_GPIO_EXTI_Rising_Callback()                                       |
|                                                                             |
| [3] Pipeline & Loop Penjadwalan:                                            |
|     - app_metering_step()      [E1 -> E2 -> E3 synchronous tick pipeline]   |
|     - app_metering_task_loop()  [FreeRTOS / Baremetal Task Loop]            |
|     - meter_system_get_measurements()                                       |
+-----------------------------------------------------------------------------+
         |                                |                            |
         v                                v                            v
+-------------------+           +-------------------+        +----------------+
|      Pilar E1     |           |      Pilar E2     |        |    Pilar E3    |
| (Metrologi Listrik|           | (Komunikasi DLMS/ |        | (Aplikasi HMI, |
|      ADE9000)     |           |       COSEM)      |        | Tamper, Relay) |
+-------------------+           +-------------------+        +----------------+
```

---

## 3. Rincian Implementasi

### A. Header Baru: `NonSecure/Core/Inc/meter_system_bridge.h`
Mendeklarasikan antarmuka bersih untuk inisialisasi, loop metering, dan akses telemetri:
```c
void meter_system_bringup(UART_HandleTypeDef *huart_opt, UART_HandleTypeDef *huart_modem, ADC_HandleTypeDef *hadc_telemetry);
void app_metering_step(void);
void app_metering_task_loop(void);
const meter_measurements_t *meter_system_get_measurements(void);
```

### B. Implementasi Baru: `NonSecure/Core/Src/meter_system_bridge.c`
- Mengisolasi variabel statis `s_meas` (pengukuran listrik) dan pointer `s_console_uart`.
- Mengimplementasikan `bridge_console_uart_tx()` untuk retargeting `printf` serial non-blocking.
- Meng-override implementasi `__weak` callback HAL STM32 (`HAL_UART_RxCpltCallback`, `HAL_GPIO_EXTI_Falling_Callback`, `HAL_GPIO_EXTI_Rising_Callback`) langsung di tingkat linker file terpisah.
- Mengatur interval periodik pada `app_metering_step()` dengan proteksi rollover tick `HAL_GetTick()`.

### C. Pembersihan `NonSecure/Core/Src/main.c`
- **`USER CODE BEGIN Includes`**: Hanya menyertakan `#include "meter_system_bridge.h"`.
- **`USER CODE BEGIN PV`**: Dibersihkan (variabel `g_meas` dipindah ke bridge).
- **`USER CODE BEGIN 0`**: Dibersihkan (fungsi `console_uart_tx` dipindah ke bridge).
- **`USER CODE BEGIN 2`**: Diringkas menjadi 1 baris pemanggilan:
  ```c
  /* Inisialisasi terpadu seluruh sistem dan Tiga Pilar (E1, E2, E3) via bridge */
  meter_system_bringup(&huart3, &huart2, &hadc4);
  ```
- **`USER CODE BEGIN 4`**: Dikosongkan seluruhnya (123 baris callback HAL dan loop metering berhasil dipindahkan ke bridge).

---

## 4. Hasil Verifikasi & Uji Kompilasi

### 1. Kompilasi Target ARM Cortex-M33 TrustZone
- **NonSecure Target (`ALLIN_NS.elf`)**:
  - Compiler: `arm-none-eabi-gcc` 14.3.1
  - RAM: 64,864 Bytes (12.37% dari 512 KB)
  - ROM Flash: 141,096 Bytes (13.46% dari 1 MB)
  - **Hasil**: **0 Error, 0 Warning**
- **Secure Target (`ALLIN_S.elf`)**:
  - Status: Up to date, **0 Error, 0 Warning**

### 2. CTest Suite Host (E3)
```text
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo
1/9 Test #1: Integration_E3_Demo ..............   Passed    0.05 sec
    Start 2: Unit_E3_Display_Tamper
2/9 Test #2: Unit_E3_Display_Tamper ...........   Passed    0.04 sec
    Start 3: Unit_E3_Power_Relay
3/9 Test #3: Unit_E3_Power_Relay ..............   Passed    0.04 sec
    Start 4: Unit_E3_FSM_Statechart
4/9 Test #4: Unit_E3_FSM_Statechart ...........   Passed    0.04 sec
    Start 5: Unit_E3_Load_Profile
5/9 Test #5: Unit_E3_Load_Profile .............   Passed    0.04 sec
    Start 6: Unit_E3_SPLN_Display_Tamper
6/9 Test #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.04 sec
    Start 7: Stress_E3_Final_System_Audit
7/9 Test #7: Stress_E3_Final_System_Audit .....   Passed    0.04 sec
    Start 8: Unit_E3_Console_CLI
8/9 Test #8: Unit_E3_Console_CLI ..............   Passed    0.04 sec
    Start 9: Unit_E3_Board_Profile
9/9 Test #9: Unit_E3_Board_Profile ............   Passed    0.03 sec

100% tests passed, 0 tests failed out of 9
```

