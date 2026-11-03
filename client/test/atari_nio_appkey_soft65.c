#include <stdint.h>
#include <string.h>
#include "../src/platform.h"
#include "fn_raw.h"
#include "fn_protocol.h"
#include "fujinet-nio.h"

/* Keep these IDs in step with the descriptive cases in the soft65c02 script. */
enum {
    VALUE, RAW_ERROR, NOT_FOUND, SHORT_HEADER, WRONG_OFFSET, TOO_LONG,
    TRUNCATED_VALUE, MAX_VALUE, INIT_ERROR, HIGH_LENGTH, BAD_VERSION,
    KEY_ZERO, KEY_FF, EMPTY_VALUE
};
uint8_t rx_ring[256], test_case, test_seen, test_result;
static uint8_t calls, raw_bad;

uint8_t net_nio_init(void) { return test_case == INIT_ERROR ? FN_ERR_IO : 0; }

uint8_t fn_raw_call(uint8_t dev, uint8_t cmd, const void *payload, uint16_t length,
                    void *result, uint16_t capacity, fn_raw_response_t *response)
{
    static const uint8_t range[] = {0,0,0,0,64,0};
    const char *uri = test_case == KEY_ZERO ? "persist:///FujiNet/00010100.key" :
                      test_case == KEY_FF ? "persist:///FujiNet/000101ff.key" :
                                            "persist:///FujiNet/0001010b.key";
    const uint8_t *p = payload;
    uint8_t *out = result;
    uint8_t i, n = test_case == MAX_VALUE ? 64 : (test_case == EMPTY_VALUE ? 0 : 3);
    ++calls;
    if (dev != FN_DEVICE_FILE || cmd != FN_CMD_FILE_READ || p != rx_ring + 80 ||
        result != payload || length != 40 || capacity != 74 ||
        p[0] != 1 || p[1] != 31 || p[2] != 0 ||
        memcmp(p + 3, uri, 31) || memcmp(p + 34, range, 6)) raw_bad = 1;
    if (test_case == RAW_ERROR) return FN_ERR_IO;
    memset(out, 0, 74);
    out[0] = 1; out[8] = n;
    for (i = 0; i < n; ++i) out[10 + i] = 'A' + (i & 15);
    response->status = test_case == NOT_FOUND ? FN_ERR_NOT_FOUND : 0;
    response->payload_length = test_case == SHORT_HEADER ? 9 :
                               test_case == TRUNCATED_VALUE ? 12 : 10 + n;
    if (test_case == WRONG_OFFSET) out[4] = 1;
    if (test_case == TOO_LONG) {
        /* Synthetic malformed metadata: report 75 bytes despite capacity 74.
         * No byte past capacity is written. This isolates the 64-byte limit
         * from the separate truncated-reply check. */
        out[8] = 65;
        response->payload_length = 75;
    }
    if (test_case == HIGH_LENGTH) {
        out[9] = 1;
        response->payload_length = 269; /* full claimed length, not truncated */
    }
    if (test_case == BAD_VERSION) out[0] = 2;
    return FN_OK;
}

void test_func(void)
{
    uint8_t n, i;
    uint8_t key = test_case == KEY_ZERO ? 0 : (test_case == KEY_FF ? 255 : 11);
    uint8_t expected = test_case == MAX_VALUE ? 64 :
                       (test_case == VALUE || test_case == KEY_ZERO || test_case == KEY_FF ? 3 : 0);
    test_seen = test_case; test_result = 0xff;
    memset(rx_ring, 0xa5, sizeof(rx_ring));
    calls = raw_bad = 0;
    n = net_appkey(key);
    if (n != expected || raw_bad || calls != (test_case == INIT_ERROR ? 0 : 1) ||
        rx_ring[2 + n] != 0) { test_result = 1; return; }
    for (i = 0; i < n; ++i) if (rx_ring[2+i] != 'A' + (i & 15)) { test_result = 2; return; }
    /* Scratch starts at 80; every other byte in the output area is a guard. */
    if (rx_ring[0] != 0xa5 || rx_ring[1] != 0xa5) { test_result = 3; return; }
    for (i = 3 + n; i < 80; ++i) if (rx_ring[i] != 0xa5) { test_result = 3; return; }
    test_result = 0;
}
