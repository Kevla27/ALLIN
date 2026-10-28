# Log Perubahan 028 (LOG-028): Parser Perintah Konsol Serial Interaktif (CLI) di console_io.c

## 1. Ringkasan Eksekutif
- **ID Log:** `LOG-028`
- **Tanggal:** 27 Oktober 2026
- **Kategori:** Antarmuka Diagnostik & Pemeliharaan (Pilar E3 Console CLI)
- **Dasar Acuan Standar:**
  - **Spesifikasi & Desain Sistem Smart Meter 3-Fasa STM32U585** (Port Debug Serial USART2 @ 115200 / 19200 bps)
  - **SPLN D3.006: 2021** (Tabel 4 Alarm Internal, Tabel 5 FSM Relai, Tabel 6 Sabotase, Tabel 18 Tiga Register Log FIFO, dan Tabel E.1/E.2 Parameter Pengukuran)
- **Status Verifikasi:**
  - **Host Unit Test:** Lulus 100% (8/8 suites CTest, termasuk uji suite baru `Unit_E3_Console_CLI` pada `test_console_cli.c`).
  - **Target ARM Cortex-M33 (NonSecure ELF):** 0 Error, 0 Warning (`ROM: 13.33%`, `RAM: 12.37%`).
  - **Target ARM Cortex-M33 (Secure ELF):** 0 Error, 0 Warning.

---

## 2. Latar Belakang & Masalah Teknis

Sebelum perbaikan ini, modul konsol serial (`E3/app/console/src/console_io.c`) hanya mengimplementasikan pengiriman teks satu arah melalui syscall Newlib `_write()` (retargeting `printf()`) dan flag pengaman `console_io_set_mute()` untuk proteksi biner DLMS.

Belum tersedia fasilitas antarmuka baris perintah interaktif (**Interactive Command Line Interface / CLI**) dua arah untuk keperluan:
1. **Inspeksi Operasional Cepat:** Membaca status instan kontaktor/relai, alarm internal hardware, status sabotase fisik, pengukuran 3-fasa, dan akumulasi energi tanpa perlu bergantung pada software HES DLMS yang kompleks.
2. **Pengujian Pabrikasi & Kalibrasi (Factory Test & Bring-Up):** Menstimulasi injeksi event sabotase, memicu alarm internal, dan menguji respons proteksi pemutusan/rekoneksi relai.
3. **Audit Log Forensik Lapangan:** Menampilkan isi ketiga register log FIFO non-volatile (Alarm Log, Event Log, dan Tamper Log sesuai SPLN Butir 12.3 Tabel 18).

---

## 3. Rincian Perubahan & Solusi Teknis

1. **Mesin Penerima & Buffer RX Baris Perintah (`console_cli_process_rx_byte` / `console_cli_process_rx_buffer`):**
   - Mendukung kapasitas buffer hingga 128 karakter (`CONSOLE_CMD_MAX_LEN 128`).
   - Penanganan interaktif lengkap terminal:
     - Deteksi karakter akhir baris `\r` (CR) dan `\n` (LF) untuk memicu parser dan mencetak prompt `SMART-METER> `.
     - Penanganan tombol *Backspace* (`\b` dan `0x7F`) dengan umpan balik visual `\b \b` ke terminal emulator.
     - Proteksi *character echo* saat status konsol dibungkam (`s_console_is_muted`).
     - Pemecahan baris menjadi pasangan argumen `argc` dan token `argv[]` (maksimal 8 argumen).

2. **Perintah-Perintah Diagnostik Terpadu (Command Table):**
   - `help` / `?`: Menampilkan daftar seluruh perintah yang tersedia, deskripsi, dan sintaks parameter.
   - `status`: Menampilkan ringkasan status operasi meter (Status Relai & Alasan Trip, Mask Alarm Internal ERR00..07, Sabotase Fisik Aktif, Julat Tegangan Fasa Lo/Hi Volt, Status Proteksi DLMS).
   - `meas`: Menampilkan telemetri kelistrikan 3-fasa instan ($V_R, V_S, V_T, I_R, I_S, I_T, I_N, P_{tot}, Q_{tot}, S_{tot}, \text{PF}, \text{Freq}$).
   - `energy`: Menampilkan akumulasi energi aktif/reaktif transaksi serta stand per tarif ToU WBP, LWBP1, dan LWBP2.
   - `relay [status | trip <1..7> | reconnect | confirm]`:
     - Membaca status relai (`CONNECTED`, `DISCONNECTED`, `READY_ON`).
     - Memutus relai dengan alasan (Manual, Power Limiter, Remote, UnderVolt, OverVolt).
     - Mengirim instruksi rekoneksi remote (`READY_ON`).
     - Mengonfirmasi rekoneksi kontak relai secara fisik (`CONNECTED`).
   - `tamper [status | test <vector_id> <1|0>]`: Inspeksi dan simulasi injeksi event sabotase fisik/elektrik.
   - `alarm [status | set <hex> | clear]`: Inspeksi dan simulasi trigger alarm internal hardware (ERR00..ERR07).
   - `log <alarm | event | tamper>`: Membaca entri dari 3 register log FIFO non-volatile (SPLN Butir 12.3 Tabel 18).
   - `demand [status | reset]`: Membaca status Maximum Demand 15-menit berjalan dan mereset MD ke Cumulative MD (CMD).
   - `mute <1|0>`: Mengaktifkan atau menonaktifkan mode senyap konsol untuk mencegah interferensi paket biner DLMS.
   - `echo <teks>` & `clear` / `cls`: Utilitas verifikasi koneksi serial dan pembersih layar ANSI (`\033[2J\033[H`).

3. **Pemisahan Hook Modular (`console_meter_hooks_t`):**
   - Modul `console_io.c` tetap 100% terisolasi dari arsitektur hardware mikrokontroler melalui struktur fungsi penunjuk (hooks), sehingga dapat diuji secara terisolasi di simulator/Host PC dan langsung dipasangkan ke layanan `e3_meter_app_init()` pada firmware board STM32U585.

---

## 4. Hasil Verifikasi & Uji Mandiri (CTest & Hardware Target)

### A. Eksekusi Unit Test CTest Host (`Unit_E3_Console_CLI`)
```text
====================================================
 RUNNING INTERACTIVE SERIAL CLI UNIT TESTS (E3/CLI) 
====================================================

[TEST] Testing CLI Core Commands Execution...
  -> Help & Status Verified OK
  -> Measurements 3-Phase & Energy Verified OK
  -> Relay Lifecycle (Connected -> Disconnected -> Ready_On -> Connected) OK
  -> Internal Alarm Injected & Cleared OK
  -> Tamper Injection & Log Retrieval (Alarm, Event, Tamper FIFO) OK
  -> Maximum Demand Reset Verified OK
  -> Mute Protection Verified OK
   [PASS] CLI Core Commands Execution OK!

[TEST] Testing Interactive RX Stream Parsing & Line Editing (Backspace)...
   [PASS] Interactive RX Stream Parsing & Line Editing Verified OK!

====================================================
 ALL INTERACTIVE SERIAL CLI TESTS PASSED 100%!      
====================================================
```

### B. Kompilasi Target ARM Cortex-M33 (STM32U585VIT6)
- **NonSecure Target (`ALLIN_NS.elf`):**
  - ROM/Flash: 139.744 Bytes (13,33% dari 1 MB)
  - RAM: 64.864 Bytes (12,37% dari 512 KB)
  - Status: **0 Error, 0 Warning**.
- **Secure Target (`ALLIN_S.elf`):**
  - Status: **0 Error, 0 Warning**.

