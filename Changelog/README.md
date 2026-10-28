# Riwayat dan Log Perubahan Firmware Smart Meter 3-Fasa (ALLIN)

Dokumen ini adalah indeks utama pencatatan seluruh perubahan, penyesuaian, dan perbaikan kode yang dilakukan pada proyek firmware **ALLIN (STM32U585VIT6)**. Setiap ada perubahan pada file source code atau konfigurasi, catatan baru akan selalu ditambahkan ke dalam folder ini agar Anda dapat melakukan audit dan tracing secara transparan.

---

## Daftar Riwayat Perubahan (Changelog Index)

| ID Log | Tanggal | Topik Utama | File yang Terdampak | Status |
| :--- | :--- | :--- | :--- | :--- |
| [**LOG-001**](001_arsitektur_dan_tiga_pilar.md) | 26-10-2026 | Integrasi Tiga Pilar (E1 Metrologi, E2 DLMS/COSEM, E3 HMI/Tamper) & Konstrain Pinout | `NonSecure/Core/Src/main.c`, `NonSecure/CMakeLists.txt` | Selesai |
| [**LOG-002**](002_freertos_trustzone.md) | 26-10-2026 | Integrasi FreeRTOS v11 CMSIS-RTOS2 & TrustZone ARM Cortex-M33 Secure Port | `Secure/CMakeLists.txt`, `NonSecure/Core/Src/app_freertos.c`, `Middlewares/Third_Party/FreeRTOS/...` | Selesai |
| [**LOG-003**](003_low_power_tickless_idle.md) | 26-10-2026 | Aktivasi Mode Hemat Daya (Low-Power Tickless Idle & Last Gasp) | `NonSecure/Core/Inc/FreeRTOSConfig.h`, `NonSecure/Core/Src/main.c` | Selesai |
| [**LOG-004**](004_telemetri_periferal_lengkap.md) | 27-10-2026 | Penyempurnaan Telemetri Periferal, Pembersihan Dead Code `while(1)`, WDI Heartbeat, ADC4 Baterai, DLMS USART3 | `NonSecure/Core/Src/main.c` | Selesai |
| [**LOG-005**](005_penyempurnaan_power_rail_dan_exti.md) | 27-10-2026 | Penyempurnaan Power Rail Periferal, Chip Select SPI, Interupsi ADE9000 & Validasi Relai | `NonSecure/Core/Src/main.c` | Selesai |
| [**LOG-006**](006_perbaikan_presisi_waktu_dan_energi.md) | 27-10-2026 | Perbaikan Presisi Waktu RTC, Akumulasi Energi Wh (MilliJoule), dan Sinkronisasi Timer Pembatas Beban | `NonSecure/Core/Src/main.c` | Selesai |
| [**LOG-007**](007_modularisasi_arsitektur_e1_e2_e3.md) | 27-10-2026 | Modularisasi Arsitektur Tiga Pilar (E1 Metrologi, E2 DLMS/COSEM, E3 Meter App) & Pembersihan main.c | `E1/Metrology/*`, `E2/Inc/*`, `E2/Src/*`, `E3/app/*`, `NonSecure/Core/Src/main.c`, `NonSecure/CMakeLists.txt` | Selesai |
| [**LOG-008**](008_optimasi_throughput_spi_dan_freertos_stack.md) | 27-10-2026 | Optimasi Throughput SPI ADE9000 & Proteksi Stack/Heap FreeRTOS | `E1/SPI/SPI.c`, `NonSecure/Core/Inc/FreeRTOSConfig.h`, `NonSecure/Core/Src/app_freertos.c` | Selesai |
| [**LOG-009**](009_penyempurnaan_ui_tamper_relay_dan_metrologi.md) | 27-10-2026 | Penyempurnaan Rantai Fungsional HMI Tamper, Rekoneksi Relai, & Metrologi | `E1/Metrology/e1_metrology_service.c`, `E3/app/e3_meter_app.c` | Selesai |
| [**LOG-010**](010_penyesuaian_lengkap_dokumen_acuan.md) | 27-10-2026 | Audit & Penyesuaian Lengkap Firmware terhadap Dokumen Acuan (`PIN.docx`, `Desain Sistem.pdf`, `Spesifikasi .pdf`, `SPLN D3.006:2021`) | `ALLIN.ioc`, `Secure/*`, `Secure_nsclib/*`, `NonSecure/*`, `E1/*`, `E2/*`, `E3/*` | Selesai |
| [**LOG-011**](011_integrasi_ade9000_metrology_2.md) | 27-10-2026 | Integrasi Pembaruan Metrologi `ADE9000_Metrology_2` (Daya Reaktif, Semu, PF, dan Energi Reaktif 4-Kuadran) | `E1/*`, `E2/*`, `E3/app/display/inc/display.h`, `scratch/run_all_tests.py` | Selesai |
| [**LOG-012**](012_penambahan_halaman_energi_reaktif_e3.md) | 27-10-2026 | Penambahan Halaman Tampilan Energi Reaktif (ER_BIL) SPLN D3.006 Tabel E.1 No 8 pada Pilar E3 (Display & HMI) | `E3/app/display/inc/display.h`, `E3/app/display/src/display.c`, `E3/tests/test_spln_display_tamper.c`, `E3/CMakeLists.txt` | Selesai |
| [**LOG-013**](013_integrasi_energi_tou_wbp_lwbp_dan_oled_ssd1306.md) | 27-10-2026 | Integrasi Energi Aktif per Tarif (WBP & LWBP), Koneksi Driver Layar OLED SSD1306 (I2C3), dan Feedback Audio Tombol | `E3/app/display/*`, `E3/drivers/oled/*`, `E3/app/e3_meter_app.c`, `NonSecure/CMakeLists.txt` | Selesai |
| [**LOG-014**](014_implementasi_audio_cadence_buzzer_tahap2.md) | 27-10-2026 | Implementasi Audio Cadence Buzzer SPLN D3.006 (Tahap 2) & Integrasi Feedback Tombol | `E3/drivers/buzzer/*`, `E3/app/e3_meter_app.c`, `E3/tests/test_spln_display_tamper.c`, `E3/CMakeLists.txt` | Selesai |
| [**LOG-015**](015_finalisasi_fitur_penagihan_lanjutan_tahap3.md) | 27-10-2026 | Finalisasi Fitur Penagihan Lanjutan (Tahap 3), Audit Ketahanan Ekstrem & Pemantapan Versi Final Firmware | `E3/app/profiling/*`, `E3/app/display/*`, `E3/app/e3_meter_app.*`, `E3/tests/*`, `E3/CMakeLists.txt` | Selesai |
| [**LOG-016**](016_penyempurnaan_carousel_hmi_spln_lengkap.md) | 27-10-2026 | Penyempurnaan 100% Carousel HMI SPLN D3.006: 2021 (Auto-Scroll Tabel E.1 & Manual-Scroll Tabel E.2) | `E3/app/display/*`, `E3/app/e3_meter_app.c`, `E3/tests/*` | Selesai |
| [**LOG-017**](017_penambahan_carousel_ea_exp3_ea_imp_spln.md) | 27-10-2026 | Penambahan Halaman Carousel EA_EXP3 (Tarif 3) & EA_IMP (Energi Impor) SPLN D3.006 (100% Tabel E.1) | `E3/app/display/*`, `E3/app/e3_meter_app.c`, `E3/tests/*` | Selesai |
| [**LOG-018**](018_kelengkapan_penuh_tabel_e2_spln.md) | 27-10-2026 | Kelengkapan Penuh 100% Parameter Tampilan Scrolling Tombol (Tabel E.2) SPLN D3.006: 2021 | `E3/app/display/*`, `E3/tests/*` | Selesai |
| [**LOG-019**](019_catatan_audit_kekurangan_file_awal_e1_e2_e3.md) | 27-10-2026 | Catatan & Laporan Audit Kekurangan File Awal Pilar E1 (Metrologi), E2 (DLMS), dan E3 (HMI/Tamper) | Seluruh Pilar E1, E2, E3 | Selesai |
| [**LOG-020**](020_implementasi_tombol_simultan_reconnect_e3.md) | 27-10-2026 | Implementasi Konfirmasi Tombol Simultan (▲ + ▼) untuk Rekoneksi Relai (SPLN D3.006 Tabel 5 No 12) | `E3/app/*`, `NonSecure/Core/Src/main.c`, `E3/tests/*` | Selesai |
| [**LOG-021**](021_implementasi_rotasi_multi_error_3s_spln.md) | 27-10-2026 | Implementasi Rotasi Kode Error Bergantian per 3 Detik (SPLN D3.006 Butir 6.11) | `E3/app/display/*`, `E3/tests/*` | Selesai |
| [**LOG-022**](022_implementasi_indikator_lo_hi_volt_spln.md) | 27-10-2026 | Implementasi Peringatan Tampilan "Lo Volt" dan "Hi Volt" (SPLN D3.006: 2021 Butir 10.3 Tabel 9) | `E3/app/display/*`, `E3/tests/*` | Selesai |
| [**LOG-023**](023_koreksi_perilaku_tamper_medan_magnet_dan_tutup_meter.md) | 27-10-2026 | Koreksi Perilaku Sabotase Medan Magnet & Tutup Meter (SPLN D3.006: 2021 Tabel 6) | `E3/app/tamper/*`, `E3/app/e3_meter_app.c`, `E3/tests/*` | Selesai |
| [**LOG-024**](024_respons_relai_alarm_internal_hardware_tabel4.md) | 27-10-2026 | Respons Buka Relai & Indikator LED Kuning pada Alarm Internal Hardware (SPLN D3.006: 2021 Tabel 4) | `E3/app/tamper/*`, `E3/app/display/*`, `E3/app/e3_meter_app.*`, `E3/tests/*` | Selesai |
| [**LOG-025**](025_proteksi_cutoff_relai_tegangan_rendah_dan_lebih.md) | 27-10-2026 | Proteksi Cut-Off Relai Tegangan Rendah (<110V) & Tegangan Lebih (>280V) (SPLN D3.006: 2021 Butir 10.3 Tabel 9) | `E3/app/fsm/*`, `E3/app/e3_meter_app.c`, `E3/tests/*` | Selesai |
| [**LOG-026**](026_tiga_register_log_fifo_dan_error_register.md) | 27-10-2026 | Tiga Register Log FIFO Non-Volatile Terpisah & Error Register (SPLN D3.006: 2021 Butir 12.3 Tabel 18) | `E3/app/tamper/*`, `E3/app/e3_meter_app.*`, `E3/tests/*` | Selesai |
| [**LOG-027**](027_perbaikan_struktur_axdr_dlms_load_profile.md) | 27-10-2026 | Perbaikan Ketidakcocokan Elemen Struktur A-XDR DLMS Load Profile & Tamper Log | `E3/app/profiling/*`, `E3/tests/*` | Selesai |
| [**LOG-028**](028_parser_perintah_konsol_serial_cli.md) | 27-10-2026 | Parser Perintah Konsol Serial Interaktif (CLI) di console_io.c | `E3/app/console/*`, `E3/app/e3_meter_app.c`, `E3/tests/*` | Selesai |
| [**LOG-029**](029_audit_akhir_integrasi_sudut_fasa_tamper_dan_eob.md) | 27-10-2026 | Audit Akhir & Integrasi Sudut Fasa E1, Evaluasi Tamper Elektrikal Lengkap (ERR21/ERR22/ERR27), serta Pemicu Otomatis End-of-Billing (EoB) | `E1/Metrology/e1_metrology_service.c`, `E3/app/e3_meter_app.c`, `E3/tests/test_fsm_statechart.c` | Selesai |
| [**LOG-030**](030_arsitektur_board_profile_config.md) | 28-10-2026 | Arsitektur Fleksibilitas Perangkat Keras via Board Profile Configuration (HAL) untuk Penggantian Display (OLED vs Segment LCD), Relai, NVRAM & Sensor Tamper | `E3/app/board_profile_config.h`, `E3/app/e3_meter_app.*`, `E3/CMakeLists.txt`, `E3/tests/test_board_profile.c` | Selesai |
| [**LOG-031**](031_pemindahan_glue_code_meter_system_bridge.md) | 28-10-2026 | Pemisahan Glue Code dari main.c ke Dedicated System Bridge Module (`meter_system_bridge`) | `NonSecure/Core/Inc/meter_system_bridge.h`, `NonSecure/Core/Src/meter_system_bridge.c`, `NonSecure/Core/Src/main.c`, `NonSecure/CMakeLists.txt` | Selesai |
| [**LOG-032**](032_integrasi_hardware_rtc_oled_dan_tou.md) | 28-10-2026 | Integrasi Hardware RTC STM32U5 ke Jam/Tanggal Layar OLED & Otomasi Tarif ToU (WBP/LWBP) | `Secure/Core/Src/*`, `E3/app/e3_meter_app.*`, `E3/app/console/*`, `E3/tests/*` | Selesai |
| [**LOG-033**](033_prosedur_listrik_padam_last_gasp_oled.md) | 28-10-2026 | Prosedur Listrik Padam (Last Gasp) Layar OLED, Emergency Flash Checkpoint, Off-Power Standby 10 Detik & CLI Simulasi (`acfail` & `button`) | `E3/app/display/*`, `E3/drivers/oled/*`, `E3/app/profiling/*`, `E3/app/e3_meter_app.*`, `E3/app/console/*`, `E3/tests/*` | Selesai |
| [**LOG-034**](034_arsitektur_sakelar_beban_tiruan_metrologi.md) | 28-10-2026 | Arsitektur Sakelar Diagnostik Metrologi Tiruan (Synthetic Load Generator) via CLI (`load`, `volt`, `curr`, `synth`) & Isolasi Bersih 0-Byte Produksi Massal | `E3/app/board_profile_config.h`, `E3/app/console/*`, `E3/app/e3_meter_app.*`, `NonSecure/Core/Src/meter_system_bridge.c`, `E3/tests/*` | Selesai |
| [**LOG-035**](035_visualisasi_ikon_grafis_layar_oled.md) | 28-10-2026 | Visualisasi Ikon Grafis Status Bar Khusus Layar OLED (Relai Kontaktor, Baterai Gauge, Tamper Warning Triangle & Tarif ToU Badge) | `E3/app/display/*`, `E3/drivers/oled/*` | Selesai |
| [**LOG-036**](036_manajemen_power_saving_stop2_mode.md) | 28-10-2026 | Manajemen Power Saving Ekstrem STM32U5 (Stop 2 Mode saat Listrik Padam, Wakeup EXTI/RTC & Restorasi Jam Adaptif) | `E3/drivers/power/*`, `E3/app/board_profile_config.h`, `E3/app/e3_meter_app.*`, `E3/app/console/*`, `E3/tests/*`, `NonSecure/CMakeLists.txt` | Selesai |

---

## Aturan Standar Pencatatan (Protocol)
Setiap catatan perubahan wajib menyertakan:
1. **Latar Belakang / Alasan Perubahan**: Mengapa baris kode tersebut diubah atau ditambahkan.
2. **Daftar File & Baris yang Berubah**: Path file lengkap dan lokasi blok kode STM32CubeMX (`USER CODE BEGIN` / `USER CODE END`).
3. **Diff / Perbandingan Kode Sebelum vs Sesudah**: Potongan kode nyata sebelum dan setelah perubahan.
4. **Dampak terhadap Hardware & Kompilasi**: Hasil build RAM/ROM dan pengaruh terhadap pinout STM32U585.
