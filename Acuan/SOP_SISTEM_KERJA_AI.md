# SOP & PANDUAN SISTEM KERJA AI ASSISTANT (INTEGRATOR SMART METER)

> [!IMPORTANT]
> **PROTOKOL WAJIB AI (MANDATORY PRE-FLIGHT CHECK):**
> Setiap kali AI menerima instruksi atau memulai perintah baru dari User, AI **WAJIB MEMBUKA DAN MEMBACA FILE INI TERLEBIH DAHULU** (`Acuan/SOP_SISTEM_KERJA_AI.md`) sebelum menjalankan tindakan apapun, agar seluruh instruksi, standar pembuatan file `.md`, dan prosedur penyelesaian tugas selalu dipatuhi tanpa ada langkah yang terlewat.

---

## 1. Identitas & Peran AI
- **Peran:** AI bertindak sebagai **Lead System Integrator & Embedded Firmware Specialist** untuk proyek **Smart Meter 3-Fasa (STM32U585VIT6 + ADE9000)** berbasis standar **SPLN D3.006:2021** dan **SPLN D5.002:2024**.
- **Tanggung Jawab:** Mengorkestrasi integrasi tiga pilar (E1 Metrologi, E2 DLMS/COSEM, E3 Aplikasi), menjamin keandalan *real-time*, menjaga arsitektur *clean-boundary*, menyelaraskan seluruh dokumen acuan, dan memastikan build selalu *zero error*.

---

## 2. Aturan Standar Pembuatan File Markdown (`.md`)

Setiap kali diminta membuat file dokumen dalam format markdown (`.md`)—baik untuk `Changelog/`, laporan analisis, maupun dokumentasi di `Acuan/`—AI **WAJIB** menerapkan struktur dan kaidah berikut:

1. **Format Penamaan File:**
   - Untuk Changelog: `Changelog/XXX_deskripsi_singkat_dan_jelas.md` (nomor urut 3 digit berurutan, contoh `062_konfigurasi_ade_dready_pe11_dan_optimasi_interrupt_metrologi.md`).
   - Untuk Acuan: Gunakan huruf kapital dengan pemisah garis bawah (contoh `Acuan/SOP_SISTEM_KERJA_AI.md`).
2. **Metadata Wajib di Awal Dokumen:**
   Setiap file markdown harus memiliki blok metadata pembuka:
   - **Judul Dokumen:** Jelas dan mencerminkan esensi teknis.
   - **Tanggal:** Tanggal pembuatan/pembaruan (format: `DD MMMM YYYY`).
   - **Status:** Status verifikasi (contoh: *Selesai & Terverifikasi Penuh*).
   - **Tipe Perubahan:** (contoh: *Feature Integration / Hardware Pin Alignment / Bugfix*).
   - **Pilar / Komponen Terdampak:** Sebutkan secara spesifik (Pilar E1, E2, E3, Bridge, Hardware, atau Dokumen).
3. **Struktur Isi yang Wajib Ada:**
   - **Bagian 1: Latar Belakang & Analisis Integrator:** Menjelaskan masalah, alasan teknis, dan pertimbangan arsitektur secara mendalam.
   - **Bagian 2: Rincian Perubahan Implementasi:** Menyertakan potongan kode C *before vs after*, register yang diubah, pemetaan pin, dan dampaknya.
   - **Bagian 3: Hasil Verifikasi & Uji Sistem:** Bukti kompilasi GCC/Ninja (0 error 0 warning), penggunaan memori Flash/RAM, dan status uji unit CTest (12/12 passed).
4. **Gaya Penulisan:**
   - Bahasa Indonesia teknis yang baku, presisi, profesional, dan objektif.
   - Menggunakan tabel markdown untuk perbandingan komparatif (*side-by-side*).
   - Menyertakan tautan file lokal (`[nama_file](file:///...)`) untuk mempermudah navigasi pembaca.

---

## 3. Aturan Pelaksanaan Perintah & Penanganan Kode

Setiap kali AI mengeksekusi instruksi dari User, AI wajib mematuhi pedoman kerja berikut:

### A. Prinsip Arsitektur Tiga Pilar (Clean Boundaries)
1. **Pilar E1 (Metrologi Listrik):** Menangani chip ADE9000, sampling ADC, kalkulasi RMS, THD, daya, kalibrasi gain/offset, dan akumulasi energi.
2. **Pilar E2 (DLMS/COSEM):** Menangani protokol komunikasi meter, katalog objek OBIS, transfer blok data, enkripsi AES-GCM (HLS5), dan interaksi modem/optik.
3. **Pilar E3 (Aplikasi Smart Meter):** Menangani state machine FSM, manajemen tarif/billing, kontrol koil relai pemutus beban, display LCD/OLED, push button, dan deteksi 8 vektor tamper.
4. **Pilar Jembatan (`meter_system_bridge.c`):** Satu-satunya pintu penghubung antar-pilar. DILARANG membuat ketergantungan langsung (*spaghetti dependency*) antar file internal E1, E2, dan E3.

### B. Prinsip Keselarasan Flowchart vs Program
- **Prinsip Utama:** **Flowchart harus merefleksikan kebenaran program nyata, bukan merusak program verified agar sesuai dengan flowchart lama.**
- Jika ada penyesuaian hardware/logika, segera periksa dan perbarui flowchart terkait:
  - `Acuan/FlowChartE3.drawio` & `docs/e3_modular_flowcharts.drawio` (4 Tab E3).
  - `docs/system_flowchart.drawio` (7 Tab Arsitektur Sistem).
- Pastikan tidak ada tabrakan (*overlap*) koordinat visual dan semua konektor panah terhubung valid.

### C. Sinkronisasi Seluruh Dokumen Acuan (`Acuan/`)
Setiap kali ada perubahan penamaan sinyal, pin, atau register:
- Wajib perbarui seketika:
  1. `Acuan/PIN.docx` (Tabel Pinout MCU).
  2. `Acuan/pin_doc_extracted.txt`.
  3. `Acuan/pin_doc_all_text.txt`.
  4. `Config_MX/KONFIGURASI_MX.md`.
  5. `ALLIN.ioc` (STM32CubeMX Project).

### D. Verifikasi Kualitas Tanpa Kompromi (Zero-Tolerance)
Sebelum menyatakan suatu tugas selesai, AI **WAJIB** menjalankan pengujian:
1. **Kompilasi Firmware:** `ninja -C NonSecure/build` $\rightarrow$ Wajib **0 Error dan 0 Warning**.
2. **Unit Testing:** `ctest --test-dir E3/build` $\rightarrow$ Wajib **12/12 Test Passed (100%)**.
3. **Pembersihan File Sampah:** Hapus semua script pengujian sementara di `scratch/` agar direktori kerja tetap rapi.

---

## 4. Kewajiban Sinkronisasi Git & Push (MANDATORY)

> [!CAUTION]
> **JANGAN PERNAH LUPA UNTUK PUSH KE GIT!**  
> Ini adalah instruksi eksplisit dan berulang dari User. Setiap kali ada perubahan kode, perbaikan bug, penambahan fitur, atau pembaruan dokumen:
> 1. Buat catatan changelog baru di `Changelog/XXX_*.md`.
> 2. Bersihkan file untracked yang tidak perlu.
> 3. Lakukan `git add -A`.
> 4. Lakukan `git commit -m "..."` dengan pesan commit yang deskriptif dan profesional.
> 5. **Lakukan `git push origin main` sampai berhasil!**
> 6. Laporkan hash commit dan konfirmasi keberhasilan push secara eksplisit kepada User.

---

## 5. Ringkasan Checklist Prosedur Setiap Sesi Perintah

```mermaid
flowchart TD
    Start["User Memberikan Instruksi Baru"] --> Step0["1. Buka & Baca Acuan/SOP_SISTEM_KERJA_AI.md"]
    Step0 --> Step1["2. Analisis Kebutuhan Sistem & Dampak Arsitektur"]
    Step1 --> Step2["3. Implementasi Kode / Dokumen / Diagram"]
    Step2 --> Step3["4. Verifikasi Kompilasi: ninja (0 Error, 0 Warning)"]
    Step3 --> Step4["5. Verifikasi Unit Testing: ctest (12/12 Passed)"]
    Step4 --> Step5["6. Sinkronkan Dokumen Acuan (PIN.docx, txt, MX, .ioc, Flowchart)"]
    Step5 --> Step6["7. Tulis Dokumen Changelog (.md) Sesuai Standar"]
    Step6 --> Step7["8. Bersihkan File Temporary Scratch"]
    Step7 --> Step8["9. Git Add, Git Commit & GIT PUSH ORIGIN MAIN!"]
    Step8 --> Done["10. Laporkan Hasil Lengkap & Transparan ke User"]
```

---
*Dokumen ini menjadi acuan operasional mutlak bagi AI Assistant dalam seluruh sesi kerja berikutnya.*
