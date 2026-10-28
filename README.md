# ALLIN: Firmware Smart Meter 3-Fasa STM32U585VIT6

Firmware meteran listrik pintar (Smart Electricity Meter) 3-fasa produksi terpadu yang memenuhi standar **SPLN D3.006:2021** (dan SPLN D3.006-1:2017) berbasis mikrokontroler **STM32U585VIT6 (ARM Cortex-M33)** dengan arsitektur keamanan **ARM TrustZone** dan **FreeRTOS v11 CMSIS-RTOS2**.

---

## 🏛️ Arsitektur Tiga Pilar (Three-Pillar Architecture)

Firmware ini dirancang dengan prinsip **Clean & Modular Decoupled Architecture**:

```
                                  [ STM32U585VIT6 HAL / RTOS ]
                                                │
                                                ▼
                                    [ meter_system_bridge ]
                                  (Glue Code / Jembatan Sistem)
                                 /              │              \
                                /               │               \
                               ▼                ▼                ▼
                     ┌──────────────────┐ ┌──────────────┐ ┌──────────────────────┐
                     │ PILAR E1         │ │ PILAR E2     │ │ PILAR E3             │
                     │ Metrologi ADE9000│ │ DLMS / COSEM │ │ Aplikasi Smart Meter │
                     │ 4-Kuadran Energi │ │ AMI Security │ │ HMI, Tamper & Relay  │
                     └──────────────────┘ └──────────────┘ └──────────────────────┘
```

1. **Pilar E1: Metrologi Listrik (`E1/`)**
   - Driver Analog Devices **ADE9000** via High-Speed SPI (10 MHz).
   - Pengukuran tegangan, arus, frekuensi, faktor daya, daya aktif/reaktif/semu per fasa (R, S, T) & total.
   - Akumulasi energi aktif & reaktif 4-kuadran impor/ekspor dengan resolusi milliJoule.

2. **Pilar E2: Komunikasi DLMS/COSEM (`E2/`)**
   - Lapisan HDLC & APDU standar IEC 62056 / Blue Book.
   - Dual port komunikasi: Port Optik Lokal (USART3 @ 9600 bps) & Port Modem Remote (USART2 @ 19200 bps).
   - Model objek COSEM (OBIS code) lengkap dengan otentikasi High-Level Security (HLS) & enkripsi AES-GCM-128.

3. **Pilar E3: Aplikasi Smart Meter, Proteksi & HMI (`E3/`)**
   - **HMI Carousel Display**: Auto-scroll (Tabel E.1) & manual scroll tombol (Tabel E.2). Mendukung layar OLED SSD1306 (I2C3) maupun Custom Segment LCD HT1621.
   - **Tamper Manager**: Deteksi 10 vektor sabotase (buka tutup terminal/meter, medan magnet, netral hilang/bypass, arus balik, reverse power) dengan 3 log FIFO terpisah (Tabel 18).
   - **Power Relay Manager (Breaker FSM)**: Kontrol kontaktor latching bistabil dengan proteksi tegangan rendah (<110V), tegangan lebih (>280V), arus lebih, dan rekoneksi tombol simultan (▲ + ▼).
   - **Load Profiling & Billing**: Perekaman data profile 15-menitan non-volatile (Billing Profile & Load Profile 1 & 2) dengan format A-XDR.
   - **Board Profile Configuration (`board_profile_config.h`)**: Abstraksi fleksibilitas konfigurasi display, relai, NVRAM, dan buzzer tanpa merusak kode inti.

4. **System Bridge (`meter_system_bridge`)**
   - Mengenkapsulasi seluruh callback interupsi hardware HAL STM32 (`HAL_UART_RxCpltCallback`, `HAL_GPIO_EXTI_Falling_Callback`, `HAL_GPIO_EXTI_Rising_Callback`).
   - Menjaga file autogenerasi STM32CubeMX (`main.c`) tetap bersih dan aman dari risiko tertimpa saat konfigurasi chip diperbarui.

---

## 🛠️ Panduan Build & Pengujian

### 1. Kompilasi Target ARM Cortex-M33 (Dual-Core TrustZone)

Pastikan toolchain `arm-none-eabi-gcc` dan `ninja` telah terinstal di sistem.

```bash
# Build Secure Target (ALLIN_S.elf)
ninja -C Secure/build

# Build Non-Secure Target (ALLIN_NS.elf)
ninja -C NonSecure/build
```

### 2. Pengujian Host (CTest - 9 Test Suites)

Pengujian logika unit dan integrasi dapat dijalankan langsung di mesin host tanpa ketergantungan perangkat keras fisik:

```bash
ctest --test-dir E3/build --output-on-failure
```

Daftar suite pengujian terverifikasi:
- `Integration_E3_Demo`
- `Unit_E3_Display_Tamper`
- `Unit_E3_Power_Relay`
- `Unit_E3_FSM_Statechart`
- `Unit_E3_Load_Profile`
- `Unit_E3_SPLN_Display_Tamper`
- `Stress_E3_Final_System_Audit`
- `Unit_E3_Console_CLI`
- `Unit_E3_Board_Profile`

---

## 📚 Dokumentasi & Riwayat Perubahan

- **Master Panduan File & Fundamental E3**: [E3/PANDUAN_ARSITEKTUR_DAN_FILE_E3.md](file:///g:/PPPP/ALLIN/E3/PANDUAN_ARSITEKTUR_DAN_FILE_E3.md)
- **Indeks Riwayat Perubahan Lengkap**: [Changelog/README.md](file:///g:/PPPP/ALLIN/Changelog/README.md) (LOG-001 hingga LOG-031)
