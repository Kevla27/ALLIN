# Spesifikasi Variabel DLMS/COSEM Smart Meter 3-Fasa (ALLIN)
## Berdasarkan SPLN D3.002:2021 & IEC 62056-6-2

Dokumen ini mendefinisikan daftar variabel objek DLMS/COSEM yang didukung oleh firmware smart meter ALLIN, mencakup **Logical Name (OBIS Code)**, **Class ID**, **Tipe Data**, **Satuan & Skalar**, serta **Deskripsi Nilai (Value Source)**.

---

### Format Logical Name (OBIS)
Setiap objek COSEM diidentifikasi oleh 6 byte: `{A, B, C, D, E, F}`:
- **A**: Media (*1 = Listrik, 0 = Abstrak/General*)
- **B**: Kanal Pengukuran (*0 = Tidak ada kanal spesifik*)
- **C**: Besaran Fisik (*misal: 1 = Arus/Energi Aktif, 32 = Tegangan Fase R, 96 = Layanan/Identifikasi*)
- **D**: Tipe Pengukuran (*misal: 7 = Sesaat/Instantaneous, 8 = Akumulasi/Time-Integral, 1 = Penomoran*)
- **E**: Indeks Tarif / Fase (*misal: 0 = Total, 1 = Fase R/Tarif 1, 2 = Fase S/Tarif 2*)
- **F**: Periode / Billing (*255 = Nilai terkini / tanpa histori historis*)

---

## 1. Identifikasi & Konfigurasi Perangkat (Management & Device ID)

Objek-objek ini digunakan oleh sistem HES (*Head-End System*) dan teknisi PLN melalui Port Optik untuk identifikasi dan konfigurasi meter.

| No | Nama Variabel | Logical Name (OBIS) | Class ID | Tipe Data | Representasi Nilai (Value / Contoh) | Keterangan & Sumber Nilai |
|:---|:---|:---|:---:|:---|:---|:---|
| 1 | **Logical Device Name (LDN)** | `0.0.42.0.0.255` | 1 (*Data*) | `VisibleString` | `"STM32U585-AMI-01"` | Identitas unik perangkat COSEM |
| 2 | **Meter ID / Serial Number** | `0.0.96.1.0.255` | 1 (*Data*) | `VisibleString` | `"530000000001"` | Nomor seri meter (12 digit) dari `METER_CONFIG_METER_ID` |
| 3 | **ID Pelanggan (IDPEL)** | `0.0.96.1.2.255` | 1 (*Data*) | `VisibleString` | `"54321678901"` | ID Pelanggan PLN (11-12 digit) dari `METER_CONFIG_CUSTOMER_ID` |
| 4 | **Tipe Meter** | `0.0.96.1.1.255` | 1 (*Data*) | `VisibleString` | `"ALLIN-3P-4W"` | Tipe meter 3-fasa 4-kawat |
| 5 | **Versi Firmware (Software)**| `1.0.0.2.0.255` | 1 (*Data*) | `VisibleString` | `"v1.0.0-sprint1"` | Versi rilis firmware meter |
| 6 | **Versi Hardware** | `0.0.96.96.0.255` | 1 (*Data*) | `VisibleString` | `"HW-STM32U585-REV1"` | Revisi hardware PCB meter |
| 7 | **Unit PLN ID** | `0.0.96.1.3.255` | 1 (*Data*) | `VisibleString` | `"PLN-UID-JAYA"` | Kode unit PLN pengelola |
| 8 | **Mode Ekspor-Impor** | `1.0.96.99.0.255` | 1 (*Data*) | `Unsigned` | `1` (*Net-Metering*) / `0` (*Normal*) | Konfigurasi arah energi |

---

## 2. Pengukuran Listrik Sesaat (Instantaneous Electrical Values)

Diperbarui secara kontinu dari IC Metrologi ADE9000 (Pilar E1) ke Register Snapshot COSEM.

| No | Parameter | Logical Name (OBIS) | Class ID | Tipe Data | Scaler / Unit | Contoh Nilai Terbaca | Sumber Metrologi (E1 ADE9000) |
|:---|:---|:---|:---:|:---|:---:|:---|:---|
| 1 | **Tegangan Fase R ($V_R$)** | `1.0.32.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ V}$ (mV) | `220000` ($220.0\text{ V}$) | `snapshot->phase_voltage_millivolts[0]` |
| 2 | **Tegangan Fase S ($V_S$)** | `1.0.52.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ V}$ (mV) | `220000` ($220.0\text{ V}$) | `snapshot->phase_voltage_millivolts[1]` |
| 3 | **Tegangan Fase T ($V_T$)** | `1.0.72.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ V}$ (mV) | `220000` ($220.0\text{ V}$) | `snapshot->phase_voltage_millivolts[2]` |
| 4 | **Arus Fase R ($I_R$)** | `1.0.31.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ A}$ (mA) | `5000` ($5.000\text{ A}$) | `snapshot->phase_current_milliamps[0]` |
| 5 | **Arus Fase S ($I_S$)** | `1.0.51.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ A}$ (mA) | `5000` ($5.000\text{ A}$) | `snapshot->phase_current_milliamps[1]` |
| 6 | **Arus Fase T ($I_T$)** | `1.0.71.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ A}$ (mA) | `5000` ($5.000\text{ A}$) | `snapshot->phase_current_milliamps[2]` |
| 7 | **Arus Netral ($I_N$)** | `1.0.91.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ A}$ (mA) | `0` ($0.000\text{ A}$) | `sample->raw_value` |
| 8 | **Frekuensi Sistem ($f$)** | `1.0.14.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{-3}\text{ Hz}$ | `50000` ($50.000\text{ Hz}$) | ADE9000 Frequency Line Period |
| 9 | **Faktor Daya Total ($PF$)**| `1.0.13.7.0.255` | 3 (*Register*) | `DoubleLong` (signed) | $10^{-3}$ | `950` ($0.950$) | Total Power Factor |
| 10 | **Faktor Daya Fase R** | `1.0.33.7.0.255` | 3 (*Register*) | `DoubleLong` (signed) | $10^{-3}$ | `950` ($0.950$) | Phase R Power Factor |
| 11 | **Faktor Daya Fase S** | `1.0.53.7.0.255` | 3 (*Register*) | `DoubleLong` (signed) | $10^{-3}$ | `950` ($0.950$) | Phase S Power Factor |
| 12 | **Faktor Daya Fase T** | `1.0.73.7.0.255` | 3 (*Register*) | `DoubleLong` (signed) | $10^{-3}$ | `950` ($0.950$) | Phase T Power Factor |
| 13 | **Daya Aktif Total ($+P$)** | `1.0.1.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{0}\text{ W}$ (Watt) | `3300` ($3300\text{ W}$) | Active Power Import Total |
| 14 | **Daya Aktif Ekspor ($-P$)**| `1.0.2.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{0}\text{ W}$ (Watt) | `0` ($0\text{ W}$) | Active Power Export Total |
| 15 | **Daya Reaktif Total ($+Q$)**| `1.0.3.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{0}\text{ var}$ | `1084` ($1084\text{ var}$) | Reactive Power Import Total |
| 16 | **Daya Reaktif Total ($-Q$)**| `1.0.4.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{0}\text{ var}$ | `0` ($0\text{ var}$) | Reactive Power Export Total |
| 17 | **Daya Semu Total ($+S$)** | `1.0.9.7.0.255` | 3 (*Register*) | `DoubleLongUnsigned` | $10^{0}\text{ VA}$ | `3473` ($3473\text{ VA}$) | Apparent Power Import Total |

---

## 3. Akumulasi Energi (Energy Registers)

Register energi total dan per kuadran yang tersimpan secara kumulatif dan dicadangkan ke NVRAM Flash.

| No | Parameter | Logical Name (OBIS) | Class ID | Tipe Data | Scaler / Unit | Contoh Nilai | Keterangan |
|:---|:---|:---|:---:|:---|:---:|:---|:---|
| 1 | **Energi Aktif Impor ($+A$)** | `1.0.1.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ Wh}$ | `12543000` ($12543.0\text{ kWh}$) | Akumulasi energi aktif masuk |
| 2 | **Energi Aktif Ekspor ($-A$)** | `1.0.2.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ Wh}$ | `0` ($0.0\text{ kWh}$) | Akumulasi energi aktif kirim |
| 3 | **Energi Reaktif Impor ($+R$)**| `1.0.3.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ varh}$ | `4120000` ($4120.0\text{ kvarh}$) | Akumulasi energi reaktif masuk |
| 4 | **Energi Reaktif Ekspor ($-R$)**| `1.0.4.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ varh}$ | `0` ($0.0\text{ kvarh}$) | Akumulasi energi reaktif keluar |
| 5 | **Energi Reaktif Kuadran 1 ($Q1$)** | `1.0.5.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ varh}$ | `4120000` | Reaktif induktif impor ($+P, +Q$) |
| 6 | **Energi Reaktif Kuadran 2 ($Q2$)** | `1.0.6.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ varh}$ | `0` | Reaktif kapasitif ekspor ($-P, +Q$) |
| 7 | **Energi Reaktif Kuadran 3 ($Q3$)** | `1.0.7.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ varh}$ | `0` | Reaktif induktif ekspor ($-P, -Q$) |
| 8 | **Energi Reaktif Kuadran 4 ($Q4$)** | `1.0.8.8.0.255` | 3 (*Register*) | `Long64Unsigned` | $10^{0}\text{ varh}$ | `0` | Reaktif kapasitif impor ($+P, -Q$) |

---

## 4. Waktu, Profil Beban & Keamanan (Clock, Profiles & Security)

Objek sistem untuk sinkronisasi waktu, perekaman riwayat (*Load Profile & Event Log*), dan parameter keamanan DLMS.

| No | Nama Objek | Logical Name (OBIS) | Class ID | Tipe Data / Struktur | Contoh Nilai / Format | Keterangan |
|:---|:---|:---|:---:|:---|:---|:---|
| 1 | **COSEM Clock** | `0.0.1.0.0.255` | 8 (*Clock*) | `OctetString` (12 Byte) | `2026-11-03 11:35:00 (+07:00)` | Hardware RTC STM32U5 |
| 2 | **Event Log (Tamper/Alarm)** | `0.0.99.98.0.255` | 7 (*Profile Generic*) | Array of Structures | `{Clock, EventCode}` | 3 FIFO Log (@ 30 event SPLN Tabel 18) |
| 3 | **Billing Profile** | `1.0.98.1.0.255` | 7 (*Profile Generic*) | Array of Structures | `{Clock, ActiveImp, ActiveExp}` | Rekam penagihan bulanan (Billing) |
| 4 | **Load Profile 15-Menit** | `1.0.99.1.0.255` | 7 (*Profile Generic*) | Array of Structures | Buffer interval 15 menit (96 kanal/hari) | Profil riwayat beban |
| 5 | **Security Setup** | `0.0.43.0.0.255` | 64 (*Security Setup*) | Multi-Atribut | Policy: `0x03`, Suite: `0` (AES-GCM-128) | Manajemen kunci autentikasi & enkripsi |
| 6 | **Disconnector (Relai)** | `0.0.96.3.10.255` | 70 (*Disconnect Control*) | `Boolean` / `Enum` | `0` = Terbuka, `1` = Tertutup | Kontrol sakelar pemutus daya |
| 7 | **Status Register (Alarm)** | `0.0.96.10.4.255` | 1 (*Data*) | `DoubleLongUnsigned` | `0x00000000` (Bitmask Error/Tamper) | Register kode status error SPLN |

---

## 5. Ringkasan Hubungan E2 (DLMS/COSEM) dan E3 (Meter App)

```
       [ HES / Port Optik PLN ]
                  │
                  ▼ (Request OBIS)
     ┌────────────────────────┐
     │   Pilar E2: DLMS/COSEM │
     │  (ami_cosem_model.c)   │
     └───────────┬────────────┘
                 │
        (Update/Get Data)
                 │
                 ▼
     ┌────────────────────────┐
     │ Pilar E3: Meter App    │
     │  (e3_meter_app.c)      │
     ├────────────────────────┤
     │ - Display OLED / LCD   │ ◄── Menampilkan IDPEL, Meter ID, Tegangan, Arus, dll.
     │ - NVRAM Storage        │ ◄── Menyimpan IDPEL & Event Log ke SPI Flash
     │ - Tamper & Relay FSM   │ ◄── Menangani pembukaan relai dan log sabotase
     └────────────────────────┘
```

