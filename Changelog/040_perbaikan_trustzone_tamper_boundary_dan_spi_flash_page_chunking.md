# LOG-040: Perbaikan TrustZone Tamper Boundary & SPI Flash Page-Program Chunking 256-Byte

**Tanggal:** 28 Oktober 2026  
**Status:** SELESAI & TERVERIFIKASI  
**Target:** STM32U585VIT6 (ARM Cortex-M33 TrustZone), Pilar E3, NVRAM W25Qxx SPI2  

---

## 1. Latar Belakang & Identifikasi Bug Mendalam

Dalam audit mendalam terhadap seluruh basis kode ALLIN (Pilar E1, E2, E3, Secure World, dan Non-Secure World), ditemukan **2 potensi bug kritis pada level perangkat keras fisik**:

### A. Bug 1: Pelanggaran Batas TrustZone & False Positive Tamper Active-Low (PC13, PE6, PE4)
* **Lokasi Berkas:** [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c#L490-L506) dan [`E3/drivers/power/src/bsp_power_mode.c`](file:///g:/PPPP/ALLIN/E3/drivers/power/src/bsp_power_mode.c#L246-L252)
* **Akar Masalah:**
  1. Pin sakelar sensor proteksi sabotase fisik:
     - `PC13` (`TAMP_CASE_OPEN` / TAMP_IN1)
     - `PE6` (`TAMP_TERM_COVER` / TAMP_IN3)
     - `PE4` (`SENSOR_MAGNET` / TAMP_IN7)
     dikonfigurasi sebagai **Secure GPIO** (`GPIO_PIN_SEC`) dalam arsitektur TrustZone (`Secure/Core/Src/main.c`).
  2. Berdasarkan spesifikasi arsitektur ARMv8-M Security Attribution Unit (SAU/IDAU) pada STM32U5, pembacaan register `IDR` pin yang dialokasikan ke Secure World dari kode Non-Secure (`HAL_GPIO_ReadPin()`) diperlakukan sebagai **RAZ (Read-As-Zero)** atau bernilai 0 (`GPIO_PIN_RESET`).
  3. Pada profil papan produksi massal ([`BOARD_PROFILE_PRODUCTION_SEGLCD`](file:///g:/PPPP/ALLIN/E3/app/board_profile_config.h#L131-L133)), sensor penutup meter menggunakan sakelar mikro tipe *Normally Closed* (Active-Low / `BOARD_CFG_TAMPER_CASE_OPEN_LVL == 0`).
  4. Akibatnya, evaluasi `(HAL_GPIO_ReadPin(...) == GPIO_PIN_RESET)` selalu menghasilkan nilai `true` secara permanen saat boot. Hal ini langsung memicu sabotase `TAMPER_VECTOR_CASE_OPEN`, memutuskan relai beban pelanggan, mencatat `RUSAK` di log, dan mengunci meter sejak pertama kali dinyalakan.

### B. Bug 2: Data Corruption Akibat Wrap-Around Halaman 256-Byte SPI Flash NOR
* **Lokasi Berkas:** [`E3/app/profiling/src/nvram_storage.c`](file:///g:/PPPP/ALLIN/E3/app/profiling/src/nvram_storage.c#L18-L58)
* **Akar Masalah:**
  1. Chip SPI NOR Flash eksternal (seri Winbond W25Qxx, Macronix, Micron) memiliki batasan perangkat keras fisik berupa ukuran halaman (*page size*) **256 byte** (`0xXX00` – `0xXXFF`).
  2. Perintah *Page Program* (`0x02` PP) mensyaratkan transmisi maksimal hingga batas akhir halaman 256 byte. Jika transmisi data melebihi 256 byte dalam satu siklus CS, alamat internal SPI flash akan mengalami **wrap-around** ke byte awal halaman yang sama (`0xXX00`), menimpa byte awal berulang kali dan merusak data.
  3. Fungsi [`nvram_save_tamper_log_snapshot()`](file:///g:/PPPP/ALLIN/E3/app/profiling/src/nvram_storage.c) menyimpan struktur `tamper_context_t` yang berukuran sekitar ~1.800 byte (termasuk buffer 3 FIFO Log SPLN Tabel 18).
  4. Implementasi awal memanggil `HAL_SPI_Transmit(&hspi2, ..., len, 20U)` dalam satu siklus `0x02` tunggal tanpa pemecahan (*chunking* per 256-byte page boundary) dan tanpa *polling* status busy (`0x05` RDSR1 WIP bit). Akibatnya, seluruh struktur data snapshot tamper log dan profil beban berukuran besar akan mengalami korupsi data permanen di memori flash.

---

## 2. Rincian Perbaikan yang Diterapkan

### 1. Isolasi Penuh Pembacaan Tamper Melalui Gateway TrustZone NSC
* **[`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c):**
  Pada target fisik (`EMBEDDED_HARDWARE_TARGET`), seluruh evaluasi sensor tamper fisik dialihkan 100% menggunakan fungsi NSC [`SECURE_GetTamperStatusMask()`](file:///g:/PPPP/ALLIN/Secure/Core/Src/secure_nsc.c#L168-L195). Pembacaan `HAL_GPIO_ReadPin` langsung dari Non-Secure dinonaktifkan sehingga kebal dari fenomena RAZ TrustZone.
  ```c
#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t sec_tamp_mask = SECURE_GetTamperStatusMask();
    bool tamp_case = ((sec_tamp_mask & (1U << 0)) != 0U);
    bool tamp_term = ((sec_tamp_mask & (1U << 1)) != 0U);
    bool tamp_mag  = ((sec_tamp_mask & (1U << 2)) != 0U);
#else
    bool tamp_case = (HAL_GPIO_ReadPin(...) == ...);
    ...
#endif
  ```
* **[`E3/drivers/power/src/bsp_power_mode.c`](file:///g:/PPPP/ALLIN/E3/drivers/power/src/bsp_power_mode.c):**
  Menghapus panggilan ilegal `HAL_GPIO_ReadPin(TAMP_CASE_OPEN_GPIO_Port, ...)` pada mode hemat daya `bsp_power_enter_stop2()`, dan murni mengandalkan `SECURE_GetTamperStatusMask()`.
* **[`Secure/Core/Src/secure_nsc.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/secure_nsc.c):**
  Menambahkan pengecekan tingkat logika sakelar konfiguratif (`BOARD_CFG_TAMPER_*_LVL`) di dalam Secure World sebelum menetapkan bit status tamper.

### 2. Implementasi Page-Program Chunking & Busy-Wait pada SPI2 NOR Flash
* **[`E3/app/profiling/src/nvram_storage.c`](file:///g:/PPPP/ALLIN/E3/app/profiling/src/nvram_storage.c):**
  * Menambahkan fungsi `spi2_flash_wait_busy()` yang membaca register status 1 (`0x05` RDSR1) hingga bit `WIP` (*Write In Progress*, bit 0) bernilai 0 sebelum dan sesudah operasi tulis/baca.
  * Mengubah `spi2_flash_write_bytes()` menjadi algoritma *looping* pemecah halaman (*page-chunking*) yang menghitung sisa kapasitas halaman 256-byte (`chunk = 256U - (flash_offset & 0xFFU)`).
  * Setiap *chunk* dikirim dengan siklus `Write Enable (0x06)` dan `Page Program (0x02)` tersendiri dengan *CS toggling* yang teratur, menjamin struktur data sebesar apa pun (~1.8 KB `tamper_context_t` atau ~5 KB `load_profile`) tersimpan utuh tanpa korupsi *wrap-around*.

---

## 3. Hasil Verifikasi & Pengujian

### A. Pengujian Host CTest (12 Test Suite)
```
Test project G:/PPPP/ALLIN/E3/build
 1/12 Test  #1: Integration_E3_Demo ..............   Passed    0.02 sec
 2/12 Test  #2: Unit_E3_Display_Tamper ...........   Passed    0.01 sec
 3/12 Test  #3: Unit_E3_Power_Relay ..............   Passed    0.02 sec
 4/12 Test  #4: Unit_E3_FSM_Statechart ...........   Passed    0.01 sec
 5/12 Test  #5: Unit_E3_Load_Profile .............   Passed    0.01 sec
 6/12 Test  #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.01 sec
 7/12 Test  #7: Stress_E3_Final_System_Audit .....   Passed    0.01 sec
 8/12 Test  #8: Unit_E3_Console_CLI ..............   Passed    0.01 sec
 9/12 Test  #9: Unit_E3_Board_Profile ............   Passed    0.01 sec
10/12 Test #10: Unit_E3_Power_Mode_Stop2 .........   Passed    0.01 sec
11/12 Test #11: Unit_E3_NVRAM_Storage ............   Passed    0.01 sec
12/12 Test #12: Unit_E3_RTOS_Tasks ...............   Passed    0.01 sec

100% tests passed, 0 tests failed out of 12 (0.19 sec)
```

### B. Kompilasi Silang ARM Cortex-M33 (GCC Bare-metal)
* **Secure Binary (`ALLIN_S.elf`):**
  * ROM: 24,000 B / 1016 KB (2.31%)
  * RAM: 12,056 B / 256 KB (4.60%)
  * ROM_NSC: 160 B / 8 KB (1.95%)
  * Status: 0 Errors, 0 Warnings
* **Non-Secure Binary (`ALLIN_NS.elf`):**
  * ROM: 174,224 B / 1 MB (16.62%)
  * RAM: 65,336 B / 512 KB (12.46%)
  * Status: 0 Errors, 0 Warnings
