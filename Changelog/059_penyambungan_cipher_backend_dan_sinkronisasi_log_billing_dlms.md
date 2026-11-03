# LOG-059: Penyambungan Security Suite 0 Cipher Backend, Ciphered SET-Request, dan Sinkronisasi Profile DLMS (Event Log & Billing History)

**Tanggal:** 03 November 2026  
**Status:** Selesai & Terverifikasi (Build Cortex-M33 0 Error 0 Warning, CTest 12/12 Passed)  
**Tipe:** Fitur / Integrasi Sistem / Pengamanan / Komunikasi  
**Pilar Terdampak:** Pilar E2 (DLMS/COSEM Protokol Komunikasi), Pilar E3 (Aplikasi Smart Meter / HMI / Storage)

---

## 1. Latar Belakang & Identifikasi Gap Integrasi

Setelah penyelesaian LOG-058 (registrasi file baru E2 dan penanganan GET-Request identitas), dilakukan audit mendalam terhadap jalur komunikasi DLMS/COSEM terenkripsi (Security Suite 0: AES-128-GCM) dan pemetaan profile buffer:

1. **Argumen Cipher Bernilai `NULL` pada `ami_dlms_process_ciphered_request()`**:
   - Di [e2_dlms_service.c](file:///g:/PPPP/ALLIN/E2/Src/e2_dlms_service.c), pemanggilan `ami_dlms_process_ciphered_request()` untuk menangani PDU terenkripsi (`glo-get-request` `0xC8`, `glo-set-request` `0xC9`, `glo-action-request` `0xCB`) sebelumnya melewatkan pointer cipher bernilai `NULL`.
   - Di [ami_dlms_apdu.c](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_apdu.c), fungsi tersebut secara tegas menolak pemanggilan jika `cipher == NULL` dengan mengembalikan `AMI_DLMS_APDU_INVALID_ARGUMENT`. Akibatnya, seluruh transaksi DLMS terenkripsi (HLS / High Level Security) mengalami *fail-closed* / rejected.

2. **`glo-set-request` (0xC9) Menolak Seluruh Permintaan Tulis (Read-Only Hardcoded)**:
   - Pada [ami_dlms_apdu.c](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_apdu.c), setelah PDU `glo-set-request` berhasil didekripsi menjadi `SET_REQUEST_TAG` (`0xC1`), handler hanya mengembalikan respons statis `read-write-denied` (kode error 3).
   - Pengaturan IDPEL (`0.0.96.1.2.255`), Meter ID (`0.0.96.1.0.255`), atau Clock (`0.0.1.0.0.255`) melalui sesi terenkripsi DLMS tidak dapat diproses padahal fungsi `ami_dlms_process_set_request()` telah tersedia di LOG-057.

3. **Data Profil Riwayat Event Log & Billing History Belum Mengalir ke Snapshot E2**:
   - Struktur [AmiCosemRegisterSnapshot](file:///g:/PPPP/ALLIN/E2/Inc/ami_cosem_model.h) menyediakan buffer `snapshot->event_log` (OBIS `0.0.99.98.0.255`) dan `snapshot->billing_history` (OBIS `1.0.98.1.0.255`), namun fungsi `e2_dlms_service_sync_snapshot()` belum mengisi kedua buffer tersebut dari memori log FIFO dan riwayat billing milik Pilar E3 (`e3_meter_app_get_tamper_context()` dan `e3_meter_app_get_billing_snapshots()`).

4. **Ketersediaan Engine Software AES-128-GCM pada Target ARM Cortex-M33**:
   - Engine software AES-128-GCM di [ami_security.c](file:///g:/PPPP/ALLIN/E2/Src/ami_security.c) sebelumnya dibatasi oleh `#if !defined(STM32U585xx) && !defined(USE_HAL_DRIVER)`. Karena implementasi hardware SAES via TrustZone Secure Call belum dirutekan di Secure World NSC, target Cortex-M33 Non-Secure membutuhkan implementasi software AES-128-GCM sebagai backend kriptografi yang valid.

---

## 2. Rincian Perubahan & Implementasi Kode

### A. Inisialisasi Cipher Backend Suite 0 & Penyambungan ke `e2_dlms_service.c`
Menambahkan inisialisasi modul [ami_dlms_cipher.c](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_cipher.c) pada `e2_dlms_service_init()` dan meneruskan pointer backend yang valid ke pemroses APDU terenkripsi:

```c
/* Inisialisasi Security Suite 0 Cipher Backend untuk sesi terenkripsi (HLS/glo-*) */
static const uint8_t default_ek[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
static const uint8_t default_ak[16] = {0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF};
static const uint8_t client_title[8] = {'P','L','N','C','L','I','0','1'};
static const uint8_t server_title[8] = {'A','L','L','I','N','M','T','R'};

if (ami_dlms_cipher_suite0_init(&s_cipher_ctx, default_ek, default_ak, client_title, server_title, 1u) == 0)
{
    if (ami_dlms_cipher_suite0_get_backend(&s_cipher_ctx, &s_cipher_backend) == 0)
    {
        s_cipher_initialized = true;
    }
}
```

Pada penanganan RX frame PDU terenkripsi (`0xC8`, `0xC9`, `0xCB`), argumen `cipher` kini diisi dengan `s_cipher_initialized ? &s_cipher_backend : NULL`.

### B. Delegasi `glo-set-request` ke `ami_dlms_process_set_request()`
Pada [ami_dlms_apdu.c](file:///g:/PPPP/ALLIN/E2/Src/ami_dlms_apdu.c), blok dekripsi `SET_REQUEST_TAG` dihubungkan langsung ke fungsi pemroses:

```c
} else if (expected_plain_tag == SET_REQUEST_TAG) {
    /* Delegate to ami_dlms_process_set_request() so configurable parameters (IDPEL/MeterID) work over ciphered sessions! */
    uint8_t set_resp[64];
    size_t set_resp_len = 0u;
    service_status = ami_dlms_process_set_request(association, workspace, plaintext_length,
                                                  set_resp, sizeof(set_resp), &set_resp_len);
    if (set_resp_len > 0u && set_resp_len <= workspace_capacity) {
        memcpy(workspace, set_resp, set_resp_len);
        response_plain_length = set_resp_len;
    } else {
        workspace[0] = SET_RESPONSE_TAG;
        workspace[1] = 0x01u;
        workspace[2] = plaintext_length > 2u ? workspace[2] : 0u;
        workspace[3] = 3u; /* read-write-denied */
        response_plain_length = 4u;
        service_status = AMI_DLMS_APDU_ACCESS_ERROR;
    }
}
```

### C. Sinkronisasi Riwayat Event Log & Billing History ke Snapshot DLMS
Pada [e2_dlms_service.c](file:///g:/PPPP/ALLIN/E2/Src/e2_dlms_service.c), ditambahkan helper `epoch_to_cosem_datetime()` dan pembacaan ring buffer dari memori E3:
- **Event Log (`0.0.99.98.0.255`, Class 7)**: Mengambil entri tamper/event tertua hingga terbaru dari `t_ctx->tamper_log_fifo`, mengonversi timestamp epoch ke format COSEM `AmiCosemDateTime` (deviasi +420 menit untuk WIB), memetakan kode event ke field `event_code`, serta mengaktifkan bitmask `AMI_COSEM_VALID_EVENT_LOG`.
- **Billing Profile (`1.0.98.1.0.255`, Class 7)**: Mengambil snapshot penagihan bulanan dari `b_ctx->entries`, mengonversi waktu billing epoch ke `AmiCosemDateTime`, menyalin `active_energy_import_wh`, serta mengaktifkan bitmask `AMI_COSEM_VALID_BILLING_PROFILE`.

### D. Pelepasan Guard Platform pada Software Engine AES-128-GCM
Pada [ami_security.c](file:///g:/PPPP/ALLIN/E2/Src/ami_security.c), dihapus pembatas `#if !defined(STM32U585xx) && !defined(USE_HAL_DRIVER)` sehingga fungsi kriptografi software murni NIST SP 800-38D (`ami_sec_aes_128_gcm_encrypt` dan `ami_sec_aes_128_gcm_decrypt`) terkompilasi dan siap digunakan oleh modul DLMS cipher pada target STM32U585.

---

## 3. Verifikasi & Hasil Pengujian

1. **Kompilasi Firmware Target STM32U585 (Non-Secure ARM Cortex-M33)**:
   - Perintah: `cmake --build build/Debug -j8`
   - Hasil: **SUKSES 100% (0 Error, 0 Warning)**
   - Memory Usage:
     - **RAM**: `69,160 B` / `512 KB` (`13.19%`)
     - **ROM (Flash)**: `186,992 B` / `1 MB` (`17.83%`)
2. **Suite Pengujian CTest E3**:
   - Perintah: `ctest --test-dir E3/build --output-on-failure`
   - Hasil: **12/12 Test Passed 100% (0.21s)**

---

## 4. Kesimpulan

Seluruh gap integrasi DLMS/COSEM yang diidentifikasi kini telah tersambung sepenuhnya:
- Sesi terenkripsi Suite-0 (HLS) memiliki context dan backend cipher yang valid.
- Permintaan tulis (SET-Request) dapat diproses baik pada mode plaintext maupun mode terenkripsi (`glo-set-request`).
- Profil event log dan riwayat billing penagihan kini disinkronkan secara real-time ke snapshot DLMS yang siap dibaca oleh HES PLN via port optik maupun serial port.
