# LOG-046: Implementasi Ikon Grafis Panah Arah Aliran Energi Aktif dan Penyempurnaan Indikator Baterai Layar OLED (SPLN D3.006:2021 Butir 6.11 Gambar 6)

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Application, UI/Display OLED SSD1306, HMI)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan klausul **SPLN D3.006:2021 Butir 6.11 Layar Tampilan (Halaman 39, Gambar 6 Layout Layar LCD Tiga Fasa)**:
* `▪` **[Simbol Baterai]** Menginformasikan status baterai normal. Bila baterai rusak/mencapai nilai threshold-nya ($2,7\text{ V}$), maka simbol berkedip.
* `▪` **[Simbol Panah Arah]** Menginformasikan arah energi aktif terbalik salah satu atau lebih fase.

---

## 2. Permasalahan Sebelum Perbaikan

1. **Simbol Baterai:**  
   Sudah ada, namun alokasi horizontalnya dapat dirapikan ($8\times 7$ piksel pada $X: 16..23$) agar memberikan ruang yang ideal bagi simbol panah arah aliran energi aktif.
2. **Simbol Panah Arah Energi Terbalik:**  
   Sebelumnya, saat terjadi aliran energi aktif terbalik (`is_reverse_power = true` akibat ekspor PLTS), driver OLED hanya mencetak teks string `"REV"` di koordinat $X=81..104$ dan **menimpa tulisan arus fasa `IRST`**. Hal ini tidak sesuai dengan Gambar 6 SPLN yang mensyaratkan **simbol panah grafis**, sekaligus menghilangkan visibilitas teknisi terhadap kehadiran fasa arus aktif $I_R, I_S, I_T$.

---

## 3. Rincian Perubahan Implementasi

### A. Penambahan Ikon Grafis Panah Arah Energi Aktif (`E3/drivers/oled/src/bsp_display_oled.c`)
* Menambahkan fungsi presisi piksel `oled_draw_power_direction_arrow(uint8_t x, uint8_t y, bool is_reverse, bool blink)`:
  * Berukuran $5\times 7$ piksel pada koordinat $X: 25..29, Y: 1..7$.
  * **Aliran Normal / Impor PLN ($+P$):** Menampilkan panah segitiga solid mengarah ke kanan ($[\blacktriangleright]$).
  * **Aliran Terbalik / Ekspor PLTS ($-P$):** Menampilkan panah segitiga solid berbalik mengarah ke kiri ($[\blacktriangleleft]$) dan berkedip (*blinking* 1 Hz) untuk menarik perhatian teknisi lapangan.

### B. Preservasi Penuh Indikator Fasa Arus `IRST`
* Karena arah aliran energi kini telah divisualisasikan secara dedicated oleh ikon grafis panah $[\blacktriangleright]$ / $[\blacktriangleleft]$, string arus fasa `IRST` di $X: 81..104$ **selalu ditampilkan secara utuh** tanpa pernah tertimpa teks `"REV"`.

### C. Susunan Lengkap Baris 1 Status Bar OLED ($128\times 32$)
Seluruh elemen berjejer rapi sesuai urutan Gambar 6 Butir 6.11 SPLN:
1. **Relai Kontaktor:** $X = 0..7$ (8 px)
2. **Tamper Warning Segitiga:** $X = 8..14$ (7 px)
3. **Baterai 2-Level Gauge:** $X = 16..23$ (8 px) $\rightarrow$ Berkedip jika $< 2,7\text{ V}$ (`ERR03`)
4. **Panah Arah Energi:** $X = 25..29$ (5 px) $\rightarrow$ $[\blacktriangleright]$ Maju / $[\blacktriangleleft]$ Terbalik
5. **Kuadran Daya $\bigoplus$:** $X = 31..37$ (7 px) $\rightarrow$ 4-Kuadran real-time
6. **Badge Tarif ToU:** $X = 39..54$ (16 px) $\rightarrow$ `WBP`, `LW1`, `LW2`
7. **Tegangan Fasa `URST`:** $X = 56..79$ (24 px)
8. **Arus Fasa `IRST`:** $X = 81..104$ (24 px)
9. **Register Lampiran E / OBIS:** $X = 106..127$ (22 px)

---

## 4. Hasil Verifikasi

* **CTest Suite:** 12/12 unit test lulus 100%.
* **ARM GCC Target STM32U585:** Kompilasi target firmware selesai tanpa error maupun warning baru.
