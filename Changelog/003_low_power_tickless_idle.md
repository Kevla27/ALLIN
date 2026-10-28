# LOG-003: Aktivasi Mode Hemat Daya (Low-Power Tickless Idle & Last Gasp)

- **Tanggal**: 26 Oktober 2026
- **Status**: Selesai & Terverifikasi

---

## 1. Latar Belakang & Kebutuhan
Smart Meter 3-Fasa harus memenuhi kriteria efisiensi energi SPLN:
1. Saat meter beroperasi normal, CPU tidak boleh membuang energi saat sedang menunggu (*idle*) di sela-sela interval tugas 100 ms.
2. Saat terjadi pemadaman listrik PLN (*AC Fail*), energi cadangan pada superkapasitor harus dihemat agar meter sempat mencatat event pemadaman listrik (*Last Gasp*).

---

## 2. Rincian Perubahan Kode

### A. Aktivasi Tickless Idle di `NonSecure/Core/Inc/FreeRTOSConfig.h`
Menambahkan definisi tickless idle bawaan FreeRTOS Cortex-M33:
```c
/* USER CODE BEGIN Defines */
/* Section where parameter definitions can be added (for instance, to override default ones in FreeRTOS.h) */
#define configUSE_TICKLESS_IDLE 1
/* USER CODE END Defines */
```
**Mekanisme Kerja**:
- Ketika tidak ada task yang siap jalan (misal selama `osDelay(100)`), scheduler FreeRTOS menonaktifkan interupsi SysTick, menghitung durasi tidur, dan mengeksekusi instruksi assembly `WFI` (Wait For Interrupt).
- CPU STM32U585 memasuki mode Sleep hemat daya.
- CPU akan langsung bangun seketika jika ada interupsi perangkat keras eksternal (tombol, optik DLMS, sensor tamper) atau saat batas waktu delay tercapai.

### B. Penanganan Pemadaman Listrik (Last Gasp) di `NonSecure/Core/Src/main.c`
Memanfaatkan sinyal dari pin `PC3` (`AC_FAIL_DET`) via EXTI3 interrupt callback:
```c
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    ...
    else if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik Padam (Last Gasp): Matikan beban periferal berdaya tinggi */
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_RESET);
    }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    ...
    else if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik PLN Normal Kembali: Hidupkan kembali periferal */
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
    }
}
```
Hasilnya, beban daya tinggi langsung dimatikan saat tegangan PLN turun, melindungi superkapasitor dari pengurasan berlebih.

