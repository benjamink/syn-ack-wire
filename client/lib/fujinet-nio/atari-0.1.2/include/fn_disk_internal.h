#ifndef FN_DISK_INTERNAL_H
#define FN_DISK_INTERNAL_H

#include <stdint.h>

#include "fn_internal.h"
#include "fn_protocol.h"
#include "fn_raw.h"
#include "fujinet-nio.h"

enum {
    FN_DISK_CMD_MOUNT = 0x01,
    FN_DISK_CMD_UNMOUNT = 0x02,
    FN_DISK_CMD_READ_SECTOR = 0x03,
    FN_DISK_CMD_WRITE_SECTOR = 0x04,
    FN_DISK_CMD_INFO = 0x05,
    FN_DISK_CMD_CLEAR_CHANGED = 0x06,
    FN_DISK_CMD_RESTORE_BOOT = 0x0A,
    FN_DISK_CMD_FLUSH = 0x0E,
    FN_DISK_CMD_INSPECT = 0x0F
};

enum {
    /* The fixed Info fields end after sectorCount. */
    FN_DISK_INFO_RESPONSE_SIZE = 12,
    /* Older peers omit lastError; newer peers may append it. */
    FN_DISK_INFO_RESPONSE_SIZE_WITH_LAST_ERROR = 13
};

/* DiskDevice calls are synchronous.  On cc65, avoid two more 512-byte static
 * arrays: use the opposite shared packet buffers so fn_raw_call() can build
 * its request in _fn_req_buf, receive into _fn_resp_buf, then copy the parsed
 * payload into _fn_req_buf.  The Disk application request originates in
 * _fn_resp_buf, so neither copy overlaps.  This preserves the full 512-byte
 * packet capacity while keeping public Disk APIs usable in Atari RAM. */
#if defined(__CC65__)
#define disk_request _fn_resp_buf
#define disk_reply _fn_req_buf
#endif

/* The DiskError of the last call (fn_disk_common.c). */
extern uint8_t fn_disk_error;

uint8_t fn_disk_status(uint8_t status);
uint8_t fn_disk_failure_error(const uint8_t *payload, uint16_t length);
uint8_t fn_disk_call(uint8_t command, const void *request,
                     uint16_t request_length, void *reply,
                     uint16_t reply_capacity, uint16_t *reply_length);
uint8_t fn_disk_parse_info(const uint8_t *reply, uint16_t length,
                           fn_disk_info_t *info);

uint16_t fn_disk_get_u16le(const uint8_t *p);
uint32_t fn_disk_get_u32le(const uint8_t *p);
void fn_disk_put_u16le(uint8_t *p, uint16_t value);
void fn_disk_put_u32le(uint8_t *p, uint32_t value);

#endif
