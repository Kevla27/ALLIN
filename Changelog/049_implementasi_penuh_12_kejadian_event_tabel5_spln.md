# LOG-049: Verifikasi dan Implementasi Penuh 12 Kejadian Event Sesuai SPLN D3.006:2021 Tabel 5 (Halaman 45)

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Application, Tamper & Event Manager, Power Limiter, Display OLED, Relay FSM)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan klausul **SPLN D3.006:2021 Butir 7.4 Respons Terhadap Event (Halaman 45, Tabel 5)**:
Meter harus mampu mendeteksi kejadian event, serta menampilkan dan merekam indikator dan waktu kejadian event sesuai **Tabel 5**:

| No. urut | Jenis kejadian | Respons meter | Keterangan |
| :---: | :--- | :--- | :--- |
| **1** | Power supply failure | Rekam data | Kondisi dimana pasokan listrik dari jaringan terputus |
| **2** | Power supply up | Rekam data | Kondisi dimana pasokan listrik dari jaringan masuk kembali |
| **3** | Perubahan setting time-of-use (ToU) | Rekam data | Parameter konfigurasi zona tarif ToU diperbarui |
| **4** | Perubahan setting (selain ToU) | Rekam data | Parameter meter selain ToU (IDPEL, batas daya, baudrate, dll.) |
| **5** | Stop-measurement | Rekam data | Deteksi anomali pengukuran atau sampling berhenti, dsb. |
| **7** | Perubahan status I/O (termasuk trigger test) | Rekam data | Digital input kontrol bantu (AUX IN1 / AUX IN2) |
| **8** | Perubahan status relai/kontaktor internal | Rekam data | Relai membuka/menutup (trip, disconnect, reconnect) |
| **9** | Meter reset | Rekam data | Reset mikrokontroler (Power-on, Software, Watchdog) |
| **10** | Perubahan tanggal dan waktu | Rekam data | Sinkronisasi / pembaruan kalender RTC |
| **11** | Asymmetric power | Rekam data | Deviasi arus fasa > 30% per rumus Butir 7.4 |
| **12** | Meter ready remote reconnect | • Teks "READY ON"<br>• LED kuning berkedip<br>• Rekam data | Menunggu aksi tekan tombol atas dan bawah secara bersamaan |
| **13** | Power limiter threshold exceeded | • Rekam data | Beban melebihi batas daya dan durasi toleransi habis |

*(Catatan: Penomoran resmi SPLN melompati angka 6, sehingga terdapat total 12 jenis kejadian event).*

---

## 2. Hasil Audit dan Pemenuhan Implementasi

Sebelum audit ini, sebagian besar event penting (seperti Power Fail, Power Up, Relai, Ready Reconnect, Asymmetric Power, Power Limiter, I/O Change, dan Time Change) telah aktif di sistem. Namun event administratif dan anomali sampling berikut belum terhubung secara penuh ke aplikasi runtime:
1. **No. 9 (Meter Reset):** Event belum dicatat otomatis saat inisialisasi boot firmware (`e3_meter_app_init()`).
2. **No. 3 & 4 (Perubahan Setting ToU & Non-ToU):** Belum memiliki antarmuka notifikasi terpadu saat konfigurasi diubah.
3. **No. 5 (Stop-Measurement):** Belum terikat otomatis saat deteksi anomali sampling ADC (`SPLN_ALARM_ADC_ERROR`) terjadi.

---

## 3. Rincian Perubahan Implementasi

### A. Pencatatan Otomatis Meter Reset pada Saat Boot (`e3_meter_app.c`)
* Di dalam `e3_meter_app_init()`:
  * Begitu NVM tamper snapshot selesai dimuat, sistem langsung merekam kejadian `SPLN_LOG_EVENT_METER_RESET` ke Event Log FIFO:
    ```c
    spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_METER_RESET, true, s_now_epoch_sec, 0);
    ```

### B. Binding Otomatis Stop-Measurement (`e3_meter_app.c`)
* Di dalam fungsi `log_internal_alarm_events()` dan `log_internal_alarm_recovery()`:
  * Saat `SPLN_ALARM_ADC_ERROR` terdeteksi aktif, secara otomatis mencatat `SPLN_LOG_EVENT_STOP_MEASUREMENT` (`is_asserted = true`).
  * Saat sampling ADC pulih, secara otomatis mencatat akhir kejadian (`is_asserted = false`).

### C. Penambahan API Notifikasi Konfigurasi (`e3_meter_app.h` & `e3_meter_app.c`)
* `void e3_meter_app_notify_tou_change(void)`: Merekam `SPLN_LOG_EVENT_TOU_CHANGE`.
* `void e3_meter_app_notify_setting_change(uint32_t setting_id)`: Merekam `SPLN_LOG_EVENT_NON_TOU_CHANGE`.
* `void e3_meter_app_notify_stop_measurement(bool is_stopped, uint32_t reason)`: Merekam `SPLN_LOG_EVENT_STOP_MEASUREMENT`.

### D. Verifikasi Lengkap Respons Event No. 12 (Meter Ready Remote Reconnect)
* **Tampilan Teks:** Menampilkan banner `"READY ON"` pada status relai reconnect (`DISP_PAGE_RELAY_READY_ON`).
* **Indikator Grafis:** Ikon sakelar terbuka berkedip di OLED status bar (`oled_draw_relay_icon`).
* **LED Kuning PB7:** Berkedip periodik 1 Hz (`e3_meter_app.c` baris 748-753).
* **Rekam Data:** Mencatat `SPLN_LOG_EVENT_READY_RECONNECT` ke Event Log FIFO.
* **Aksi Lapangan:** Mengharuskan penekanan tombol Atas dan Bawah secara bersamaan (`BUTTON_DIR_BOTH` / `PA0 & PB6`) untuk menyambungkan kembali beban.

---

## 4. Pengujian dan Verifikasi

1. **Unit Test Event Log FIFO (`test_e3_app.c`):**
   * Memvalidasi seluruh 12 jenis kejadian event dari No. 1 s.d No. 13 (Tabel 5) direkam ke dalam `event_log_fifo` dan memetakan 12 bit event pada *Error Register bitstring* (bit 8..19).
2. **Unit Test FSM Relai & Reconnect (`test_power_relay.c`):**
   * Memvalidasi transisi state `RELAY_STATE_READY_ON` dan penolakan reconnect sebelum aksi tombol simultan (`BUTTON_DIR_BOTH`).
3. **Hasil CTest:** 12/12 test suites lulus 100% (0 failed).
4. **Kompilasi Target ARM Cortex-M33 (STM32U585):** Berhasil di-link bersih tanpa error.
