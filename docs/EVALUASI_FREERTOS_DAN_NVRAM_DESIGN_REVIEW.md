# FreeRTOS & NVM Design Review

**Base References:**
* *Dokumen Desain Sistem Smart Meter 3 Fasa* (Section 3.3.1 p. 69, Section 3.2.3, Tables 3.9, 3.11–3.12, 3.38–3.40, 3.50, 3.54–3.56)
* `ONBOARDING_GUIDE.md`
* Source Files Pilar E3 (`rtos_tasks.c/.h`, `wcet_profiler.c/.h`, `display_task.c`, `nvram_storage.c`, `e3_meter_app.c/.h`, `board_profile_config.h`, `test_*.c`)

> [!NOTE]
> Code line numbers are omitted; references are mapped directly to document sections, table numbers, and source filenames.

> [!IMPORTANT]
> **Architectural Note:**  
> Table 3.38 specifies **4 application tasks** plus the Idle Task, whereas `ONBOARDING_GUIDE.md` describes a 5th supervisor task (`defaultTask`, $100\text{ ms}$ periodic) and `wcet_profiler.h` defines a 5th slot (`WCET_TASK_APP_STEP`). In addition, the repository contains two parallel execution paths (`rtos_tasks.c` and `e3_meter_app.c`) with separate contexts and divergent power limits ($6600\text{ W}$ vs. $11000\text{ W}$), and `rtos_tasks.c` never invokes `HAL_GPIO_WritePin` for relay actuation (updating logical state only).

---

## 1. Confirmed Gaps (Code vs. Design Document & Codebase Findings)

| # | Area | Design Reference | Code Evidence | Impact & Required Action |
| :-: | :--- | :--- | :--- | :--- |
| **1** | **Non-RMS priority ordering in specification** | Section 3.3.1, Table 3.38: Tamper (prio 4, $100\text{ ms}$), Metrology (prio 3, $1000\text{ ms}$), DLMS (prio 2, $50\text{ ms}$), UI (prio 1, $500\text{ ms}$) | `rtos_tasks.h`: numeric priorities 32 (Tamper), 28 (Metrology), 24 (DLMS), 16 (UI) — matching Table 3.38's relative order where Metrology ($1000\text{ ms}$) outranks DLMS ($50\text{ ms}$). | The design document itself specifies a non-RMS ordering ($T_{\text{DLMS}} < T_{\text{Metrology}}$ yet $P_{\text{DLMS}} < P_{\text{Metrology}}$). Reassign priorities strictly by period or evaluate deterministic feasibility via fixed-priority Response-Time Analysis (RTA). |
| **2** | **RMS bound applied without RMS prerequisites** | Section 3.3.1, Table 3.39: $U_{\text{bound}} = 4(2^{1/4}-1) \approx 75{,}68\%$ cited as proof of determinism | `rtos_tasks.h`, `wcet_profiler.h` | The classical Liu–Layland bound is invalid when task priorities do not strictly follow monotonic arrival rates ($1/T_i$) and omits shared-resource blocking times ($B_i$). |
| **3** | **WCET figures are design budgets, not DWT measurements** | Table 3.39 title & $160\text{ MHz}$ / ICACHE measurement narrative | `wcet_profiler.c` / `wcet_profiler.h`: configured budgets match Table 3.39 identically ($1{,}85\text{ ms}$, $14{,}20\text{ ms}$, $6{,}40\text{ ms}$, $18{,}50\text{ ms}$); no target hardware DWT cycle measurements exist. | Relabel document figures as *design budgets/estimates* until empirical hardware DWT cycle counter logs are recorded on silicon. |
| **4** | **Period drift, tick-based timestamps, & 15-minute profile skew** | Table 3.38 Period column | `rtos_tasks.c`: task loops use `vTaskDelay()` instead of `vTaskDelayUntil()`, 15-minute profile intervals are counted via 900 tick iterations, and timestamps are derived from boot ticks rather than RTC. | Effective task periods become $T_i + C_i + \text{interference}$, drifting the 15-minute profile interval and producing non-monotonic timestamps across MCU reboots. Transition to `vTaskDelayUntil()` and anchor all timestamps to the hardware RTC calendar. |
| **5** | **UI task implementation mismatch** | Tables 3.38–3.39: $500\text{ ms}$ periodic, $18{,}50\text{ ms}$ WCET; Section 3.3.4: $5\text{ s}$ auto-scroll | `rtos_tasks.c` spawns `task_oled128x32_carousel` (`display_task.c`: $100\text{ ms}$ `vTaskDelay` poll, $5000\text{ ms}$ / event-driven render; `wcet_record` wraps only the render block). `task_ui_display_entry` ($1000\text{ ms}$ delay) is dead code. | Eliminate or formally flag the unused stub; update Tables 3.38–3.39 to accurately reflect $100\text{ ms}$ polling + $5000\text{ ms}$ / event-driven rendering. |
| **6** | **Message queue is single-consumer, not broadcast** | Section 3.3.1 ("Message Queue" consumed by DLMS and UI) | `rtos_tasks.c`: only `task_dlms_entry` invokes queue receive; UI polls state via `display_task_is_alarm_active()`. | FreeRTOS queues are destructive (single-consumer). Update the design specification to reflect single-consumer queue usage or migrate to Event Groups / direct task notifications for multi-consumer signaling. |
| **7** | **Incorrect tamper classification, unintended magnet trip, & latched alarm** | Tables 3.12, 3.39 (9 conditions), 3.40 & 3.50 (`PE4` `SENSOR_MAGNET`: ERR25 without relay trip) | `rtos_tasks.c`: `tamper_code` is hardcoded to `METER_COVER_OPEN`, magnet detection triggers `power_relay_disconnect()`, and `current_alarm` is initialized from `display_task_is_alarm_active()` then bitwise OR-ed with sensors such that it never self-clears (only toggled via `tamper_handle_button_press_isr`). `display_task.c` hardcodes `TAMPER_VECTOR_CASE_OPEN`. | Magnet presence trips the disconnect relay erroneously, hardware alarm recovery cannot self-clear, and the display cannot distinguish `ERR20`, `ERR25`, and `RUSAK`. Map each sensor vector to its proper event code, relay action matrix, and clear condition. |
| **8** | **Missing Metrology features, zeroed checkpoint fields, & inverted import/export naming** | Table 3.39 (8-zone ToU, MD, CRC-16) | `rtos_tasks.c` implements sampling, asymmetry, power limiter, 15-minute profile, and 60 s checkpoint (`load_profile_add_entry` populates CRC-16), but 8-zone ToU and MD are absent; checkpoint ToU / `vcap_mv_at_fail` / `active_tamper_mask` are overwritten with 0 every 60 s; import/export field naming is inverted across display, NVRAM, and DLMS layers. | Implement ToU/MD logic or adjust documented module scope, populate checkpoint context fields explicitly, and standardize import/export field semantics across all software layers. |
| **9** | **Blocking SPI2 NVM operations inside high-priority tasks & watchdog fault risk** | Table 3.39, Table 3.50 (`PB12`–`PB15`), Table 3.55 ($\approx 45\text{ ms}$ sector erase) | `nvram_storage.c`: uses polling `HAL_SPI_Transmit` and `spi2_flash_wait_busy()` busy-wait (`HAL_GetTick`, $500\text{ ms}$ timeout) without yielding, called directly inside `wcet_record_start`/`stop` blocks in Tamper ($1{,}85\text{ ms}$ budget) and Metrology ($14{,}20\text{ ms}$ budget), as well as DLMS. | An $\approx 45\text{ ms}$ erase threatens the $50\text{ ms}$ DLMS deadline and substantially exceeds task WCET budgets, setting `fault_latched` in `wcet_profiler` and blocking `wcet_watchdog_supervise_and_feed()`. Furthermore, wall-clock profiling captures preemption time rather than pure execution time $C_i$. |
| **10** | **Unprotected concurrent SPI2 flash access** | Section 3.3.1 (measurement data mutex) | `nvram_storage.c`: no bus mutex or serialization mechanism across calling tasks. | Preemption mid-SPI transaction risks corrupting Chip Select (CS) state machine and NOR Flash command framing. |
| **11** | **Duplicate tamper snapshot writes per event** | Table 3.40 | `task_tamper_emergency_entry` and `rtos_dlms_process_tamper_event` both invoke `nvram_save_tamper_log_snapshot()` for the exact same event instance. | Induces two consecutive sector erase/write cycles per tamper event, one executed directly inside the highest-priority safety task. |
| **12** | **Checkpoint flash wear-out & boot-tick slot selection** | Table 3.9 ($\ge 100{,}000$ erase cycles), Table 3.55, $\ge 15$-year product lifetime target | `rtos_tasks.c` writes checkpoints every 60 s across two ping-pong sectors ($\approx 139\text{ days}$ until 100k cycle endurance exhaustion), while `e3_meter_app.c` writes every 15 min ($\approx 5{,}7\text{ years}$). Slot selection relies on boot-tick timestamps in `rtos_tasks.c`. | Both implementations fall far short of the mandatory 15-year lifetime target, and boot-tick timestamps allow an older slot to appear newer following an MCU reboot. Implement multi-sector append-only wear leveling with monotonic 32-bit sequence counters. |
| **13** | **Ignored NVM write errors & non-atomic calibration/config updates** | Tables 3.9, 3.11 (ERR00 on flash failure) | `nvram_storage.c`: erase/write return codes are ignored (returns `true` unconditionally) and ping-pong pointers advance upon failure. `nvram_save_calibration`, `nvram_save_device_config`, and tamper snapshots erase then write a single sector in-place. | Write failures go completely unreported as `ERR00`; power loss occurring mid-erase permanently destroys ADE9000 calibration, device configuration, or historical records. |
| **14** | **Flash JEDEC mismatch & inconsistent hardcoded IDPEL** | Table 3.55 (W25Q128JV), Table 3.41 (01 IDPEL & 02 METERID) | `nvram_storage.c` validates JEDEC `0xEF 0x40 0x17` (W25Q64 rather than `0x18` for W25Q128JV). `display_task.c` hardcodes `"530000000001"` for IDPEL and `"MTR530000000001"` for Meter ID (conflicting with `54321678901` in `board_profile_config.h`), reads `pr_ctx->is_asymmetry_active` unlocked, and uses a local `disp_ctx` decoupled from `g_display_ctx`. | Flash verification fails unconditionally on physical W25Q128JV hardware; customer IDPEL is hardcoded and inconsistent across modules; asymmetry state access is unsynchronized. |
| **15** | **Verification limited to isolated host unit tests** | Section 3.3.1 conclusion (determinism claim) | `test_rtos_tasks.c`, `test_nvram_storage.c`, `test_wcet_profiler.c`, `test_stress_soak.c`: evaluate RAM mirrors and utilize `i % MAX` circular indexing that masks production ring-buffer index bugs without SPI concurrency or target hardware timing. | No empirical evidence supports the zero-deadline-miss claim on silicon, and host unit tests conceal persistent storage indexing defects. |
| **16** | **Load profile ring-buffer indexing & missing sector erase** | Table 3.9 ($\ge 23$ channels, $\ge 90$ / 92 days retention) | `rtos_tasks.c` writes at post-increment count (skipping index 0 and silently failing after 100 entries / $\approx 25\text{ hours}$); `e3_meter_app.c` writes at $\text{count} - 1$ (stalling at slot 99). Sector erase only triggers when byte offset aligns to 4 KB boundaries (only entries 0 and 256 at $\approx 80\text{ B}$/entry). | Capacity (100 entries) is drastically below the 92-day statutory requirement; circular wraparound either stalls execution or attempts to write over unerased flash sectors, corrupting profile data. |
| **17** | **Uninitialized flash tamper snapshot & conflicting DLMS log symbols** | Table 3.9, Table 3.12 | On virgin/blank flash (`0xFFFFFFFF`), `total_event_count > 0` condition evaluates to true without magic-byte or CRC validation, loading corrupt data. `rtos_tasks.c` defines `dlms_tamper_log_*` (index 0 = oldest, `BUFFER_EMPTY`) conflicting with `test_stress_soak` (index 0 = newest, `UNAUTHORIZED`). | Induces corrupt tamper state on first boot and causes duplicate symbol conflicts or semantic mismatches when linking against Pilar E2 DLMS modules. |
| **18** | **UTC vs. WIB timezone offset in ToU/EoB & unverified stack bounds** | Table 3.38 (Tamper 1 KB, UI 2 KB), Section 3.1.2 (ToU/EoB) | Tariff zones (WBP 17:00–22:00) and EoB billing freeze (10:00) are evaluated via `epoch / 3600 % 24` without UTC+7 offset. Tamper (1 KB stack) calls `printf` and SPI HAL routines; UI (2 KB stack) allocates `display_context_t`, `meas`, and `spln_frame` directly on the stack. | Tariff switching and EoB snapshots trigger 7 hours late unless epoch is adjusted to local time; Tamper and UI tasks risk fatal stack overflow during peak interrupt preemption. |

---

## 2. Additional Task Evaluation

### Task_NVM_Writer — Recommended (Architecturally Required)
* **Rationale:**  
  Resolves **Gap #9** (Blocking SPI2 operations) and **Gap #10** (Unprotected flash bus access), while establishing a single, dedicated storage serialization owner for **Gap #11** (paired with producer-side event deduplication so exactly one write request is queued per tamper event).  
  Isolating polling and busy-wait SPI2 transactions ($\approx 45\text{ ms}$ sector erase) into a dedicated, lowest-priority worker task:
  1. Completely eliminates long blocking storage delays from safety-critical tasks (`Task_Tamper_Emergency`), legal billing tasks (`Task_Metrology_Profiling`), and communication tasks (`Task_DLMS_Server`).
  2. Prevents artificial `fault_latched` trips within `wcet_profiler` during normal flash write cycles, safeguarding the external hardware watchdog feed (`wcet_watchdog_supervise_and_feed()`).
  3. Eliminates complex cross-task SPI2 mutex contention and unbounded priority inversion risks.
* **Prerequisite:**  
  Must be paired with a multi-sector append-only or ring checkpoint layout equipped with monotonic 32-bit sequence counters to ensure flash endurance reaches the $\ge 15$-year statutory lifespan and survives unnotified MCU resets (**Gap #12**).

---

## 3. Response-Time Analysis (RTA) & Schedulability

Because task priorities are assigned on a **Criticality-Monotonic** basis rather than strictly proportional to rate monotonic periods ($1/T_i$), classical Liu–Layland utilization bounds are insufficient. Deterministic schedulability must be formally evaluated using **Fixed-Priority Response-Time Analysis (RTA)**:

$$R_i = C_i + B_i + \sum_{j \in hp(i)} \left\lceil \frac{R_i}{T_j} \right\rceil C_j \quad \text{dengan syarat feasibility:} \quad R_i \le D_i$$

Di mana:
* $R_i$ = Waktu respons kasus terburuk (*Worst-Case Response Time*) dari tugas $i$.
* $C_i$ = Waktu eksekusi kasus terburuk (*WCET*) tugas $i$.
* $B_i$ = Waktu pemblokiran maksimum akibat penguncian sumber daya bersama (*Blocking Time*).
* $hp(i)$ = Himpunan semua tugas yang memiliki prioritas lebih tinggi daripada tugas $i$.
* $T_j$ = Periode kedatangan tugas $j$.
* $D_i$ = Tenggat waktu penyelesaian (*Deadline*) tugas $i$ ($D_i = T_i$).

---

### A. Current Baseline (4 Tasks + Inline Flash Erase)

Mengacu pada anggaran waktu eksekusi (*WCET design budgets*) pada Tabel 3.39:
* $C_{\text{Tamper}} = 1{,}85\text{ ms}$ ($T_1 = 100\text{ ms}$, Prio 32)
* $C_{\text{Metrology}} = 14{,}20\text{ ms}$ ($T_2 = 1000\text{ ms}$, Prio 28)
* $C_{\text{DLMS}} = 6{,}40\text{ ms}$ ($T_3 = 50\text{ ms}$, Prio 24)
* $C_{\text{UI}} = 18{,}50\text{ ms}$ ($T_4 = 500\text{ ms}$, Prio 16)
* Latensi tipikal penghapusan sektor NOR Flash: $t_{\text{erase}} \approx 45{,}00\text{ ms}$ (Tabel 3.55)

#### DLMS Worst-Case Response-Time Estimate ($T = D = 50\text{ ms}$, Prioritas 24)
Ketika operasi *sector erase* ($45\text{ ms}$) dieksekusi secara *inline* di dalam alur kerja `Task_Metrology_Profiling` (Prioritas 28) bersamaan dengan satu kali aktivasi `Task_Tamper_Emergency` (Prioritas 32):

$$R_{\text{DLMS}} = C_{\text{DLMS}} + C_{\text{Tamper}} + (C_{\text{Metrology}} + t_{\text{erase}})$$

$$R_{\text{DLMS}} = 6{,}40 + 1{,}85 + (14{,}20 + 45{,}00) = \mathbf{67{,}45\text{ ms}}$$

$$\mathbf{R_{\text{DLMS}} = 67{,}45\text{ ms} > D_{\text{DLMS}} (50\text{ ms}) \quad \longrightarrow \quad \text{\textbf{PELANGGARAN TENGGAT WAKTU (DEADLINE MISS)!}}}$$

> [!WARNING]
> Ini merupakan estimasi analitis berbasis anggaran (*budget-based analytical estimate*). Hal ini membuktikan secara matematis bahwa eksekusi *flash erase* secara sinkron di dalam tugas berprioritas tinggi menyebabkan pelanggaran tenggat waktu pada `Task_DLMS_Server`.

*Catatan Tambahan — Utilisasi CPU Dasar (Tanpa Erase):*
$$U = \frac{1{,}85}{100} + \frac{14{,}20}{1000} + \frac{6{,}40}{50} + \frac{18{,}50}{500} = 0{,}0185 + 0{,}0142 + 0{,}1280 + 0{,}0370 = \mathbf{19{,}77\%}$$
*(Nilai ini disajikan murni sebagai referensi utilisasi nominal; utilisasi komputasi tidak dapat digunakan sebagai bukti keterjadwalan tunggal pada sistem non-RMS).*

---

### B. Proposed 5-Task Architecture (Penambahan Task_NVM_Writer Prioritas Terendah)

Dengan mengalihkan seluruh operasi baca/tulis/hapus Flash ke `Task_NVM_Writer` (Prioritas 12, di bawah UI; *overhead* `xQueueSend` asinkron non-blocking $\approx 0{,}05\text{ ms}$):

| Prioritas | Nama Tugas | Karakteristik Aktivitas / Periode $T_i$ | Deadline $D_i$ | Budget $C_i$ (Estimasi) | $B_i$ | Waktu Respons $R_i$ (Awal) | Status Keterjadwalan |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **32 (Tertinggi)** | `Task_Tamper` | *Event-driven* ($T_{\min} = 100\text{ ms}$) | $100\text{ ms}$ | $1{,}90\text{ ms}$ | $0{,}00\text{ ms}$ *(TBD)* | **$1{,}90\text{ ms}$** | **Memenuhi ($R_i \le D_i$)** |
| **28** | `Task_Metrology` | Periodik: $1000\text{ ms}$ | $1000\text{ ms}$ | $14{,}25\text{ ms}$ | $0{,}00\text{ ms}$ *(TBD)* | **$16{,}15\text{ ms}$** | **Memenuhi ($R_i \le D_i$)** |
| **24** | `Task_DLMS` | Periodik: $50\text{ ms}$ | $50\text{ ms}$ | $6{,}45\text{ ms}$ | $0{,}00\text{ ms}$ *(TBD)* | **$22{,}60\text{ ms}$** | **Memenuhi ($R_i \le D_i$)** |
| **16** | `Task_UI` | *Polling:* $100\text{ ms}$; *Render:* $5000\text{ ms}$ / *event* | $100\text{ ms}$ *(poll)* | $18{,}50\text{ ms}$ *(peak render)* | $0{,}00\text{ ms}$ *(TBD)* | **$41{,}10\text{ ms}$** | **Memenuhi ($R_i \le D_i$)** |
| **12 (Terendah)** | `Task_NVM_Writer` | *Event-driven* (Antrean NVM) | $1000\text{ ms}$ *(provisional)* | $\approx 48{,}00\text{ ms}$ *(erase + write)* | $0{,}00\text{ ms}$ *(TBD)* | **$95{,}55\text{ ms}$** | **Memenuhi ($R_i \le D_i$)** |

#### Catatan Penurunan Rumus & Asumsi Analisis:
1. **`Task_Metrology`:**
   $$R_{\text{Metrology}} = C_{\text{Metrology}} + C_{\text{Tamper}} = 14{,}25 + 1{,}90 = \mathbf{16{,}15\text{ ms}} \le 1000\text{ ms}$$
2. **`Task_DLMS`:**
   $$R_{\text{DLMS}} = C_{\text{DLMS}} + C_{\text{Tamper}} + C_{\text{Metrology}} = 6{,}45 + 1{,}90 + 14{,}25 = \mathbf{22{,}60\text{ ms}} \le 50\text{ ms}$$
3. **`Task_UI` (Siklus Render Puncak):**
   $$R_{\text{UI}} = C_{\text{UI}} + C_{\text{Tamper}} + C_{\text{Metrology}} + \left\lceil \frac{34{,}65}{50} \right\rceil C_{\text{DLMS}} = 18{,}50 + 1{,}90 + 14{,}25 + 1 \times (6{,}45) = \mathbf{41{,}10\text{ ms}} \le 100\text{ ms}$$
   *(Pada pemodelan RTA presisi, siklus polling 100 ms dan perenderan 5000 ms disarankan dipartisi menjadi beban kerja terpisah).*
4. **`Task_NVM_Writer`:**
   Mengasumsikan skenario terburuk di mana 1 siklus render penuh UI ($18{,}50\text{ ms}$) terjadi selama jendela penulisan NVM:
   $$R_{\text{NVM}} = C_{\text{NVM}} + C_{\text{Tamper}} + C_{\text{Metrology}} + \left\lceil \frac{89{,}10}{50} \right\rceil C_{\text{DLMS}} + C_{\text{UI}}$$
   $$R_{\text{NVM}} = 48{,}00 + 1{,}90 + 14{,}25 + 2 \times (6{,}45) + 18{,}50 = 48{,}00 + 1{,}90 + 14{,}25 + 12{,}90 + 18{,}50 = \mathbf{95{,}55\text{ ms}} \le 1000\text{ ms}$$

---

### C. Parameter yang Perlu Diverifikasi Lebih Lanjut pada Target Hardware
1. **Validasi Waktu Eksekusi $C_{\text{NVM}}$:** Nilai $\approx 48{,}00\text{ ms}$ adalah estimasi analitis berdasarkan durasi tipikal penghapusan sektor ($45\text{ ms}$) ditambah pemograman halaman ($3\text{ ms}$). Nilai aktual harus diuji terhadap variasi proses fabrikasi flash chip dan kondisi batas tegangan.
2. **Spesifikasi Formal Tenggat Waktu $D_{\text{NVM}}$:** Nilai $1000\text{ ms}$ saat ini merupakan batas provisial; tenggat waktu resmi harus disesuaikan dengan kebutuhan ketahanan pemadaman jala-jala (*power-down last-gasp backup window* superkapasitor).
3. **Waktu Pemblokiran Kritis ($B_i$):** Nilai $B_i$ sementara diasumsikan $0{,}00\text{ ms}$ dan wajib divalidasi berdasarkan durasi *critical section* (`taskENTER_CRITICAL()`) serta waktu penahanan mutex bus SPI pada kode produksi.
4. **Langkah Verifikasi Selanjutnya:**
   * Lakukan instrumentasi penghitung siklus *Data Watchpoint and Trace* (DWT) ARM Cortex-M33 pada target perangkat keras riil guna mencatat waktu eksekusi murni $C_i$ (bebas dari preemption).
   * Ukur penggunaan memori *stack* menggunakan `uxTaskGetStackHighWaterMark()` pada seluruh tugas untuk menjamin ketahanan sistem terhadap *stack overflow*.
