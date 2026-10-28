# LOG-044: Penyesuaian Indikator Fasa Layar OLED dan Frame Tampilan Sesuai Standar SPLN D3.006:2021 (UR, US, UT dan IR, IS, IT)

**Tanggal:** 28 Oktober 2026  
**Pilar Terkait:** Pilar E3 (Application, UI/Display, HMI & Console)  
**Status:** Selesai, Terintegrasi, dan Terverifikasi (12/12 CTest Lulus & Kompilasi ARM Cortex-M33 Sukses)

---

## 1. Latar Belakang & Rujukan Standar Resmi

Berdasarkan audit terhadap dokumen acuan resmi **SPLN D3.006:2021 ("Spesifikasi Meter Statip Pembayaran Pascabayar dan Prabayar Berbasis Komunikasi Seluler")**:

1. **Butir 6.11 Tampilan (Halaman 39–40, Gambar 6 Layout Layar LCD Tiga Fasa):**
   * Baris pertama (Simbol Status):
     > * $\text{XXX}$ menginformasikan indikasi tegangan fase $\mathbf{U_R, U_S, U_T}$;
     > * $\text{YYY}$ menginformasikan indikasi arus fase $\mathbf{I_R, I_S, I_T}$;
     > * Arah aliran daya aktif ($\rightarrow$ atau $\leftarrow$).
   * Baris ketiga (Area Teks & Informasi):
     > * **Informasi register sebanyak 7 karakter (Lampiran E);**
     > * **Kode respons**, jika terdapat beberapa kode respons yang aktif bersamaan maka ditampilkan bergantian dengan interval waktu 3 detik;
     > * Satuan.

2. **Lampiran E, Tabel E.1 & Tabel E.2 (Daftar Besaran Ukur & Register Penagihan, Halaman 72–73):**
   * Register Tegangan Fasa R, S, T didefinisikan secara resmi sebagai **`UR`**, **`US`**, **`UT`** (V, 1 desimal).
   * Register Arus Fasa R, S, T, N didefinisikan secara resmi sebagai **`IR`**, **`IS`**, **`IT`**, **`IN`** (A, 3 desimal).
   * Sudut Tegangan & Arus didefinisikan sebagai **`Sdt_UR`**, **`Sdt_US`**, **`Sdt_UT`**, **`Sdt_IR`**, **`Sdt_IS`**, **`Sdt_IT`**.
   * Daya & Energi: **`P_EXP`**, **`P_IMP`**, **`Q_EXP`**, **`Q_IMP`**, **`EA_EXP1..3`**, **`EA_EXP`**, **`EA_IMP`**, **`ER_BIL`**, **`S_MAX`**, **`P_LIM`**, **`EoB`**, **`CSUM`**, **`PLN`**, **`IDPEL`**, **`METERID`**.

---

## 2. Masalah yang Ditemukan pada Implementasi Sebelumnya

1. **Penggunaan Notasi IEC Eropa ($L_1, L_2, L_3$ & $I_1, I_2, I_3$):**
   * Pada status bar OLED baris 1 (`bsp_display_oled.c`), indikator fasa tegangan ditampilkan sebagai string `L123` (atau `L-23` jika fasa mati) dan arus sebagai `I123`.
   * Di `display.c`, buffer simbol status baris 1 juga mencetak `"L---"` dan `"I---"`.
   * Hal ini menyalahi nomenklatur PLN yang secara tegas menggunakan penamaan fasa **R, S, T** ($\mathbf{U_R, U_S, U_T}$ untuk tegangan dan $\mathbf{I_R, I_S, I_T}$ untuk arus).

2. **Ketiadaan Tampilan Nama Register Lampiran E pada Layar OLED:**
   * Sebelumnya, pojok kanan atas OLED (X=107..127) hanya menampilkan kode OBIS numerik penagihan (`32.07`, `52.07`, `72.07`, `01.08.1`, dll.).
   * Teknisi PLN di lapangan memerlukan informasi register teks 7-karakter resmi (Lampiran E) seperti `UR`, `US`, `UT`, `P_EXP`, `EA_EXP1` untuk memverifikasi besaran ukur tanpa harus menghafal seluruh kode OBIS.

---

## 3. Rincian Perubahan & Perbaikan

### A. Pembaharuan Struktur Data Frame Layar (`E3/app/display/inc/display.h`)
* Menambahkan atribut `reg_info[12]` untuk menampung nama register resmi SPLN Lampiran E.
* Menambahkan flag `show_reg_alt` yang berganti setiap 2,5 detik (2500 ms) agar layar OLED menampilkan nama register Lampiran E bergantian dengan kode OBIS secara mulus.

### B. Pemetaan Nama Register & Simbol Fasa (`E3/app/display/src/display.c`)
1. **Perubahan Simbol Status Bar:**
   * Buffer tegangan diubah dari `"L---"` menjadi `"U---"`, dengan huruf aktif `'R'`, `'S'`, `'T'` menghasilkan string visual **`URST`** (atau **`U-ST`** jika fasa R padam/drop $<110\text{ V}$).
   * Buffer arus diubah dari `"I---"` dengan angka `'1'`, `'2'`, `'3'` menjadi huruf aktif `'R'`, `'S'`, `'T'` menghasilkan string visual **`IRST`** (atau **`I-ST`** jika arus fasa R bernilai 0 mA).
2. **Pemetaan Register Lampiran E:**
   * Seluruh $\approx 45$ halaman pengukuran dan status dipetakan ke register resmi:
     * Tegangan: `UR` (fasa R), `US` (fasa S), `UT` (fasa T).
     * Arus: `IR` (fasa R), `IS` (fasa S), `IT` (fasa T), `IN` (netral).
     * Sudut fasa: `Sdt_UR`, `Sdt_US`, `Sdt_UT`, `Sdt_IR`, `Sdt_IS`, `Sdt_IT`.
     * Daya: `P_EXP`, `P_IMP`, `Q_EXP`, `Q_IMP`, `S_MAX`, `P_LIM`.
     * Energi: `EA_EXP1..3`, `EA_EXP`, `EA_IMP1..3`, `EA_IMP`, `ER_BIL`.
     * Penagihan & Sistem: `IDPEL`, `METERID`, `EoB`, `CSUM`, `PLN`.
3. **Penyelarasan Tampilan Konsol ASCII CLI:**
   * Judul halaman pada mode CLI diperbarui dari `PHASE 1` menjadi `PHASE R (UR)`, `PHASE S (US)`, `PHASE T (UT)`, `PHASE R (IR)`, `PHASE S (IS)`, `PHASE T (IT)`, dan `NEUTRAL (IN)`.

### C. Driver Layar Grafis OLED (`E3/drivers/oled/src/bsp_display_oled.c`)
1. **Status Bar Fasa:**
   * Koordinat X=57 dirender dengan string `URST` (font 6x8).
   * Koordinat X=82 dirender dengan string `IRST` (font 6x8) saat arus normal atau `REV` saat arah daya aktif berbalik (ekspor PLTS).
2. **Area Register Kanan Atas (X=107..127):**
   * Menampilkan `reg_info` (misal: `UR`, `US`, `UT`, `IR`, `P_EXP`) bergantian dengan `obis_code` (misal: `32.07`, `52.07`, `72.07`, `31.07`, `01.07`) berdasarkan flag `show_reg_alt`.
   * Teks 2 karakter seperti `UR` diletakkan di X=115..126 piksel tanpa mengalami pemotongan (*clipping*).

### D. Penyelarasan Unit Test (`E3/tests/test_spln_display_tamper.c`)
* Assertion `L-23` pada pengujian drop fasa tegangan diperbarui menjadi `U-ST`.
* Menambahkan assertion untuk memverifikasi nilai `frame.reg_info` (`UR`, `US`, `UT`, `IR`, dll.) pada setiap halaman pengukuran.

### E. Penyelarasan Dokumentasi Arsitektur (`E3/docs/PANDUAN_DAN_RANGKUMAN_SISTEM_E3.md`)
* Memperbarui diagram layout OLED 128x32 dan tabel halaman carousel menjadi standar `URST`, `IRST`, dan register `UR`, `US`, `UT`.

---

## 4. Hasil Verifikasi

### A. Pengujian Unit Test CTest Suite (Host x86_64)
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

### B. Kompilasi Target Fisik ARM GCC Cortex-M33 (STM32U585)
```text
[1/6] Performing build step for 'ALLIN_S'
[2/2] Linking C executable ALLIN_S.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       12056 B       256 KB      4.60%
             ROM:       24072 B      1016 KB      2.31%
         ROM_NSC:         160 B         8 KB      1.95%

[4/6] Performing build step for 'ALLIN_NS'
[13/13] Linking C executable ALLIN_NS.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       68992 B       512 KB     13.16%
             ROM:      174632 B         1 MB     16.65%
```
* Kompilasi berhasil 100% tanpa error maupun warning baru.

