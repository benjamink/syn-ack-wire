#ifndef FN_MSDOS_H
#define FN_MSDOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FN_MSDOS_IOCTL_SIGNATURE "FUJI"
#define FN_MSDOS_IOCTL_VERSION   1
#define FN_MSDOS_IOCTL_MAX_DATA  512
#define FN_MSDOS_IOCTL_MAX_URI   255
#define FN_MSDOS_IOCTL_MAX_PATH  127
#define FN_MSDOS_IOCTL_MAX_UNITS 8
#define FN_MSDOS_F5_INT          0xF5
#define FN_MSDOS_F5_DETECT       0x0000
#define FN_MSDOS_F5_RESPONSE     0xF501
#define FN_MSDOS_F5_NIO_CALL     0xF502

enum {
    FN_MSDOS_IOCTL_QUERY = 0,
    FN_MSDOS_IOCTL_GET_STATE = 1,
    FN_MSDOS_IOCTL_SET_STATE = 2,
    FN_MSDOS_IOCTL_GET_UNIT_MAP = 3,
    FN_MSDOS_IOCTL_SET_UNIT_MAP = 4,
    FN_MSDOS_IOCTL_NIO_CALL = 5
};

typedef struct {
    uint8_t command;
    char signature[4];
    uint8_t unit;
    uint8_t version;
    uint8_t max_units;
} fn_msdos_ioctl_query_t;

typedef struct {
    uint8_t command;
    char signature[4];
    uint8_t unit;
    uint8_t version;
    uint8_t max_units;
    uint16_t current_uri_len;
    uint16_t display_path_len;
    char current_uri[FN_MSDOS_IOCTL_MAX_URI + 1];
    char display_path[FN_MSDOS_IOCTL_MAX_PATH + 1];
} fn_msdos_ioctl_state_t;

typedef struct {
    uint8_t command;
    char signature[4];
    uint8_t unit;
    uint8_t version;
    uint8_t max_units;
    uint8_t slot;
} fn_msdos_ioctl_unit_map_t;

typedef struct {
    uint8_t command;
    char signature[4];
    uint8_t unit;
    uint8_t version;
    uint8_t device;
    uint8_t nio_command;
    uint8_t nio_status;
    uint16_t request_len;
    uint16_t response_len;
    uint8_t data[FN_MSDOS_IOCTL_MAX_DATA];
    uint8_t diag_error;
    uint8_t diag_status;
    uint16_t diag_rx_len;
    uint16_t diag_expected_len;
    uint8_t diag_lsr;
} fn_msdos_ioctl_nio_call_t;

void fn_msdos_serial_set_com(uint8_t com_number);
void fn_msdos_ioctl_set_drive(uint8_t drive);
uint8_t fn_msdos_ioctl_find_drive(void);
uint16_t fn_msdos_ioctl_last_error(void);
uint8_t fn_msdos_ioctl_last_detail(void);
uint8_t fn_msdos_ioctl_last_nio_status(void);
uint8_t fn_msdos_ioctl_last_device(void);
uint8_t fn_msdos_ioctl_last_command(void);
uint16_t fn_msdos_ioctl_last_response_len(void);
uint8_t fn_msdos_ioctl_last_diag_error(void);
uint8_t fn_msdos_ioctl_last_diag_status(void);
uint16_t fn_msdos_ioctl_last_diag_rx_len(void);
uint16_t fn_msdos_ioctl_last_diag_expected_len(void);
uint8_t fn_msdos_ioctl_last_diag_lsr(void);

enum {
    FN_MSDOS_IOCTL_DETAIL_NONE = 0,
    FN_MSDOS_IOCTL_DETAIL_DOS_ERROR = 1,
    FN_MSDOS_IOCTL_DETAIL_BAD_SIGNATURE = 2,
    FN_MSDOS_IOCTL_DETAIL_BAD_RESPONSE_LEN = 3,
    FN_MSDOS_IOCTL_DETAIL_NIO_STATUS = 4
};

#ifdef __cplusplus
}
#endif

#endif /* FN_MSDOS_H */
