#ifndef FN_RAW_H
#define FN_RAW_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CMOC_VERSION_
    #include <cmoc.h>
#else
    #include <stdint.h>
#endif

typedef struct {
    uint8_t status;
    uint16_t payload_length;
} fn_raw_response_t;

uint8_t fn_raw_call(uint8_t device,
                    uint8_t command,
                    const void *payload,
                    uint16_t payload_length,
                    void *reply,
                    uint16_t reply_capacity,
                    fn_raw_response_t *response);

#ifdef __cplusplus
}
#endif

#endif /* FN_RAW_H */
