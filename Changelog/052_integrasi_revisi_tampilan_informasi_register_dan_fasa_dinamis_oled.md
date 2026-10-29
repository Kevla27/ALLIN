# Integrasi Revisi Tampilan Informasi Register dan Indikator Fasa Dinamis Layar OLED (LOG-052)

## 1. Latar Belakang
Mengintegrasikan pembaruan kode dari folder `E3/Revisi` ke dalam pohon kode utama firmware:
1. **Penempatan Langsung Informasi Register Acuan:**
   Pada layar tampilan, kolom acuan *Informasi Register* (seperti `IDPEL`, `EA_EXP1`, `EA_EXP2`, `EA_EXP3`, `EA_EXP`, `EA_IMP`, `ER_BIL`, `UR`, `US`, `UT`, `IR`, `IS`, `IT`, `IN`) ditampilkan langsung pada area identifikasi tampilan register.
2. **Indikator Fasa Dinamis (Opsi A) pada Baris 1 Status Bar OLED:**
   - Halaman Fasa R: Menyalakan fasa `UR  ` dan `IR  `.
   - Halaman Fasa S: Menyalakan fasa `U S ` dan `I S `.
   - Halaman Fasa T: Menyalakan fasa `U  T` dan `I  T`.
   - Halaman Arus Netral: Menampilkan `IN`.
   - Halaman Non-Metrologi (IDPEL, waktu, alarm): Indikator fasa dikosongkan agar Baris 1 lapang.
   - Halaman Total 3-Fasa: Menampilkan `URST` dan `IRST` (atau ` REV` saat aliran balik).
3. **Penyempurnaan Penataan Posisi X Teks:**
   Penataan posisi nilai numerik besar (Font 11x18) di tengah layar dan teks panjang (Font 7x10) dengan kalkulasi batas margin dinamis agar tidak menimpa label register maupun label satuan.

---

## 2. File yang Terdampak & Perubahan
1. **`E3/app/display/inc/display.h`**:
   - Menambahkan field `display_page_id_t page_id;` pada struct `display_spln_frame_t`.
   - Memperbesar buffer `scroll_index_zz` dari 4 menjadi 16 byte (`char scroll_index_zz[16];`) agar muat menampung teks informasi register (`"EA_EXP1"`, `"IDPEL"`, dll.) tanpa terpotong (*truncation*).
2. **`E3/drivers/oled/src/bsp_display_oled.c`**:
   - Menerapkan logika render fasa dinamis per halaman aktif (`spln_frame->page_id`).
   - Menerapkan kalkulasi posisi `X` teks angka dan teks status agar tidak saling menimpa.
3. **`E3/app/display/src/display.c`**:
   - Mengisi `frame->page_id = ctx->current_page;`.
   - Mengisi `frame->scroll_index_zz` dengan nama register langsung (`"IDPEL"`, `"UR"`, `"US"`, `"UT"`, `"EA_EXP1"`, dll.).
   - Menjaga persistensi pengisian `frame->obis_code` untuk tampilan OBIS pada layar dan komunikasi.
4. **`E3/tests/test_spln_display_tamper.c`**:
   - Menyesuaikan asersi pengujian unit ke nilai informasi register acuan.
5. **Penghapusan Folder `E3/Revisi/`**:
   - Berkas di dalam `E3/Revisi/` (`bsp_display_oled.c` dan `display.c`) dihapus setelah diterapkan secara penuh.

---

## 3. Hasil Pengujian & Kompilasi
- **CTest Suite:** 12/12 Test Suites Lulus 100% (0 Failed).
- **Target ARM Cortex-M33 Build:** `ALLIN_S.elf` dan `ALLIN_NS.elf` sukses ter-link 100% bersih (`RAM: 13.16%`, `ROM: 16.89%`).
