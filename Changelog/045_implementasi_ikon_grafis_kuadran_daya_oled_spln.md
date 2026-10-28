# LOG-045: Implementasi Render Ikon Grafis Kuadran Daya 4-Kuadran pada Layar OLED Sesuai SPLN D3.006:2021 (Butir 6.11 & Lampiran A)

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Application, UI/Display OLED SSD1306, HMI)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan dokumen acuan resmi **SPLN D3.006:2021 ("Spesifikasi Meter Statip Pembayaran Pascabayar dan Prabayar Berbasis Komunikasi Seluler")**:

1. **Butir 6.11 Layar Tampilan (Halaman 39, Gambar 6 Layout Layar LCD Tiga Fasa):**
   * Baris pertama mewajibkan tampilan simbol status fungsional:
     > `▪` $\bigoplus$ **Menginformasikan kuadran;**
     > `▪` Status baterai normal / berkedip jika $< 2,7\text{ V}$;
     > `▪` Arah energi aktif terbalik salah satu atau lebih fase;
     > `▪ XXX` Indikasi tegangan fase $U_R, U_S, U_T$;
     > `▪ YYY` Indikasi arus fase $I_R, I_S, I_T$;
     > `▪` Status relai/kontaktor terbuka.

2. **Lampiran A (Halaman 43, Gambar A.1 Four-Quadrant Power Flow Directions - IEEE Std 1459-2010):**
   * **Kuadran I (Kanan Atas / Q1):** Daya Aktif Impor/Maju ($+P$), Daya Reaktif Induktif ($+Q$) $\rightarrow$ Beban Induktif (misal: motor induksi AC, transformator).
   * **Kuadran II (Kiri Atas / Q2):** Daya Aktif Ekspor/Mundur ($-P$), Daya Reaktif Induktif ($+Q$) $\rightarrow$ Pembangkit PLTS Atap menginjeksi daya aktif dengan PF lagging.
   * **Kuadran III (Kiri Bawah / Q3):** Daya Aktif Ekspor/Mundur ($-P$), Daya Reaktif Kapasitif ($-Q$) $\rightarrow$ Pembangkit PLTS Atap menginjeksi daya aktif dengan PF leading.
   * **Kuadran IV (Kanan Bawah / Q4):** Daya Aktif Impor/Maju ($+P$), Daya Reaktif Kapasitif ($-Q$) $\rightarrow$ Beban Kapasitif (misal: kapasitor bank pabrik).

---

## 2. Permasalahan Sebelum Perbaikan

Meskipun backend metrologi (Pilar E1) dan lapisan data HMI (Pilar E3) telah menghitung kuadran daya via `frame->power_quadrant` (1..4), **driver layar grafis OLED SSD1306 (`bsp_display_oled.c`) belum merender simbol lingkaran kuadran $\bigoplus$ ini ke baris status bar OLED**. Akibatnya, pada tampilan fisik, teknisi di lapangan tidak dapat melihat sektor kuadran mana yang sedang bekerja.

---

## 3. Rincian Perubahan Implementasi

### A. Evaluasi Kondisi Beban Idle/Nol (`E3/app/display/src/display.c`)
* Menyempurnakan evaluasi kuadran daya agar saat meter dalam kondisi idle/tanpa beban ($I_R, I_S, I_T < 20\text{ mA}$ serta $P=0$ dan $Q=0$), nilai kuadran disetel ke `0`:
  * `power_quadrant = 0` $\rightarrow$ Lingkaran dengan tanda silang kosong $\bigoplus$ (kondisi siap/tanpa beban).
  * `power_quadrant = 1` $\rightarrow$ Kuadran 1 (+P, +Q), sektor Kanan-Atas aktif.
  * `power_quadrant = 2` $\rightarrow$ Kuadran 2 (-P, +Q), sektor Kiri-Atas aktif.
  * `power_quadrant = 3` $\rightarrow$ Kuadran 3 (-P, -Q), sektor Kiri-Bawah aktif.
  * `power_quadrant = 4` $\rightarrow$ Kuadran 4 (+P, -Q), sektor Kanan-Bawah aktif.

### B. Fungsi Penggambaran Ikon Grafis Kuadran (`E3/drivers/oled/src/bsp_display_oled.c`)
* Menambahkan fungsi presisi piksel `oled_draw_quadrant_icon(uint8_t x, uint8_t y, uint8_t quadrant)`:
  * Berukuran $7\times 7$ piksel (diameter 7, radius 3) dengan koordinat lokal $X: 28..34, Y: 1..7$.
  * Menggambar lingkaran luar dan garis silang sumbu $P$ (horizontal pada $Y=4$) dan sumbu $Q$ (vertikal pada $X=31$).
  * Mengarsir padat (*solid fill*) area sektor $2\times 2$ piksel sesuai kuadran yang sedang aktif:
    * Kuadran 1: `ssd1306_FillRectangle(x + 4, y + 1, x + 5, y + 2, White)` (Kanan-Atas)
    * Kuadran 2: `ssd1306_FillRectangle(x + 1, y + 1, x + 2, y + 2, White)` (Kiri-Atas)
    * Kuadran 3: `ssd1306_FillRectangle(x + 1, y + 4, x + 2, y + 5, White)` (Kiri-Bawah)
    * Kuadran 4: `ssd1306_FillRectangle(x + 4, y + 4, x + 5, y + 5, White)` (Kanan-Bawah)

### C. Penataan Ulang Tata Letak Status Bar Baris 1 OLED ($128\times 32$)
Seluruh elemen status bar diatur ulang dengan alokasi horizontal yang proporsional dan memiliki pemisah bersih 1 piksel tanpa tabrakan:
1. **Relai Kontaktor:** $X = 0..8$ (lebar 9 px)
2. **Tamper Warning Segitiga:** $X = 9..15$ (lebar 7 px)
3. **Baterai 3-Level Gauge:** $X = 17..25$ (lebar 9 px)
4. **Ikon Kuadran Daya $\bigoplus$:** $X = 28..34$ (lebar 7 px)
5. **Badge Tarif ToU `WBP`/`LW1`/`LW2`:** $X = 36..54$ (lebar 19 px)
6. **Indikator Tegangan Fasa `URST`:** $X = 56..79$ (lebar 24 px)
7. **Indikator Arus Fasa `IRST`/`REV`:** $X = 81..104$ (lebar 24 px)
8. **Kode Register Lampiran E / OBIS:** $X = 106..127$ (lebar 22 px)

### D. Unit Test Evaluasi 4-Kuadran (`E3/tests/test_spln_display_tamper.c`)
* Menambahkan `[UJI 13] Verifikasi Kuadran Daya Listrik 4-Kuadran (SPLN Gambar 6 & Lampiran A)`:
  * Menguji kondisi idle ($P=0, Q=0$) menghasilkan `power_quadrant == 0`.
  * Menguji kondisi $P>0, Q>0$ menghasilkan `power_quadrant == 1`.
  * Menguji kondisi $P<0, Q>0$ menghasilkan `power_quadrant == 2`.
  * Menguji kondisi $P<0, Q<0$ menghasilkan `power_quadrant == 3`.
  * Menguji kondisi $P>0, Q<0$ menghasilkan `power_quadrant == 4`.

---

## 4. Hasil Verifikasi

1. **Unit Test CTest (12/12 Passed 100%):**
   ```text
   Test project G:/PPPP/ALLIN/E3/build
    1/12 Test  #1: Integration_E3_Demo ..............   Passed
    2/12 Test  #2: Unit_E3_Display_Tamper ...........   Passed
    3/12 Test  #3: Unit_E3_Power_Relay ..............   Passed
    4/12 Test  #4: Unit_E3_FSM_Statechart ...........   Passed
    5/12 Test  #5: Unit_E3_Load_Profile .............   Passed
    6/12 Test  #6: Unit_E3_SPLN_Display_Tamper ......   Passed
    7/12 Test  #7: Stress_E3_Final_System_Audit .....   Passed
    8/12 Test  #8: Unit_E3_Console_CLI ..............   Passed
    9/12 Test  #9: Unit_E3_Board_Profile ............   Passed
   10/12 Test #10: Unit_E3_Power_Mode_Stop2 .........   Passed
   11/12 Test #11: Unit_E3_NVRAM_Storage ............   Passed
   12/12 Test #12: Unit_E3_RTOS_Tasks ...............   Passed

   100% tests passed, 0 tests failed out of 12
   ```

2. **Kompilasi Firmware Target ARM Cortex-M33 (STM32U585):**
   ```text
   [1/6] Performing build step for 'ALLIN_S'
   Memory region         Used Size  Region Size  %age Used
                RAM:       12056 B       256 KB      4.60%
                ROM:       24072 B      1016 KB      2.31%
            ROM_NSC:         160 B         8 KB      1.95%

   [4/6] Performing build step for 'ALLIN_NS'
   Memory region         Used Size  Region Size  %age Used
                RAM:       68992 B       512 KB     13.16%
                ROM:      175240 B         1 MB     16.71%
   ```
   Kompilasi sukses 100% bersih tanpa error.

