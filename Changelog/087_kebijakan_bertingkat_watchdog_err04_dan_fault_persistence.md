# LOG-087: Kebijakan Bertingkat Supervisor Watchdog (ERR04), Persistensi Fault RTC_BKP_DR10 & Integrasi OLED/CLI

- **Tanggal:** 25 November 2026
- **Status:** Selesai & Terverifikasi (CTest 13/13 Passed, `ALLIN_S.elf` & `ALLIN_NS.elf` 0 Error/0 Warning)
- **Tipe Perubahan:** Fault Tolerance / Hardware Watchdog Policy / TrustZone Boot Fault Persistence
- **Pilar / File Terdampak:** `Secure_nsclib/secure_nsc.h`, `Secure/Core/Src/secure_nsc.c`, `Secure/Core/Src/stm32u5xx_it.c`, `NonSecure/Core/Src/stm32u5xx_it.c`, `NonSecure/Core/Src/app_freertos.c`, `E3/app/fsm/inc/rtos_tasks.h`, `E3/app/fsm/src/rtos_tasks.c`, `E3/app/display/src/display_task.c`, `E3/app/console/src/console_io.c`, `docs/PERANCANGAN_ARSITEKTUR_FREERTOS_8_TASK.md`, `docs/freertos_8_tasks_flowchart.drawio.xml`, `E3/tests/test_rtos_tasks.c`

---

## 1. Latar Belakang & Tujuan

Sebelumnya, perilaku supervisor watchdog belum membedakan antara task antarmuka/komunikasi yang macet dengan kegagalan inti MCU atau hilangnya kristal/clock, sehingga berisiko memicu trip relai utama (`ERR04`) secara berlebihan atau sebaliknya kehilangan konteks reset ketika MCU di-reset oleh IC Watchdog eksternal (`PD12`).

Pembaruan ini menerapkan kebijakan bertingkat sesuai ketentuan **SPLN D3.006: 2021 Tabel 4 No. 5 (`ERR04`)**:
1. **Error Ringan / Task Non-Kritis (`DLMSTask`, `OLEDTask`, `ModemTask`):** Tidak menghentikan pulsa Watchdog `PD12`, tidak me-latch `ERR04`, dan tidak membuka relai pelanggan (`WDT_SEVERITY_MINOR_DEGRADED`), serta pulih otomatis saat task aktif kembali.
2. **Glitch Sementara pada Task Kritis (`< 3` kali beruntun):** Supervisor menahan toggle `PD12` agar IC Watchdog eksternal me-reset MCU, mencatat *reset streak* di Backup Register RTC (`RTC_BKP_DR10`), tanpa me-latch `ERR04` atau memutus relai saat pulih. Setelah sistem berjalan stabil selama **`10.000 ms`** (`WDT_STABLE_UPTIME_CLEAR_MS`), *reset streak* direset kembali ke `0`.
3. **Kegagalan Fatal (`ERR04` Latch + Buka Relai + LED Alarm Kuning):** Hanya aktif apabila:
   - Reset akibat gangguan task kritis terjadi **`>= 3` kali beruntun** (`WDT_FATAL_RESET_THRESHOLD = 3U`), atau
   - Terjadi trap perangkat keras kritis: `NMI_Handler` (HSE/CSS clock loss), `HardFault_Handler`, `MemManage_Handler`, `BusFault_Handler`, `UsageFault_Handler`, `SecureFault_Handler`, atau `vApplicationStackOverflowHook`.

---

## 2. Rincian Perubahan Implementasi

1. **Persistensi Register Fault Lintas TrustZone (`RTC_BKP_DR10`):**
   - Menambahkan API NSC di [`Secure_nsclib/secure_nsc.h`](file:///g:/PPPP/ALLIN/Secure_nsclib/secure_nsc.h) & [`Secure/Core/Src/secure_nsc.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/secure_nsc.c): `SECURE_ReadBootFaultReg()`, `SECURE_WriteBootFaultReg()`, dan `SECURE_StampFatalFault()`.
   - Mengintegrasikan `SECURE_StampFatalFault()` di ISR fault handler Secure ([`Secure/Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/ALLIN/Secure/Core/Src/stm32u5xx_it.c)), Non-Secure ([`NonSecure/Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/stm32u5xx_it.c)), dan [`vApplicationStackOverflowHook()`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/app_freertos.c#L115-L122).
2. **Supervisor & Klasifikasi Watchdog RTOS:**
   - Memisahkan `WATCHDOG_CRITICAL_TASKS_MASK` (`TamperTask`, `ProfileTask`, `HealthTask`, `StorageTask`) dan `WATCHDOG_NONCRITICAL_TASKS_MASK` (`DLMSTask`, `OLEDTask`, `ModemTask`) di [`rtos_tasks.h`](file:///g:/PPPP/ALLIN/E3/app/fsm/inc/rtos_tasks.h#L60-L95) dan [`rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/rtos_tasks.c#L350-L470).
   - Menambahkan evaluasi boot `rtos_watchdog_on_boot()`, `rtos_watchdog_clear_err04()`, dan pelaporan status `wdt_supervisor_status_t` yang disinkronkan ke `OLEDTask` ([`display_task.c`](file:///g:/PPPP/ALLIN/E3/app/display/src/display_task.c)) dan CLI `wdt` ([`console_io.c`](file:///g:/PPPP/ALLIN/E3/app/console/src/console_io.c#L862-L940)).
3. **Dokumentasi & Diagram Arsitektur 8-Task:**
   - Menambahkan dokumen [`docs/PERANCANGAN_ARSITEKTUR_FREERTOS_8_TASK.md`](file:///g:/PPPP/ALLIN/docs/PERANCANGAN_ARSITEKTUR_FREERTOS_8_TASK.md) dan [`docs/freertos_8_tasks_flowchart.drawio.xml`](file:///g:/PPPP/ALLIN/docs/freertos_8_tasks_flowchart.drawio.xml).

---

## 3. Verifikasi Unit Test
- **`Unit_E3_RTOS_Tasks` ([`test_rtos_tasks.c`](file:///g:/PPPP/ALLIN/E3/tests/test_rtos_tasks.c#L157-L271)):** Teruji skenario *Minor Degraded* tanpa trip relai (8a), glitch kritis 1× pulih & reset streak di 10.000 ms (8b), loop reset fatal 3× melatch `ERR04` & memicu trip relai yang bertahan lintas reboot hingga clear resmi (8c), dan simulasi trap `WDT_SRC_CLOCK_NMI` (Test 8).
