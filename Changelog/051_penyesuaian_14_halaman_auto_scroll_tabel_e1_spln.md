# Penyesuaian 14 Halaman Auto-Scroll Layar Tampilan Otomatis Sesuai Tabel E.1 SPLN (LOG-051)

## 1. Latar Belakang
Berdasarkan arahan pengguna terhadap tabel acuan **Tabel E.1 ("Informasi layar tampilan otomatis")** dengan instruksi ketat *"jangan kurang jangan tambah"*, urutan rotasi tampilan otomatis (*auto-scroll carousel*) pada meter dibatasi secara tepat pada **14 halaman acuan** dalam urutan baku dari item No. 1 hingga No. 14.

Parameter-parameter lainnya (seperti Daya Total, Frekuensi, Jam RTC, Sudut Fasa, dan Maximum Demand) dikembalikan secara murni ke mode *manual-scroll* (Tabel E.2) menggunakan tombol tekan, sehingga tidak mencampuri rotasi otomatis berkala (carousel) saat meter beroperasi normal.

---

## 2. Rincian 14 Halaman Auto-Scroll (Tabel E.1)
| No. | Informasi Layar Otomatis | Register | Satuan | Desimal | Sifat Nilai | Kode `zz` | Enum ID Halaman |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **01** | ID Pelanggan | `IDPEL` | - | 0 | - | `"01"` | `DISP_PAGE_IDPEL` |
| **02** | Energi aktif kirim Tarif 1 (WBP) | `EA_EXP1` | `kWh` | 3 | Stand akhir sesaat | `"02"` | `DISP_PAGE_ENERGY_WBP` |
| **03** | Energi aktif kirim Tarif 2 (LWBP1) | `EA_EXP2` | `kWh` | 3 | Stand akhir sesaat | `"03"` | `DISP_PAGE_ENERGY_LWBP1` |
| **04** | Energi aktif kirim Tarif 3 (LWBP2) | `EA_EXP3` | `kWh` | 3 | Stand akhir sesaat | `"04"` | `DISP_PAGE_ENERGY_LWBP2` |
| **05** | Energi aktif kirim (Total Ekspor) | `EA_EXP` | `kWh` | 3 | Stand akhir sesaat | `"05"` | `DISP_PAGE_ENERGY_ACTIVE_EXPORT` |
| **06** | Energi aktif terima (Total Impor) | `EA_IMP` | `kWh` | 3 | Stand akhir sesaat | `"06"` | `DISP_PAGE_ENERGY_ACTIVE_IMPORT` |
| **07** | Energi reaktif billing (kompensasi) | `ER_BIL` | `kVARh` | 3 | Stand akhir sesaat | `"07"` | `DISP_PAGE_REACTIVE_ENERGY` |
| **08** | Tegangan fase R | `UR` | `V` | 1 | Sesaat | `"08"` | `DISP_PAGE_VOLTAGE_R` |
| **09** | Tegangan fase S | `US` | `V` | 1 | Sesaat | `"09"` | `DISP_PAGE_VOLTAGE_S` |
| **10** | Tegangan fase T | `UT` | `V` | 1 | Sesaat | `"10"` | `DISP_PAGE_VOLTAGE_T` |
| **11** | Arus fase R | `IR` | `A` | 3 | Sesaat | `"11"` | `DISP_PAGE_CURRENT_R` |
| **12** | Arus fase S | `IS` | `A` | 3 | Sesaat | `"12"` | `DISP_PAGE_CURRENT_S` |
| **13** | Arus fase T | `IT` | `A` | 3 | Sesaat | `"13"` | `DISP_PAGE_CURRENT_T` |
| **14** | Arus netral | `IN` | `A` | 3 | Sesaat | `"14"` | `DISP_PAGE_CURRENT_N` |

---

## 3. File yang Terdampak & Perubahan
1. **`E3/app/display/inc/display.h`**:
   - Menyusun ulang urutan enum `display_page_id_t` sehingga 14 halaman auto-scroll berada pada rentang indeks kontinu `0` s.d. `13`.
   - Menambahkan konstanta batas halaman otomatis:
     ```c
     #define DISP_AUTO_PAGE_COUNT   14U
     #define DISP_AUTO_PAGE_FIRST   DISP_PAGE_IDPEL
     #define DISP_AUTO_PAGE_LAST    DISP_PAGE_CURRENT_N
     ```
2. **`E3/app/display/src/display.c`**:
   - Memperbarui fungsi `display_process_tick()` untuk melakukan perputaran siklus auto carousel murni pada rentang `DISP_AUTO_PAGE_FIRST` hingga `DISP_AUTO_PAGE_LAST`.
   - Menyelaraskan string kode nomor urut `scroll_index_zz` pada seluruh 14 halaman (`"01"` hingga `"14"`).
   - Memastikan presisi 3 desimal untuk `EA_EXP` dan `ER_BIL` serta satuan baku `"kVARh"`.
   - Mengatur kondisi akhir halaman 14: jika terdapat status tamper aktif akan menampilkan alarm (`AL`), jika terdapat rekoneksi relai tertunda akan menampilkan `READY ON` (`on`), atau berputar kembali ke halaman 01 (`IDPEL`).
3. **`E3/tests/test_e3_app.c` & `E3/tests/test_spln_display_tamper.c`**:
   - Memperbarui pengujian unit agar menguji seluruh 14 halaman rotasi otomatis, kode nomor urut register, presisi desimal, dan logika *wrap-around*.

---

## 4. Hasil Verifikasi & Kompilasi
- **CTest Suite:** 12/12 Test Suites Lulus 100% (0 Failed).
- **Target ARM Cortex-M33 Build:** `ALLIN_S.elf` dan `ALLIN_NS.elf` sukses terkompilasi dan tertaut bersih (`RAM: 13.16%`, `ROM: 16.90%`).
