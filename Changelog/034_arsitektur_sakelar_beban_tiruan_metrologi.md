# Log Perubahan 034: Arsitektur Sakelar Diagnostik Metrologi Tiruan (Synthetic Load Generator) via CLI & Isolasi Bersih 0-Byte Produksi Massal

**ID Log:** LOG-034  
**Tanggal:** 28 Oktober 2026  
**Topik Utama:** Arsitektur Sakelar Diagnostik Metrologi Tiruan (Synthetic Load Generator) via CLI (`load`, `volt`, `curr`, `synth`) & Isolasi Bersih 0-Byte Produksi Massal  
**Penanggung Jawab:** Metrology & System Integration Lead (Pilar E1 & E3)

---

## 1. Latar Belakang & Urgensi Rekayasa
Pada tahap purwarupa (*prototyping*) di mana modul layar OLED SSD1306 telah tersedia namun perangkat keras metrologi ADE9000 belum tersambung ke jala-jala listrik PLN bertegangan tinggi (3-fasa 400V AC), tim rekayasa membutuhkan sarana untuk:
1. **Verifikasi Visual Layar OLED:** Menguji bahwa seluruh halaman Carousel SPLN D3.006 (Volt, Arus, Daya Aktif/Reaktif/Semu, Cos $\phi$, dan akumulasi kWh) merender angka secara dinamis dan responsif terhadap perubahan beban.
2. **Pengujian Proteksi & State Machine:** Menguji respons relai terhadap beban lebih (*overload* >11 kW), tegangan rendah (*under-voltage* <110V), tegangan lebih (*over-voltage* >280V), dan ketidakseimbangan fasa (*asymmetry*) langsung dari meja kerja lab.
3. **Persyaratan Sertifikasi & Produksi Massal PLN:** Kode tiruan/diagnostik ini **tidak boleh mencemari atau membebani memori firmware produksi massal**, serta harus dapat dihilangkan secara instan dan bersih tanpa memodifikasi logika inti meter.

---

## 2. Desain Arsitektur Sakelar (*Clean Conditional Switch Architecture*)

Solusi diimplementasikan menggunakan arsitektur sakelar terpusat pada `board_profile_config.h`:
```c
/* ====================================================================
 * 6. SAKELAR DIAGNOSTIK & INJEKSI BEBAN TIRUAN (R&D / FACTORY BENCH)
 * ==================================================================== */
#ifndef BOARD_CFG_ENABLE_SYNTHETIC_LOAD
#define BOARD_CFG_ENABLE_SYNTHETIC_LOAD  1  /**< 1 = Aktifkan CLI beban tiruan, 0 = Produksi massal / Sertifikasi resmi PLN */
#endif
```

### Mekanisme Isolasi 0-Byte Saat Produksi (`BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 0`):
* Seluruh deklarasi struktur data `synthetic_load_config_t` di `console_io.h` tidak dikompilasi.
* Seluruh handler perintah CLI (`cmd_load`, `cmd_volt`, `cmd_curr`, `cmd_synth`) dan entri tabel di `console_io.c` dihilangkan.
* Seluruh percabangan pengalihan beban di `meter_system_bridge.c` dipotong total oleh C Preprocessor.
* **Hasil Verifikasi Kompilasi STM32U585:**
  - Flash/ROM berkurang tepat **4.088 Bytes** (dari 148.624 B menjadi 144.536 B).
  - RAM berkurang **64 Bytes** (buffer struktur diagnostik).
  - Jejak kode di ROM adalah **0 byte**, menjamin kepatuhan penuh terhadap audit keamanan firmware PLN.

---

## 3. Fitur Interaktif Saat Mode Diagnostik Aktif (`BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1`)

1. **Perintah `load <P_Watt> [Q_var] [pf_permille]`:**
   - Mengatur daya aktif (Watt), reaktif (var), dan faktor daya.
   - Menghitung arus fasa seimbang secara otomatis: $I_{phase} = \frac{P}{3 \times V \times \text{PF}}$.
   - Contoh: `load 5000` mengatur $P = 5000$ W, $Q = 500$ var, $\text{PF} = 0.990$, $I = 7.652$ A.
   - Menguji trip relai pembatas: `load 15000` (melebihi batas 11.000 W) memicu timer pemutusan relai kontaktor.

2. **Perintah `volt <Vr_V> [Vs_V] [Vt_V]`:**
   - Mengatur tegangan fasa R, S, T.
   - Contoh: `volt 230` mengatur tegangan seimbang 230.0 V.
   - Contoh: `volt 90` menguji proteksi cut-off tegangan rendah (*Lo Volt* <110V).
   - Contoh: `volt 290` menguji proteksi cut-off tegangan tinggi (*Hi Volt* >280V).
   - Contoh: `volt 230 0 230` menguji deteksi hilang fasa S (ERR24).

3. **Perintah `curr <Ir_mA> [Is_mA] [It_mA] [In_mA]`:**
   - Mengatur arus fasa dan kawat netral dalam satuan milliAmpere.
   - Contoh: `curr 20000 5000 5000` menguji alarm ketidakseimbangan fasa (*asymmetry*).
   - Contoh: `curr 10000 10000 10000 5000` menguji deteksi sabotase kawat netral bypass (ERR23).

4. **Perintah `synth <on|off|status|reset>`:**
   - `synth on` : Mengaktifkan generator beban tiruan.
   - `synth off`: Menonaktifkan generator dan kembali seketika ke pembacaan sensor fisik chip ADE9000.
   - `synth status`: Menampilkan status lengkap parameter generator beban tiruan.
   - `synth reset` : Mengembalikan beban ke kondisi nominal standar (220.0 V, 15.0 A, 9.9 kW, PF 0.990).

5. **Akumulasi Energi Wh Dinamis di Loop Sistem:**
   - Diimplementasikan pada `meter_system_bridge.c` menggunakan kalkulator integer square-root `isqrt64()` (tanpa ketergantungan library `math.h` yang lambat).
   - Energi aktif terakumulasi secara kontinu sesuai rumus $E = \sum (P \times \Delta t)$, sehingga angka kWh pada layar OLED dan data model OBIS DLMS bergerak naik secara realistis.

---

## 4. Hasil Pengujian & Kompilasi

### A. Pengujian Unit Test Konsol CLI Host (CTest - 9/9 Suites 100% Passed)
```text
Test project G:/PPPP/ALLIN/E3/build
    Start 1: Integration_E3_Demo ..............   Passed    0.09 sec
    Start 2: Unit_E3_Display_Tamper ...........   Passed    0.08 sec
    Start 3: Unit_E3_Power_Relay ..............   Passed    0.09 sec
    Start 4: Unit_E3_FSM_Statechart ...........   Passed    0.07 sec
    Start 5: Unit_E3_Load_Profile .............   Passed    0.08 sec
    Start 6: Unit_E3_SPLN_Display_Tamper ......   Passed    0.14 sec
    Start 7: Stress_E3_Final_System_Audit .....   Passed    0.19 sec
    Start 8: Unit_E3_Console_CLI ..............   Passed    0.15 sec
    Start 9: Unit_E3_Board_Profile ............   Passed    0.08 sec

100% tests passed, 0 tests failed out of 9
```

### B. Kompilasi Target Hardware ARM Cortex-M33 (Dual-Domain STM32U585)
- **Secure Domain (`Secure/build`):** ROM 23.592 B (2.27%), RAM 12.048 B (4.60%) — *0 Error, 0 Warning*.
- **Non-Secure Domain (`NonSecure/build`) [Mode Sakelar Aktif = 1]:** ROM 148.624 B (14.17%), RAM 65.064 B (12.41%) — *0 Error, 0 Warning*.
- **Non-Secure Domain (`NonSecure/build`) [Mode Sakelar Non-Aktif = 0]:** ROM 144.536 B (13.78%), RAM 65.000 B (12.40%) — *0 Error, 0 Warning (4.088 Byte ROM dibebaskan)*.
