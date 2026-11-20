# 📘 Panduan Orientasi Anggota Baru Pilar E3
### *Smart Meter 3-Fasa Firmware (STM32U585)*

Selamat datang di tim **Pilar E3**! Dokumen ini disusun sebagai panduan orientasi ringkas dan praktis agar setiap pengembang baru dapat memahami filosofi arsitektur, struktur kode, tata kelola memori, serta alur kerja pengujian di Pilar E3 dalam waktu singkat.

---

## 1. Peta Besar: Di Mana Posisi Pilar E3?

Arsitektur firmware Smart Meter 3-Fasa ini dirancang modular menjadi **Tiga Pilar Utama**:

```
┌────────────────────────────────────────────────────────────────────────┐
│                          FIRMWARE SMART METER                          │
├───────────────────┬────────────────────────────┬───────────────────────┤
│   PILAR E1        │        PILAR E2            │       PILAR E3        │
│  (Sensor/Tangan)  │    (Komunikasi/Mulut)      │   (Aplikasi & Otak)   │
├───────────────────┼────────────────────────────┼───────────────────────┤
│ • Chip ADE9000    │ • Protokol DLMS/COSEM      │ • Display & HMI       │
│ • Sampling Teg/Arus│ • Lapisan HDLC & A-XDR     │ • Deteksi Tamper      │
│ • Kalkulasi kWh   │ • Keamanan AES-128 GCM     │ • Kontrol Relai Beban │
│ • Harmonisa / THD │ • Kamus Objek OBIS         │ • Load Profile        │
│ • Sudut Fasa      │ • Port Optik & Port Modem  │ • Penyimpanan NVRAM   │
│                   │                            │ • Multi-Task FreeRTOS │
└───────────────────┴────────────────────────────┴───────────────────────┘
```

> **Peran E3:** E3 adalah pengendali utama (*system orchestrator*). E3 mengambil data mentah pengukuran dari E1, menyajikannya ke layar HMI, mengambil keputusan proteksi pemutusan beban (relai), mengawasi sabotase fisik, menyimpan riwayat pemakaian ke memori Flash SPI, dan melayani sinkronisasi data ke server DLMS/COSEM (E2).

---

## 2. Struktur Direktori & Navigasi Kode

Repositori E3 terorganisasi ke dalam modul-modul modular di bawah folder `app/` dan `drivers/`:

```text
E3/
├── app/
│   ├── board_profile_config.h   ⭐ Single Source of Truth pengaturan perangkat keras
│   ├── e3_meter_app.c/.h        Orchestrator aplikasi & siklus langkah (step loop)
│   ├── display/
│   │   ├── inc/display.h        Model tampilan HMI SPLN D3.006 (Tabel E.1 & E.2)
│   │   ├── src/display.c        Logika render teks, angka tarif ToU & ikon grafis
│   │   └── src/display_task.c   Task FreeRTOS carousel 5s & instant tamper wake
│   ├── fsm/
│   │   ├── fsm_statechart.c     State machine siklus daya meter
│   │   ├── power_relay_manager.c Kontrol relai (overload VA, asimetri, <184V / >276V)
│   │   └── rtos_tasks.c         4 Task FreeRTOS, IPC Queue & Mutex snapshot
│   ├── tamper/
│   │   └── tamper_manager.c     Deteksi sabotase (Case, Terminal, Magnet) & 3 Log FIFO
│   ├── profiling/
│   │   ├── load_profile.c       Perekaman 23-kanal load profile 15-menit & billing
│   │   ├── nvram_storage.c      Driver Winbond W25Q64 (8MB), Sector Erase 4KB, Ping-Pong
│   │   └── wcet_profiler.c      Pengukur waktu CPU (DWT 160MHz) & Watchdog pin PD12
│   └── console/
│       └── console_io.c         Interactive CLI serial (115200 bps) untuk simulasi
├── drivers/
│   ├── oled/                    Driver fisik OLED SSD1306 (I2C3) & Font SPLN
│   └── power/                   Manajemen daya hemat (Stop 2 mode ~4.5 uA)
├── tests/                       🧪 13 unit & integration tests CTest (100% lulus)
├── CMakeLists.txt               Konfigurasi build mandiri host PC
├── PANDUAN_ARSITEKTUR_DAN_FILE_E3.md  Dokumentasi arsitektur komprehensif
└── README.md                    Ikhtisar proyek
```

---

## 3. Aturan Konfigurasi: Jangan Hardcode!

Setiap nilai ambang batas, polaritas pin GPIO, atau opsi perangkat keras **wajib** merujuk ke:
👉 [**`app/board_profile_config.h`**](app/board_profile_config.h)

Berkas ini menyediakan 3 profil papan:
1. `BOARD_PROFILE_PROTOTYPE_OLED` (Default Dev Board: Layar OLED 128x32, Flash W25Q64).
2. `BOARD_PROFILE_PRODUCTION_SEGLCD` (Produksi Massal: LCD Segmen HT1621, Flash W25Q64).
3. `BOARD_PROFILE_HEADLESS_BENCH` (CI / Automated Factory Bench).

---

## 4. Arsitektur 4 Task Preemptive FreeRTOS

Firmware E3 berjalan secara preemptive berbasis CMSIS-RTOS V2 dengan alokasi prioritas Rate-Monotonic Scheduling (RMS):

| Task Name | Prioritas (CMSIS) | Periode ($T_i$) | Stack | Tanggung Jawab Utama |
| :--- | :---: | :---: | :---: | :--- |
| **`Task_Tamper_Emergency`** | `osPriorityRealtime` (32) | 100 ms | 1 KB | Memeriksa sabotase fisik melalui TrustZone (`SECURE_GetTamperStatusMask`), memutus relai kontaktor seketika (*instant trip*), dan memicu *instant refresh* OLED. |
| **`Task_Metrology_Profiling`** | `osPriorityHigh` (28) | 1000 ms | 2 KB | Mengakuisisi ADE9000 & kalkulasi beban tiruan, publikasi data thread-safe, simpan *checkpoint* energi Ping-Pong 60s, dan rekam profil beban 15 menit. |
| **`Task_DLMS_Server`** | `osPriorityNormal` (24) | 50 ms | 4 KB | Mengonsumsi antrean sabotase dari `xTamperQueue`, menaikkan counter OBIS, dan melayani komunikasi HDLC port optik/modem. |
| **`Task_UI_Display`** | `osPriorityBelowNormal` (16) | 100 ms | 2 KB | Memutar auto-scroll frame 5 detik (Tabel E.1) dan merender status bar/ikon grafis. |
| **`defaultTask` (Supervisor)** | `osPriorityLow` (8) | 100 ms | 2 KB | Memproses antrean karakter CLI serial, memantau *deadline* seluruh task via `wcet_watchdog_supervise_and_feed()`, dan memberi makan Watchdog hardware pin **PD12** (`EXT_WDI_PULSE`). |

> 🔒 **Aturan Thread-Safety:**  
> Jangan pernah membaca atau menulis variabel data pengukuran secara langsung lintas task! Selalu gunakan:
> - `rtos_meter_data_publish(&meas)` untuk mempublikasikan data.
> - `rtos_meter_data_get_snapshot(&out_meas)` untuk membaca snapshot yang diproteksi `xMeasMutex`.

---

## 5. Tata Kelola Memori Non-Volatile (Winbond W25Q64 - 8 MByte)

Penyimpanan data tagihan dan parameter kalibrasi pada bus SPI2 (`PB12..PB15`) menggunakan **Winbond W25Q64 (64 Mbit / 8 MByte)**:

### Peta Alokasi Sektor (4 KB per Sektor):
* **Sektor 0 (`0x000000`):** Parameter Kalibrasi ADE9000 (`CALB` + CRC32).
* **Sektor 1 (`0x001000`):** Energy Checkpoint **Slot A** (Ping-Pong).
* **Sektor 2 (`0x002000`):** Snapshot Tamper Context & FIFO Logbook.
* **Sektor 3 (`0x003000`):** Energy Checkpoint **Slot B** (Ping-Pong).
* **Sektor 4 (`0x004000`):** Device Configuration (IDPEL & Meter ID).
* **Sektor 5..2047 (`0x005000+`):** Ring Buffer Load Profile 23-Kanal (kapasitas hingga > 2,7 tahun).

### Aturan Baku Flash:
1. **Gunakan Opcode `0x20` (Sector Erase 4 KB):** Dilarang keras menggunakan block erase 64 KB (`0xD8`) karena akan merusak sektor tetangga.
2. **Ping-Pong Dual-Buffering:** Stand energi ditulis bergantian pada Slot A dan Slot B dengan validasi CRC32 untuk mencegah korupsi data akibat pemadaman mendadak (*power failure safe*).
3. **Verifikasi JEDEC ID:** Chip diverifikasi saat boot dengan membaca opcode `0x9F` (ID Winbond W25Q64: `0xEF 0x40 0x17`).

---

## 6. Alur Kerja Pengembangan Harian (Workflow)

### A. Menjalankan Unit Test di Host PC (Cepat tanpa Hardware)
Anda dapat memodifikasi logika dan menjalankan pengujian tanpa board fisik:
```powershell
# Kompilasi & Jalankan 13 Pengujian CTest
ninja -C G:/PPPP/ALLIN/E3/build
ctest --test-dir G:/PPPP/ALLIN/E3/build --output-on-failure
```

### B. Kompilasi ke Target Mikrokontroler STM32U585
Setelah pengujian lokal berhasil, kompilasi firmware target:
```powershell
ninja -C G:/PPPP/ALLIN/NonSecure/build
```
*Pastikan hasil build selalu 0 Errors dan 0 Warnings.*

---

## 7. Tiga "Aturan Emas" untuk Anggota Baru

1. **Wajib Rujuk SPLN D3.006: 2021:**
   Seluruh parameter tampilan HMI, batas tegangan ($<184\text{ V}$ dan $>276\text{ V}$), rotasi error 3 detik, dan 12 kejadian event diatur oleh standar PLN. Konsultasikan dokumen acuan di folder `Acuan/` sebelum mengubah parameter.
2. **Jaga Skor CTest 13/13 Tetap Hijau:**
   Setiap pull request / commit baru tidak boleh merusak satupun dari 13 unit test yang sudah ada.
3. **Catat Setiap Perubahan pada Changelog:**
   Setiap penambahan fitur atau perbaikan bug wajib didokumentasikan di folder `Changelog/` dengan ID berurutan (`LOG-xxx`) agar riwayat audit proyek tetap transparan.
