# Pemisahan Menjadi 4 Data Store Terdedikasi pada DFD Level 1 (Tiga Pilar)

**Tanggal:** 10 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Data Architecture & DFD Specification Refinement (Kaidah Baku Yourdon/DeMarco & SPLN D3.006)  
**Komponen Terdampak:** Dokumentasi Sistem (`Acuan/State_DF_D.xml`, `docs/system_dfd_diagram.drawio.xml`)

---

## 1. Latar Belakang & Analisis Arsitektur

Pada diagram **DFD Level 1 (Dekomposisi Tiga Pilar)** sebelumnya, media penyimpanan data persisten non-volatile (NVM) digambarkan sebagai satu blok gabungan monolitik:
`ds_nvm` (`D1..D3 | MEMORI PERSISTEN NVM (NOR FLASH SPI2 W25Qxx & INTERNAL)`).

Secara kaidah baku DFD (*Gane & Sarson / Yourdon & DeMarco*) dan kepatuhan arsitektur *firmware* Smart Meter SPLN D3.006:2021, konsolidasi tersebut memiliki kelemahan:
1. Menyamarkan karakteristik fisik partisi penyimpanan (memori internal berkecepatan tinggi penahan *Last-Gasp* vs. memori eksternal SPI Flash berkapasitas besar).
2. Menggabungkan kategori data yang memiliki siklus hidup (*lifecycle*) dan pemicu pembaruan yang sangat berbeda (data transaksi energi real-time, data deret berkala beban, antrean melingkar/FIFO logbook kejadian, dan parameter skedul tarif kontraktual).

Sesuai arahan pengguna, entitas data store pada diagram DFD Level 1 dipecah dari 1 entitas menjadi **4 Data Store terdedikasi**.

---

## 2. Rincian 4 Data Store Terdedikasi (DFD Level 1)

Empat Data Store baru ditempatkan secara simetris di bawah **Proses 3.0 (Sub-Sistem Logika Aplikasi & Sistem / Pilar E3)** dalam formasi grid $2 \times 2$ yang lapang dengan ruang koridor tengah 85 px untuk perutean aliran data:

### A. `ds_d1`: `D1 | FLASH CHECKPOINT ENERGI & STATUS RELAI`
- **Tipe Media:** Memori Flash Internal / NVRAM Berkecepatan Tinggi
- **Fungsi:** Menyimpan register snapshot energi aktif/reaktif dan status relai kontaktor saat interupsi darurat *Last-Gasp* ($<15\text{ ms}$, pin `PC3` `AC_FAIL_DET`), serta memulihkan data tersebut saat proses *booting* sistem.
- **Aliran Data:**
  - `df_e3_d1_w` (Tulis): `Penyimpanan Checkpoint Last-Gasp (<15ms)` (`p3_0` $\rightarrow$ `ds_d1`)
  - `df_d1_e3_r` (Baca): `Restorasi Energi & Status Relai Saat Boot` (`ds_d1` $\rightarrow$ `p3_0`)

### B. `ds_d2`: `D2 | RING BUFFER LOGBOOK KEJADIAN (FIFO)`
- **Tipe Media:** SPI Flash Eksternal `W25Q32` (SPI2, `PB12..PB15`)
- **Fungsi:** Mengelola 3 register logbook melingkar FIFO terpisah ($\ge 30$ entri per logbook sesuai SPLN D3.006): Log Tamper (9 vektor sabotase), Log Alarm (8 alarm internal hardware `ERR00`..`ERR07`), dan Log Event (12 kejadian transaksi & pemadaman).
- **Aliran Data:**
  - `df_e3_d2_w` (Tulis): `Pencatatan Event Sabotase, Alarm HW & Relai Trip` (`p3_0` $\rightarrow$ `ds_d2`)
  - `df_d2_e3_r` (Baca): `Pembacaan Riwayat Logbook FIFO Kejadian` (`ds_d2` $\rightarrow$ `p3_0`)

### C. `ds_d3`: `D3 | BUFFER PROFIL BEBAN 15-MENIT & BILLING`
- **Tipe Media:** SPI Flash Eksternal `W25Q32` (SPI2)
- **Fungsi:** Menyimpan deret berkala profil beban 23-kanal dengan interval integrasi 15-menit (retensi 60 hingga 90 hari), pencatatan nilai puncak *Maximum Demand* (MD) bulanan, serta rekaman penagihan *billing* 12-bulan.
- **Aliran Data:**
  - `df_e3_d3_w` (Tulis): `Perekaman Interval 15-M, Peak MD & Billing Bulanan` (`p3_0` $\rightarrow$ `ds_d3`)
  - `df_d3_e3_r` (Baca): `Data Riwayat Profil Beban & Rekaman Billing Bulanan` (`ds_d3` $\rightarrow$ `p3_0`)

### D. `ds_d4`: `D4 | TABEL TARIF ToU & KONFIGURASI KONTRAK`
- **Tipe Media:** SPI Flash Eksternal `W25Q32` (SPI2)
- **Fungsi:** Menyimpan skedul tarif WBP/LWBP 8-zona waktu, daya batas kontrak pelanggan (VA), IDPEL, konstanta meter, dan parameter kalibrasi tera.
- **Aliran Data:**
  - `df_e3_d4_w` (Tulis): `Pembaruan Skedul ToU, IDPEL & Parameter` (`p3_0` $\rightarrow$ `ds_d4`)
  - `df_d4_e3_r` (Baca): `Parameter Tarif 8-Zona, Daya Kontrak & Kalibrasi` (`ds_d4` $\rightarrow$ `p3_0`)

---

## 3. Optimasi Geometri & Perutean Bebas Tabrakan (*Zero Collision*)

1. **Simetri Grid & Koridor Pusat:**
   - Kolom Kiri (`ds_d1`, `ds_d2`): `x = 540`, `width = 280` (spans $540..820$). Jarak aman ke `ent_user` ($x=220..500$) adalah $40\text{ px}$.
   - Kolom Kanan (`ds_d4`, `ds_d3`): `x = 905`, `width = 280` (spans $905..1185$). Jarak aman ke `ent_load` ($x=1200..1490$) adalah $15\text{ px}$.
   - Koridor Pusat: $x=820$ hingga $x=905$ (lebar $85\text{ px}$, simetris pada garis tengah $x=865$).
2. **Koneksi Vertikal Murni `D1` & `D4`:**
   - Aliran tulis dan baca untuk `D1` dan `D4` berjalan tegak lurus vertikal tanpa belokan ($0\text{ bend point}$) di sisi kiri dan kanan bawah `Proses 3.0`.
3. **Koneksi Koridor Pusat `D2` & `D3`:**
   - Aliran tulis dan baca untuk `D2` dan `D3` memanfaatkan koridor pusat dengan belokan ortogonal $90^\circ$ tunggal yang rapi, menghindari pemotongan pada *shape* lain.
4. **Isolasi Aliran Pelanggan & Beban:**
   - Aliran `df_e3_user_disp` (ke `ent_user`) dialihkan melalui $y=535$ di sisi luar kiri ($x=430$), dan `df_e3_load_trip` (ke `ent_load`) dialihkan melalui $y=535$ di sisi luar kanan ($x=1272$), sehingga terbebas $100\%$ dari perpotongan garis dengan panah-panah *data store*.

---

## 4. Hasil Verifikasi Otomasi & Uji

- **Integritas DFD:**
  - Total Vertices: 13
  - Total Edges: 25
  - Broken Edges / Orphan Ports: **0**
  - Bounding Box Overlaps: **0**
- **Verifikasi Kompilasi & Tes:**
  - Build `NonSecure/build`: **0 errors, 0 warnings**
  - Unit Tests `ctest --test-dir E3/build`: **12/12 passed (100%)**
- **Sinkronisasi Berkas:**
  - `Acuan/State_DF_D.xml` (Tab 3: `DFD Level 1: Dekomposisi Tiga Pilar`) $\rightarrow$ Diperbarui & Terverifikasi
  - `docs/system_dfd_diagram.drawio.xml` (Tab 2: `DFD Level 1: Dekomposisi Tiga Pilar`) $\rightarrow$ Diperbarui & Terverifikasi
