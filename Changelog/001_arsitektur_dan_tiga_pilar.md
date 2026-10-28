# LOG-001: Integrasi Tiga Pilar (E1, E2, E3) & Konstrain Pinout

- **Tanggal**: 26 Oktober 2026
- **Status**: Selesai & Terverifikasi

---

## 1. Latar Belakang & Tujuan
Menyatukan kode sumber dari 3 pilar utama pengembangan Smart Meter 3-Fasa ke dalam satu proyek gabungan (ALLIN):
1. **Pilar E1 (Metrologi)**: Pengukuran tegangan, arus, daya, energi, dan frekuensi fasa R, S, T via SPI ADE9000 & Mock Emulasi.
2. **Pilar E2 (DLMS/COSEM)**: Data model snapshot AMI OBIS code dan protokol transfer data meter.
3. **Pilar E3 (HMI, Tamper, Relay Limiter & Load Profile)**: Standar tampilan LCD 17 carousel SPLN D3.006:2021 Gambar 4, proteksi manipulasi fisik (case tamper, cover tamper, magnet), serta pencatatan berkala 15-Minute Load Profile.

---

## 2. File yang Dibuat / Dimodifikasi

1. **`NonSecure/CMakeLists.txt`**:
   - Menambahkan *include directories* untuk modul E1, E2, dan E3:
     ```cmake
     target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
         ${CMAKE_CURRENT_SOURCE_DIR}/../E1
         ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Metrology
         ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Mock
         ${CMAKE_CURRENT_SOURCE_DIR}/../E1/RegisterMap
         ${CMAKE_CURRENT_SOURCE_DIR}/../E1/SPI
         ${CMAKE_CURRENT_SOURCE_DIR}/../E2/Inc
         ${CMAKE_CURRENT_SOURCE_DIR}/../E3/app/display/inc
         ${CMAKE_CURRENT_SOURCE_DIR}/../E3/app/tamper/inc
         ${CMAKE_CURRENT_SOURCE_DIR}/../E3/app/fsm/inc
         ${CMAKE_CURRENT_SOURCE_DIR}/../E3/app/profiling/inc
         ${CMAKE_CURRENT_SOURCE_DIR}/../E3/app/console/inc
         ${CMAKE_CURRENT_SOURCE_DIR}/../E3/drivers/buzzer/inc
     )
     ```
   - Menambahkan *source files* implementasi:
     - `E1/Metrology/metrology.c`, `E1/Mock/ade9000_mock.c`, `E1/SPI/ade9000_spi.c`
     - `E2/Src/ami_dlms_cosem.c`, `E2/Src/ami_dlms_hdlc.c`
     - `E3/app/display/src/display.c`, `E3/app/tamper/src/tamper_manager.c`, `E3/app/fsm/src/power_relay_fsm.c`, `E3/app/profiling/src/load_profile.c`, `E3/drivers/buzzer/src/bsp_alarm_buzzer.c`

2. **`NonSecure/Core/Src/main.c`**:
   - Menghubungkan pembacaan register ADE9000 ke `meas` (meter measurements struct).
   - Memasukkan data `meas` ke `dlms_snapshot` (E2).
   - Mengirim data ke LCD carousel dan load profiler (E3).

---

## 3. Konstrain Khusus Hardware yang Diterapkan

- **Pin `PC9` (`NC_PC9`) dan `PC15` (`NC_PC15`)**:
  - Ditetapkan secara ketat sebagai **Not Connected (NC) / Unused** sesuai arsitektur *Single-coil Shunt Trip* pada board.
- **Relai Shunt Trip**:
  - Dikendalikan melalui pulsa trigger pada **`PC8` (`RELAY_TRIP`)** dengan umpan balik sensor kontak pada **`PC7` (`RELAY_STATUS`)**.
- **RTC Calibration Output**:
  - Pin **`PB2` (`RTC_CAL_OUT`)** dikonfigurasikan mengeluarkan frekuensi presisi 512 Hz (`RTC_CALIBOUTPUT_512HZ` via `RTC_OUTPUT_REMAP_POS1`).

