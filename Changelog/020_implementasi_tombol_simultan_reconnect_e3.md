# Log 020: Implementasi Konfirmasi Tombol Simultan (▲ + ▼) untuk Rekoneksi Relai (SPLN D3.006 Tabel 5 No 12)

## 1. Konteks & Tujuan
Memenuhi persyaratan keselamatan kelistrikan mandatori pada **SPLN D3.006: 2021 Tabel 5 No. 12 (Meter ready remote reconnect)**:
> *"Meter belum dapat remote reconnect jika belum dilakukan action. Action berupa tekan tombol atas dan bawah secara bersamaan."*

---

## 2. Modifikasi Teknis

### A. Tipe Data Antarmuka Tombol (`E3/app/display/inc/display.h`)
- Menambahkan nilai `BUTTON_DIR_BOTH` pada enum `button_dir_t`:
  ```c
  typedef enum {
      BUTTON_DIR_UP = 0,
      BUTTON_DIR_DOWN,
      BUTTON_DIR_BOTH  /* Tekan tombol atas & bawah secara bersamaan (SPLN Tabel 5 No 12) */
  } button_dir_t;
  ```

### B. Handler Hardware EXTI (`NonSecure/Core/Src/main.c`)
- Memodifikasi `HAL_GPIO_EXTI_Falling_Callback()`:
  - Saat interupsi `BTN_SCROLL_UP_Pin` aktif, sistem mengecek level pin `BTN_SCROLL_DN_Pin`. Bila sama-sama LOW (`GPIO_PIN_RESET`), sistem memancarkan event `BUTTON_DIR_BOTH`.
  - Hal serupa berlaku saat interupsi `BTN_SCROLL_DN_Pin` aktif.

### C. FSM Relai Aplikasi Meter (`E3/app/e3_meter_app.c`)
- Memperketat konfirmasi rekoneksi relai status `READY ON`:
  - Penekanan tombol tunggal (hanya ▲ atau hanya ▼) **tidak akan** menyambungkan relai kontaktor (hanya menggulir tampilan).
  - Relai kontaktor **hanya tersambung (kembali ke `CONNECTED`)** apabila diterima event `BUTTON_DIR_BOTH` (tombol ▲ dan ▼ ditekan bersamaan).
  - Saat tersambung, sinyal `RELAY_TRIP_Pin` dinonaktifkan (`GPIO_PIN_RESET`), `LED_ALM_Pin` dipadamkan, dan buzzer membunyikan nada konfirmasi.

### D. Pengujian Unit & Validasi (`E3/tests/test_power_relay.c`)
- Menambahkan fungsi uji `test_simultaneous_button_reconnect()`:
  - Verifikasi bahwa penekanan tunggal UP atau DOWN tidak membatalkan status `READY ON` [PASS].
  - Verifikasi bahwa penekanan simultan `BUTTON_DIR_BOTH` berhasil memicu transisi relai kembali ke `CONNECTED` [PASS].

---

## 3. Hasil Pengujian & Kompilasi
- **CTest Suite E3/build:** 7/7 test passed (100%).
- **Target ARM Cortex-M33 TrustZone:**
  - `ALLIN_S.elf` & `ALLIN_NS.elf`: **0 Error, 0 Warning**.
  - NonSecure RAM: 11.91% (62.432 B / 512 KB).
  - NonSecure Flash/ROM: 12.31% (129.128 B / 1 MB).

