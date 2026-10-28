# LOG-038: Stempel Magic & Pembacaan Fisik SPI2 pada Checkpoint NVRAM (`nvram_storage.c` & `e3_meter_app.c`)

**Tanggal:** 28 Oktober 2026  
**Status:** Selesai & Terverifikasi Kompilasi (ARM GCC Secure + NonSecure, 11/11 CTest Passed 100%)

---

## 1. Latar Belakang & Masalah Teknis

Pada pengujian dan audit pemulihan data setelah siklus daya padam (*power-cycle* / pemadaman total listrik):
1. **Kegagalan Stempel Magic (*Missing Magic Stamp*):**
   - Struktur data `nvram_energy_checkpoint_t` mendefinisikan konstanta stempel integritas `NVRAM_ENERGY_CHECKPOINT_MAGIC = 0x454E4759` (`"ENGY"`).
   - Pada saat fungsi `e3_meter_app_on_ac_fail()` dipanggil, variabel `energy_cp` diinisialisasi `{0}` namun field `energy_cp.magic` tidak pernah diisi.
   - Akibatnya, data checkpoint tersimpan dengan `magic = 0x00000000`. Ketika meter menyala kembali dan memanggil `nvram_load_energy_checkpoint()`, validasi `out_checkpoint->magic == NVRAM_ENERGY_CHECKPOINT_MAGIC` selalu mengembalikan `false`, sehingga data stand energi kWh dan tarif ToU hasil simpanan darurat diabaikan.
2. **Ketiadaan Pembacaan Fisik SPI2 (*Missing Physical SPI2 Read*):**
   - Pada `nvram_storage.c`, penulisan ke memori eksternal sudah mengimplementasikan `spi2_flash_write_bytes()` via `HAL_SPI_Transmit(&hspi2, ...)`.
   - Namun, fungsi `spi2_flash_read_bytes()` belum tersedia, dan `nvram_load_energy_checkpoint()` hanya membaca dari variabel RAM mirror `simulated_energy_checkpoint_flash`.
   - Ketika meter mengalami mati total (*hard reboot* / pemadaman melebihi ketahanan superkapasitor), variabel RAM mirror di BSS section akan ter-reset ke 0, sehingga tanpa pembacaan fisik dari chip SPI NOR Flash / FRAM di bus `SPI2` (`PB12`–`PB15`), meter tidak dapat memulihkan stand kWh sebelumnya.

---

## 2. Solusi Rekayasa & Implementasi

1. **Implementasi Pembacaan Fisik SPI2 (`spi2_flash_read_bytes`):**
   - Menggunakan perintah standar SPI NOR Flash / FRAM `SPI_FLASH_CMD_READ = 0x03U`.
   - Mengirim header 4-byte `[0x03, A23..A16, A15..A8, A7..A0]` dengan kontrol `FLASH_SPI2_CS` (`PB12`).
   - Menerima payload data byte via `HAL_SPI_Receive(&hspi2, data, len, 50U)` dengan batas *finite non-blocking timeout* 50 ms.
2. **Pemberian Stempel Magic Otomatis (*Auto Magic Stamping*):**
   - Di `nvram_storage.c`: Dalam fungsi `nvram_save_energy_checkpoint()`, stempel `magic` dijamin selalu diset ke `NVRAM_ENERGY_CHECKPOINT_MAGIC` sebelum ditulis ke RAM mirror maupun fisik SPI2 Flash.
   - Di `e3_meter_app.c`: Dalam `e3_meter_app_on_ac_fail()` dan saat interval periodik 15-menit Load Profile, field `energy_cp.magic = NVRAM_ENERGY_CHECKPOINT_MAGIC;` diisi secara eksplisit.
3. **Validasi & Pemulihan Fisik Saat Booting:**
   - Dalam `nvram_load_energy_checkpoint()`, target hardware membaca memori fisik pada offset `0x001000U`. Jika stempel `flash_cp.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC`, data fisik disalin ke RAM mirror dan diserahkan ke aplikasi.
   - Dalam `e3_meter_app_init()`, data checkpoint yang berhasil dimuat langsung mengisi `s_last_active_energy_wh`, `s_disp_ctx.meas_buffer.active_energy_import_wh`, `s_disp_ctx.meas_buffer.active_energy_wh`, `s_disp_ctx.meas_buffer.reactive_energy_varh`, dan seluruh zona tarif ToU `s_tou_tariff_wh[]`.
4. **Pembaruan Pembacaan Fisik Tamper & Load Profile:**
   - Menambahkan pembacaan fisik SPI2 untuk `nvram_load_tamper_log_snapshot()` pada offset `0x002000U` dan `nvram_read_load_profile_entry()` pada offset `0x004000U + (idx * sizeof(entry))`.
5. **Penambahan Suite Pengujian Otomatis CTest #11:**
   - Menambahkan `test_nvram_energy_checkpoint_magic_stamp()` ke `E3/tests/test_nvram_storage.c`.
   - Mendaftarkan target `test_nvram_storage` ke `E3/CMakeLists.txt` (11/11 tests pass 100%).

---

## 3. Berkas yang Diubah

1. `E3/app/profiling/src/nvram_storage.c`: Implementasi `spi2_flash_read_bytes()`, auto-stamping magic `NVRAM_ENERGY_CHECKPOINT_MAGIC`, dan integrasi pembacaan fisik SPI2 Flash.
2. `E3/app/e3_meter_app.c`: Inisialisasi eksplisit stempel magic pada checkpoint AC Fail & checkpoint periodik 15-menit, serta pemulihan stand buffer display saat booting.
3. `E3/tests/test_nvram_storage.c`: Penambahan unit test `test_nvram_energy_checkpoint_magic_stamp()`.
4. `E3/CMakeLists.txt`: Registrasi pengujian `Unit_E3_NVRAM_Storage` ke dalam runner CTest.

---

## 4. Hasil Verifikasi & Pengujian

- **Host CTest (x86_64 GCC):**
  - `11/11 Test Suites Passed 100%` (0 error, 0 fail).
  - Test `#11: Unit_E3_NVRAM_Storage` lulus pengujian validasi magic stamp `0x454E4759` ("ENGY").
- **Target ARM Cortex-M33 (ARM GCC 14.3.1):**
  - `ALLIN_S.elf`: `23,656 B ROM / 12,048 B RAM` (Selesai tanpa peringatan).
  - `ALLIN_NS.elf`: `171,856 B ROM (16.39%) / 65,160 B RAM (12.43%)` (Kompilasi & Link sukses).

