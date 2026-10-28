# Log Perubahan 035: Visualisasi Ikon Grafis Status Bar Khusus Layar OLED (Relai, Baterai Gauge, Tamper Warning & Tarif ToU Badge)

**ID Log:** LOG-035  
**Tanggal:** 28 Oktober 2026  
**Topik Utama:** Visualisasi Ikon Grafis Status Bar Khusus Layar OLED (Relai Kontaktor, Baterai Gauge, Tamper Warning Triangle & Tarif ToU Badge)  
**Penanggung Jawab:** Display & HMI Lead (Pilar E3)

---

## 1. Latar Belakang & Urgensi Rekayasa
Pada modul purwarupa **OLED SSD1306 (128x32 / 128x64 I2C3)** yang tiba lebih dahulu, layar memiliki kapabilitas matriks piksel monokrom penuh (bukan sekadar segmen angka 7-segment). Sebelumnya, Baris 1 (Header status) dirender menggunakan karakter teks ASCII biasa (`OK`, `!ALM`, `[B]`).

Agar pengalaman pengguna (*user experience*) dan pengujian visual di laboratorium menjadi intuitif dan interaktif saat modul OLED fisik dihubungkan, Baris 1 status bar ditingkatkan menjadi **ikonografi grafis piksel presisi tinggi** yang merefleksikan seluruh status pilar E3 (relai kontaktor, baterai, sabotase, tarif ToU, fasa L123, dan aliran arus REV) secara langsung.

---

## 2. Rincian Implementasi Ikonografi Grafis (`bsp_display_oled.c`)

### A. Ikon Fisik Relai Kontaktor (`oled_draw_relay_icon`, X: 1..11, Y: 1..8)
- **Terminal Kontak:** Dua kutub titik 2x2 piksel pada sisi kiri dan kanan.
- **Status Normal (Relai Terhubung):** Garis horizontal ganda menghubungkan kedua terminal ($o\text{---}o$), menandakan beban konsumen tersambung normal.
- **Status Trip / Buka (Overload / Under-Volt / Over-Volt):** Garis kontak terangkat miring ke atas ($o/\ \ o$), memberikan visualisasi jelas bahwa kontaktor telah memutus aliran listrik.
- **Status READY ON:** Garis kontak berkedip periodik 1 Hz, menandakan meter dalam status menunggu penekanan tombol simultan (▲ + ▼) untuk penyambungan kembali (SPLN Tabel 5 No 12).

### B. Ikon Sabotase / Alarm Internal (`oled_draw_tamper_alarm_icon`, X: 13..21, Y: 0..8)
- **Status Normal:** Ikon tanda centang halus (✓) selebar 7 piksel, menandakan seluruh sensor hardware dan parameter kelistrikan dalam keadaan sehat.
- **Status Sabotase / Alarm Hardware Aktif:** Simbol segitiga peringatan dengan tanda seru di dalamnya (▲!), memberikan indikasi visual tegas terhadap upaya pembobolan meter atau kegagalan internal.

### C. Indikator Pengukur Baterai Grafis (`oled_draw_battery_gauge`, X: 24..35, Y: 1..7)
- Menggambar kontur bodi baterai 11x7 piksel dengan tonjolan kutub positif pada sisi kanan.
- **Tegangan Normal (>2.7 V):** Terisi 3 baris blok level penuh (*solid level bars*).
- **Tegangan Lemah (ERR03 <=2.7 V):** Berkedip pada 1 blok level terendah sesuai persyaratan SPLN D3.006 Butir 6.11.

### D. Badge Status Tarif ToU Prominen (`oled_draw_tariff_badge`, X: 38..56, Y: 0..8)
- **Tarif 1: WBP (Waktu Beban Puncak 17:00 - 22:00):** Kotak putih padat terbalik (*inverted badge*) dengan teks hitam `WBP`.
- **Tarif 2: LWBP1 (Luar Waktu Beban Puncak Malam 22:00 - 06:00):** Teks putih `LW1`.
- **Tarif 3: LWBP2 (Luar Waktu Beban Puncak Siang 06:00 - 17:00):** Teks putih `LW2`.

### E. Indikator Fasa, Arah Aliran Arus & Kode OBIS (X: 59..127, Y: 0..8)
- **Tegangan Fasa (X: 59):** Indikator `L123` dengan strip `-` jika fasa hilang.
- **Arus & Aliran Daya (X: 80):** Indikator `I123` atau teks terbalik `REV` saat daya aktif bernilai negatif (aliran daya balik/ekspor PLTS).
- **Kode OBIS (X: 101..127):** Kode register SPLN rata kanan (misal `01.08`, `32.07`, `96.01`).
- **Garis Pemisah (Y: 9):** Garis pemisah halus (putus-putus saat normal, tebal pekat saat alarm aktif).

---

## 3. Integrasi Struktur Data Tampilan (`display.h` & `display.c`)
Menambahkan field status baru pada struktur `display_spln_frame_t`:
```c
bool is_relay_closed;         /* Status fisik relai kontaktor (true: Terhubung/Closed, false: Trip/Buka) */
bool is_relay_ready_on;       /* Status relai READY ON (siap sambung kembali dengan tombol ▲+▼) */
bool is_tamper_active;        /* Status sabotase terdeteksi (case open, terminal open, magnet, dll) */
uint8_t active_tariff_zone;   /* Zona tarif aktif (0: WBP, 1: LWBP1, 2: LWBP2) */
```
Field-field ini secara otomatis dievaluasi dan diisi dalam fungsi `display_render_spln_frame()` berdasarkan waktu nyata, status FSM relai, dan status tamper.

---

## 4. Hasil Pengujian & Kompilasi

### A. Pengujian Unit Test CTest Host (9/9 Suites - 100% Passed)
```text
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo ..............   Passed    0.11 sec
    Start 2: Unit_E3_Display_Tamper ...........   Passed    0.09 sec
    Start 3: Unit_E3_Power_Relay ..............   Passed    0.08 sec
    Start 4: Unit_E3_FSM_Statechart ...........   Passed    0.08 sec
    Start 5: Unit_E3_Load_Profile .............   Passed    0.09 sec
    Start 6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.15 sec
    Start 7: Stress_E3_Final_System_Audit .....   Passed    0.19 sec
    Start 8: Unit_E3_Console_CLI ..............   Passed    0.14 sec
    Start 9: Unit_E3_Board_Profile ............   Passed    0.07 sec

100% tests passed, 0 tests failed out of 9
```

### B. Kompilasi Target Hardware ARM Cortex-M33 (GCC STM32U585)
- **Secure Domain (`Secure/build`):** ROM 23.592 B (2.27%), RAM 12.048 B (4.60%) — *0 Error, 0 Warning*.
- **Non-Secure Domain (`NonSecure/build`):** ROM 149.896 B (14.30%), RAM 65.064 B (12.41%) — *0 Error, 0 Warning*.

