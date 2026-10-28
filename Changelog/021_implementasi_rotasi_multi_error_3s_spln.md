# Log 021: Implementasi Rotasi Kode Error Bergantian per 3 Detik (SPLN D3.006 Butir 6.11)

## 1. Konteks & Dasar Standar SPLN
Berdasarkan dokumen acuan **SPLN D3.006: 2021 Butir 6.11 (Hal. 16)**:
> *"Jika terdapat beberapa kode respons (`ERRxx`) yang aktif bersamaan maka ditampilkan bergantian dengan interval waktu 3 detik."*

Sebelumnya, penanganan tampilan kode error masih menggunakan rantai `if ... else if` statis, sehingga apabila terjadi lebih dari satu kejadian sabotase atau alarm hardware secara simultan (misalnya Cover Terminal Terbuka `ERR20` dan Arus Terbalik `ERR26`), tampilan hanya menampilkan satu kode urutan pertama secara permanen.

---

## 2. Modifikasi Teknis

### A. Antarmuka Layanan Display (`E3/app/display/inc/display.h`)
- Menambahkan deklarasi fungsi pemilih error berotasi waktu:
  ```c
  void display_get_combined_error_info(uint32_t tamper_mask, uint32_t alarm_mask, uint32_t epoch_sec,
                                       char *out_text, size_t text_sz, char *out_code, size_t code_sz);
  ```

### B. Algoritma Rotasi Dinamis Berbasis Waktu (`E3/app/display/src/display.c`)
- Mengimplementasikan `display_get_combined_error_info`:
  - Mengumpulkan seluruh bit sabotase aktif ($N_{tamper}$) dari `active_tamper_mask`.
  - Mengumpulkan seluruh bit alarm hardware internal aktif ($N_{alarm}$) dari `active_alarm_mask`.
  - Menghitung total masalah aktif: $N_{total} = N_{tamper} + N_{alarm}$.
  - Menggunakan fungsi indeks siklis waktu: `sel_idx = (epoch_sec / 3U) % N_total`.
  - Setiap 3 detik berlalu, tampilan berganti secara deterministik ke kode error berikutnya.
- Menghubungkan fungsi ini ke `display_render_frame` (CLI) dan `display_get_frame` (HMI LCD fisik Gambar 4).
- Mempertahankan backward compatibility untuk `display_get_spln_tamper_info` dan `display_get_spln_alarm_info`.

### C. Pengujian Unit & Verifikasi Regresi (`E3/tests/test_spln_display_tamper.c`)
- Menambahkan blok pengujian `[UJI 11] Verifikasi Rotasi Multi-Error Bergantian per 3 Detik`:
  - Menguji dua kondisi sabotase simultan (`ERR20` dan `ERR26`):
    - $t = 0\text{ s} \dots 2\text{ s} \rightarrow$ Kode `ERR20` [PASS]
    - $t = 3\text{ s} \dots 5\text{ s} \rightarrow$ Kode `ERR26` [PASS]
    - $t = 6\text{ s} \dots 8\text{ s} \rightarrow$ Kembali ke `ERR20` [PASS]
  - Menguji kombinasi Sabotase (`ERR20`) + Hardware Alarm (`ERR03` Low Battery):
    - $t = 0\text{ s} \rightarrow$ `ERR20`, $t = 3\text{ s} \rightarrow$ `ERR03` [PASS].

---

## 3. Hasil Kompilasi & Pengujian
- **CTest Suite E3/build:** 7/7 test passed (100% Lulus).
- **Target ARM Cortex-M33 TrustZone NonSecure (`ALLIN_NS.elf`):**
  - Kompilasi: **0 Error, 0 Warning**.
  - RAM: 62.432 B (11.91%).
  - ROM: 129.608 B (12.36%).

