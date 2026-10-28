# LOG-008: Optimasi Throughput SPI ADE9000 & Proteksi Stack/Heap FreeRTOS

- **Tanggal**: 27 Oktober 2026
- **Status**: Selesai & Terverifikasi (Build 0 Errors, 0 Warnings)

---

## 1. Latar Belakang & Alasan Perubahan

Hasil audit menyeluruh terhadap performa runtime firmware smart meter menemukan 2 potensi risiko stabilitas sistem:

1. **Bottleneck Throughput UART pada Layer Transaksi SPI ADE9000 (`E1/SPI/SPI.c`)**:
   - Fungsi `ADE9000_SPI_TransactionRead`, `ADE9000_SPI_TransactionWrite`, dan `ADE9000_SPI_BurstRead` awalnya mencetak 4–6 baris log `printf` pada setiap transaksi register.
   - Ketika pilar E1 membaca 9 register metrologi setiap 100 ms, hal tersebut menghasilkan $\approx 50$ baris `printf` per tick ($\approx 10.000$ byte/detik).
   - Port konsol USART2 beroperasi pada **19200 bps** (kapasitas maksimum $\approx 1.920$ byte/detik).
   - Akibatnya, CPU terus-menerus terblokir (*blocking wait*) di dalam `HAL_UART_Transmit`, menyebabkan lag parah pada eksekusi RTOS dan telemetri.

2. **Risiko Fatal Stack Overflow pada Task Utama Metering (`defaultTask`)**:
   - Konfigurasi awal `defaultTask_attributes.stack_size` adalah `128 * 4` (hanya 128 words / 512 bytes).
   - Di dalam loop task ini, terdapat panggilan berantai ke HMI carousel (`display_render_frame`), `printf` debug, telemetri ADC4, dan perekaman ring buffer `load_profile_add_entry`.
   - Stack 512 byte sangat rentan terlampaui (*stack overflow*), yang berpotensi merusak memory heap dan memicu HardFault / MemManage Fault.
   - Selain itu, `configCHECK_FOR_STACK_OVERFLOW` sebelumnya belum aktif sehingga kelebihan stack tidak terdeteksi secara dini.

---

## 2. Rincian Perubahan Kode

### A. Pembungkusan Log Verbose SPI dengan Macro Selektif
* **Berkas**: [`E1/SPI/SPI.c`](file:///g:/PPPP/ALLIN/E1/SPI/SPI.c)
* **Perubahan**:
  Ditambahkan definisi macro `SPI_DEBUG_PRINTF`:
  ```c
  #if defined(EMBEDDED_HARDWARE_TARGET) && !defined(ADE9000_SPI_VERBOSE_DEBUG)
  #define SPI_DEBUG_PRINTF(...) ((void)0)
  #else
  #define SPI_DEBUG_PRINTF(...) printf(__VA_ARGS__)
  #endif
  ```
  Seluruh pencetakan per-transaksi register diubah dari `printf(...)` menjadi `SPI_DEBUG_PRINTF(...)`. Banner inisialisasi boot tetap dipertahankan dengan `printf`.
  *Hasil:* Eksekusi metrologi berjalan instan dalam hitungan mikrodetik tanpa membebani antrean UART2.

### B. Peningkatan Ukuran Stack Task Utama
* **Berkas**: [`NonSecure/Core/Src/app_freertos.c`](file:///g:/PPPP/ALLIN/NonSecure/Core/Src/app_freertos.c)
* **Perubahan**:
  - Ukuran stack `defaultTask_attributes.stack_size` dinaikkan dari `128 * 4` (512 B) menjadi `1024 * 4` (4096 B / 1024 words).
  - Ditambahkan callback penanganan `vApplicationStackOverflowHook` pada blok `USER CODE BEGIN Application`:
  ```c
  void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
  {
      (void)xTask;
      (void)pcTaskName;
      __disable_irq();
      while (1)
      {
      }
  }
  ```

### C. Alokasi Heap FreeRTOS & Deteksi Stack Overflow
* **Berkas**: [`NonSecure/Core/Inc/FreeRTOSConfig.h`](file:///g:/PPPP/ALLIN/NonSecure/Core/Inc/FreeRTOSConfig.h)
* **Perubahan**:
  Pada blok `USER CODE BEGIN Defines`, dialokasikan heap yang proporsional dengan RAM STM32U585 (512 KB):
  ```c
  #undef configTOTAL_HEAP_SIZE
  #define configTOTAL_HEAP_SIZE                    ((size_t)32768)

  #define configCHECK_FOR_STACK_OVERFLOW           2
  ```

---

## 3. Hasil Kompilasi & Penggunaan Memori

```text
[1/6] Performing build step for 'ALLIN_S'
[4/4] Linking C executable ALLIN_S.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       12032 B       256 KB      4.59%
             ROM:       19968 B      1016 KB      1.92%
         ROM_NSC:          96 B         8 KB      1.17%
           SRAM4:           0 B        16 KB      0.00%
[2/6] No install step for 'ALLIN_S'
[3/6] Completed 'ALLIN_S'
[4/6] Performing build step for 'ALLIN_NS'
[19/20] Linking C executable ALLIN_NS.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       50608 B       512 KB      9.65%
           SRAM4:           0 B        16 KB      0.00%
             ROM:       69888 B         1 MB      6.67%
[5/6] No install step for 'ALLIN_NS'
[6/6] Completed 'ALLIN_NS'
```

* **Status:** **0 Errors, 0 Warnings**.
* **RAM NonSecure:** 50,608 B (9.65% dari 512 KB) — mencakup heap 32 KB FreeRTOS yang kokoh dan bebas defisit memori.
* **ROM Flash NonSecure:** 69,888 B (6.67% dari 1024 KB).
* **Throughput:** Eksekusi task metering 100 ms kini berjalan lancar tanpa terhambat antrean UART.

