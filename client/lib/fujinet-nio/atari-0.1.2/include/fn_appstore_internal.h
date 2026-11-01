#ifndef FN_APPSTORE_INTERNAL_H
#define FN_APPSTORE_INTERNAL_H

#include "fn_internal.h"

enum {
    FN_APPSTORE_PROTOCOL_VERSION = 1,
    FN_APPSTORE_PREFIX_MAX = 1 + 2 + 255 + 2 + 255
};

uint8_t fn_appstore_validate_io(fn_appstore_io_t *io, uint16_t min_capacity);
uint8_t fn_appstore_call(fn_appstore_io_t *io,
                         uint8_t command,
                         uint16_t request_len,
                         uint16_t *response_len);
uint16_t fn_appstore_build_prefix(fn_appstore_io_t *io,
                                  const char *namespace_name,
                                  const char *key,
                                  uint8_t key_required);

#endif
