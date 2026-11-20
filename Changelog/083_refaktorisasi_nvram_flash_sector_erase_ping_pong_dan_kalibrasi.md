# Log Perubahan 083: Refaktorisasi NVRAM SPI Flash W25Q32JV (Sector Erase 4KB, Dual-Slot Ping-Pong, & Kalibrasi ADE9000)

- **Judul Dokumen:** Refaktorisasi Driver NVRAM SPI Flash W25Q32JV Sesuai Bab 3.3.3 Desain Perangkat Lunak & Firmware E3 dan SPLN D3.006: 2021 (Prioritas 1)
- **Tanggal:** 20 November 2026
- **Status:** Selesai & Terverifikasi Penuh (0 Error, 0 Warning, CTest 13/13 Passed, 100%)
- **Tipe Perubahan:** Architecture Realignment / Flash Storage Hardening / Metrology Calibration Persistence
- **Pilar / Komponen Terdampak:** Pilar E3 (`E3/app/profiling/inc/nvram_storage.h`, `E3/app/profiling/src/nvram_storage.c`), Jembatan Sistem (`NonSecure/Core/Src/meter_system_bridge.c`), Unit Tests (`E3/tests/test_nvram_storage.c`).

---

## Bagian 1: Latar Belakang & Analisis Integrator

Berdasarkan audit mendalam terhadap **Bab 3.3.3 Manajemen Penyimpanan Non-Volatile (NVRAM)** pada dokumen *Desain Perangkat Lunak & Firmware E3* serta **SPLN D3.006: 2021 Butir 12.3**, ditemukan beberapa kelemahan kritis pada implementasi NVRAM SPI Flash (Winbond W25Q32JV) sebelumnya:

1. **Kesalahan Perintah Hapus Sektor SPI Flash (Sector Erase vs Block Erase):**
   - Implementasi sebelumnya menggunakan kode instruksi `0xD8` (Block Erase 64 KB). Padahal partisi data meter dialokasikan per sektor 4 KB (`0x1000` byte).
   - Menjalankan `0xD8` pada partisi 4 KB menyebabkan *collateral corruption* (sektor-sektor partisi tetangga ikut terhapus secara tidak sengaja).
   - **Koreksi:** Mengubah instruksi hapus sektor menjadi kode baku **`0x20` (Sector Erase 4 KB)** sesuai datasheet W25Q32JV.

2. **Ketiadaan Mekanisme Ping-Pong (Dual-Buffering) Stand Energi:**
   - Stand energi aktif/reaktif periodik (tiap 60 detik) sebelumnya disimpan langsung pada alamat tetap. Apabila terjadi pemadaman listrik (*brownout / power failure*) persis saat penulisan sektor Flash sedang berlangsung, data stand energi rusak (*corrupted*) dan nilai meter hilang.
   - **Koreksi:** Mengimplementasikan partisi **Ping-Pong Dual-Buffering** dengan dua slot terpisah:
     - **Slot A:** Alamat `0x001000` (Sektor 1, 4 KB).
     - **Slot B:** Alamat `0x003000` (Sektor 3, 4 KB).
   - Setiap kali penulisan checkpoint 60 detik, sistem melakukan rotasi slot secara bergantian dengan menaikkan *sequence number* monotonik dan memeriksa integritas CRC32. Saat *boot*, sistem memindai kedua slot dan memilih slot dengan urutan sequence tertinggi yang memiliki validasi CRC32 cocok.

3. **Hilangnya Partisi Persistensi Kalibrasi ADE9000:**
   - Register kalibrasi AIGAIN, AVGAIN, AIRMSOS, AVRMSOS, dsb. dari AFE ADE9000 sebelumnya hanya diinisialisasi secara hardcoded di RAM atau hilang saat reset.
   - **Koreksi:** Mengalokasikan sektor khusus pada alamat **`0x002000` (Sektor 2, 4 KB)** untuk menyimpan struktur `ade9000_calibration_nvram_t` lengkap dengan magic header `0x43414C42` (`CALB`), versi skema, dan checksum CRC32.

---

## Bagian 2: Rincian Modifikasi Kode Program

### 1. Header Penyimpanan NVRAM (`E3/app/profiling/inc/nvram_storage.h`)
- Menambahkan pemetaan partisi sektor 4 KB:
  ```c
  #define NVRAM_SECTOR_ERASE_OPCODE      0x20  /* W25Q32JV 4 KB Sector Erase */
  #define NVRAM_SECTOR_SIZE_BYTES        4096U /* 4 KB */

  #define NVRAM_ADDR_ENERGY_PING_SLOT_A  0x001000U /* Sector 1 (4 KB) */
  #define NVRAM_ADDR_CALIBRATION         0x002000U /* Sector 2 (4 KB) */
  #define NVRAM_ADDR_ENERGY_PONG_SLOT_B  0x003000U /* Sector 3 (4 KB) */
  ```
- Menambahkan struktur data kalibrasi ADE9000:
  ```c
  typedef struct {
      uint32_t magic;           /* 0x43414C42 ('CALB') */
      uint16_t version;         /* Versi struktur kalibrasi */
      uint16_t reserved;
      int32_t  aigain;
      int32_t  bigain;
      int32_t  cigain;
      int32_t  n_gain;
      int32_t  avgain;
      int32_t  bvgain;
      int32_t  cvgain;
      int32_t  airmsos;
      int32_t  birmsos;
      int32_t  cirmsos;
      int32_t  avrmsos;
      int32_t  bvrmsos;
      int32_t  cvrmsos;
      int32_t  aphcal32;
      int32_t  bphcal32;
      int32_t  cphcal32;
      uint32_t crc32;
  } ade9000_calibration_nvram_t;
  ```
- Mendeklarasikan fungsi API baru:
  - `nvram_save_energy_checkpoint_pingpong()`
  - `nvram_load_energy_checkpoint_pingpong()`
  - `nvram_save_calibration()`
  - `nvram_load_calibration()`

### 2. Implementasi Driver NVRAM (`E3/app/profiling/src/nvram_storage.c`)
- Mengganti opcode pada rutinitas hapus sektor dari `0xD8` menjadi `0x20`.
- Mengimplementasikan logika dual-slot ping-pong write & recovery saat boot:
  - Validasi *magic signature* `0x454E4759` (`ENGY`).
  - Validasi integritas CRC32 pada setiap slot.
  - Pemilihan record terbaru berdasarkan `seq_num` tertinggi.
- Mengimplementasikan penulisan dan pembacaan struktur kalibrasi ADE9000 dengan proteksi CRC32.

### 3. Pengujian Unit Test Suite (`E3/tests/test_nvram_storage.c`)
- Menambahkan pengujian komprehensif:
  - `test_nvram_sector_erase_opcode()`: Memverifikasi opcode `0x20`.
  - `test_nvram_pingpong_alternating()`: Memverifikasi penulisan bergantian Slot A dan Slot B beserta pemulihan otomatis saat salah satu slot mengalami kegagalan/korupsi CRC.
  - `test_nvram_calibration_persistence()`: Memverifikasi persistensi dan validasi CRC32 pada parameter gain & offset ADE9000.

---

## Bagian 3: Verifikasi & Hasil Pengujian

- **Unit Test CTest Host:**
  ```text
  Test #11: Unit_E3_NVRAM_Storage ............ Passed    0.03 sec
  100% tests passed, 0 tests failed out of 13
  ```
- **Kompilasi Firmware Target ARM:**
  - Build `ALLIN_NS.elf` sukses tanpa peringatan (*0 warnings, 0 errors*).
- **Integritas Git Commit:**
  - Commit ID: `e228206`
  - Push status: Sukses ke `origin/main`.
