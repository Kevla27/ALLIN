# LOG-009: Penyempurnaan Rantai Fungsional HMI Tamper, Rekoneksi Relai, & Metrologi

- **Tanggal**: 27 Oktober 2026
- **Status**: Selesai & Terverifikasi (Build 0 Errors, 0 Warnings)

---

## 1. Latar Belakang & Alasan Perubahan

Dalam rangka memastikan kepatuhan 100% terhadap standar SPLN D3.006:2021 dan memastikan tidak ada mata rantai fungsional (*missing links*) yang terlewat, dilakukan audit logika interaksi antar-modul. Ditemukan 4 area integrasi yang perlu disempurnakan:

1. **Sinkronisasi Status Sabotase ke HMI Display (`display_set_tamper_status`)**:
   - Fungsi `tamper_process_signal` mendeteksi sabotase fisik (Case Open `PC13`, Terminal Cover `PE6`, Magnetic Tamper `PE4`), namun status bitmask aktifnya belum diteruskan ke objek tampilan `display_context`.
   - Akibatnya, Halaman 16 Carousel (`DISP_PAGE_TAMPER_ALARM`) yang memuat teks SPLN "PERIKSA"/"RUSAK" dan kode error (`ERR20`, `ERR25`) belum otomatis muncul di layar LCD.
   - **Solusi**: Memanggil `display_set_tamper_status(&s_disp_ctx, tamper_get_active_mask(&s_tamper_ctx))` di dalam loop siklis `e3_meter_app_step`.

2. **Integrasi Status READY ON & Aksi Konfirmasi Tombol Lokal**:
   - Berdasarkan SPLN D3.006:2021 Tabel 5 No. 12, relai pembatas pasca-perintah sambung kembali (*remote reconnect*) harus berada di status perantara `READY ON`. Layar meter wajib menampilkan status siap sambung dan relai hanya boleh menyambung fisik setelah tombol meter ditekan secara lokal oleh konsumen.
   - Sebelumnya, fungsi `display_set_relay_ready_on` dan `power_relay_confirm_reconnect` belum tersambung ke event penekanan tombol navigasi.
   - **Solusi**: 
     - Menghubungkan `display_set_relay_ready_on(&s_disp_ctx, power_relay_is_ready_on(&s_relay_ctx))` ke siklus HMI sehingga Halaman 17 Carousel aktif saat status `READY ON`.
     - Menambahkan verifikasi `power_relay_confirm_reconnect` di dalam callback tombol `e3_meter_app_on_button`.

3. **Proteksi Konversi Nilai Negatif Floating-Point pada Metrologi (`e1_metrology_service.c`)**:
   - Jika terjadi daya balik (*reverse power*) atau deviasi AFE ADE9000, nilai daya aktif $\sum P$ bernilai negatif.
   - Mengonversi *negative float* langsung ke tipe *unsigned integer* (`uint32_t`) merupakan *undefined behavior* pada standar C99/C11 dan dapat membungkus nilai menjadi $4$ miliar, yang berisiko merusak akumulasi Wh.
   - **Solusi**: Menambahkan penjaga batas `(total_p > 0.0f) ? (uint32_t)total_p : 0U` serta sanitasi tegangan dan arus.

4. **Kalibrasi Mandiri Konverter ADC4 (`HAL_ADCEx_Calibration_Start`)**:
   - Untuk menjamin presisi 12-bit SAR ADC4 dalam membaca ambang kritis tegangan baterai $\le 2,7\text{ V}$ (`ERR03`) dan superkapasitor $\le 2,0\text{ V}$ (`ERR06`), kalibrasi offset ADC4 dijalankan saat inisialisasi boot.

---

## 2. Rincian Perubahan Kode

### A. Pilar E1: [`E1/Metrology/e1_metrology_service.c`](file:///g:/PPPP/ALLIN/E1/Metrology/e1_metrology_service.c)
```c
/* C. Simpan ke struct pengukuran standar dengan proteksi nilai negatif */
meas->voltage_r_dvolts = (v_r > 0.0f) ? (uint16_t)(v_r * 10.0f) : 0U;
meas->voltage_s_dvolts = (v_s > 0.0f) ? (uint16_t)(v_s * 10.0f) : 0U;
meas->voltage_t_dvolts = (v_t > 0.0f) ? (uint16_t)(v_t * 10.0f) : 0U;

meas->current_r_mamps = (i_r > 0.0f) ? (uint32_t)(i_r * 1000.0f) : 0U;
meas->current_s_mamps = (i_s > 0.0f) ? (uint32_t)(i_s * 1000.0f) : 0U;
meas->current_t_mamps = (i_t > 0.0f) ? (uint32_t)(i_t * 1000.0f) : 0U;

float total_p = p_r + p_s + p_t;
meas->active_power_w    = (total_p > 0.0f) ? (uint32_t)total_p : 0U;
```

### B. Pilar E3: [`E3/app/e3_meter_app.c`](file:///g:/PPPP/ALLIN/E3/app/e3_meter_app.c)
```c
/* Kalibrasi ADC4 pada inisialisasi */
if (s_hadc_telemetry != NULL)
{
    HAL_ADCEx_Calibration_Start(s_hadc_telemetry, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

/* Sinkronisasi status alarm sabotase & status READY ON ke HMI Display */
display_set_tamper_status(&s_disp_ctx, tamper_get_active_mask(&s_tamper_ctx));
display_set_relay_ready_on(&s_disp_ctx, power_relay_is_ready_on(&s_relay_ctx));

/* Konfirmasi lokal penyambungan kembali relai saat tombol ditekan */
void e3_meter_app_on_button(button_dir_t dir)
{
    if (power_relay_is_ready_on(&s_relay_ctx))
    {
        power_relay_confirm_reconnect(&s_relay_ctx, s_now_epoch_sec);
        display_set_relay_ready_on(&s_disp_ctx, false);
    }
    display_handle_button_press(&s_disp_ctx, dir);
}
```

---

## 3. Hasil Kompilasi Firmware

```text
[1/6] Performing build step for 'ALLIN_S'
ninja: no work to do.
[2/6] No install step for 'ALLIN_S'
[3/6] Completed 'ALLIN_S'
[4/6] Performing build step for 'ALLIN_NS'
[1/3] Building C object CMakeFiles/ALLIN_NS.dir/G_/PPPP/ALLIN/E1/Metrology/e1_metrology_service.c.obj
[2/3] Building C object CMakeFiles/ALLIN_NS.dir/G_/PPPP/ALLIN/E3/app/e3_meter_app.c.obj
[3/3] Linking C executable ALLIN_NS.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       50608 B       512 KB      9.65%
           SRAM4:           0 B        16 KB      0.00%
             ROM:       71184 B         1 MB      6.79%
[5/6] No install step for 'ALLIN_NS'
[6/6] Completed 'ALLIN_NS'
```
* **Status:** **0 Errors, 0 Warnings**.

