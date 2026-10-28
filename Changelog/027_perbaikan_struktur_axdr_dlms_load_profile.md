# Log Perubahan 027 (LOG-027): Perbaikan Ketidakcocokan Elemen Struktur A-XDR DLMS Load Profile & Tamper Log

## 1. Ringkasan Eksekutif
- **ID Log:** `LOG-027`
- **Tanggal:** 27 Oktober 2026
- **Kategori:** Komunikasi DLMS/COSEM (Pilar E3 Profiling / DLMS Adapter)
- **Dasar Acuan Standar:**
  - **DLMS / COSEM Green Book & Blue Book** (Spesifikasi Class ID 7 Profile Generic & A-XDR Structure Encoding)
  - **SPLN D3.006: 2021 Butir 12.3 & Lampiran G** (Capture Objects Profil Beban 15-Menit & Log Kejadian Meter 3-Fasa)
- **Status Verifikasi:**
  - **Host Unit Test:** Lulus 100% (7/7 suites CTest, termasuk verifikasi rekursif A-XDR stream decoding).
  - **Target ARM Cortex-M33 (NonSecure ELF):** 0 Error, 0 Warning (`ROM: 12.69%`, `RAM: 12.34%`).

---

## 2. Latar Belakang & Masalah Teknis

Sebelum perbaikan ini, modul adapter pemformatan A-XDR (`E3/app/profiling/src/load_profile_dlms_adapter.c`) memiliki cacat fatal (structure element count mismatch) yang dapat menyebabkan parser DLMS di Head-End System (HES) atau meter test bench mengalami **`BUFFER_UNDERFLOW`** atau kesalahan dekompresi sintaks APDU:

1. **Struktur Load Profile (`load_profile_encode_dlms_entry`):**
   - Header A-XDR mengekodifikasi:
     `out_buf[idx++] = DLMS_AXDR_TAG_STRUCTURE;`
     `out_buf[idx++] = 11;` (Menyatakan struktur terdiri atas 11 elemen).
   - Namun, implementasi aktual hanya menuliskan **5 elemen**:
     `[Timestamp, Active Energy Import, Voltage R, Current R, Status Mask]`.
   - Fasa S dan Fasa T (Tegangan S/T, Arus S/T) serta Energi Aktif Ekspor (-A, PLTS) dan Energi Reaktif Impor (+R) belum di-encode, sehingga decoder DLMS yang membaca tag `11` akan menuntut 6 elemen tambahan dan terputus di tengah stream buffer.

2. **Struktur Tamper Log (`tamper_log_encode_dlms_entry`):**
   - Komentar kode menuliskan: `/* Structure of 3 elements: [Timestamp, Event Code, Active Mask] */`.
   - Namun byte panjang struktur justru diisi:
     `out_buf[idx++] = 4;` (Menyatakan struktur memiliki 4 elemen).
   - Elemen yang di-encode aktual hanya **3 elemen**, menghasilkan ketidakcocokan tag length.

---

## 3. Rincian Perubahan & Solusi Teknis

1. **Penyempurnaan 11 Elemen Profil Beban 3-Fasa Lengkap (`load_profile_encode_dlms_entry`):**
   - Selaras dengan `load_profile_entry_t`, struktur 11 elemen profil beban diisi penuh dengan parameter kelistrikan 3-fasa dan transaksi PLTS:
     1. `Timestamp`: Octet-String (12 byte Date-Time DLMS)
     2. `Active Energy Import (+A)`: Long64-Unsigned (8 byte, Wh)
     3. `Active Energy Export (-A)`: Long64-Unsigned (8 byte, Wh - PLTS)
     4. `Reactive Energy Import (+R)`: Long64-Unsigned (8 byte, varh)
     5. `Voltage R`: Long-Unsigned (2 byte, x0.1 V)
     6. `Voltage S`: Long-Unsigned (2 byte, x0.1 V)
     7. `Voltage T`: Long-Unsigned (2 byte, x0.1 V)
     8. `Current R`: Double-Long-Unsigned (4 byte, x0.001 A)
     9. `Current S`: Double-Long-Unsigned (4 byte, x0.001 A)
     10. `Current T`: Double-Long-Unsigned (4 byte, x0.001 A)
     11. `Status Mask`: Double-Long-Unsigned (4 byte, bitmask alarm/tamper)
   - Total panjang A-XDR ter-encode: tepat **72 byte** (memenuhi batas alokasi buffer $\ge 72$ byte).

2. **Koreksi Tag Structure Tamper Log (`tamper_log_encode_dlms_entry`):**
   - Mengoreksi panjang struktur dari `4` menjadi `3`, konsisten 100% dengan komentar dan jumlah elemen aktual `[Timestamp, Event Vector, Active Mask]` (24 byte).

3. **Penambahan Fungsi A-XDR Standar SPLN 4-Elemen (`spln_log_encode_dlms_entry`):**
   - Menambahkan encoder DLMS A-XDR untuk struktur rekaman `spln_standard_log_entry_t` (SPLN Butir 12.3):
     `[Timestamp (12B), Event Code (Long-U), Status Flags (Long-U), Parameter Value (Double-Long-U)]` (27 byte).

4. **Validator Parsing A-XDR (`dlms_decode_verify_structure`):**
   - Menyediakan parser rekursif verifikasi struktur A-XDR untuk memastikan aliran byte ter-encode tidak underflow, cocok dengan jumlah elemen target, dan memiliki panjang presisi.

---

## 4. Diff / Perbandingan Kode

### A. `E3/app/profiling/src/load_profile_dlms_adapter.c`
```diff
--- a/E3/app/profiling/src/load_profile_dlms_adapter.c
+++ b/E3/app/profiling/src/load_profile_dlms_adapter.c
@@ -23,7 +23,7 @@ void dlms_encode_datetime(uint32_t epoch_sec, uint8_t *out_buf) {
 }
 
 size_t load_profile_encode_dlms_entry(const load_profile_entry_t *entry, uint8_t *out_buf, size_t max_len) {
-    if (entry == NULL || out_buf == NULL || max_len < 80) return 0;
+    if (entry == NULL || out_buf == NULL || max_len < 72) return 0;
 
     size_t idx = 0;
 
@@ -43,15 +43,45 @@ size_t load_profile_encode_dlms_entry(const load_profile_entry_t *entry, uint8_t
         out_buf[idx++] = (uint8_t)(entry->active_energy_import_wh >> (i * 8));
     }
 
+    /* 3. Active Energy Export (-A) - Long64-Unsigned (Dukungan Aliran PLTS) */
+    out_buf[idx++] = DLMS_AXDR_TAG_LONG64_U;
+    for (int i = 7; i >= 0; i--) {
+        out_buf[idx++] = (uint8_t)(entry->active_energy_export_wh >> (i * 8));
+    }
+
+    /* 4. Reactive Energy Import (+R) - Long64-Unsigned */
+    out_buf[idx++] = DLMS_AXDR_TAG_LONG64_U;
+    for (int i = 7; i >= 0; i--) {
+        out_buf[idx++] = (uint8_t)(entry->reactive_energy_import_varh >> (i * 8));
+    }
+
-    /* 3. Voltage R (Long-Unsigned x0.1V) */
+    /* 5. Voltage R (Long-Unsigned x0.1V) */
     out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
     out_buf[idx++] = (uint8_t)(entry->voltage_r_dvolts >> 8);
     out_buf[idx++] = (uint8_t)(entry->voltage_r_dvolts & 0xFF);
 
+    /* 6. Voltage S (Long-Unsigned x0.1V) */
+    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
+    out_buf[idx++] = (uint8_t)(entry->voltage_s_dvolts >> 8);
+    out_buf[idx++] = (uint8_t)(entry->voltage_s_dvolts & 0xFF);
+
+    /* 7. Voltage T (Long-Unsigned x0.1V) */
+    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
+    out_buf[idx++] = (uint8_t)(entry->voltage_t_dvolts >> 8);
+    out_buf[idx++] = (uint8_t)(entry->voltage_t_dvolts & 0xFF);
+
-    /* 4. Current R (Double-Long-Unsigned x0.001A) */
+    /* 8. Current R (Double-Long-Unsigned x0.001A) */
     out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
     out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 24);
     out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 16);
     out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 8);
     out_buf[idx++] = (uint8_t)(entry->current_r_mamps & 0xFF);
 
+    /* 9. Current S (Double-Long-Unsigned x0.001A) */
+    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
+    out_buf[idx++] = (uint8_t)(entry->current_s_mamps >> 24);
+    out_buf[idx++] = (uint8_t)(entry->current_s_mamps >> 16);
+    out_buf[idx++] = (uint8_t)(entry->current_s_mamps >> 8);
+    out_buf[idx++] = (uint8_t)(entry->current_s_mamps & 0xFF);
+
+    /* 10. Current T (Double-Long-Unsigned x0.001A) */
+    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
+    out_buf[idx++] = (uint8_t)(entry->current_t_mamps >> 24);
+    out_buf[idx++] = (uint8_t)(entry->current_t_mamps >> 16);
+    out_buf[idx++] = (uint8_t)(entry->current_t_mamps >> 8);
+    out_buf[idx++] = (uint8_t)(entry->current_t_mamps & 0xFF);
+
-    /* 5. Status Mask (Double-Long-Unsigned) */
+    /* 11. Status Mask (Double-Long-Unsigned) */
     out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
     out_buf[idx++] = (uint8_t)(entry->status_mask >> 24);
     out_buf[idx++] = (uint8_t)(entry->status_mask >> 16);
@@ -67,11 +97,11 @@ size_t tamper_log_encode_dlms_entry(const tamper_event_entry_t *entry, uint8_t *
-    out_buf[idx++] = 4;
+    out_buf[idx++] = 3;
```

---

## 5. Dampak terhadap Kompilasi & Pengujian

- **Host Unit Test:** Lulus 100% (7/7 tests passed).
  - Penambahan suite uji `test_load_profile_dlms_axdr_encoding()` memverifikasi byte per byte ke-11 elemen load profile, 3 elemen tamper log, dan 4 elemen SPLN standard log.
- **Kompilasi Target ARM Cortex-M33 (NonSecure):**
  - Text/ROM: 133.112 Bytes (12,69% dari 1 MB)
  - Data/RAM: 64.680 Bytes (12,34% dari 512 KB)
  - Warning/Error: **0 Error, 0 Warning**.

