/* Exercise the network adapter against the public NIO API, without a device. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/platform.h"
#include <fujinet-nio.h>
#include <fn_raw.h>
#include <fn_protocol.h>

unsigned char rx_ring[256], net_hush, nio_test_clock;
static unsigned char read_data[256], read_flags, read_result, proc, write_result;
static unsigned int read_len, write_calls, read_calls, last_read_cap;
static unsigned long seen_read_offset, seen_write_offset;
static char opened_url[FN_MAX_URL_LEN + 1];
static unsigned char sent[16], sent_len;
static unsigned char appkey_mode;
static unsigned char pbi, hsio_result, device_result, disk_result;
static unsigned char set_calls, device_calls, disk_calls, get_calls, get_result, speed = 4;
static unsigned char last_index;
void nio_after(void) {}
uint8_t fn_atari_u1mb_pbi(void) { return pbi; }
uint8_t fn_fuji_get_hsio(uint8_t *index) { ++get_calls; if (get_result) return get_result; *index = speed; return FN_OK; }
uint8_t fn_fuji_set_hsio(uint8_t index) { ++set_calls; last_index = index; return hsio_result; }
uint8_t fn_atari_u1mb_clear_devices(uint8_t mask) { assert(mask == 15); ++device_calls; return device_result; }
uint8_t fn_atari_u1mb_clear_disks(uint16_t mask) { assert(mask == 0x7fff); ++disk_calls; return disk_result; }
unsigned char net_nio_standard_sio(void);
unsigned char net_nio_restore_sio(void);
uint8_t fn_init(void) { return FN_OK; }
uint8_t fn_open(fn_handle_t *h, uint8_t method, const char *url, uint8_t flags)
{ assert(method == 0 && flags == FN_OPEN_KEEP); strcpy(opened_url, url); *h = 7; return 0; }
uint8_t fn_atari_proc_enable(void) { return 0; }
void fn_atari_proc_disable(void) {}
uint8_t fn_atari_proc_pending(void) { unsigned char p = proc; proc = 0; return p; }
uint8_t fn_close(fn_handle_t h) { assert(h == 7); return 0; }
uint8_t fn_write(fn_handle_t h, uint32_t off, const uint8_t *data, uint16_t len, uint16_t *n)
{ assert(h == 7); ++write_calls; seen_write_offset = off; *n = 0; if (write_result) return write_result;
  memcpy(sent, data, len); sent_len = len; *n = len; return 0; }
uint8_t fn_read(fn_handle_t h, uint32_t off, uint8_t *buf, uint16_t max, uint16_t *n, uint8_t *flags)
{ assert(h == 7); ++read_calls; seen_read_offset = off; last_read_cap = max; *n = 0; *flags = 0;
  if (read_result) return read_result;
  assert(read_len <= max); memcpy(buf, read_data, read_len);
  *n = read_len; *flags = read_flags; return 0; }
uint16_t fn_read_available(void) { return 0; }
uint8_t fn_raw_call(uint8_t device, uint8_t command, const void *payload, uint16_t length,
                    void *reply, uint16_t cap, fn_raw_response_t *response)
{
  unsigned char *out = reply;
  assert(device == FN_DEVICE_FILE && command == FN_CMD_FILE_READ && length == 40 && cap == 74);
  assert(payload == rx_ring + 80 && reply == payload);
  assert(!memcmp((const unsigned char *)payload + 3, "persist:///FujiNet/0001010b.key", 31));
  assert(!memcmp((const unsigned char *)payload + 34, "\0\0\0\0\x40\0", 6));
  memset(out, 0, 74); out[0] = 1; out[8] = 13;
  memcpy(out + 10, "TCP://host:9/", 13); response->status = 0; response->payload_length = 23;
  if (appkey_mode == 1) response->status = FN_ERR_NOT_FOUND;
  if (appkey_mode == 2) response->payload_length = 9;
  if (appkey_mode == 3) out[4] = 1;
  if (appkey_mode == 4) out[8] = 65;
  if (appkey_mode == 5) response->payload_length = 22;
  return 0;
}

int main(void)
{
    char long_url[FN_MAX_URL_LEN + 2];
    assert(net_nio_standard_sio() == FN_ERR_UNSUPPORTED && !set_calls && !device_calls && !disk_calls);
    pbi = 1; get_result = FN_ERR_UNSUPPORTED;
    assert(net_nio_standard_sio() == FN_ERR_UNSUPPORTED && !set_calls);
    get_result = 0; hsio_result = FN_ERR_UNSUPPORTED;
    assert(net_nio_standard_sio() == FN_ERR_UNSUPPORTED && set_calls == 1 && !device_calls && !disk_calls);
    hsio_result = 0; device_result = FN_ERR_IO;
    assert(net_nio_standard_sio() == FN_ERR_IO && device_calls == 1 && !disk_calls);
    device_result = 0; disk_result = FN_ERR_IO;
    assert(net_nio_standard_sio() == FN_ERR_IO && disk_calls == 1);
    disk_result = 0;
    assert(net_nio_standard_sio() == FN_OK && disk_calls == 2);
    assert(last_index == FN_FUJI_HSIO_OFF && get_calls == 2);
    assert(net_nio_standard_sio() == FN_OK && set_calls == 4);
    speed = 9; /* later external changes do not overwrite the saved value */
    hsio_result = FN_ERR_IO;
    assert(net_nio_restore_sio() == FN_ERR_IO && last_index == 4);
    hsio_result = 0;
    assert(net_nio_restore_sio() == FN_OK && last_index == 4);
    assert(net_nio_restore_sio() == FN_OK && set_calls == 6);
    /* The first real init captures and disables once, before the AppKey. */
    pbi = 1; speed = 4;
    for (appkey_mode = 1; appkey_mode <= 5; ++appkey_mode)
        assert(net_appkey(11) == 0 && rx_ring[2] == 0);
    assert(set_calls == 7 && device_calls == 5 && disk_calls == 4 && get_calls == 3);
    appkey_mode = 0;
    assert(net_appkey(11) == 13 && !strcmp((char *)rx_ring + 2, "TCP://host:9/"));
    rx_ring[0] = 'N'; rx_ring[1] = ':';
    assert(net_open((char *)rx_ring) == 0 && !strcmp(opened_url, "tcp://host:9"));
    net_close();
    memset(long_url, 'a', FN_MAX_URL_LEN);
    memcpy(long_url, "tcp://", 6);
    long_url[FN_MAX_URL_LEN] = 0;
    assert(net_open(long_url) == 0 && strlen(opened_url) == FN_MAX_URL_LEN);
    net_close();
    long_url[FN_MAX_URL_LEN] = 'a'; long_url[FN_MAX_URL_LEN + 1] = 0;
    assert(net_open(long_url) == FN_ERR_URL_TOO_LONG);
    assert(net_open("N:TCP://example:6502/") == 0);
    assert(!strcmp(opened_url, "tcp://example:6502"));
    tx[0] = 'I'; tx[1] = 7; tx[2] = 0;
    assert(net_send(3) == 0 && write_calls == 0);
    tx[0] = 'X'; /* queued data must not alias the game's shared TX buffer */
    write_result = FN_ERR_NOT_READY;
    assert(net_poll() == 0 && write_calls == 1 && read_calls == 0);
    write_result = 0; read_result = FN_ERR_NOT_READY;
    assert(net_poll() == 0 && sent_len == 3 && sent[0] == 'I' && read_calls == 1);
    assert(net_poll() == 0 && read_calls == 1); /* no interrupt, no polling */
    proc = 1; read_result = FN_ERR_BUSY;
    assert(net_poll() == 0 && read_calls == 2);
    rx_head = 20; rx_tail = 250; proc = 1;
    memcpy(read_data, "ABCDEF", 6); read_len = 6; read_result = 0; read_flags = 0;
    assert(net_poll() == 0 && last_read_cap == 6 && seen_read_offset == 0);
    assert(rx_tail == 0 && !memcmp(rx_ring + 250, "ABCDEF", 6));
    read_data[0] = 'G'; read_len = 1; read_flags = FN_READ_EOF;
    assert(net_poll() == 0 && seen_read_offset == 6 && rx_tail == 1 && rx_ring[0] == 'G');
    assert(net_poll() == NET_CLOSED);
    tx[0] = 'H'; assert(net_send(1) == 0 && seen_write_offset == 3 && sent[0] == 'H');
    net_close();
    assert(net_nio_restore_sio() == FN_OK && last_index == 4);
    speed = FN_FUJI_HSIO_OFF;
    assert(net_nio_standard_sio() == FN_OK && last_index == FN_FUJI_HSIO_OFF);
    assert(net_nio_restore_sio() == FN_OK && last_index == FN_FUJI_HSIO_OFF);
    speed = 4;
    assert(net_nio_standard_sio() == FN_OK);
    device_result = FN_ERR_IO;
    assert(net_nio_restore_sio() == FN_ERR_IO && last_index == 4);
    device_result = 0;
    assert(net_nio_standard_sio() == FN_OK && last_index == FN_FUJI_HSIO_OFF);
    assert(net_nio_restore_sio() == FN_OK && last_index == 4);
    puts("NIO Atari adapter tests passed");
    return 0;
}
