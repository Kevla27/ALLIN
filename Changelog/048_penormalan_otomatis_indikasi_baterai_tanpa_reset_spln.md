# LOG-048: Penormalan Otomatis Indikasi Baterai Tanpa Reset Sesuai Standar Layar LCD SPLN D3.006:2021 Butir 6.11 Halaman 39

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Application, UI/Display OLED SSD1306, Tamper & Hardware Internal Alarm, Board Configuration)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan klausul **SPLN D3.006:2021 Butir 6.11 Layar Tampilan (Halaman 39, Catatan Simbol LCD Tiga Fasa)**:
* `▪` **[Simbol Baterai]**: *"Menginformasikan status baterai normal. Bila baterai rusak/mencapai nilai threshold-nya (2,7 V), maka simbol berkedip. \*)"*
* `CATATAN: *) Indikasi baterai kembali normal dapat dilakukan secara otomatis tanpa me-reset."*

Dan pada **SPLN D3.006:2021 Tabel 4 (Halaman 44)**:
* `3 | Low battery (threshold 2,7 V) | Rekam data | Simbol baterai berkedip | - | ERR03 | Aktif | Penggantian baterai`

Di lapangan, saat baterai meter lama yang habis/lemah diganti dengan baterai baru oleh petugas teknisi PLN, meter harus mendeteksi pemulihan tegangan baterai secara mandiri dan mengembalikan indikasi baterai ke status normal tanpa mengharuskan pemadaman daya atau reset mikrokontroler (*zero-downtime hot-swap battery recovery*).

---

## 2. Permasalahan Sebelum Perbaikan

1. **Alarm Baterai Terkunci (*Latched Indefinitely*):**  
   Saat tegangan baterai drop di bawah 2,7 V, flag `SPLN_ALARM_LOW_BATTERY` di-set ke dalam `s_internal_alarm_mask`. Namun, ketika baterai baru dipasang atau tegangan kembali normal di atas 2,7 V, kode firmware tidak pernah menghapus flag tersebut karena fungsi pemulihan alarm internal (`clear_internal_alarm`) belum tersedia.
2. **Layar Tetap Menampilkan ERR03 & Berkedip:**  
   Akibat alarm yang terkunci, tampilan OLED tetap menampilkan ikon baterai berkedip dan kode `ERR03` tetap berputar dalam carousel kesalahan hingga meter di-reset secara manual.
3. **Register Error & FIFO Log:**  
   Event pemulihan (*deassertion*, `status_flags = 0`) belum tercatat ke dalam FIFO log alarm saat baterai kembali sehat, dan bit alarm pada *Error Register bitstring* tetap tertinggal dalam kondisi aktif.

---

## 3. Rincian Perubahan Implementasi

### A. Penambahan API Pemulihan Alarm Internal Hardware (`e3_meter_app.h` & `e3_meter_app.c`)
* Menambahkan fungsi `void e3_meter_app_clear_internal_alarm(uint32_t alarm_flags)`:
  * Menghapus bit alarm tertentu dari `s_internal_alarm_mask`.
  * Memanggil `log_internal_alarm_recovery()` untuk mencatat event deassertion ke `alarm_log_fifo` dengan parameter nilai tegangan pulih (`status_flags = 0`).
  * Membersihkan bit alarm terkait pada `error_register_bitstring`.
  * Memperbarui konteks tampilan `display_set_internal_alarm(&s_disp_ctx, s_internal_alarm_mask)`.

### B. Evaluasi Telemetri Otomatis dengan Histeresis 50 mV (`e3_meter_app.c`)
* Di dalam siklus periodik telemetri ADC4 (setiap 5 detik):
  * **Ambang Batas Baterai Lemah:** $V_{\text{bat}} < 2700\text{ mV}$ (`BOARD_CFG_ADC4_BATT_LOW_MV`) $\rightarrow$ Memicu `SPLN_ALARM_LOW_BATTERY` (`ERR03`, simbol berkedip).
  * **Ambang Batas Pemulihan Otomatis:** $V_{\text{bat}} \ge 2750\text{ mV}$ (`BOARD_CFG_ADC4_BATT_REC_MV`, histeresis 50 mV) dan alarm sedang aktif $\rightarrow$ Memanggil `e3_meter_app_clear_internal_alarm(SPLN_ALARM_LOW_BATTERY)` secara otomatis.
* Menambahkan logika pemulihan otomatis serupa untuk superkapasitor ($V_{\text{cap}} \ge 2100\text{ mV}$ per `BOARD_CFG_ADC4_SUPERCAP_REC_MV`) dan LSE Clock (saat clock sehat kembali).

### C. Konfigurasi Profil Papan Terpusat (`board_profile_config.h`)
* Menambahkan konstanta batas pemulihan otomatis pada ketiga profil papan (`BOARD_PROFILE_PROTOTYPE_OLED`, `BOARD_PROFILE_PRODUCTION_SEGLCD`, dan `BOARD_PROFILE_HEADLESS_BENCH`):
  * `#define BOARD_CFG_ADC4_BATT_REC_MV 2750U`
  * `#define BOARD_CFG_ADC4_SUPERCAP_REC_MV 2100U`

### D. Interaktivitas Serial CLI (`console_io.h` & `console_io.c`)
* Memperluas perintah `alarm clear [mask_hex]`: Memungkinkan pembersihan alarm secara spesifik atau menyeluruh tanpa reboot.
* Memperluas perintah `power`:
  * Menampilkan tegangan baterai terkini secara real-time: `Tegangan Baterai : 3.600 V (NORMAL, Ambang Normal >= 2.70V)`.
  * Menambahkan sub-perintah `power batt <mv>` untuk menyimulasikan drop dan pemulihan tegangan baterai via terminal CLI.

---

## 4. Pengujian dan Verifikasi

1. **Unit Test Layout & HMI Display (`test_spln_display_tamper.c`):**
   * Menambahkan `[UJI 15] Verifikasi Penormalan Indikasi Baterai Tanpa Reset (SPLN Hal 39)`:
     * Kondisi $V_{\text{bat}} \le 2,7\text{ V}$: `frame.is_battery_low == true` (berkedip) dan `frame.unit == "ERR03"`.
     * Penormalan baterai: `frame.is_battery_low == false` (solid 2 garis level), `frame.unit != "ERR03"`, dan `alarm_icon_active == false`.
2. **Unit Test FIFO Log & Error Register (`test_e3_app.c`):**
   * Memvalidasi pemulihan alarm baterai (`spln_log_record_alarm` dengan `is_asserted = false`): bit alarm baterai pada `error_register_bitstring` terhapus bersih menjadi 0.
3. **Unit Test Serial CLI (`test_console_cli.c`):**
   * Memvalidasi perintah `alarm clear 0x08` (menghapus spesifik alarm baterai) dan simulasi `power batt 2650`.
4. **Unit Test Konfigurasi Papan (`test_board_profile.c`):**
   * Memvalidasi konstanta `BOARD_CFG_ADC4_BATT_REC_MV == 2750U` dan `BOARD_CFG_ADC4_SUPERCAP_REC_MV == 2100U`.
5. **Kompilasi Target ARM Cortex-M33:**
   * Firmware `ALLIN_S.elf` dan `ALLIN_NS.elf` sukses dikompilasi dengan ARM GCC tanpa error maupun warning.
