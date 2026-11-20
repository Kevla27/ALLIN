# Smart Meter AMI 3-Fasa Pascabayar

## Product Overview (Gambaran Umum Produk)

Smart Meter AMI 3-Fasa Pascabayar adalah instrumen pengukuran energi listrik sambungan tidak langsung 3-fasa 4-kawat untuk kelas industri. Perangkat ini dirancang dengan arsitektur modular dua-chip yang memisahkan domain metrologi (menggunakan IC ADE9000) dan domain pemrosesan aplikasi utama (berbasis MCU Arm 32-bit STM32U585) untuk menjaga integritas keamanan metrologi legal. Instrumen ini dilengkapi dengan layar LCD STN 3-baris alfanumerik untuk pemantauan lokal, antarmuka kontrol bantu pemutus beban, serta kompartemen komunikasi modular yang memungkinkan telemetri nirkabel terenkripsi menuju Server HES PLN. Sistem catu dayanya menggunakan topologi Quasi-Resonant (QR) Flyback Terisolasi dengan dukungan energi cadangan superkapasitor dan baterai litium primer untuk menjamin kelangsungan operasi saat pemadaman.

## Problem (Masalah yang Diselesaikan)

- **Pencurian dan Sabotase Fisik:** Rentannya meteran listrik terhadap manipulasi kecurangan pencurian energi, seperti pembongkaran penutup meter mekanis dan interferensi fluks magnetik eksternal tingkat tinggi (hingga 500 mT).
- **Kehilangan Data dan Visibilitas Saat Pemadaman:** Risiko hilangnya register energi tagihan dan ketidakmampuan sistem untuk mengirimkan peringatan kepada pihak utilitas ketika catu daya jala-jala listrik terputus total.
- **Keamanan Siber & Pembaruan Sistem:** Kebutuhan infrastruktur utilitas untuk melakukan pertukaran data yang kebal dari penyadapan, serta kemampuan melakukan pembaruan perangkat lunak jarak jauh (Secure FOTA) tanpa merusak parameter kalibrasi metrologi.

## Goal (Tujuan)

- **Akurasi Metrologi Presisi:** Menghasilkan komputasi pengukuran energi aktif yang memenuhi Kelas 0.5S/0.2S (IEC 62053-22) dan energi reaktif Kelas 2 (IEC 62053-23), serta mengukur parameter kualitas daya dan distorsi harmonisa hingga orde ke-50.
- **Ketahanan & Keamanan Anti-Sabotase 24/7:** Mendeteksi anomali kelistrikan dan intrusi mekanis/magnetik secara otonom baik dalam kondisi bertegangan maupun padam, guna memicu pemutusan relai beban eksternal (shunt trip) dan merekam kejadian pada logbook tamper FIFO.
- **Konektivitas Telemetri Andal:** Menyediakan komunikasi jarak jauh dengan protokol DLMS/COSEM yang dienkripsi secara perangkat keras (AES-128/256-GCM) untuk mengirimkan data profil beban 15-menit, tarif Time-of-Use (ToU), dan transmisi alarm darurat pemadaman (last-gasp) selama minimal 5 detik yang ditopang oleh superkapasitor.

## Target Pengguna

Berdasarkan diagram konteks arsitektur sistem level 0, instrumen ini dirancang untuk berinteraksi dengan entitas pengguna berikut:

- **Pusat Data AMI (Server HES/MDMS PLN):** Entitas server pusat milik utilitas yang secara otonom mengumpulkan data penagihan, profil beban 90-hari, serta menerbitkan perintah kendali jarak jauh (seperti sinkronisasi waktu, disconnect control, dan Secure FOTA).
- **Teknisi Lapangan PLN & Otoritas Metrologi Legal (Ditmet):** Personel operasional yang bertugas melakukan konfigurasi lapangan, audit diagnostik menggunakan port optik inframerah lokal, serta menjalankan verifikasi kalibrasi metrologi legal melalui pembacaan kedipan pulsa LED (1.000 imp/kWh dan 1.000 imp/kvarh) di komparator meja tera.
- **Pelanggan Industri dan Manajer Energi Fasilitas:** Pengguna akhir (konsumen) yang memantau indeks konsumsi, arah aliran daya, dan kode alarm (seperti "RUSAK" atau "PERIKSA") melalui panel visual LCD, serta mengeksekusi interlok keselamatan penyambungan kembali beban secara fisik dengan menekan tombol navigasi pada meter.
