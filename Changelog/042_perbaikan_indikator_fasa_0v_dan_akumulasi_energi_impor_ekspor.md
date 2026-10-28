# LOG-042: Perbaikan Indikator Fasa 0V OLED (Header L-23) & Isolasi Akumulator Energi Impor/Ekspor Metrologi

**Tanggal:** 28 Oktober 2026  
**Status:** SELESAI & TERVERIFIKASI  
**Target:** Pilar E1 (Metrologi ADE9000), Pilar E3 (Display OLED & HMI), STM32U585VIT6  

---

## 1. Latar Belakang & Analisis Bug Mendalam

Dalam audit baris demi baris lanjutan yang mencakup subsistem Metrologi E1 dan Visualisasi Display E3, ditemukan **2 bug logika perangkat keras**:

### A. Bug Indikator Fasa Tegangan & Arus Menampilkan 'Aktif' pada Kondisi Terputus Total (0 Volt / 0 mA)
* **Lokasi Berkas:** [`E3/app/display/src/display.c` Baris 1140–1152](file:///g:/PPPP/ALLIN/E3/app/display/src/display.c#L1140-L1152)
* **Akar Masalah:**
  1. Pada evaluasi bitmask fasa aktif (`frame->voltage_phases` dan `frame->current_phases`), terdapat kondisi perbandingan:
     ```c
     if (ctx->meas_buffer.voltage_r_dvolts > 0 && ctx->meas_buffer.voltage_r_dvolts < 1100U)
         frame->voltage_phases &= ~0x01;
     ```
  2. Kondisi `voltage_r_dvolts > 0` menyebabkan saat fasa R mati total atau kabel pasokan terputus (`voltage_r_dvolts == 0`), kondisi `(0 > 0)` bernilai `false`.
  3. Akibatnya, bit 0 (`0x01`) TIDAK di-reset, dan layar OLED tetap menampilkan indikator tegangan fasa 1 aktif (`L123` alih-alih `L-23`). Hal serupa juga terjadi pada fasa S, fasa T, serta fasa arus R, S, dan T saat bernilai 0 mA.
* **Solusi yang Diterapkan:**
  Menghapus syarat redundant `> 0`. Setiap tegangan di bawah ambang 110.0V (`< 1100U`) atau arus di bawah 20 mA (`< 20U`) — termasuk kondisi mati total 0V / 0mA — langsung memadamkan indikator fasa terkait secara akurat sesuai SPLN D3.006: 2021 Butir 6.11.

---

### B. Potensi Kebocoran Energi Lintas Arah (*Energy Cross-Contamination*) pada Metrologi E1
* **Lokasi Berkas:** [`E1/Metrology/e1_metrology_service.c` Baris 19–21 & 246–265](file:///g:/PPPP/ALLIN/E1/Metrology/e1_metrology_service.c#L19-L21)
* **Akar Masalah:**
  1. Sebelumnya, `e1_metrology_service.c` hanya menggunakan satu variabel akumulator sisa miliJoule tunggal (`s_energy_mj_accum`).
  2. Jika meter beroperasi dengan daya kirim positif (+A) hingga terakumulasi misalnya 3.500.000 mJ (mendekati 1 Wh), lalu arah daya berbalik menjadi negatif (-A / ekspor PLTS) selama sesaat (+100.000 mJ), akumulator menyentuh 3.600.000 mJ dan langsung menambah stand energi impor (`active_energy_import_wh++`).
  3. Hal ini menyebabkan 3.500.000 mJ energi kirim (+A) keliru terhitung ke dalam kuadran energi terima/impor (-A).
* **Solusi yang Diterapkan:**
  Memisahkan akumulator presisi tinggi menjadi dua variabel independen:
  * `s_energy_exp_mj_accum`: Khusus mengakumulasi daya kirim (+A) saat `active_power_w >= 3`.
  * `s_energy_imp_mj_accum`: Khusus mengakumulasi daya terima (-A) saat `active_power_w <= -3`.
  Kedua arah memiliki siklus rollover 3.600.000 mJ $\rightarrow$ 1 Wh tersendiri tanpa risiko kontaminasi energi silang.

---

## 2. Hasil Verifikasi & Kompilasi

### A. Pengujian Host CTest (12 Test Suite Lengkap)
```
Test project G:/PPPP/ALLIN/E3/build
 1/12 Test  #1: Integration_E3_Demo ..............   Passed    0.06 sec
 2/12 Test  #2: Unit_E3_Display_Tamper ...........   Passed    0.06 sec
 3/12 Test  #3: Unit_E3_Power_Relay ..............   Passed    0.05 sec
 4/12 Test  #4: Unit_E3_FSM_Statechart ...........   Passed    0.04 sec
 5/12 Test  #5: Unit_E3_Load_Profile .............   Passed    0.05 sec
 6/12 Test  #6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.09 sec
 7/12 Test  #7: Stress_E3_Final_System_Audit .....   Passed    0.12 sec
 8/12 Test  #8: Unit_E3_Console_CLI ..............   Passed    0.09 sec
 9/12 Test  #9: Unit_E3_Board_Profile ............   Passed    0.04 sec
10/12 Test #10: Unit_E3_Power_Mode_Stop2 .........   Passed    0.04 sec
11/12 Test #11: Unit_E3_NVRAM_Storage ............   Passed    0.05 sec
12/12 Test #12: Unit_E3_RTOS_Tasks ...............   Passed    0.05 sec

100% tests passed, 0 tests failed out of 12 (0.76 sec)
```

### B. Kompilasi Silang ARM Cortex-M33 (GCC Bare-metal)
* **`ALLIN_S.elf`:** ROM: 24,056 B (2.31%), RAM: 12,056 B (4.60%), ROM_NSC: 160 B (1.95%) — **0 Errors, 0 Warnings**
* **`ALLIN_NS.elf`:** ROM: 174,464 B (16.64%), RAM: 65,344 B (12.46%) — **0 Errors, 0 Warnings**

