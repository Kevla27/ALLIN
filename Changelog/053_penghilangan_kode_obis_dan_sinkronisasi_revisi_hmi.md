# Penghilangan Kode OBIS dari Layar Tampilan SPLN Gambar 4 dan Sinkronisasi Revisi HMI (LOG-053)

## 1. Latar Belakang
Berdasarkan standar fisik tampilan layar SPLN D3.006-1:2021 Gambar 4 ("Tampilan LCD"), informasi yang ditampilkan pada layar fisik meter pintar 3-fasa hanya terdiri atas:
- **Baris 1 (Status Bar):** Ikon status (relai, modem, alarm/tamper, baterai, arah aliran energi, kuadran) dan indikator fasa tegangan & arus (`URST IRST` / `URST REV`).
- **Baris 2 (Nilai Utama & Informasi Register):** Indeks/nama register acuan di sisi kiri (`scroll_index_zz` seperti `IDPEL`, `EA_EXP1`, `UR`, dll.), nilai numerik utama di tengah (`main_value`), dan satuan besaran listrik di sisi kanan (`unit` seperti `V`, `A`, `kW`, `kWh`).
- **Kode OBIS (`01.08.0`, `96.01`, dll.):** Merupakan pengenal objek pada lapisan protokol komunikasi (DLMS/COSEM Pilar E2) dan **tidak perlu/tidak boleh ditampilkan pada layar fisik LCD/OLED meter**.

Selain itu, dilakukan sinkronisasi menyeluruh dengan folder `Revision/` yang mempertahankan konfigurasi port hardware I2C OLED STM32U585 pada **`hi2c3` (`PC0=SCL`, `PC1=SDA`)**.

---

## 2. File yang Terdampak & Perubahan

1. **`E3/drivers/oled/src/bsp_display_oled.c` & `Revision/oled/src/bsp_display_oled.c`**:
   - Menghapus blok logika `case 5` yang sebelumnya menampilkan kode OBIS di pojok kanan Baris 1 pada halaman non-metrologi (`IDPEL`, `METER_ID`, `TIME`, `CHECKSUM`, `PLN_UNIT`, `TAMPER_ALARM`, `RELAY_READY_ON`).
   - Seluruh halaman non-metrologi kini secara konsisten masuk ke blok `default:`, menampilkan indikator fasa 3-fasa penuh (`URST IRST` / `URST REV`) sesuai SPLN Gambar 4.

2. **`E3/app/display/src/display.c` & `Revision/display/src/display.c`**:
   - Menghilangkan kode OBIS dari string baris atas `frame->header_symbols`.
   - Menghilangkan string `(01.00)` dan `(00.00)` dari output CLI `display_render_frame`.
   - Menjaga sinkronisasi tata letak informasi register `scroll_index_zz` dan pemulihan otomatis ke halaman `IDPEL` pasca normalisasi alarm.

3. **`E3/app/display/src/display_task.c` & `Revision/display/src/display_task.c`**:
   - Menghilangkan string `OBIS: %s` dari debug log UART terminal.

4. **`E3/drivers/oled/src/ssd1306.c` & `E3/drivers/oled/inc/ssd1306_conf.h`**:
   - Memastikan port hardware I2C OLED tetap mengarah ke `hi2c3` (`PC0`/`PC1`) sesuai pinout hardware STM32U585.

5. **`E3/main.c`, `E3/app/fsm/src/rtos_tasks.c`, & Seluruh File Uji `E3/tests/`**:
   - Memperbarui pemanggilan fungsi `display_init` menjadi 4 argumen (`ctx, customer_id, meter_id, carousel_interval_ms`).
   - Menyesuaikan pengujian unit display, relay, stress test, dan tamper audit terhadap format HMI acuan terbaru.

---

## 3. Hasil Pengujian & Kompilasi

- **Native CTest Suite (`E3/build`)**: 12/12 Test Suites Lulus 100% (0 Failed).
  - `Integration_E3_Demo`: PASSED
  - `Unit_E3_Display_Tamper`: PASSED
  - `Unit_E3_Power_Relay`: PASSED
  - `Unit_E3_FSM_Statechart`: PASSED
  - `Unit_E3_Load_Profile`: PASSED
  - `Unit_E3_SPLN_Display_Tamper`: PASSED
  - `Stress_E3_Final_System_Audit`: PASSED
  - `Unit_E3_Console_CLI`: PASSED
  - `Unit_E3_Board_Profile`: PASSED
  - `Unit_E3_Power_Mode_Stop2`: PASSED
  - `Unit_E3_NVRAM_Storage`: PASSED
  - `Unit_E3_RTOS_Tasks`: PASSED

- **Target ARM Cortex-M33 Build (`arm-none-eabi-gcc`)**:
  - `ALLIN_NS.elf`: **0 Error, 0 Warning**.
    - RAM: `69,032 B` / `512 KB` (`13.17%`)
    - ROM/Flash: `175,384 B` / `1 MB` (`16.73%`)
  - `ALLIN_S.elf`: **0 Error, 0 Warning**.
