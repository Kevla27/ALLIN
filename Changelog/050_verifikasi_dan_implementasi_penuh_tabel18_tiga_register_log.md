# LOG-050: Verifikasi Penuh 29 Rekaman Tiga Register Log FIFO dan Error Register Bitstring Sesuai SPLN D3.006:2021 Butir 12.3 Tabel 18

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Tamper, Event & Alarm FIFO Log Registers, Error Register, Non-Volatile Storage SPI Flash)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan klausul **SPLN D3.006:2021 Butir 12.3 Alarm/event/tamper log (Halaman 57-58)**:
* *"Untuk setiap kejadian alarm, event, dan tamper, meter harus menyimpan catatan data kejadian pada log masing-masing. Log yang harus disajikan sesuai pada Tabel 18."*
* *"Untuk keperluan evaluasi, masing-masing log alarm, event, dan tamper tersebut ditempatkan pada register memori non-volatile (tak terhapus) dan sekurang-kurangnya mampu tersimpan sebanyak 30 rekaman. Seluruh proses menggunakan metode penyimpanan FIFO (First In First Out) dan tidak dapat di-edit."*
* *"Data log alarm/event/tamper dapat diambil oleh pusat data berdasarkan indeks (selective access)."*
* *"Meter juga harus menyediakan error register yang berisi data rangkuman berupa bit-string terhadap informasi alarm, event, dan tamper yang sedang terjadi."*

---

## 2. Struktur 29 Jenis Rekaman pada SPLN Tabel 18

Tabel 18 mengelompokkan 29 jenis rekaman ke dalam 3 register log FIFO terpisah:

### A. Register 1: Alarm Log (8 Jenis Rekaman - No. 1 s.d 8)
1. Flash memory rusak/error (`ERR00`)
2. RAM rusak/error (`ERR01`)
3. Clock loss (kembali ke waktu awal chipset) (`ERR02`)
4. Low battery threshold 2,7 V (`ERR03`)
5. Mikroprosesor tidak berfungsi normal (`ERR04`)
6. Kegagalan sampling data pengukuran pada ADC (`ERR05`)
7. Superkapasitor rusak/tidak terhubung/terlepas (`ERR06`)
8. Relai/shunt trip gagal membuka/menutup (`ERR07`)

### B. Register 2: Event Log (12 Jenis Rekaman - No. 9 s.d 19 & 29)
9. Power supply failure
10. Power supply up
11. Perubahan setting time-of-use (ToU)
12. Perubahan setting (selain ToU)
13. Stop-measurement
14. Perubahan status I/O (termasuk trigger test)
15. Perubahan status relai/kontaktor internal
16. Meter reset
17. Perubahan tanggal dan waktu
18. Asymmetric power
19. Meter ready remote shunt trip ON
29. Power limiter threshold exceeded

### C. Register 3: Tamper Log (9 Jenis Rekaman - No. 20 s.d 28)
20. Pembukaan tutup meter (`CASE_OPEN` / `RUSAK`)
21. Pembukaan tutup terminal (`TERMINAL_OPEN` / `ERR20`)
22. Pengawatan tidak sesuai urutan fase (`WRONG_SEQUENCE` / `ERR21`)
23. Pengawatan arus dan tegangan tidak sefase (`CROSS_PHASE` / `ERR22`)
24. Kawat netral putus / bypass (`MISSING_NEUTRAL` / `ERR23`)
25. Hilang tegangan 1 atau 2 fase (`PHASE_LOSS` / `ERR24`)
26. Induksi medan magnet dari luar $\le 500\text{ mT}$ (`MAGNETIC_FIELD` / `ERR25`)
27. Reverse power (`REVERSE_POWER` / `ERR26`)
28. Pengukuran pada kuadran 4 dengan threshold $\text{PF} < 0,85$ (`LOW_PF_QUAD4` / `ERR27`)

---

## 3. Implementasi Arsitektur Firmware ALLIN

1. **3 Buffer FIFO Non-Volatile Terpisah (`tamper_manager.h` & `tamper_manager.c`):**
   * `spln_log_fifo_t alarm_log_fifo`: Menyimpan minimal 30 entri FIFO (rolling buffer) untuk Alarm Log.
   * `spln_log_fifo_t event_log_fifo`: Menyimpan minimal 30 entri FIFO untuk Event Log.
   * `spln_log_fifo_t tamper_log_fifo`: Menyimpan minimal 30 entri FIFO untuk Tamper Log.
   * Struktur setiap entri (`spln_standard_log_entry_t`):
     * `timestamp_epoch`: Waktu kejadian RTC (UNIX epoch).
     * `event_code`: Kode jenis kejadian (1..8, 1..12, 1..9).
     * `status_flags`: `1` = Mulai/Terjadi (*asserted*), `0` = Selesai/Pulih (*restored*).
     * `parameter_val`: Nilai parameter tambahan (tegangan baterai, deviasi asimetri %, daya Watt, dsb.).
2. **Selective Access Berdasarkan Indeks:**
   * Disediakan API `spln_log_get_entry_by_index()` untuk pembacaan acak maupun terurut oleh DLMS/COSEM (Class 7 Profile Generic) atau Serial CLI (`log alarm`, `log event`, `log tamper`).
3. **Error Register Real-Time (Bit-String 29 Bit Penuh):**
   * Direpresentasikan dalam satu integer 32-bit `error_register_bitstring`:
     * **Bit 0..7 (8 bit):** Status Alarm Log No. 1 s.d 8
     * **Bit 8..19 (12 bit):** Status Event Log No. 9 s.d 19 & 29
     * **Bit 20..28 (9 bit):** Status Tamper Log No. 20 s.d 28
     * Bit 29..31: Reserved (0)
   * Nilai maksimum saat seluruh kejadian aktif bersamaan: `0x1FFFFFFF` (29 bit bernilai 1).
4. **Persistensi Non-Volatile SPI Flash (`nvram_storage.c`):**
   * Seluruh snapshot `tamper_context_t` (termasuk 3 register FIFO dan bitstring Error Register) disimpan ke SPI Flash external W25Qxx pada sektor non-volatile `0x002000U` sehingga tidak hilang saat meter padam (*power outage*).

---

## 4. Hasil Pengujian dan Verifikasi

1. **Unit Test 3 Register FIFO & Error Register (`test_e3_app.c`):**
   * Memvalidasi kapasitas rolling FIFO 30 rekaman tanpa merusak urutan (*overflow handling*).
   * Memvalidasi seluruh 8 Alarm Log, 12 Event Log, dan 9 Tamper Log (total 29 jenis kejadian).
   * Memvalidasi kalkulasi bitmask Error Register menghasilkan nilai tepat `0x1FFFFFFF` (29/29 bit).
   * Memvalidasi pemulihan status (*deassertion*) membersihkan bit Error Register secara otomatis.
2. **Hasil CTest:** 12/12 Test Suites Lulus 100% (0 Failed).
3. **Kompilasi Firmware Target ARM Cortex-M33:** `ALLIN_S.elf` dan `ALLIN_NS.elf` sukses ter-link 100% bersih.

