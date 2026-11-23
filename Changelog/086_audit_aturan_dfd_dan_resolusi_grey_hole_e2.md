# LOG-086: Audit Kepatuhan Kaidah DFD dan Resolusi Anomali Grey Hole pada DFD Level 2 Pilar E2

## 1. Latar Belakang & Tujuan
Menindaklanjuti permintaan audit mendalam terhadap kesesuaian diagram **Data Flow Diagram (DFD) Logis Level 2: Dekomposisi Proses 2.0 (Security & Protokol Data DLMS/COSEM)** dengan kaidah baku DFD (Gane-Sarson / Yourdon & Coad / DeMarco), dilakukan pengujian komprehensif atas 6 aspek aturan utama rekayasa perangkat lunak:
1. Hubungan Entitas Eksternal (External Entity Constraints).
2. Integritas Hubungan Data Store (Data Store Constraints).
3. Eliminasi Anomali Proses (Pencegahan *Black Hole*, *Miracle*, dan *Grey Hole*).
4. Keseimbangan Antar-Tingkat (*Parent-Child Level Balancing*).
5. Konvensi Penamaan Semantik (*Naming Conventions*).
6. Hirarki Penomoran (*Numbering Hierarchy*).

---

## 2. Temuan Audit & Resolusi Anomali Grey Hole

### 2.1 Temuan Anomali Grey Hole pada Gambar Asli
Pada diagram sketsa awal DFD Level 2 Pilar E2 yang diunggah pengguna:
- **Sub-proses 2.4 (`Memproses Permintaan & Pemecahan Blok Data`)**:
  - Menerima `Permintaan Layanan` dari sub-proses 2.3.
  - Mengirim `Permintaan Objek` ke sub-proses 2.5.
  - Mengirim `Blok Respons Data` ke sub-proses 2.1.
- **Sub-proses 2.5 (`Mengambil & Memperbarui Data Objek Meter`)**:
  - Menerima `Permintaan Objek` dari sub-proses 2.4.
  - Membaca `Data Profil & Tagihan` dari data store `D3`, `Data Pengukuran Sesaat` dari Proses `1.0`, dan `Status Relay & Event Tamper` dari Proses `3.0`.
  - Mengirim `Perintah Kendali Beban` ke Proses `3.0` dan `Pembaruan Status` ke data store `D3`.
  - **TITIK MASALAH (Grey Hole)**: Tidak terdapat aliran data balasan dari sub-proses 2.5 kembali ke sub-proses 2.4. Secara aturan formal DFD, sub-proses 2.4 tidak memiliki input data objek yang dapat dipecah menjadi `Blok Respons Data`. Data yang diambil oleh 2.5 terhenti (*dead-end*) dan output pada 2.4 kehilangan bahan bakunya (*insufficient input transformation*).

### 2.2 Resolusi Rekayasa Sistem
Ditambahkan aliran data baru:
- **ID Edge**: `f2_5_4_data`
- **Label**: `Data Objek Terambil`
- **Source**: Sub-proses `2.5` (`exitX=0, exitY=0.7`)
- **Target**: Sub-proses `2.4` (`entryX=1, entryY=0.7`)
- **Penyesuaian**: Edge `f2_4_5` (`Permintaan Objek`) dinaikkan jalurnya ke `exitY=0.35, entryY=0.35` untuk memastikan pemisahan visual dan semantik yang bersih tanpa tumpang tindih (*zero collision/overlap*).

---

## 3. Matriks Kepatuhan Kaidah DFD Sistem

| No | Kaidah / Aturan Formal DFD | Status Evaluasi | Penjelasan & Bukti Verifikasi |
| :---: | :--- | :---: | :--- |
| **1** | **Hubungan Entitas Eksternal**<br>• Tidak boleh Entity-to-Entity<br>• Tidak boleh Entity-to-Store<br>• Hanya terhubung ke Proses | **PASSED**<br>(100% Sesuai) | `HES` dan `Petugas PLN` hanya terhubung ke Sub-proses `2.1`. Tidak ada kontak langsung ke entitas lain maupun data store. |
| **2** | **Hubungan Data Store**<br>• Tidak boleh Store-to-Store<br>• Tidak boleh Store-to-Entity<br>• Wajib memiliki siklus Baca & Tulis | **PASSED**<br>(100% Sesuai) | • `D5`: Dibaca oleh 2.2 (`Kunci Autentikasi`), ditulis oleh 2.3 (`Kunci Baru`).<br>• `D6`: Dibaca oleh 2.3 (`Counter Terakhir`), ditulis oleh 2.3 (`Counter Baru`).<br>• `D2, D3`: Dibaca oleh 2.5 (`Data Profil & Tagihan`), ditulis oleh 2.5 (`Pembaruan Status`). |
| **3** | **Anomali Aliran Proses**<br>• Bebas Black Hole (In tanpa Out)<br>• Bebas Miracle (Out tanpa In)<br>• Bebas Grey Hole (In tidak cukup / Out tak bersumber) | **PASSED**<br>(100% Sesuai) | Seluruh sub-proses (`2.1` s.d `2.5`) memiliki input dan output yang proporsional. Grey Hole antara 2.4 dan 2.5 telah diselesaikan melalui penambahan edge `f2_5_4_data`. |
| **4** | **Keseimbangan Hirarki (Balancing)**<br>Boundary Level 1 $\equiv$ Boundary Level 2 | **PASSED**<br>(100% Sesuai) | Seluruh aliran yang melintasi batas Proses 2.0 di DFD Level 1 (HES in/out, Petugas in/out, E1 in, E3 in/out, D5 read/write, D6 read/write, D2/D3 read/write) identik dan seimbang dengan boundary di Level 2. |
| **5** | **Konvensi Penamaan Semantik**<br>• Proses: Kata Kerja Aktif Transitif<br>• Aliran Data: Frasa Kata Benda<br>• Data Store: Frasa Kata Benda Jamak | **PASSED**<br>(100% Sesuai) | • Proses: `Memvalidasi...`, `Mengautentikasi...`, `Mengenkripsi...`, `Memproses...`, `Mengambil...`<br>• Aliran: `Frame Masuk`, `Permintaan Asosiasi`, `Pesan Terenkripsi`, dll.<br>• Data Store: `Penyimpanan Kunci Kriptografi`, dll. |
| **6** | **Hirarki Penomoran Unik**<br>Parent 2.0 $\rightarrow$ Children 2.1 .. 2.5 | **PASSED**<br>(100% Sesuai) | Sub-proses konsisten menggunakan nomor turunan `2.1` s.d `2.5`. Data Store diselaraskan ke `D5`, `D6`, dan `D2, D3` untuk mencegah konflik dengan `D1` Metrologi di Level 1. |

---

## 4. File yang Terdampak
1. [`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) — Ditambahkan edge `f2_5_4_data` dan disesuaikan docking `f2_4_5`.
2. [`Acuan/State_DF_D.xml`](file:///g:/PPPP/ALLIN/Acuan/State_DF_D.xml) — Sinkronisasi penuh dengan diagram sistem.
3. [`Changelog/086_audit_aturan_dfd_dan_resolusi_grey_hole_e2.md`](file:///g:/PPPP/ALLIN/Changelog/086_audit_aturan_dfd_dan_resolusi_grey_hole_e2.md) — Dokumen laporan audit kepatuhan DFD.
4. [`Changelog/README.md`](file:///g:/PPPP/ALLIN/Changelog/README.md) — Registrasi indeks log perubahan.

---

## 5. Hasil Verifikasi Sistem & Kompilasi
- **Verifikasi Topologi DFD (`scratch/verify_all_dfd.py`)**:
  - `[DFD_Level_0_Context]`: 8 nodes, 10 edges (0 broken, 0 overlaps).
  - `[dfd_level_1_three_pillars]`: 14 nodes, 26 edges (0 broken, 0 overlaps).
  - `[dfd_level_2_pilar_e3]`: 16 nodes, 32 edges (0 broken, 0 overlaps).
  - `[dfd_level_2_pilar_e2]`: 13 nodes, 19 edges (0 broken, 0 overlaps).
- **Firmware Compilation (`ninja -C NonSecure/build`)**: 0 Error, 0 Warning.
- **Unit Test Suite (`ctest --test-dir E3/build`)**: 13/13 Tests Passed (100%).
