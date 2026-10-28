# Log Perubahan 026: Tiga Register Log FIFO Non-Volatile Terpisah & Error Register (SPLN D3.006: 2021 Butir 12.3 Tabel 18)

## 1. Latar Belakang & Analisis Dokumen Acuan
Sesuai dengan ketentuan **SPLN D3.006: 2021 Butir 12.3 (Hal. 33–34)** dan **Tabel 18 ("Informasi pada alarm/event/tamper log")**:
1. **Pemisahan 3 Buku Log Terpisah (Masing-masing $\ge 30$ Rekaman FIFO Non-Volatile)**:
   - *"Untuk setiap kejadian alarm, event, dan tamper, meter harus menyimpan catatan data kejadian pada log masing-masing. Log yang harus disajikan sesuai pada Tabel 18."*
   - *"masing-masing log alarm, event, dan tamper tersebut ditempatkan pada register memori non-volatile (tak terhapus) dan sekurang-kurangnya mampu tersimpan sebanyak 30 rekaman. Seluruh proses menggunakan metode penyimpanan FIFO (First In First Out) dan tidak dapat di-edit."*
   - **Log 1 (Alarm Log - 8 Jenis Kejadian)**: Flash memory rusak (`ERR00`), RAM rusak (`ERR01`), Clock loss (`ERR02`), Low battery (`ERR03`), MCU abnormal (`ERR04`), Kegagalan ADC (`ERR05`), Superkapasitor rusak (`ERR06`), Relai/shunt trip gagal (`ERR07`).
   - **Log 2 (Event Log - 12 Jenis Kejadian)**: Power failure, Power up, ToU change, Non-ToU change, Stop-measurement, I/O change, Relay status change, Meter reset, Date/time change, Asymmetric power, Meter ready remote shunt trip ON, Power limiter exceeded.
   - **Log 3 (Tamper Log - 9 Jenis Kejadian)**: Case open (`RUSAK`), Terminal open (`ERR20`), Wrong sequence (`ERR21`), Cross phase (`ERR22`), Missing neutral (`ERR23`), Phase loss (`ERR24`), Magnetic induction $\le 500\text{ mT}$ (`ERR25`), Reverse power (`ERR26`), Quad 4 $PF < 0{,}85$ (`ERR27`).
2. **Akses Data Berdasarkan Indeks (Selective Access by Index)**:
   - *"Data log alarm/event/tamper dapat diambil oleh pusat data berdasarkan indeks (selective access)."*
3. **Error Register (Bit-String Rangkuman Real-Time)**:
   - *"Meter juga harus menyediakan error register yang berisi data rangkuman berupa bit-string terhadap informasi alarm, event, dan tamper yang sedang terjadi."*

---

## 2. File dan Komponen yang Terdampak
1. [`E3/app/tamper/inc/tamper_manager.h`](file:///g:/PPPP/ALLIN/E3/app/tamper/inc/tamper_manager.h):
   - Definisi enum kode kejadian standar: `spln_alarm_log_code_t` (1..8), `spln_event_log_code_t` (1..12), `spln_tamper_log_code_t` (1..9).
   - Struktur universal rekaman `spln_standard_log_entry_t` dan buffer FIFO `spln_log_fifo_t` (30 rekaman).
   - Penambahan tiga instans register log FIFO ke dalam `tamper_context_t` (`alarm_log_fifo`, `event_log_fifo`, `tamper_log_fifo`) dan `error_register_bitstring`.
   - Deklarasi API perekaman, pembacaan selektif berdasarkan indeks, dan pembacaan Error Register.
2. [`E3/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/ALLIN/E3/app/tamper/src/tamper_manager.c):
   - Implementasi `fifo_push_entry` dengan proteksi rolling circular buffer non-destructive.
   - Implementasi `spln_log_record_alarm`, `spln_log_record_event`, `spln_log_record_tamper`, `spln_log_get_entries`, `spln_log_get_entry_by_index`, dan `spln_log_get_error_register`.
3. [`E3/app/e3_meter_app.h`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.h) & [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c):
   - Integrasi otomatis perekaman Power failure & Power up ke Event Log.
   - Integrasi perekaman Asymmetric power, Power limiter exceeded, Relay change, dan Ready Reconnect ke Event Log.
   - Integrasi kegagalan internal hardware ke Alarm Log.
   - Integrasi fungsi getter konteks `e3_meter_app_get_tamper_context()`.
4. [`E3/tests/test_e3_app.c`](file:///g:/PPPP/ALLIN/E3/tests/test_e3_app.c):
   - Penambahan test suite `test_spln_butir12_3_three_fifo_registers()` yang memverifikasi 3 register log FIFO, mekanisme rolling buffer 35 entri, selective access indeks 0..29, dan bit-string Error Register.

---

## 3. Rincian Perubahan Kode

### A. [`tamper_manager.h`](file:///g:/PPPP/ALLIN/E3/app/tamper/inc/tamper_manager.h)
```c
typedef struct {
    uint32_t timestamp_epoch;  /**< Waktu kejadian (Epoch UNIX RTC) */
    uint16_t event_code;       /**< Kode kejadian (Alarm/Event/Tamper) */
    uint16_t status_flags;     /**< 1 = Mulai (Occurred), 0 = Selesai (Restored) */
    uint32_t parameter_val;    /**< Parameter tambahan (misal nilai deviasi asimetri, daya, dll) */
} spln_standard_log_entry_t;

typedef struct {
    spln_standard_log_entry_t records[SPLN_LOG_FIFO_MAX_ENTRIES];
    size_t                    head;          /**< Indeks tulis FIFO (0..29) */
    size_t                    count;         /**< Jumlah rekaman aktif (0..30) */
    uint32_t                  total_events;  /**< Akumulator total seluruh kejadian yang pernah dicatat */
} spln_log_fifo_t;

typedef struct {
    ...
    /* 3 Register Log FIFO Non-Volatile Terpisah Sesuai SPLN D3.006: 2021 Butir 12.3 */
    spln_log_fifo_t      alarm_log_fifo;   /**< Register 1: Alarm Log (8 jenis) - Min. 30 Entri FIFO */
    spln_log_fifo_t      event_log_fifo;   /**< Register 2: Event Log (12 jenis) - Min. 30 Entri FIFO */
    spln_log_fifo_t      tamper_log_fifo;  /**< Register 3: Tamper Log (9 jenis) - Min. 30 Entri FIFO */
    uint32_t             error_register_bitstring;
} tamper_context_t;
```

### B. [`tamper_manager.c`](file:///g:/PPPP/ALLIN/E3/app/tamper/src/tamper_manager.c)
```c
bool spln_log_record_alarm(tamper_context_t *ctx, spln_alarm_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param);
bool spln_log_record_event(tamper_context_t *ctx, spln_event_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param);
bool spln_log_record_tamper(tamper_context_t *ctx, spln_tamper_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param);
bool spln_log_get_entry_by_index(const spln_log_fifo_t *fifo, size_t index, spln_standard_log_entry_t *out_entry);
uint32_t spln_log_get_error_register(const tamper_context_t *ctx);
```

---

## 4. Hasil Verifikasi & Pengujian
1. **Host CTest (x86_64 GCC / Ninja)**:
   - 7 dari 7 suite lulus (`100% tests passed, 0 tests failed`).
   - Eksekusi `test_spln_butir12_3_three_fifo_registers()` memverifikasi:
     - 35 rekaman dialirkan ke Alarm Log: kapasitas terjaga tepat 30 entri, total terhitung 35 kejadian.
     - Selective Access: pembacaan indeks 0 (rekaman terlama yang tersisa) dan indeks 29 (rekaman terbaru).
     - Event Log: perekaman kejadian pemadaman, pemulihan listrik, asimetri arus, dan trip limiter.
     - Error Register Bit-string: status aktif terpetakan secara real-time pada 32-bit register (`0x0C1A03FF`).
2. **Kompilasi Target ARM Cortex-M33 NonSecure (`ALLIN_NS.elf`)**:
   - Kompiler: `arm-none-eabi-gcc 14.3.1`
   - Memory Region Footprint:
     - **RAM**: 64,680 B / 512 KB (12.34%)
     - **ROM**: 133,112 B / 1 MB (12.69%)
   - **0 Error, 0 Warning**.

