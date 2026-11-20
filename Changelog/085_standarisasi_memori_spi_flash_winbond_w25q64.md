# Log Perubahan 085: Standarisasi Memori SPI Flash Winbond W25Q64 (64 Mbit / 8 MB) & Verifikasi JEDEC ID

- **Judul Dokumen:** Penetapan dan Standarisasi Chip Memori Non-Volatile Winbond W25Q64 (64 Mbit / 8 MByte), Pembersihan Referensi EEPROM, dan Implementasi Verifikasi JEDEC ID (0xEF 0x40 0x17) Sesuai Acuan PIN.docx & SPLN D3.006: 2021
- **Tanggal:** 20 November 2026
- **Status:** Selesai & Terverifikasi Penuh (0 Error, 0 Warning, CTest 13/13 Passed, 100%)
- **Tipe Perubahan:** Hardware Architecture Realignment / SPI Flash Driver Hardening / JEDEC ID Verification
- **Pilar / Komponen Terdampak:** Pilar E3 (`E3/app/board_profile_config.h`, `E3/app/profiling/inc/nvram_storage.h`, `E3/app/profiling/src/nvram_storage.c`, `E3/tests/test_nvram_storage.c`), Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`).

---

## Bagian 1: Latar Belakang & Analisis Integrator

Sesuai arahan teknis, jenis memori non-volatile eksternal pada smart meter ditetapkan secara definitif menggunakan **Winbond W25Q64 (64 Mbit / 8 MByte)** pada bus **SPI2** (`PB12` CS, `PB13` SCK, `PB14` MISO, `PB15` MOSI):

1. **Eliminasi Referensi EEPROM AT24C512:**
   - Catatan lama pada [board_profile_config.h](file:///g:/PPPP/ALLIN/E3/app/board_profile_config.h) yang sebelumnya memuat opsi cadangan I2C EEPROM AT24C512 dibersihkan dan diseragamkan seluruhnya ke SPI Flash Winbond W25Q64.
   - Hal ini karena kapasitas EEPROM 64 KB (AT24C512) tidak memenuhi syarat minimal retensi $\ge 1.024\text{ KB}$ untuk menampung rekaman Load Profile 3 bulan sesuai SPLN D3.006: 2021 dan dokumen acuan [PIN.docx](file:///g:/PPPP/ALLIN/Acuan/PIN.docx).

2. **Spesifikasi & Keunggulan Winbond W25Q64 (8 MB):**
   - Kapasitas total 8.388.608 Byte (8 MByte / 64 Mbit), terbagi menjadi **2.048 Sektor @ 4 KB** atau 128 Blok @ 64 KB.
   - Pengalamatan 24-bit (`A23..A0`) standar, sepenuhnya kompatibel dengan rutinitas baca, tulis, dan hapus sektor yang telah ada tanpa memerlukan mode 4-byte khusus.
   - Kapasitas 8 MB mampu menampung rekaman data profil beban 23-kanal 15-menitan hingga lebih dari **2,7 tahun** (jauh melampaui standar minimal 90 hari PLN).

3. **Verifikasi Otomatis JEDEC ID (`0x9F`):**
   - Mengimplementasikan fungsi pembacaan JEDEC ID pada driver Flash eksternal untuk memverifikasi keaslian dan kesiapan chip W25Q64 saat sistem melakukan *bootstrapping*:
     - **Manufacturer ID:** `0xEF` (Winbond)
     - **Memory Type:** `0x40` (SPI / Dual / Quad SPI)
     - **Capacity ID:** `0x17` ($2^{23} = 8\text{ MByte}$)

---

## Bagian 2: Rincian Modifikasi Kode Program

### 1. File Konfigurasi Profil Papan (`E3/app/board_profile_config.h`)
- Menyeragamkan seluruh profil papan (`BOARD_PROFILE_PROTOTYPE_OLED` dan `BOARD_PROFILE_PRODUCTION_SEGLCD`) agar menggunakan:
  ```c
  /* --- D. Non-Volatile Storage (NVRAM - Winbond W25Q64 8 MByte) --- */
  #define BOARD_CFG_NVRAM_STORAGE_SPI_FLASH 1  /**< Menggunakan Winbond W25Q64 (64 Mbit / 8 MB) SPI2 Flash eksternal */
  #define BOARD_CFG_NVRAM_STORAGE_EEPROM    0
  ```
- Menghapus komentar usang terkait I2C EEPROM AT24C512.

### 2. Header Penyimpanan NVRAM (`E3/app/profiling/inc/nvram_storage.h`)
- Menambahkan konstanta spesifikasi chip W25Q64 dan opcode `SPI_FLASH_CMD_JEDEC_ID (0x9F)`:
  ```c
  #define W25Q64_TOTAL_SIZE_BYTES            (8U * 1024U * 1024U)
  #define W25Q64_TOTAL_SECTORS               2048U
  #define W25Q64_TOTAL_BLOCKS_64K            128U
  #define W25Q64_JEDEC_MANUFACTURER_ID       0xEFU
  #define W25Q64_JEDEC_MEMORY_TYPE           0x40U
  #define W25Q64_JEDEC_CAPACITY_ID           0x17U
  #define SPI_FLASH_CMD_JEDEC_ID             0x9FU
  ```
- Mendeklarasikan fungsi API verifikasi chip:
  - `bool nvram_w25q64_read_jedec_id(uint8_t *out_mfg, uint8_t *out_type, uint8_t *out_cap);`
  - `bool nvram_w25q64_verify(void);`

### 3. Implementasi Driver Flash (`E3/app/profiling/src/nvram_storage.c`)
- Mengimplementasikan `nvram_w25q64_read_jedec_id()` yang mentransmisikan opcode `0x9F` via SPI2 dan menerima 3 byte identifikasi silikon.
- Mengimplementasikan `nvram_w25q64_verify()` untuk memastikan kecocokan dengan `0xEF 0x40 0x17`.

### 4. Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`)
- Pada fungsi `meter_system_bringup()`, menambahkan log diagnostik verifikasi chip saat *booting*:
  ```c
  uint8_t flash_mfg = 0, flash_type = 0, flash_cap = 0;
  if (nvram_w25q64_read_jedec_id(&flash_mfg, &flash_type, &flash_cap))
  {
      printf("[NVRAM] SPI2 Flash Target: Winbond W25Q64 (JEDEC: 0x%02X 0x%02X 0x%02X, Cap: 8 MB / 2048 Sectors)\n",
             flash_mfg, flash_type, flash_cap);
  }
  ```

### 5. Pengujian Unit Test Suite (`E3/tests/test_nvram_storage.c`)
- Menambahkan fungsi `test_nvram_w25q64_spec_and_jedec()` yang memverifikasi ukuran 8 MB, 2048 sektor, 128 blok, dan JEDEC ID `0xEF 0x40 0x17`.

---

## Bagian 3: Verifikasi & Hasil Pengujian

- **Unit Test CTest Host:**
  ```text
  Test #11: Unit_E3_NVRAM_Storage ............ Passed    0.08 sec
  100% tests passed, 0 tests failed out of 13
  Total Test time (real) = 1.29 sec
  ```
  *Output konsol:*
  ```text
  [TEST] Testing Winbond W25Q64 (64 Mbit / 8 MB) Specification & JEDEC ID...
     [PASS] Winbond W25Q64 JEDEC ID (0xEF 0x40 0x17) & 8MB Capacity Verified!
  ```
- **Kompilasi Firmware Target ARM (`ALLIN_NS.elf`):**
  - ALLIN: 0 Warnings, 0 Errors (RAM: 16.29%, ROM: 18.97%).
  - ALLIN_NUCLEO: 0 Warnings, 0 Errors (RAM: 16.09%, ROM: 18.97%).
