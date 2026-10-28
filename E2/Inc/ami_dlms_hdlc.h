#ifndef AMI_DLMS_HDLC_H
#define AMI_DLMS_HDLC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define AMI_HDLC_FLAG 0x7Eu
#define AMI_HDLC_MAX_INFO_LENGTH 512u
#define AMI_HDLC_MAX_FRAME_BYTES 544u
#define AMI_HDLC_MAX_ADDRESS_LENGTH 4u
typedef enum { AMI_HDLC_INCOMPLETE=0, AMI_HDLC_FRAME_READY, AMI_HDLC_INVALID_ARGUMENT,
 AMI_HDLC_INVALID_FRAME, AMI_HDLC_INVALID_ADDRESS, AMI_HDLC_HCS_ERROR, AMI_HDLC_FCS_ERROR,
 AMI_HDLC_BUFFER_OVERFLOW, AMI_HDLC_BUFFER_TOO_SMALL, AMI_HDLC_INFO_TOO_LARGE,
 AMI_HDLC_UNSUPPORTED_CONTROL } AmiHdlcStatus;
typedef enum { AMI_HDLC_FRAME_I=0, AMI_HDLC_FRAME_SNRM, AMI_HDLC_FRAME_UA,
 AMI_HDLC_FRAME_DISC, AMI_HDLC_FRAME_RR, AMI_HDLC_FRAME_OTHER_CONTROL } AmiHdlcFrameType;
enum { AMI_HDLC_CONTROL_SNRM=0x93, AMI_HDLC_CONTROL_UA=0x73, AMI_HDLC_CONTROL_DISC=0x53, AMI_HDLC_CONTROL_RR=0x01 };
typedef struct { uint32_t destination_sap, source_sap; uint8_t control;
 AmiHdlcFrameType type; uint8_t payload[AMI_HDLC_MAX_INFO_LENGTH]; size_t payload_length; } HdlcFrame;
typedef struct { uint8_t frame_buffer[AMI_HDLC_MAX_FRAME_BYTES]; size_t frame_length;
 uint8_t collecting, overflow, tx_sequence, rx_sequence; } HdlcContext;
void ami_dlms_hdlc_init(HdlcContext *ctx);
uint16_t ami_dlms_hdlc_crc16(const uint8_t *data, size_t length);
AmiHdlcStatus ami_dlms_hdlc_parse_byte(HdlcContext *ctx,uint8_t byte,HdlcFrame *frame);
AmiHdlcStatus ami_dlms_hdlc_encode_frame(uint32_t destination_sap,uint32_t source_sap,
 uint8_t control,const uint8_t *payload,size_t payload_length,uint8_t *output,
 size_t output_capacity,size_t *output_length);
AmiHdlcStatus ami_dlms_hdlc_build_frame(HdlcContext *ctx,const HdlcFrame *request,
 const uint8_t *payload,size_t payload_length,uint8_t *output,size_t output_capacity,
 size_t *output_length);
#ifdef __cplusplus
}
#endif
#endif
