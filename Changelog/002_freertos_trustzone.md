# LOG-002: Integrasi FreeRTOS v11 & ARM Cortex-M33 TrustZone Secure Port

- **Tanggal**: 26 Oktober 2026
- **Status**: Selesai & Terverifikasi

---

## 1. Latar Belakang & Permasalahan
Ketika FreeRTOS diaktifkan melalui STM32CubeMX pada mikrokontroler STM32U585 yang menggunakan fitur **ARM TrustZone Security (Dual-Domain Secure & Non-Secure)**:
- Port FreeRTOS Cortex-M33 Non-Secure membutuhkan fungsi secure context management (`SecureContext_AllocateContext`, `SecureContext_SaveContext`, dll) yang dieksekusi di domain Secure.
- Awalnya terjadi *linker error: undefined reference to `SecureContext_AllocateContext`* saat mengompilasi `ALLIN_NS`.
- Selain itu, terjadi tabrakan header `main.h` antara domain Secure dan Non-Secure di `Secure/CMakeLists.txt`.

---

## 2. Rincian Perubahan Kode

### A. Penambahan File Port Secure FreeRTOS
Menyalin file port secure resmi FreeRTOS ARM_CM33 ke direktori:
`Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33/secure/`
- `secure_context.c` & `secure_context_port.c`: Mengatur alokasi FPU dan register stack per task di domain aman.
- `secure_heap.c`: Alokasi memori dinamis di domain aman.
- `secure_init.c`: Menurunkan prioritas exception Non-Secure (`SecureInit_DePrioritizeNSExceptions`).

### B. Perubahan `Secure/CMakeLists.txt`
1. Mendaftarkan file secure port ke dalam build target `ALLIN_S`:
   ```cmake
   target_sources(${CMAKE_PROJECT_NAME} PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33/secure/secure_context.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33/secure/secure_context_port.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33/secure/secure_heap.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33/secure/secure_init.c
   )
   ```
2. **Prioritas Include Directory**:
   Menempatkan `${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc` di baris pertama agar compiler menggunakan `main.h` milik Secure terlebih dahulu dan tidak tertimpa oleh `NonSecure/Core/Inc/main.h`.

### C. Konfigurasi Thread di `NonSecure/Core/Src/app_freertos.c`
1. Menaikkan ukuran stack `defaultTask` dari 128 words (512 bytes) menjadi **512 words (2048 bytes)** agar cukup untuk menampung buffer formatting display carousel dan log DLMS:
   ```c
   const osThreadAttr_t defaultTask_attributes = {
     .name = "defaultTask",
     .priority = (osPriority_t) osPriorityNormal,
     .stack_size = 512 * 4
   };
   ```
2. Menghubungkan fungsi eksekusi thread ke `app_metering_task_loop()`:
   ```c
   void StartDefaultTask(void *argument)
   {
     /* USER CODE BEGIN defaultTask */
     extern void app_metering_task_loop(void);
     app_metering_task_loop();
     /* USER CODE END defaultTask */
   }
   ```

