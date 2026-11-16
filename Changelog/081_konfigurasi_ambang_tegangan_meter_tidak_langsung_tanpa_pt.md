# Log Perubahan 081: Penyesuaian Ambang Batas Cut-Off Tegangan Meter Tersambung Tidak Langsung Tanpa PT (184 V & 276 V)

- **Judul Dokumen:** Penyesuaian Ambang Batas Cut-Off Tegangan Meter Tersambung Tidak Langsung Tanpa PT (184 V & 276 V) Sesuai SPLN D3.006: 2021 Tabel 9
- **Tanggal:** 16 November 2026
- **Status:** Selesai & Terverifikasi Penuh (0 Error, 0 Warning, CTest 12/12 Passed, 100%)
- **Tipe Perubahan:** Specification Realignment / Voltage Protection Thresholds / SPLN Compliance
- **Pilar / Komponen Terdampak:** Pilar E3 (`board_profile_config.h`, `power_relay_manager.h`, `power_relay_manager.c`, `e3_meter_app.c`, `display.h`, `display.c`), Unit Tests (`test_board_profile.c`, `test_power_relay.c`, `test_spln_display_tamper.c`).

---

## Bagian 1: Latar Belakang & Analisis Integrator

Sesuai arahan teknis, mode operasional meter disesuaikan dari tipe **Tersambung Langsung (*Direct Connected*)** menjadi **Tersambung Tidak Langsung Tanpa PT (*Semi-Direct / CT-Only Connected*)**:

1. **Rujukan Standar SPLN D3.006: 2021 Butir 10.3 Tabel 9 (*Julat Tegangan*):**
   - Pada meter tersambung langsung, rentang operasi normal adalah $0,5 U_n \text{ s/d } 1,2 U_n$ dengan batas pemutusan relai $< 110\text{ V}$ dan $> 280\text{ V}$.
   - Namun, untuk **meter tersambung tidak langsung**, standar PLN menetapkan rentang operasi yang lebih ketat yaitu:
     $$0,8 U_n \text{ s/d } 1,2 U_n$$
   - Dengan tegangan pengenal nominal sistem $U_n = 230\text{ V}$ (fasa-ke-netral):
     - **Batas Bawah (*Under-Voltage Cut-Off*):** $0,8 \times 230\text{ V} = \mathbf{184\text{ V}}$ ($1840\text{ dV}$).
     - **Batas Atas (*Over-Voltage Cut-Off*):** $1,2 \times 230\text{ V} = \mathbf{276\text{ V}}$ ($2760\text{ dV}$).

2. **Karakteristik Histeresis Pemulihan (*Auto-Reclose*):**
   - Untuk mencegah osilasi/cetak-cetek (*chattering*) pada kontak relai atau mekanisme *shunt trip* eksternal saat tegangan berfluktuasi di sekitar ambang batas:
     - **Histeresis Tegangan Rendah (+5 V):** Pemutusan terjadi saat $< 184\text{ V}$ (`1840U`), dan relai baru boleh menutup kembali (*reclose*) jika tegangan pulih di atas **$189\text{ V}$** (`1890U`).
     - **Histeresis Tegangan Tinggi (-4 V):** Pemutusan terjadi saat $> 276\text{ V}$ (`2760U`), dan relai baru boleh menutup kembali jika tegangan turun stabil di bawah **$272\text{ V}$** (`2720U`).

---

## Bagian 2: Rincian Modifikasi Kode Program

### 1. File Konfigurasi Profil Papan (`E3/app/board_profile_config.h`)
- Mengubah ambang batas relai untuk seluruh profil papan (OLED, Segment LCD, dan Bench):
  - `BOARD_CFG_RELAY_UNDERVOLT_DV` : dari `1100U` (110.0 V) $\rightarrow$ `1840U` (184.0 V).
  - `BOARD_CFG_RELAY_OVERVOLT_DV`  : dari `2800U` (280.0 V) $\rightarrow$ `2760U` (276.0 V).

### 2. Modul Power & Relay Manager (`E3/app/fsm/inc/power_relay_manager.h` & `src/power_relay_manager.c`)
- Memperbarui konstanta ambang SPLN:
  - `SPLN_VOLT_UNDER_CUTOFF_DV` : `1840U` ($< 184,0\text{ V}$).
  - `SPLN_VOLT_UNDER_RECLOSE_DV`: `1890U` ($> 189,0\text{ V}$, histeresis 5 V).
  - `SPLN_VOLT_OVER_CUTOFF_DV`  : `2760U` ($> 276,0\text{ V}$).
  - `SPLN_VOLT_OVER_RECLOSE_DV` : `2720U` ($< 272,0\text{ V}$, histeresis 4 V).
- Memperbarui string alasan pemutusan pada `power_relay_reason_to_string`:
  - `RELAY_REASON_UNDER_VOLTAGE`: `"UNDER_VOLTAGE (<184V)"`.
  - `RELAY_REASON_OVER_VOLTAGE`: `"OVER_VOLTAGE (>276V)"`.

### 3. Logika Meter Application (`E3/app/e3_meter_app.c`)
- Menyesuaikan deteksi anomali hilang fasa (ERR24) agar merujuk ke konstanta terpusat `BOARD_CFG_RELAY_UNDERVOLT_DV` ($1840\text{ dV}$).

### 4. Modul Display & Indikator Tegangan (`E3/app/display/inc/display.h` & `src/display.c`)
- Menyesuaikan fungsi `display_get_voltage_status`:
  - Nilai di bawah $184,0\text{ V}$ diklasifikasikan sebagai `DISPLAY_VOLT_LOW` (memunculkan indikator status `"Lo Volt"`).
  - Nilai di atas $276,0\text{ V}$ diklasifikasikan sebagai `DISPLAY_VOLT_HIGH` (memunculkan indikator status `"Hi Volt"`).
- Penanda keberadaan fasa tegangan pada header frame SPLN (`UR, US, UT`) kini dievaluasi aktif jika tegangan $\ge 184,0\text{ V}$.

### 5. Penyesuaian Unit Test Suite (`E3/tests/`)
- `test_board_profile.c`: Menegaskan `BOARD_CFG_RELAY_UNDERVOLT_DV == 1840U` dan `BOARD_CFG_RELAY_OVERVOLT_DV == 2760U`.
- `test_power_relay.c`: Menguji siklus proteksi tegangan baru:
  - Fasa R $180\text{ V} (< 184\text{ V}) \rightarrow$ TRIP.
  - Fasa R $186\text{ V}$ (histeresis) $\rightarrow$ tetap DISCONNECTED.
  - Fasa R $192\text{ V} (> 189\text{ V}) \rightarrow$ Pulih otomatis CONNECTED.
  - Fasa T $280\text{ V} (> 276\text{ V}) \rightarrow$ TRIP.
  - Fasa T $274\text{ V}$ (histeresis) $\rightarrow$ tetap DISCONNECTED.
  - Fasa T $270\text{ V} (< 272\text{ V}) \rightarrow$ Pulih otomatis CONNECTED.
- `test_spln_display_tamper.c`: Menegaskan evaluasi `Lo Volt` pada $< 184\text{ V}$ dan `Hi Volt` pada $> 276\text{ V}$.

---

## Bagian 3: Verifikasi & Hasil Pengujian

1. **Kompilasi Firmware Target STM32U585:**
   ```bash
   ninja -C NonSecure/build
   ```
   **Hasil:** `0 error, 0 warning`. Ukuran binary: Flash $18,55\%$, RAM $14,15\%$.

2. **Pengujian Regresi Otomatis (CTest):**
   ```bash
   ctest --test-dir E3/build --output-on-failure
   ```
   **Hasil:** **12/12 test lulus (100%)**.
