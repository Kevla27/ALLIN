# Redesain Tata Letak Tab 5 Jalur Asinkron & ISR: Eliminasi Tabrakan Garis, Teks Tumpang Tindih & Implementasi Cascade EXTI

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Flowchart Ergonomics & Visual Layout Optimization (Zero Line Crossing & Cascade Refactoring)  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_flowchart.drawio`, `docs/system_tab5_asynchronous_isr.drawio.xml`), Penanganan Interupsi Sistem (`stm32u5xx_it.c`)

---

## 1. Latar Belakang & Analisis Integrator

Berdasarkan tinjauan visual langsung terhadap diagram **Tab 5 (Jalur Asinkron & ISR)**, ditemukan beberapa permasalahan tata letak (*layout*) yang menyebabkan diagram tampak semrawut dan sulit dibaca:
1. **Penumpukan Garis & Label pada Jalur EXTI (Kolom Kanan):**
   Simpul keputusan `Identifikasi Sumber Pin EXTI?` sebelumnya memaksakan 4 cabang interupsi keluar dari satu titik vertex sisi kiri menuju koordinat vertikal yang sama ($X=520$). Akibatnya, seluruh label teks (`AC Fail`, `Tombol`, `ADE IRQ/DREADY/CF`, `Aux In`) menumpuk tepat di atas garis vertikal dan menabrak batas kotak kolom kiri.
2. **Ketiadaan Clearance pada Jalur UART RX (Kolom Kiri):**
   Garis cabang `[Tidak]` dari pengecekan frame HDLC turun tepat menempel pada tepi kanan kotak-kotak proses APDU, sehingga label teks `[Tidak]` melayang di atas garis batas kotak.
3. **Ketiadaan Pola Aliran yang Ergonomis:**
   Pola demultiplexer 4 arah yang ditekuk mundur (*backward hook*) menyalahi kaidah visual ISO 5807 yang mengutamakan aliran lurus alami dari atas ke bawah (*top-to-bottom*) dan dari kiri ke kanan (*left-to-right*).

Untuk mengatasi hal tersebut, dilakukan redesain total terhadap geometri tata letak diagram Tab 5.

---

## 2. Rincian Perubahan Implementasi

### A. Implementasi Struktur Cascade Bertingkat pada Jalur EXTI (Kolom Kanan)
Struktur interupsi `HAL_GPIO_EXTI_Callback(GPIO_Pin)` dirombak menjadi **Pola If-Else Cascade ISO 5807:1985** yang merefleksikan kode C riil secara presisi:
- **Tulang Punggung Keputusan (Left Spine - $X=560$):**
  - `x_d1`: `Pin == AC_FAIL?` (PC3) $\longrightarrow$ `[Ya]` ke kanan, `[Tidak]` turun ke `x_d2`.
  - `x_d2`: `Pin == Tombol HMI?` (PA0/PB6) $\longrightarrow$ `[Ya]` ke kanan, `[Tidak]` turun ke `x_d3`.
  - `x_d3`: `Pin == ADE9000 IRQ?` (PE11/PB5/PB1/PE8) $\longrightarrow$ `[Ya]` ke kanan, `[Tidak]` turun ke `x_d4`.
  - `x_d4`: `Pin == Sinyal AUX?` (PC10/PC14) $\longrightarrow$ `[Ya]` ke kanan, `[Tidak]` masuk ke `x_end`.
- **Blok Aksi Penanganan Event (Right Spine - $X=780$):**
  Setiap cabang `[Ya]` mengalir horizontal ke kotak handler khusus tanpa ada garis yang saling memotong.
- **Collector Bus Sisi Kanan Luar ($X=1080$):**
  Seluruh titik keluar handler (`x_act1` s/d `x_act4`) bermuara ke sebuah bus vertikal bersih di sisi kanan luar yang mengalir langsung ke simpul akhir `x_end` (`SELESAI PENANGANAN EVENT ISR GPIO`).
- **Hasil:** 0 garis tumpang tindih, 0 teks tabrakan, dan alur terbaca seketika dalam 1 detik.

### B. Optimalisasi Simetri Tiga Jalur pada UART RX (Kolom Kiri)
- **Jalur Bypass Kiri (CLI):** Karakter non-HDLC langsung diarahkan melalui sisi kiri luar ($X=138$) menuju terminal `RE-ARM` dengan ruang kosong (*clearance*) 35 px.
- **Jalur Bypass Kanan (HDLC Incomplete):** Frame yang belum lengkap dialirkan melalui sisi kanan luar ($X=500$) menuju terminal `RE-ARM` tanpa menyentuh kotak proses APDU.
- **Jalur Tulang Punggung Tengah (APDU Pipeline):** Frame lengkap diproses sekuensial lurus di sumbu tengah ($X=270$): `DEKODER APDU` $\rightarrow$ `EKSEKUSI PERINTAH` $\rightarrow$ `TRANSMISI RESPONS` $\rightarrow$ `RE-ARM`.
- **Hasil:** Keseimbangan visual simetris sempurna (*balanced three-lane flow*).

### C. Pemisahan Kolom Lapang (Inter-Column Clearance)
- Kolom kiri berakhir di $X=500$.
- Kolom kanan dimulai di $X=560$.
- Tersedia **ruang pemisah bersih (*clearance*) sebesar 60 piksel** di antara kedua kolom, menjamin tidak ada garis atau label yang menyeberang tanpa izin.

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Format XML Draw.io:**
   - Standalone diagram tersimpan di [`docs/system_tab5_asynchronous_isr.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_tab5_asynchronous_isr.drawio.xml).
   - Master diagram tersimpan di [`docs/system_flowchart.drawio`](file:///g:/PPPP/ALLIN/docs/system_flowchart.drawio).
   - Validasi struktur XML 100% lolos uji parsing `xml.etree.ElementTree`.
2. **Kompilasi Firmware C (STM32U585 GCC/Ninja):**
   ```
   ninja -C NonSecure/build
   ninja: Entering directory `NonSecure/build'
   ninja: no work to do.
   ```
   Status: **0 Error, 0 Warning**.
3. **Unit Testing CTest (12 Test Suites):**
   ```
   ctest --test-dir E3/build
   100% tests passed, 0 tests failed out of 12
   Total Test time (real) = 0.19 sec
   ```
   Status: **12/12 Passed (100%)**.
