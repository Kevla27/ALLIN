# LOG-006: Perbaikan Presisi Waktu RTC, Akumulasi Energi Wh, dan Timer Pembatas Beban

- **Tanggal**: 27 Oktober 2026
- **Status**: Selesai & Terverifikasi (Build 0 Errors, 0 Warnings)

---

## 1. Analisis Mendalam & Masalah Kritis yang Ditemukan

Saat melakukan audit detail pada baris per baris matematika di fungsi `app_metering_step()`:

1. **Bug Pemotongan Integer Jam/Waktu (`now_epoch_sec`)**:
   - *Masalah*: Kode sebelumnya menggunakan `now_epoch_sec += (delta_ms / 1000);`. Karena interval task berjalan tiap `delta_ms = 100` ms, pembagian integer di bahasa C menghasilkan:
     $$\frac{100}{1000} = 0$$
   - *Dampak*: Nilai `now_epoch_sec` tidak pernah bertambah dan macet di nilai awal. Akibatnya, jam di layar tidak pernah berjalan dan perekaman berkala 15-Minute Load Profile (`now_epoch_sec - last_lp_record_sec >= 900`) tidak akan pernah terpicu!

2. **Bug Pemotongan Integer Akumulasi Energi (`active_energy_wh`)**:
   - *Masalah*: Rumus lama menggunakan:
     `meas.active_energy_wh += (meas.active_power_w * delta_ms) / 3600000ULL;`
     Pada daya terukur normal (misal $3.000\text{ W}$ sampai $10.000\text{ W}$):
     $$10.000 \times 100 = 1.000.000$$
     $$\frac{1.000.000}{3.600.000} = 0\text{ (integer division)}$$
   - *Dampak*: Untuk daya di bawah $36.000\text{ W}$, penambahan energi selalu bernilai **0 Wh** pada setiap tick 100 ms, sehingga meteran tidak akan pernah mencatat akumulasi pemakaian kWh konsumen!

3. **Laju Timer Pembatas Daya Terlalu Cepat (10x)**:
   - *Masalah*: Fungsi `power_relay_process_power` dipanggil setiap tick 100 ms. Di dalamnya terdapat counter `ctx->over_power_timer_sec++`.
   - *Dampak*: Jika batas toleransi overload diatur 5 detik, relai akan trip dalam waktu 5 tick (500 ms) karena counter bertambah 10 kali lebih cepat dari seharusnya.

---

## 2. Solusi & Perubahan Kode pada `NonSecure/Core/Src/main.c`

### A. Akumulator Presisi Milidetik & Detik
Menghitung `delta_ms` riil dari tick hardware STM32 (`HAL_GetTick()`) dan mengakumulasikan milidetik ke detik bulat:
```c
  static uint32_t last_step_tick_ms  = 0;
  static uint32_t s_ms_accumulator   = 0;
  static uint64_t s_energy_mj_accum  = 0;

  uint32_t current_tick_ms = HAL_GetTick();
  uint32_t delta_ms = (last_step_tick_ms == 0) ? 100 : (current_tick_ms - last_step_tick_ms);
  last_step_tick_ms = current_tick_ms;
  if (delta_ms == 0) delta_ms = 1;

  /* Akumulasi detik epoch secara presisi */
  s_ms_accumulator += delta_ms;
  bool is_one_sec_tick = false;
  if (s_ms_accumulator >= 1000)
  {
    uint32_t elapsed_sec = s_ms_accumulator / 1000;
    now_epoch_sec += elapsed_sec;
    s_ms_accumulator %= 1000;
    is_one_sec_tick = true;

    /* Update waktu ke display setiap detik */
    display_set_time(&disp_ctx, now_epoch_sec);
  }
```

### B. Akumulasi Energi Presisi Tinggi (MilliJoule / Watt-Millisecond)
Akumulasi dilakukan pada unit terkecil ($W \times \text{ms}$) tanpa ada sisa desimal yang terbuang. Setiap akumulator mencapai $3.600.000\text{ W}\cdot\text{ms}$ ($1\text{ Wh}$), register energi kWh bertambah 1:
```c
  /* Akumulasi energi aktif presisi tinggi (3.600.000 W*ms = 1 Wh) */
  s_energy_mj_accum += ((uint64_t)meas.active_power_w * (uint64_t)delta_ms);
  while (s_energy_mj_accum >= 3600000ULL)
  {
    meas.active_energy_wh++;
    s_energy_mj_accum -= 3600000ULL;
  }
```

### C. Sinkronisasi Pembatas Daya 1-Detik
Fungsi `power_relay_process_power` kini hanya dipanggil saat flag `is_one_sec_tick == true` (tepat 1 kali per detik):
```c
  /* D. Jalankan State Machine Pembatas Beban & Relai (PC8) */
  bool relay_closed_fb = (HAL_GPIO_ReadPin(RELAY_STATUS_GPIO_Port, RELAY_STATUS_Pin) == GPIO_PIN_SET);
  if (is_one_sec_tick)
  {
    power_relay_process_power(&relay_ctx, (int32_t)meas.active_power_w, now_epoch_sec);
  }
```

---

## 3. Hasil Kompilasi & Verifikasi
- **Status Build**: `ALLIN_S.elf` & `ALLIN_NS.elf` sukses dikompilasi dengan **0 Errors, 0 Warnings**.
- **Memory Consumption**:
  - `RAM`: 26040 Bytes (4.97%)
  - `Flash ROM`: 69896 Bytes (6.67%)
- **Hasil Fungsional**:
  - Jam dan tanggal epoch berjalan presisi 1 detik per detik nyata.
  - Akumulasi Wh dan kWh mencatat pemakaian daya berapapun secara akurat tanpa *underflow*.
  - Profil beban 15 menit tersimpan tepat setiap 900 detik.
  - Timer toleransi kelebihan daya (overload trip) menghitung tepat 5 detik sebelum memutuskan Shunt Trip.

