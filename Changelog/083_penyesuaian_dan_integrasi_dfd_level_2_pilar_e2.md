# Penyesuaian dan Integrasi DFD Level 2 Pilar E2 (Sub-sistem Protokol DLMS/COSEM & Security)

**Tanggal:** 23 November 2026  
**Status:** Selesai & Terverifikasi Penuh (0 Broken Edges, 0 Overlaps, Ninja Build 0 Error 0 Warning, CTest 13/13 Passed)  
**Tipe Perubahan:** Architecture & System Design Alignment / Data Flow Diagram (DFD Level 2)  
**Komponen Terdampak:** Dokumentasi Sistem ([`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml)), Dokumen Acuan ([`Acuan/State_DF_D.xml`](file:///g:/PPPP/ALLIN/Acuan/State_DF_D.xml)), Rantai Integrasi Tiga Pilar (E1, E2, E3).

---

## 1. Latar Belakang & Analisis Integrator

Pengguna mengunggah diagram **DATA FLOW DIAGRAM (DFD) LOGIS LEVEL 2: DEKOMPOSISI PROSES 2.0 (SECURITY & PROTOKOL DATA)** dan meminta agar diagram tersebut disesuaikan serta dimasukkan ke dalam runtutan DFD sistem yang telah dibangun sebelumnya (DFD Level 0, DFD Level 1, dan DFD Level 2 Pilar E3).

Berdasarkan audit komparasi terhadap arsitektur keseluruhan dan standar rekayasa perangkat lunak (*Design for ECE* & IEEE/ISO DFD balancing rules), diidentifikasi beberapa poin penyesuaian (*alignment*):

1. **Resolusi Benturan Penomoran Data Store (*Data Store Collision Resolution*):**
   * Pada diagram masukan awal, proses E2 menggunakan nomor lokal `D1: Penyimpanan Kunci Kriptografi`, `D2: Penyimpanan Counter Anti-Replay`, dan `D3: Penyimpanan Profil & Logbook Meter`.
   * Penomoran ini bertabrakan dengan Data Store Global di DFD Level 1 di mana `D1` adalah *Flash Checkpoint Energi*, `D2` adalah *Ring Buffer Logbook FIFO*, `D3` adalah *Buffer Profil Beban 15-M*, dan `D4` adalah *Tabel Tarif ToU*.
   * **Solusi Penyelarasan:**
     - Kunci Kriptografi diselaraskan menjadi **`D5 | PENYIMPANAN KUNCI KRIPTOGRAFI`** (Master Key, Encryption Key, AK HLS5).
     - Pelacak Anti-Replay diselaraskan menjadi **`D6 | PENYIMPANAN COUNTER ANTI-REPLAY`** (NVM Invocation Counter).
     - Profil & Logbook Meter diselaraskan langsung merujuk ke data store global yang sudah ada di Level 1: **`D2, D3 | PENYIMPANAN PROFIL BEBAN & LOGBOOK METER`**.

2. **Konsistensi Entitas Eksternal & Batas Proses:**
   * Entitas diselaraskan dengan Level 0 dan Level 1:
     - `HES / Sistem Pusat` $\rightarrow$ **`HES / SISTEM PUSAT PLN (HES / AMI SYSTEM)`**
     - `Petugas P2TL / HHU` $\rightarrow$ **`PETUGAS PLN (HHU / P2TL) & BANGKU UJI TERA`**
     - `1.0 (Pilar E1) Metrologi & DSP` (Proses Tetangga)
     - `3.0 (Pilar E3) Aplikasi & Beban` (Proses Tetangga)

3. **Penyempurnaan Keseimbangan DFD Level 1 (*DFD Balancing*):**
   * Di Level 2 E2, terdapat aliran `Data Pengukuran Sesaat (s_meas)` dari `1.0` ke `2.5`.
   * Agar Level 1 dan Level 2 berimbang sempurna (*balanced*), ditambahkan panah aliran data langsung `f_10_20_meas` (`Data Pengukuran Sesaat (s_meas)`) dari `1.0` ke `2.0` pada DFD Level 1.

---

## 2. Struktur Tab DFD Lengkap Sistem (4 Tab)

Berkas [`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) kini memiliki struktur terpadu:

1. **Tab 1: `DFD Level 0: Diagram Konteks` (`DFD_Level_0_Context`)**
   * Proses tunggal `0.0 SMART METER AMI 3-FASA`.
   * 6 Entitas Eksternal (Jaringan Listrik, Sensor Tamper, Pelanggan, Beban, Petugas PLN, HES).
2. **Tab 2: `DFD Level 1: Dekomposisi Tiga Pilar` (`dfd_level_1_three_pillars`)**
   * `1.0 Sub-sistem Metrologi & DSP (ADE9000)`
   * `2.0 Sub-sistem Protokol DLMS/COSEM (Security & Komunikasi)`
   * `3.0 Sub-sistem Logika Aplikasi & Manajemen Beban`
   * 4 Data Store Global (`D1`, `D2`, `D3`, `D4`) + Aliran Data Lintas Pilar.
3. **Tab 3: `DFD Level 2: Dekomposisi Pilar E3` (`dfd_level_2_pilar_e3`)**
   * 5 Sub-proses (`3.1` s.d `3.5`) untuk Billing ToU, Tamper & Alarm, Relai Breaker, HMI OLED, dan Last-Gasp Mode.
4. **Tab 4: `DFD Level 2: Dekomposisi Pilar E2` (`dfd_level_2_pilar_e2`) [BARU]**
   * `2.1 Memvalidasi & Mengemas Frame Komunikasi` (HDLC / Wrapper / CRC / UART DMA)
   * `2.2 Mengautentikasi Identitas & Sesi Klien` (AARQ / AARE / Anti-Bruteforce HLS5)
   * `2.3 Mengenkripsi & Mendekripsi Pesan Data` (AES-GCM-128 / Security Suite 0)
   * `2.4 Memproses Permintaan & Pemecahan Blok Data` (xDLMS GET/SET/ACTION & DataBlock Transfer)
   * `2.5 Mengambil & Memperbarui Data Objek Meter` (Katalog Objek COSEM / OBIS Mapping)
   * Data Store: `D5` (Kunci Kriptografi), `D6` (Counter Anti-Replay), `D2, D3` (Profil Beban & Logbook).
   * Aliran Antar-Pilar: Sinyal Pengukuran dari `1.0`, Perintah Kendali Beban ke `3.0`, dan Status Relai/Tamper dari `3.0`.

---

## 3. Hasil Pengujian & Verifikasi

* **Verifikasi Topologi DFD (`scratch/verify_all_dfd.py`):**
  * `[DFD_Level_0_Context]`: 8 nodes, 10 edges | Broken: 0 | Overlaps: 0
  * `[dfd_level_1_three_pillars]`: 14 nodes, 26 edges | Broken: 0 | Overlaps: 0
  * `[dfd_level_2_pilar_e3]`: 16 nodes, 32 edges | Broken: 0 | Overlaps: 0
  * `[dfd_level_2_pilar_e2]`: 13 nodes, 18 edges | Broken: 0 | Overlaps: 0
* **Verifikasi Kompilasi Target NonSecure:**
  * Toolchain: Arm GNU Toolchain + Ninja (`ninja -C NonSecure/build`)
  * Hasil: **0 Error, 0 Warning** (RAM: 85.38 KB / 16.29%, ROM: 198.88 KB / 18.97%).
* **Verifikasi Unit Test CTest:**
  * Hasil: **13/13 Test Passed (100%)** (Total time: 1.73 s).
