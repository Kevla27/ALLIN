/**
 * @file load_profile_dlms_adapter.h
 * @brief Adapter Pemformatan A-XDR DLMS COSEM Class ID 7 untuk E3
 *        Sesuai DLMS Green/Blue Book & SPLN D3.006: 2021 Butir 12.3
 */

#ifndef LOAD_PROFILE_DLMS_ADAPTER_H
#define LOAD_PROFILE_DLMS_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "load_profile.h"
#include "tamper_manager.h"

/* Tag Tipe Data A-XDR DLMS */
#define DLMS_AXDR_TAG_BOOLEAN       0x03
#define DLMS_AXDR_TAG_STRUCTURE     0x02
#define DLMS_AXDR_TAG_OCTET_STRING  0x09
#define DLMS_AXDR_TAG_DOUBLE_LONG_U 0x06
#define DLMS_AXDR_TAG_LONG_U        0x12
#define DLMS_AXDR_TAG_LONG          0x10
#define DLMS_AXDR_TAG_UNSIGNED      0x11
#define DLMS_AXDR_TAG_LONG64_U      0x15

/**
 * @brief Mengodekan timestamp Epoch UNIX ke format DLMS Date-Time 12-byte
 */
void dlms_encode_datetime(uint32_t epoch_sec, uint8_t *out_buf);

/**
 * @brief Mengodekan 1 entri Load Profile C menjadi A-XDR Structure DLMS (11 Elemen)
 *        [Timestamp, +A, -A, +R, V_R, V_S, V_T, I_R, I_S, I_T, Status_Mask]
 * @param entry Pointer ke entri load profile
 * @param out_buf Buffer tujuan penulisan A-XDR
 * @param max_len Kapasitas maksimum buffer (min 72 byte)
 * @return Jumlah byte yang di-encode, atau 0 bila error
 */
size_t load_profile_encode_dlms_entry(const load_profile_entry_t *entry, uint8_t *out_buf, size_t max_len);

/**
 * @brief Mengodekan 1 entri Tamper Log C menjadi A-XDR Structure DLMS (3 Elemen)
 *        [Timestamp, Event Vector, Active Mask]
 * @param entry Pointer ke entri tamper log legacy
 * @param out_buf Buffer tujuan penulisan A-XDR
 * @param max_len Kapasitas maksimum buffer (min 24 byte)
 * @return Jumlah byte yang di-encode, atau 0 bila error
 */
size_t tamper_log_encode_dlms_entry(const tamper_event_entry_t *entry, uint8_t *out_buf, size_t max_len);

/**
 * @brief Mengodekan 1 entri SPLN Standard Log C menjadi A-XDR Structure DLMS (4 Elemen)
 *        [Timestamp, Event Code, Status Flags, Parameter] (SPLN Butir 12.3)
 * @param entry Pointer ke entri log standar SPLN
 * @param out_buf Buffer tujuan penulisan A-XDR
 * @param max_len Kapasitas maksimum buffer (min 27 byte)
 * @return Jumlah byte yang di-encode, atau 0 bila error
 */
size_t spln_log_encode_dlms_entry(const spln_standard_log_entry_t *entry, uint8_t *out_buf, size_t max_len);

/**
 * @brief Parser validator format A-XDR Structure DLMS
 * @param buf Buffer stream A-XDR
 * @param len Panjang data stream
 * @param expected_elements Jumlah elemen struktur yang diharapkan
 * @return true bila struktur valid dan elemen cocok persis, false jika malformed/underflow
 */
bool dlms_decode_verify_structure(const uint8_t *buf, size_t len, uint8_t expected_elements);

#ifdef __cplusplus
}
#endif

#endif /* LOAD_PROFILE_DLMS_ADAPTER_H */
