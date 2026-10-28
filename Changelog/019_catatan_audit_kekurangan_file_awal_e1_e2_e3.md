# Log 019: Catatan & Laporan Audit Kekurangan File Awal Pilar E1, E2, dan E3

Dokumen ini merupakan catatan retrospektif dan audit teknis komprehensif yang menginventarisasi seluruh kekurangan, celah fungsional (*functional gaps*), dan deviasi arsitektur pada **file awal Pilar E1, E2, dan E3** sebelum dilakukan pembenahan terpadu berbasis standar **SPLN D3.006: 2021**, **Desain Sistem.pdf**, **Spesifikasi .pdf**, dan **PIN.docx**.

---

## 1. Ringkasan Eksekutif Kesenjangan (*Gap Analysis*)

Pada kondisi awal proyek, firmware memiliki kode monolitik mentah pada `main.c` dengan beberapa modul pustaka terpisah yang belum terhubung secara kohesif:

- **Pilar E1 (Metrologi):** Hanya membaca RMS tegangan dan arus dasar, belum menghitung energi reaktif billing penalti kVARh ($\cos\varphi < 0.85$), belum memisahkan energi reaktif 4-kuadran, belum mengukur arus netral $I_N$, belum mengekstrak sudut diagram fasor, dan driver SPI blocking tanpa dukungan mode daya rendah (*Last Gasp*).
- **Pilar E2 (DLMS/COSEM):** Hanya memiliki parser dasar untuk variabel sesaat, belum memiliki objek *Profile Generic* (Class ID 7) untuk data historikal (Lampiran F) dan Load Profile 24-kanal (Lampiran G), belum memiliki pemetaan OBIS multi-tarif ToU dan net-metering PLTS, serta belum ada manajemen dual-port UART (optik vs modem).
- **Pilar E3 (HMI, Tamper, Relai, Profiling):** Hanya memiliki 14 halaman konsol teks biasa tanpa layout fisik resmi SPLN Gambar 4, belum memiliki 35 parameter Tabel E.2, belum mendukung 9 vektor sabotase lengkap (Tabel 6) dan 8 alarm kerusakan internal hardware (Tabel 4), relai belum memiliki kurva overload bertingkat dan status pengaman `READY ON`, buzzer tanpa kadensi denyut 200 ms, serta ketiadaan Maximum Demand 15-menit, snapshot billing 12 bulan, dan wake-up LCD 10 detik saat listrik padam.

---

## 2. Rincian Kekurangan Per Pilar

### A. Pilar E1: Metrologi (AFE ADE9000 & Driver SPI)
1. **Ketiadaan Energi Reaktif Billing Kompensasi ($ER_{BIL}$)** (SPLN Butir 6.1 & Tabel E.1 No 8):
   - Standar PLN mewajibkan penalti kVARh bila $\cos\varphi < 0.85$ ($\tan\varphi > 0.62$). File awal belum memiliki formula ini.
2. **Ketiadaan Akumulasi Energi Reaktif 4-Kuadran ($Q_1, Q_2, Q_3, Q_4$)**:
   - Belum ada pemisahan energi reaktif induktif dan kapasitif pada arah ekspor dan impor.
3. **Pengabaian Kanal Pengukuran Arus Netral ($I_N$)**:
   - Register `NIRMS` (`0x20C`) tidak dibaca secara siklis, sehingga bypass kawat netral tidak terdeteksi.
4. **Ketiadaan Ekstraksi Sudut Fasa / Diagram Fasor**:
   - Register *time difference zero-crossing* ADE9000 (`ANGL_VA_VB`, `ANGL_VB_VC`, `ANGL_VA_IA`, dll.) tidak dikonversi ke derajat fasor ($0^\circ \dots 360^\circ$).
5. **Driver SPI Blocking & Absennya Manajemen Daya Rendah (*Last Gasp*)**:
   - SPI blocking lambat. Saat listrik padam (`AC_FAIL_DET`), ADE9000 tidak dimasukkan ke mode Sleep (`RUN = 0`), sehingga superkapasitor cepat habis sebelum data tersimpan.
6. **Monolitik & Polusi di `main.c`**:
   - Logika SPI ADE9000 tertumpuk di `main.c` dan tidak aman dipanggil dari thread RTOS.

### B. Pilar E2: Komunikasi & Protokol (DLMS/COSEM)
1. **Objek Profil Generik (*Profile Generic* — Class ID 7) Kosong**:
   - Belum ada ring buffer circular untuk **Historical Data 54 parameter** (Lampiran F) dan **Load Profile 24 kanal** interval 15 menit (Lampiran G).
2. **Kekurangan Pemetaan Kode OBIS Kritis**:
   - Belum ada OBIS ToU 3-Tarif (`01.08.1`, `01.08.2`, `01.08.3`).
   - Belum ada OBIS aliran daya dua arah PLTS (`02.08.0`, `02.07.0`, `04.04.0`).
   - Belum ada OBIS Maximum Demand 15-menit (`01.06.0`, `01.02.0`, `01.01.0`, `09.06.0`).
3. **Ketiadaan Routing Komunikasi Port Ganda (*Dual UART*)**:
   - Belum memisahkan tugas port optik lokal (USART2) dan port modem jarak jauh (USART3).
4. **Keamanan Asosiasi Belum Terstandarisasi**:
   - Belum ada pembagian level otentikasi (Public, LLS, HLS GMAC Suite 0/1).

### C. Pilar E3: HMI Layar, Tamper, Relai Pembatas Beban, Profiling & Buzzer
1. **Modul Tampilan (HMI & Display)**:
   - Awalnya hanya 14 halaman konsol teks.
   - Format fisik LCD Gambar 4 (Simbol Baris 1, Nilai Baris 2, OBIS/Satuan Baris 3, zz pojok) belum ada.
   - 35 parameter Tabel E.2 tidak tersedia (sudut fasa, kVARh, daya impor, kVA semu, MD timestamp, CMD, MD reset counter, CSUM, EoB, PLN unit, LCD test).
   - Driver OLED I2C3 belum terhubung ke siklus render.
2. **Modul Deteksi Sabotase (Tamper Manager)**:
   - Hanya 1–2 event kasar. Belum mendukung 9 vektor sabotase lengkap SPLN Tabel 6 (Case Open, Terminal Open, Urutan Fasa Terbalik, Cross Phase, Bypass Netral, Hilang Fasa, Medan Magnet 500 mT, Reverse Power, Low PF Kuadran 4).
   - Belum ada penanganan 8 alarm internal hardware SPLN Tabel 4 (`ERR00` s.d. `ERR07`).
3. **Modul Relai Pembatas Beban (Power Relay Manager)**:
   - Pemutusan beban dilakukan instan tanpa toleransi waktu durasi kelebihan daya (*Overload Duration Threshold*).
   - Belum ada timer lockout dan fitur pengaman `READY ON` (SPLN Tabel 5 No 12).
4. **Modul Audio Buzzer (BSP Alarm Buzzer)**:
   - Buzzer tidak memiliki modulasi denyut 200 ms ON / 200 ms OFF saat alarm aktif (SPLN klausul 6.14) dan beep pendek 50 ms untuk feedback tombol.
5. **Modul Profiling & Ketahanan Daya (*Last Gasp*)**:
   - Belum ada pelacak Maximum Demand 15-menit.
   - Belum ada ring buffer snapshot billing 12 bulan berintegritas CRC-16.
   - Belum ada wake-up display 10 detik saat listrik padam menggunakan tombol (SPLN klausul 6.11).

---

## 3. Matriks Komparasi: Kondisi Awal vs Kondisi Terkini

| Parameter / Fitur | Status File Awal | Status Terkini (Log 001–018) | Referensi Standar |
| :--- | :---: | :---: | :--- |
| **Energi Reaktif Billing ($ER_{BIL}$)** | ❌ Absen | ✅ Aktif ($\cos\varphi < 0.85$) | SPLN Tabel E.1 No 8 |
| **Energi Reaktif 4-Kuadran ($Q_1 \dots Q_4$)** | ❌ Absen | ✅ Terakumulasi Presisi | SPLN Butir 6.1 |
| **Arus Netral ($I_N$) & Sudut Fasor ($Sdt\_U/I$)** | ❌ Absen | ✅ Terbaca & Terfasilitasi | SPLN Tabel E.2 No 22-31 |
| **Halaman Auto-Carousel** | ❌ 14 Halaman Biasa | ✅ **15 / 15 Lengkap (100%)** | SPLN Tabel E.1 |
| **Halaman Manual Tombol** | ❌ Sangat Minim | ✅ **39 / 39 Lengkap (100%)** | SPLN Tabel E.2 |
| **Total Halaman Tampilan** | 14 Halaman | **49 Halaman Terverifikasi** | SPLN Lampiran E |
| **Format Layar Fisik SPLN Gambar 4** | ❌ Format Bebas | ✅ Format 3-Baris Resmi | SPLN Gambar 4 |
| **Driver Layar OLED Fisik (I2C3)** | ❌ Belum Terhubung | ✅ Terkoneksi & Aktif | Desain Sistem |
| **Deteksi Sabotase (Tamper)** | ❌ 1–2 Event Kasar | ✅ **9 Vektor Lengkap (Tabel 6)** | SPLN Tabel 6 (ERR20–27) |
| **Alarm Kerusakan Hardware** | ❌ Belum Ada | ✅ **8 Alarm Lengkap (Tabel 4)** | SPLN Tabel 4 (ERR00–07) |
| **Kadensi Buzzer 200 ms ON / OFF** | ❌ Statis ON/OFF | ✅ Kadensi Berdenyut Presisi | SPLN Butir 6.14 |
| **Feedback Beep Tombol (50 ms)** | ❌ Tanpa Suara | ✅ Audio Feedback Aktif | SPLN Butir 6.10 |
| **Relai Status `READY ON`** | ❌ Belum Ada | ✅ FSM Pengaman Aktif | SPLN Tabel 5 No 12 |
| **Maximum Demand 15-Menit (kW & kVA)** | ❌ Absen | ✅ Block/Sliding Window Aktif | SPLN Butir 6.1 & Tabel E.1 |
| **Snapshot Billing 12 Bulan** | ❌ Absen | ✅ Ring Buffer + CRC-16 Aktif | SPLN Butir 6.13 & Lampiran F |
| **Load Profile 15-Menit 24 Kanal** | ❌ Kerangka Kosong | ✅ Buffer 72-Byte Siap DLMS | SPLN Lampiran G |
| **Wake-Up Layar 10 Detik saat PLN Mati** | ❌ Mati Total | ✅ Aktif (Baterai / Supercap) | Desain Sistem Hal. 37 |
| **Kepatuhan TrustZone ARM Cortex-M33** | ⚠️ Belum Terpisah | ✅ Secure & NonSecure Terisolasi | Hardware STM32U585 |

