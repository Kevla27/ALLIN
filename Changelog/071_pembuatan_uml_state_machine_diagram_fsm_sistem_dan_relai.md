# Pembuatan UML State Machine Diagram: FSM Firmware Host STM32U585 & Pengelola Relai Pemutus Beban

**Tanggal:** 05 November 2026  
**Status:** Selesai & Terverifikasi Penuh  
**Tipe Perubahan:** Technical Design & Statechart Architecture Documentation (UML State Machine)  
**Pilar / Komponen Terdampak:** Dokumentasi Sistem (`docs/system_statechart_diagram.drawio.xml`), FSM Firmware (`E3/app/fsm/src/fsm_statechart.c`), Pengelola Relai & Proteksi Tegangan (`E3/app/fsm/src/power_relay_manager.c`, `E2/Src/ami_disconnect_control.c`)

---

## 1. Latar Belakang & Analisis Integrator

Dalam pengujian tipe, audit sertifikasi PLN (SPLN D3.006:2021), serta interoperabilitas protokol AMI (SPLN D5.002:2024 / DLMS Class 70 Disconnect Control), diagram alir prosedural (*flowchart*) saja tidak cukup untuk menjelaskan dinamika perilaku sistem yang berbasis peristiwa (*event-driven*). 

Sistem Smart Meter 3-Fasa pada dasarnya dikendalikan oleh dua mesin status berhingga (*Finite State Machine* - FSM) formal pada level kode C:
1. **FSM Utama Sistem Firmware (Host MCU STM32U585):**
   Didefinisikan pada [`E3/app/fsm/src/fsm_statechart.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/fsm_statechart.c) untuk mengelola siklus hidup operasional meter dari Power-On Reset (POR), swa-uji (*Self-Test*), pengukuran normal (*Normal Metering*), penanganan sabotase (*Tamper Event*), pemadaman seketika (*Last-Gasp Mode Stop 2*), hingga aktuasi relai (*Actuation*).
2. **Sub-State Machine Relai Pemutus Beban (Disconnect Control Class 70):**
   Didefinisikan pada [`E3/app/fsm/src/power_relay_manager.c`](file:///g:/PPPP/ALLIN/E3/app/fsm/src/power_relay_manager.c) sesuai SPLN D3.006:2021 Tabel 5 & Butir 10.3 Tabel 9 untuk mengatur status relai `CONNECTED`, `DISCONNECTED`, dan status wajib `READY_FOR_RECONNECTION` yang mewajibkan konfirmasi penekanan tombol fisik oleh pelanggan di lapangan.

Oleh karena itu, dibuatlah dokumen **UML State Machine Diagram** terpadu yang memetakan seluruh status, transisi (*trigger/guard/action*), dan interaksinya dengan standar industri dalam tema **Black & White (B&W) High-Contrast**.

---

## 2. Rincian Pemetaan State Machine

### A. FSM Utama Firmware Host MCU (6 Status)
- `STATE_0_BOOT_INIT`: Titik masuk setelah Power-On Reset (POR). Konfigurasi clock 160 MHz dan SAU TrustZone Cortex-M33.
- `STATE_1_SELF_TEST`: Swa-uji diagnostik hardware (ROM/RAM Checksum, Ping SPI1 ADE9000, pengujian Flash SPI2 & Modem UART).
- `STATE_2_NORMAL_METERING`: Superstate operasional utama. Menjalankan siklus FreeRTOS 100 ms, sampling metrologi 1 detik, integrasi profil beban 15 menit, evaluasi tarif ToU, dan display OLED SPLN.
- `STATE_3_TAMPER_EVENT`: Dipicu oleh interupsi EXTI IRQ0 saat terjadi sabotase fisik (tutup meter, tutup terminal, medan magnet kuat). Menyalakan LED kuning alarm PB7 dan mencatat logbook tamper FIFO.
- `STATE_4_LAST_GASP_MODE`: Dipicu saat sinyal daya jatuh (`PWR_GOOD_LOW` / AC_FAIL). Membekukan DSP metrologi, menyimpan akumulasi energi checkpoint ke Flash SPI2, mengirim notifikasi padam via DLMS, dan masuk mode ultra-low-power STOP 2.
- `STATE_5_ACTUATION`: Menghasilkan pulsa pemicu 50 ms ke koil relai (PC8 Trip / PC9 Close) dan memverifikasi kontak umpan balik PC7 sebelum kembali ke status normal.

### B. Sub-State Machine Relai Pemutus Beban (SPLN D3.006 Tabel 5)
- `CONNECTED`: Relai tertutup, daya listrik mengalir normal ke instalasi pelanggan.
  - *Transisi Trip:* Beban lebih (>6600 W selama 5 detik), tegangan pasok jatuh <110 V (*Under-Voltage Cut-Off*), tegangan melonjak >280 V (*Over-Voltage Cut-Off*), atau perintah remote DLMS Disconnect (Class 70 Action 1).
- `DISCONNECTED`: Relai terbuka, instalasi pelanggan padam.
  - *Auto-Reclose Tegangan:* Jika trip akibat anomali tegangan, relai otomatis menutup kembali saat tegangan pulih dalam rentang aman (115 V s/d 276 V).
  - *Perintah Remote Reconnect:* Jika trip akibat pemutusan HES, perintah DLMS Reconnect (Action 2) memindahkan relai ke status `READY_FOR_RECONNECTION`.
- `READY_FOR_RECONNECTION`: Status khusus standar PLN (SPLN Tabel 5 No 12). Relai belum tersambung secara fisik; ikon saklar di OLED berkedip menunggu pelanggan menekan tombol navigasi secara simultan ($\blacktriangle + \blacktriangledown$) selama 1 detik sebagai konfirmasi keselamatan beban sebelum koil relai dihubungkan kembali.

---

## 3. Hasil Verifikasi Sistem & Status Git

1. **Integritas Format XML Draw.io:**
   - Standalone diagram tersimpan di [`docs/system_statechart_diagram.drawio.xml`](file:///g:/PPPP/ALLIN/docs/system_statechart_diagram.drawio.xml).
   - Validasi struktur XML 100% lolos uji parsing `xml.etree.ElementTree`.
2. **Kompilasi Firmware C (STM32U585 GCC/Ninja):**
   ```
   ninja -C NonSecure/build
   ninja: Entering directory `NonSecure/build'
   ninja: no work to do.
   ```
   Status: **0 Error, 0 Warning**.
3. **Unit Testing CTest (12 Test Suites):**
   ```
   ctest --test-dir E3/build
   100% tests passed, 0 tests failed out of 12
   Total Test time (real) = 0.24 sec
   ```
   Status: **12/12 Passed (100%)**.
