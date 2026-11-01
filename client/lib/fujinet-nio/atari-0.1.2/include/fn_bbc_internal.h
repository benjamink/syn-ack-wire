#ifndef FN_BBC_INTERNAL_H
#define FN_BBC_INTERNAL_H

#include "fn_internal.h"

#define FN_BBC_REASON_JSON_QUERY    0x00
#define FN_BBC_REASON_SET_BODY_LEN  0x01
#define FN_BBC_REASON_WRITE_DATA    0x02
#define FN_BBC_REASON_CONTENT_TYPE  0x03
#define FN_BBC_REASON_SET_OPEN_URL  0x04
#define FN_BBC_REASON_SET_OPEN_FLAGS 0x05
#define FN_BBC_REASON_DEVICE_CALL   0x06

#define FN_BBC_STATUS_OK            0x00
#define FN_BBC_STATUS_BAD_CALL      0x01
#define FN_BBC_STATUS_JSON_FAILED   0x02
#define FN_BBC_STATUS_BAD_CHANNEL   0x03

#define FN_BBC_DIRECT_URL_MAX       127u
#define FN_BBC_OSWORD_STR_MAX       512u
#define FN_BBC_OPEN_READ            0x40
#define FN_BBC_OPEN_WRITE           0x80
#define FN_BBC_OPEN_UPDATE          0xC0

/* fn-rom BBC extension: OSBGET carry-set with A=$FE means temporary NotReady. */
#define FN_BBC_OSBGET_NOT_READY     0xFE

/* Shared 128-byte OSFIND name workspace. BBC network opens are synchronous. */
extern char fn_bbc_open_name[FN_BBC_DIRECT_URL_MAX + 1];

uint8_t __fastcall__ fn_bbc_osword78(uint8_t *block);
unsigned char __fastcall__ osfind(unsigned char mode, const char *name);
int __fastcall__ close_file(unsigned char channel);
int __fastcall__ fn_bbc_osbget(unsigned char channel);

uint8_t fn_bbc_status_to_result(uint8_t status);
uint8_t fn_bbc_probe_rom(void);
int fn_bbc_open_flags(uint8_t method);
uint8_t fn_bbc_arm_open_flags(uint8_t flags);
uint8_t fn_bbc_arm_open_url(const char *url, uint16_t len);
const char *fn_bbc_prepare_short_open_name(const char *url, uint16_t url_len);
uint8_t fn_bbc_claim_channel(fn_handle_t *handle, unsigned char channel);
uint8_t fn_bbc_device_call(uint8_t device,
                           uint8_t command,
                           const uint8_t *request,
                           uint16_t request_len,
                           uint8_t *response,
                           uint16_t response_capacity,
                           uint16_t *response_len);
uint8_t fn_bbc_device_call_raw(uint8_t device,
                               uint8_t command,
                               const uint8_t *request,
                               uint16_t request_len,
                               uint8_t *response,
                               uint16_t response_capacity,
                               uint8_t *device_status,
                               uint16_t *response_len);
uint8_t fn_bbc_file_call(uint8_t command,
                         const uint8_t *request,
                         uint16_t request_len,
                         uint8_t *response,
                         uint16_t response_capacity,
                         uint16_t *response_len);

#endif
