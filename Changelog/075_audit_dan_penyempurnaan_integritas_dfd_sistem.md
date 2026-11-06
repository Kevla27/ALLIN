# Audit & Penyempurnaan Integritas Data Flow Diagram (DFD) Sistem

**Tanggal:** 06 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Technical Design & Data Architecture Documentation (DFD Gane & Sarson / Yourdon & DeMarco)  
**Komponen Terdampak:** Dokumentasi Sistem ([`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml))

---

## 1. Latar Belakang & Analisis Integrator

Menindaklanjuti instruksi verifikasi mendalam terhadap diagram aliran data sistem (**Data Flow Diagram / DFD**), dilakukan audit komprehensif pada berkas [`docs/system_dfd_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_dfd_diagram.drawio.xml) yang mencakup:
1. **DFD Level 0:** Diagram Konteks Kotak Hitam (*Black Box*).
2. **DFD Level 1:** Dekomposisi Tiga Pilar Makro (Pilar E1 Metrologi, Pilar E2 DLMS/COSEM, dan Pilar E3 Aplikasi/HMI).

Audit mencakup verifikasi sintaks XML, keterhubungan titik sumber (*source*) dan tujuan (*target*) pada setiap panah (*edges*), kepatuhan kaidah DFD standar (keseimbangan aliran masuk/keluar, frasa benda, isolasi entitas eksternal), serta keselarasan dengan arsitektur program nyata (*firmware* STM32U585 ALLIN).

---

## 2. Temuan Audit & Perbaikan yang Diterapkan

| No | Komponen / Diagram | Masalah / Temuan Awal | Perbaikan yang Diterapkan | Status |
| :---: | :--- | :--- | :--- | :---: |
| 1 | **DFD Level 0 (Diagram 1)** | Panah `df_tech_sys` (`Perintah Audit OBIS & Set IDPEL`) kehilangan atribut `source="ent_tech"`, menyebabkan panah melayang tanpa titik jangkar di Draw.io. | Ditambahkan atribut `source="ent_tech"` dengan titik keluar `exitX=0; exitY=0.75` dan titik masuk `entryX=1; entryY=0.5`. Panah kini terjangkar 100% valid. | **Selesai** |
| 2 | **DFD Level 0 (Diagram 1)** | Kanvas diatur pada `pageWidth="827"` (A4 Portrait), padahal elemen diagram membentang hingga $X = 1380\text{ px}$, menyebabkan kanvas terpotong saat diekspor. | Disesuaikan kanvas ke dimensi proporsional `pageWidth="1460"` dan `pageHeight="1000"` (Landscape) dengan banner header $1380\text{ px}$. | **Selesai** |
| 3 | **DFD Level 0 (Diagram 1)** | Entitas beban (`ent_load`) berada di koordinat $X = 460$, melenceng 10 px dari sumbu simetris Proses `p0` ($X = 470$) dan `ent_user` ($X = 470$). Entitas `ent_tech` ($X = 1101$) melenceng dari `ent_hes` ($X = 1100$). | Dilakukan perataan (*alignment*) presisi: `ent_load` digeser ke $X = 470$, `ent_tech` digeser ke $X = 1100$, dan garis kontrol relai dirapikan secara simetris. | **Selesai** |
| 4 | **DFD Level 1 (Diagram 2)** | Penataan ulang tata letak spasial dan perutean ortogonal bebas tabrakan sesuai model Draw.io pengguna. | Mengadopsi langsung model XML pengguna: penataan koordinat `p1_0` ($X=480$), `ds_nvm` ($Y=660$), `ent_tamper` ($Y=397$), `ent_user` ($Y=750$), `ent_load` ($Y=740$), `ent_hes` ($X=1490$), dan `ent_opt` ($X=1380$). | **Selesai** |
| 5 | **DFD Level 1 (Diagram 2)** | Standarisasi tata bahasa aliran data ke/dari Data Store `D1` (sebelumnya menggunakan kata kerja imperatif *"Tulis"* dan *"Baca"*). | Diperbarui menjadi frasa benda murni baku DFD: `df_e3_ds_write` $\rightarrow$ **`Rekaman Billing & Logbook Tamper`**, dan `df_ds_e3_read` $\rightarrow$ **`Parameter Tarif & Konfigurasi`**. | **Selesai** |

---

## 3. Matriks Keseimbangan (*Balancing Matrix*) DFD Level 0 vs Level 1

Berdasarkan kaidah baku DFD Gane & Sarson / Yourdon & DeMarco, seluruh aliran data dari/ke entitas eksternal pada DFD Level 0 wajib memiliki korespondensi yang setara pada DFD Level 1:

| Entitas Eksternal | Aliran Data Level 0 (Konteks) | Aliran Data Level 1 (Tiga Pilar) | Status Balancing |
| :--- | :--- | :--- | :---: |
| **Jaringan Listrik PLN** | `Sinyal V/I Analog, Zero-Cross & Status AC_FAIL` ($\rightarrow$ Sistem) | `Sinyal V/I Analog & Status Grid` ($\rightarrow$ Proses 1.0 E1) | **Seimbang** |
| **Sensor Sabotase Fisik** | `Interupsi Sabotase Tutup/Magnet/Bypass (EXTI)` ($\rightarrow$ Sistem) | `Sinyal Sabotase (EXTI)` ($\rightarrow$ Proses 3.0 E3) | **Seimbang** |
| **Pelanggan Listrik** | `Tombol Navigasi & Konfirmasi Rekoneksi` ($\rightarrow$ Sistem) | `Input Tombol (▲/▼)` ($\rightarrow$ Proses 3.0 E3) | **Seimbang** |
| **Pelanggan Listrik** | `Tayangan OLED SPLN & LED PB7` (Sistem $\rightarrow$) | `Tampilan Layar & Status LED` (Proses 3.0 E3 $\rightarrow$) | **Seimbang** |
| **Instalasi Beban** | `Pulsa Koil Relai Breaker Trip/Close (PC8/PC9)` (Sistem $\rightarrow$) | `Kontrol Relai (Trip/Close)` (Proses 3.0 E3 $\rightarrow$) | **Seimbang** |
| **Instalasi Beban** | `Umpan Balik Status Kontak Aux (PC7)` ($\rightarrow$ Sistem) | `Feedback Kontak Relai` ($\rightarrow$ Proses 3.0 E3) | **Seimbang** |
| **Petugas Lapangan (HHU)**| `Perintah Audit OBIS & Set IDPEL (Optik USART3)` ($\rightarrow$ Sistem) | `Perintah Audit & Konfigurasi` ($\rightarrow$ Proses 2.0 E2) | **Seimbang** |
| **Petugas Lapangan (HHU)**| `Respons Frame HDLC Audit Data Meter (Optik)` (Sistem $\rightarrow$) | `Data Billing & Logbook` (Proses 2.0 E2 $\rightarrow$) | **Seimbang** |
| **Sistem Pusat PLN (HES)**| `Perintah Remote Disconnect/Reconnect & Sync RTC` ($\rightarrow$ Sistem) | `Perintah HES & Unduh Profil` ($\rightarrow$ Proses 2.0 E2) | **Seimbang** |
| **Sistem Pusat PLN (HES)**| `APDU DLMS, Profil Beban, Trap & Last-Gasp` (Sistem $\rightarrow$) | `Data DLMS (AES-128)` (Proses 2.0 E2 $\rightarrow$) | **Seimbang** |

---

## 4. Hasil Verifikasi Sistem

1. **Integritas Parsing Draw.io XML:**
   - Seluruh 8 entitas/proses dan 10 panah pada Level 0 terhubung sempurna (`source` dan `target` terverifikasi).
   - Seluruh 11 entitas/proses/datastore dan 15 panah pada Level 1 terhubung sempurna.
2. **Kompilasi Firmware C:**
   - `ninja -C NonSecure/build`: **0 Error, 0 Warning**.
3. **Unit Test CTest (Pilar E3):**
   - `ctest --test-dir E3/build`: **12/12 Passed (100%)**.

