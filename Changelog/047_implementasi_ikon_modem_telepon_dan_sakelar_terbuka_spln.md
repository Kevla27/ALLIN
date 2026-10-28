# LOG-047: Implementasi Ikon Grafis Modem Telepon Lingkaran dan Sakelar Terbuka Sesuai Standar Layar LCD SPLN D3.006:2021 Gambar 6

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Application, UI/Display OLED SSD1306, HMI, DLMS Communication Bridge)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan klausul **SPLN D3.006:2021 Butir 6.11 Layar Tampilan (Halaman 39, Gambar 6 Layout Layar LCD Tiga Fasa)**:
* `▪` **[Simbol Gagang Telepon di dalam Lingkaran]**: *"Menginformasikan meter sedang berkomunikasi dengan modem."*
* `▪` **[Simbol Sakelar Terbuka]**: *"Menginfomasikan status relai/kontaktor atau relay output untuk shunt trip terbuka."*

Kedua ikon tersebut merupakan simbol resmi pada LCD meter listrik tiga fasa PLN yang mengindikasikan status komunikasi data AMR/AMI (DLMS/COSEM HDLC) dan status pemutus beban fisik meter.

---

## 2. Permasalahan Sebelum Perbaikan

1. **Simbol Komunikasi Modem Telepon:**  
   Sebelumnya, aktivitas komunikasi modem seluler (DLMS/COSEM HDLC pada USART2) belum memiliki representasi grafis khusus pada status bar layar OLED SSD1306.
2. **Simbol Sakelar / Relai Terbuka:**  
   Representasi sebelumnya menggunakan lambang blok generik, belum menyerupai simbol skematik resmi SPLN Gambar 6 yang memiliki **dua terminal lingkaran berlubang** (`--o  o--`) dengan **bilah sakelar yang terbuka miring** (`--o / o--`).

---

## 3. Rincian Perubahan Implementasi

### A. Ikon Grafis Sakelar Terbuka / Tertutup (`bsp_display_oled.c`)
* Dirender pada koordinat $X: 0..8, Y: 1..7$ ($9\times 7$ piksel).
* **Terminal:** Memiliki dua cincin terminal bulat berongga di kiri ($X: 2..4$) dan kanan ($X: 6..8$).
* **Kondisi Terbuka (Relai Trip / Shunt Trip Aktif):**
  * Bilah sakelar terangkat diagonal ke atas (`/`) dari terminal kanan menuju kiri atas.
  * Berkedip jika meter dalam status SIAP SAMBUNG (READY ON).
* **Kondisi Tertutup (Beban Tersambung Normal):**
  * Garis horizontal lurus menyambungkan kedua terminal (`--o===o--`).

### B. Ikon Grafis Modem Telepon Lingkaran (`bsp_display_oled.c`)
* Dirender pada koordinat $X: 10..16, Y: 1..7$ ($7\times 7$ piksel).
* **Bentuk:** Lingkaran terluar $7\times 7$ piksel membungkus siluet gagang telepon (earpiece atas, gagang melengkung samping, dan mouthpiece bawah).
* **Aktivasi:** Ikon menyala/berkedip saat meter mendeteksi aktivitas komunikasi bingkai DLMS HDLC pada modem seluler (USART2), dan padam saat mode idle setelah timeout 3 detik.

### C. Plumbing Status Komunikasi Modem (`display.h`, `display.c`, `e3_meter_app.h`, `e3_meter_app.c`)
* Menambahkan field `bool is_modem_active;` pada struktur `display_context_t` dan `display_spln_frame_t`.
* Menambahkan API `display_set_modem_status(display_context_t *ctx, bool active);`.
* Menambahkan fungsi trigger `e3_meter_app_notify_modem_activity(void);` di aplikasi utama yang mengaktifkan timer pulsa 3 detik setiap kali paket data diterima/dikirimkan.

### D. Penataan Ulang Presisi Baris 1 Status Bar OLED ($128\times 32$)
Semua 10 elemen status bar diatur tanpa ada tumpang tindih piksel sepanjang 128 pixel:
1. **Sakelar Relai/Kontaktor (`--o / o--`):** $X = 0..8$ ($9\text{ px}$)
2. **Modem Telepon Lingkaran (☎):** $X = 10..16$ ($7\text{ px}$)
3. **Segitiga Sabotase / Tamper ([!]):** $X = 18..24$ ($7\text{ px}$)
4. **Baterai 2-Level Gauge ([B]):** $X = 26..33$ ($8\text{ px}$)
5. **Panah Arah Energi ([►] / [◄]):** $X = 35..39$ ($5\text{ px}$)
6. **Kuadran Daya 4-Quadrant ($\bigoplus$):** $X = 41..47$ ($7\text{ px}$)
7. **Badge Tarif ToU:** $X = 49..54$ ($6\text{ px}$)
8. **Tegangan Fasa `URST`:** $X = 56..79$ ($24\text{ px}$)
9. **Arus Fasa `IRST`:** $X = 81..104$ ($24\text{ px}$)
10. **Register Lampiran E / OBIS:** $X = 106..127$ ($22\text{ px}$)

---

## 4. Pengujian dan Verifikasi

1. **Unit Test CTest (`test_spln_display_tamper.c`):**
   * Menambahkan pengujian `[UJI 14] Verifikasi Ikon Modem Telepon & Relai Sakelar (SPLN Gambar 6)`.
   * Memvalidasi transisi state modem `is_modem_active = false` $\rightarrow$ `is_modem_active = true`.
   * Memvalidasi transisi state relai `RELAY_STATE_CLOSED` $\rightarrow$ `RELAY_STATE_OPEN`.
   * Seluruh 12 test suite CTest lulus 100% (0 fail).
2. **Kompilasi Target ARM Cortex-M33 (STM32U585):**
   * Berhasil dikompilasi dengan ARM GCC tanpa peringatan maupun error link (`ALLIN_S.elf` dan `ALLIN_NS.elf` sukses ter-generate).
