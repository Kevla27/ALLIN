# Log 022: Implementasi Peringatan Tampilan "Lo Volt" dan "Hi Volt" (SPLN D3.006: 2021 Butir 10.3 Tabel 9)

## 1. Konteks & Dasar Standar SPLN
Berdasarkan dokumen acuan **SPLN D3.006: 2021 Butir 10.3 (Hal. 27)**:
> **Tabel 9. Julat tegangan**
> - Tegangan operasi tersambung langsung: $0,5\,U_n \dots 1,2\,U_n$ (untuk $U_n = 230\text{ V}$, julatnya adalah $115\text{ V} \dots 276\text{ V}$).
> - Julat batas operasi: $< 0,5\,U_n$ dan $> 1,2\,U_n$.
> - *"Ketika $< 0,5\,U_n$ muncul tulisan **“Lo Volt”** dan ketika $> 1,2\,U_n$ muncul tulisan **“Hi Volt”**."*

Sebelumnya, meter hanya menampilkan nilai numerik tegangan terukur tanpa peringatan status teks `"Lo Volt"` maupun `"Hi Volt"` saat tegangan fasa keluar dari batas julat operasi nominal.

---

## 2. Modifikasi Teknis

### A. Tipe Data & Status Julat Tegangan (`E3/app/display/inc/display.h`)
- Menambahkan enum status batas tegangan pasok:
  ```c
  typedef enum {
      DISPLAY_VOLT_NORMAL = 0,
      DISPLAY_VOLT_LOW,     /* < 0.5 Un (< 115.0 V) -> "Lo Volt" */
      DISPLAY_VOLT_HIGH     /* > 1.2 Un (> 276.0 V) -> "Hi Volt" */
  } display_voltage_status_t;

  display_voltage_status_t display_get_voltage_status(uint32_t v_dvolts);
  ```

### B. Algoritma Evaluasi & Format Tampilan (`E3/app/display/src/display.c`)
- Mengimplementasikan fungsi evaluasi julat tegangan:
  - $V < 115.0\text{ V}$ ($< 1150\text{ dVolts}$) $\rightarrow$ `DISPLAY_VOLT_LOW`
  - $V > 276.0\text{ V}$ ($> 2760\text{ dVolts}$) $\rightarrow$ `DISPLAY_VOLT_HIGH`
- Pada HMI fisik LCD SPLN Gambar 4 (`display_render_spln_frame`):
  - Halaman `DISP_PAGE_VOLTAGE_R`, `S`, `T`: Jika tegangan berada di luar batas julat, nilai utama (`main_value`) menampilkan tulisan `"Lo Volt"` atau `"Hi Volt"` (berotasi secara periodik dengan angka tegangan terukur), dan icon alarm aktif (`!ALM`).
- Pada Format Teks Konsol CLI (`display_render_frame`):
  - Menambahkan tag label `[Lo Volt]` / `[Hi Volt]` pada judul fasa tegangan (Baris 2) dan teks keterangan pada Baris 3.
  - Mengaktifkan status alarm header `[(!)]`.

### C. Pengujian Unit & Verifikasi Regresi (`E3/tests/test_spln_display_tamper.c`)
- Menambahkan skenario **`[UJI 12] Verifikasi Indikator Lo Volt & Hi Volt (SPLN Butir 10.3 Tabel 9)`**:
  - Validasi ambang batas: $114.9\text{ V}$ (`Lo Volt`), $115.0\text{ V}$ (`Normal`), $276.0\text{ V}$ (`Normal`), $276.1\text{ V}$ / $280.0\text{ V}$ (`Hi Volt`) [PASS].
  - Simulasi fasa R tegangan rendah ($98.0\text{ V}$): `main_value` menghasilkan `"Lo Volt"`, `unit` = `"V"`, alarm = `"!ALM"`, CLI tag `[Lo Volt]` [PASS].
  - Simulasi fasa S tegangan tinggi ($285.0\text{ V}$): `main_value` menghasilkan `"Hi Volt"`, alarm = `"!ALM"`, CLI tag `[Hi Volt]` [PASS].
  - Pemulihan normal ($230.0\text{ V}$): `main_value` kembali ke `"230.0"`, alarm kembali ke `"OK"` [PASS].

---

## 3. Hasil Pengujian & Kompilasi
- **CTest Suite E3/build:** 7/7 test passed (100% Lulus).
- **Target ARM Cortex-M33 TrustZone NonSecure (`ALLIN_NS.elf`):**
  - Kompilasi: **0 Error, 0 Warning**.
  - RAM: 62.432 B (11.91%).
  - ROM: 130.936 B (12.49%).

