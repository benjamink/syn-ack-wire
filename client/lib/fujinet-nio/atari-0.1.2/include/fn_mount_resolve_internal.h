#ifndef FN_MOUNT_RESOLVE_INTERNAL_H
#define FN_MOUNT_RESOLVE_INTERNAL_H

#include "fn_internal.h"

#define FN_FILEPROTO_VERSION 0x01

uint8_t fn_mount_resolve_validate_io(fn_mount_resolve_io_t *io, uint16_t min_capacity);
uint8_t fn_mount_resolve_call(fn_mount_resolve_io_t *io,
                              uint8_t command,
                              uint16_t request_len,
                              uint16_t *response_len);
uint8_t fn_mount_resolve_build_resolve(fn_mount_resolve_io_t *io,
                                       const char *host,
                                       const char *browse_path,
                                       const char *leaf,
                                       uint16_t *request_len);
uint8_t fn_mount_resolve_build_format(fn_mount_resolve_io_t *io,
                                      const char *canonical_uri,
                                      uint16_t *request_len);
uint8_t fn_mount_resolve_parse_response(const uint8_t *payload,
                                        uint16_t payload_len,
                                        char *canonical_uri,
                                        uint16_t canonical_cap,
                                        char *display_path,
                                        uint16_t display_cap,
                                        uint8_t *flags_out);

#endif
