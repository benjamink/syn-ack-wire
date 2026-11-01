#ifndef FN_SLOT_CATALOG_INTERNAL_H
#define FN_SLOT_CATALOG_INTERNAL_H

#include "fn_internal.h"

enum {
    FN_SLOT_CATALOG_PROTOCOL_VERSION = 1
};

uint8_t fn_slot_catalog_validate_io(
    fn_slot_catalog_io_t *io, uint16_t min_capacity);
uint8_t fn_slot_catalog_call(
    fn_slot_catalog_io_t *io,
    uint8_t command,
    uint16_t request_len,
    uint16_t *response_len);
uint8_t fn_slot_catalog_parse_entry(
    fn_slot_catalog_io_t *io,
    uint16_t response_len,
    fn_slot_catalog_entry_t *out);

#endif
