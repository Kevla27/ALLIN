# LOG-007: Modularisasi Arsitektur Tiga Pilar (E1, E2, E3) & Refactoring main.c

- **Tanggal**: 27 Oktober 2026
- **Status**: Selesai & Terverifikasi (Build 0 Errors, 0 Warnings)

---

## 1. Latar Belakang & Tujuan
Sebelumnya, file `NonSecure/Core/Src/main.c` bersifat monolitik: menampung seluruh logika metrologi ADE9000, framing HDLC optik DLMS, sensor sabotase tamper, pembatas beban relai, telemetri ADC4, tampilan carousel LCD, dan perekaman load profile.

Kelemahan kode monolitik di `main.c`:
1. **Risiko Overwrite CubeMX**: Jika ada perubahan konfigurasi pin/clock di STM32CubeMX dan tombol *Generate Code* ditekan, kode logika bisnis di `main.c` berisiko tertimpa atau terhapus.
2. **Melanggar Prinsip Modularitas (*Separation of Concerns*)**: Tidak sesuai dengan rancangan arsitektur pada folder `Acuan/` (`E1` = Metrologi, `E2` = Protokol DLMS/COSEM, `E3` = Aplikasi & HMI).

---

## 2. Modul-Modul Baru yang Dibuat

### A. Pilar E1: Layanan Metrologi & Akumulasi Energi
* **Berkas**: [`E1/Metrology/e1_metrology_service.h`](file:///g:/PPPP/ALLIN/E1/Metrology/e1_metrology_service.h) & [`E1/Metrology/e1_metrology_service.c`](file:///g:/PPPP/ALLIN/E1/Metrology/e1_metrology_service.c)
* **Tanggung Jawab**:
  - Inisialisasi SPI ADE9000 & Mock Simulator (`e1_metrology_service_init`).
  - Pembacaan 9 register mentah (AVRMS, BVRMS, CVRMS, AIRMS, BIRMS, CIRMS, AWATT, BWATT, CWATT).
  - Konversi nilai mentah ke satuan teknik (dVolts, mAmps, Watt, var, va, mHz).
  - Akumulasi energi presisi tinggi menggunakan unit milliJoule/Watt-ms (`e1_metrology_service_step`).
  - Penanganan pulsa kalibrasi energi aktif (`ADE_CF1`) dan reaktif (`ADE_CF2`).
  - Kontrol mode daya ADE9000 Run/Sleep (`ADE_PM0` / `ADE_PM1`).

### B. Pilar E2: Layanan Protokol DLMS/COSEM & Port Optik
* **Berkas**: [`E2/Inc/e2_dlms_service.h`](file:///g:/PPPP/ALLIN/E2/Inc/e2_dlms_service.h) & [`E2/Src/e2_dlms_service.c`](file:///g:/PPPP/ALLIN/E2/Src/e2_dlms_service.c)
* **Tanggung Jawab**:
  - Inisialisasi parser HDLC dan UART RX interrupt port optik (`e2_dlms_service_init`).
  - Sinkronisasi nilai pengukuran listrik ke snapshot register COSEM/OBIS (`e2_dlms_service_sync_snapshot`).
  - Penanganan otomatis frame masuk HDLC di interrupt callback (`HAL_UART_RxCpltCallback`), termasuk negosiasi frame `SNRM` dan balasan `UA`.

### C. Pilar E3: Layanan Aplikasi Meter, HMI, Tamper & Diagnostik
* **Berkas**: [`E3/app/e3_meter_app.h`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.h) & [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)
* **Tanggung Jawab**:
  - Inisialisasi Display, Tamper, Relay Limiter, Load Profile, Buzzer & Power Rails (`e3_meter_app_init`).
  - Heartbeat pulse External Watchdog (`PD12`).
  - Akumulasi detik epoch UNIX dan pembaruan jam display (`e3_meter_app_step`).
  - Telemetri tegangan baterai & superkapasitor via ADC4 dengan alarm `ERR03` & `ERR06`.
  - Pemindaian sakelar sabotase fisik (`PC13`, `PE6`, `PE4`).
  - Evaluasi asimetri beban dan eksekusi pembatas daya per detik.
  - Pengendalian Shunt Trip (`PC8`), lampu LED alarm (`PB7`), dan validasi kontak feedback (`PC7` / `ERR07`).
  - Pembaruan tampilan carousel 17 halaman SPLN Gambar 4.
  - Perekaman berkala 15-Minute Load Profile.
  - Penanganan tombol navigasi (`e3_meter_app_on_button`) dan Last Gasp (`e3_meter_app_on_ac_fail`).

---

## 3. Hasil Refactoring pada `NonSecure/Core/Src/main.c`

File `main.c` sekarang menjadi sangat bersih, elegan, dan ringkas:

```c
/* Inisialisasi di USER CODE 2 */
e1_metrology_service_init();
e2_dlms_service_init(&huart3);
e3_meter_app_init("54321678901", &hadc4);

/* Eksekusi di Task FreeRTOS (USER CODE 4) */
void app_metering_step(void)
{
    static uint32_t last_step_tick_ms = 0;
    uint32_t current_tick_ms = HAL_GetTick();
    uint32_t delta_ms = (last_step_tick_ms == 0) ? 100 : (current_tick_ms - last_step_tick_ms);
    last_step_tick_ms = current_tick_ms;
    if (delta_ms == 0) delta_ms = 1;

    /* 1. Pilar E1: Akuisisi Data ADC/SPI ADE9000 & Akumulasi Energi Wh */
    e1_metrology_service_step(&g_meas, delta_ms);

    /* 2. Pilar E2: Sinkronisasi Register Data Model OBIS DLMS/COSEM */
    e2_dlms_service_sync_snapshot(e2_dlms_service_get_snapshot(), &g_meas);

    /* 3. Pilar E3: Evaluasi Tamper, Relai Shunt Trip, Telemetri Baterai, Carousel Display & Profiling */
    e3_meter_app_step(&g_meas, delta_ms);
}
```

---

## 4. Hasil Kompilasi Firmware

```text
[1/6] Performing build step for 'ALLIN_S'
ninja: no work to do.
[2/6] No install step for 'ALLIN_S'
[3/6] Completed 'ALLIN_S'
[4/6] Performing build step for 'ALLIN_NS'
[1/2] Building C object CMakeFiles/ALLIN_NS.dir/Core/Src/main.c.obj
[2/2] Linking C executable ALLIN_NS.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       26032 B       512 KB      4.97%
           SRAM4:           0 B        16 KB      0.00%
             ROM:       70232 B         1 MB      6.70%
[5/6] No install step for 'ALLIN_NS'
[6/6] Completed 'ALLIN_NS'
```
* **Status:** **0 Errors, 0 Warnings**.
* **Keuntungan:** Kode aman dari overwrite CubeMX dan 100% modular sesuai pembagian pilar di folder acuan.

