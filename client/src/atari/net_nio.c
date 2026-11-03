/* FujiNet-NIO implementation of platform.h's single TCP stream. */
#include "../platform.h"
#include <fujinet-nio.h>
#include <fn_raw.h>
#include <fn_protocol.h>
#include <string.h>

#if FN_MAX_URL_LEN > 255
#error Atari URL scratch must fit in the 256-byte receive ring
#endif

unsigned char rx_head, rx_tail, tx[16];

static fn_handle_t session;
static unsigned long read_offset, write_offset;
static unsigned char initialized, opened, pending, eof, last, connected, queued;
static unsigned char saved_hsio, saved_hsio_valid, standard_sio;
static unsigned char outgoing[16];
static uint16_t received, written;
static unsigned char flags;
/* The ring is idle while opening. Its first 129 bytes also hold the
 * normalized URL; even a lobby URL already in the ring copies forward safely. */
#define nio_url ((char *)rx_ring)
#ifdef NIO_TEST
extern unsigned char nio_test_clock;
#define CLOCK nio_test_clock
#else
#define CLOCK (*(volatile unsigned char *)0x14)
#endif

/* A compatible U1MB can cache a different speed for each SIO device. */
unsigned char net_nio_standard_sio(void)
{
    unsigned char error;
    if (standard_sio) return FN_OK;
    if (!fn_atari_u1mb_pbi()) return FN_ERR_UNSUPPORTED;
    if (!saved_hsio_valid) {
        error = fn_fuji_get_hsio(&saved_hsio);
        if (error) return error;
        saved_hsio_valid = 1;
    }
    error = fn_fuji_set_hsio(FN_FUJI_HSIO_OFF);
    if (error) return error;
    error = fn_atari_u1mb_clear_devices(15);
    if (error) return error;
    error = fn_atari_u1mb_clear_disks(0x7fff);
    if (error) return error;
    standard_sio = 1;
    return FN_OK;
}

/* Called after the title has replaced the game screen on explicit log-off. */
unsigned char net_nio_restore_sio(void)
{
    unsigned char error;
    if (!saved_hsio_valid) return FN_OK;
    error = fn_fuji_set_hsio(saved_hsio);
    if (error) return error;
    standard_sio = 0;
    error = fn_atari_u1mb_clear_devices(15);
    if (error) return error;
    error = fn_atari_u1mb_clear_disks(0x7fff);
    if (error) return error;
    saved_hsio_valid = standard_sio = 0;
    return FN_OK;
}

unsigned char net_nio_init(void)
{
    unsigned char error;
    if (initialized) return 0;
    error = fn_init();
    if (error) return error;
    (void)net_nio_standard_sio();
    initialized = 1;
    return 0;
}

#if !defined(__CC65__) || defined(NIO_TEST)
/* The lobby's published AppKey is a FileDevice file. Build its documented
 * legacy on-disk name here; no compatibility-layer code is linked. */
unsigned char __fastcall__ net_appkey(unsigned char key)
{
    static const char uri[] = "persist:///FujiNet/00010100.key";
    static const char hex[] = "0123456789abcdef";
    unsigned char *scratch = rx_ring + 80;
    fn_raw_response_t reply;
    unsigned char n = 0;
    if (net_nio_init()) return 0;
    scratch[0] = 1; scratch[1] = 31; scratch[2] = 0;
    memcpy(scratch + 3, uri, 31);
    scratch[28] = hex[key >> 4]; scratch[29] = hex[key & 15];
    scratch[34] = scratch[35] = scratch[36] = scratch[37] = 0;
    scratch[38] = 64; scratch[39] = 0;
    if (fn_raw_call(FN_DEVICE_FILE, FN_CMD_FILE_READ, scratch, 40,
                    scratch, 74, &reply) == FN_OK && reply.status == FN_OK &&
        reply.payload_length >= 10 && scratch[0] == 1 &&
        !(scratch[4] | scratch[5] | scratch[6] | scratch[7] | scratch[9]) &&
        scratch[8] <= 64 && reply.payload_length >= (uint16_t)(10 + scratch[8])) {
        n = scratch[8];
        memcpy(rx_ring + 2, scratch + 10, n);
    }
    rx_ring[2 + n] = 0;
    return n;
}

#endif

unsigned char __fastcall__ net_open(const char *url)
{
#ifdef __CC65__
    unsigned char i; /* FN_MAX_URL_LEN is 128 on cc65. */
#else
    unsigned int i;
#endif
    unsigned char error = net_nio_init();
    if (error) return error;
    /* The shared client uses legacy N: device specs; NIO expects the URL. */
    if (url[0] == 'N' && url[1] == ':') url += 2;
    for (i = 0; url[i] && i < FN_MAX_URL_LEN; ++i) nio_url[i] = url[i];
    if (url[i]) return FN_ERR_URL_TOO_LONG;
    if (i && nio_url[i - 1] == '/') --i;
    nio_url[i] = 0;
    if (i >= 3 && (nio_url[0] == 'T' || nio_url[0] == 't') &&
        (nio_url[1] == 'C' || nio_url[1] == 'c') &&
        (nio_url[2] == 'P' || nio_url[2] == 'p')) {
        nio_url[0] = 't'; nio_url[1] = 'c'; nio_url[2] = 'p';
    }
    rx_head = rx_tail = 0;
    read_offset = write_offset = 0;
    eof = 0;
    connected = queued = 0;
    error = fn_open(&session, 0, nio_url, FN_OPEN_KEEP);
    if (error) return error;
    opened = 1;
    fn_atari_proc_enable();
    pending = 1;
    last = CLOCK;
    return 0;
}

unsigned char net_poll(void)
{
    unsigned char free, n, error, now;
    if (eof) return NET_CLOSED;
    now = CLOCK;
    if (!connected && queued) {
        error = fn_write(session, write_offset, outgoing, queued, &written);
        if (error == FN_ERR_NOT_READY || error == FN_ERR_BUSY) return 0;
        if (error) return error;
        write_offset += written;
        if (written != queued) return NET_CLOSED;
        connected = 1;
        queued = 0;
    }
    if (fn_atari_proc_pending()) pending = 1;
    if (!pending && (unsigned char)(now - last) < 60) return 0;
    free = rx_head - rx_tail - 1;
    if (!free) { pending = 1; return 0; }
    n = -rx_tail;
    if (rx_tail && n < free) free = n;
    pending = 0;
    last = now;
    error = fn_read(session, read_offset, rx_ring + rx_tail, free, &received, &flags);
    if (error == FN_ERR_NOT_READY || error == FN_ERR_BUSY) return 0;
    if (error) return error;
    read_offset += received;
    rx_tail += (unsigned char)received;
    if (flags & FN_READ_EOF) {
        if (!received) return NET_CLOSED;
        eof = 1;
    }
    if ((flags & FN_READ_AVAILABLE) && fn_read_available()) pending = 1;
    /* Older firmware does not report availability; a full read may leave data. */
    if (received == free) pending = 1;
    return 0;
}

unsigned char __fastcall__ net_send(unsigned char len)
{
    unsigned char error;
    if (!connected) {
        if (queued || len > sizeof outgoing) return FN_ERR_BUSY;
        memcpy(outgoing, tx, len);
        queued = len;
        return 0;
    }
    error = fn_write(session, write_offset, tx, len, &written);
    write_offset += written;
    return error ? error : (written == len ? 0 : NET_CLOSED);
}

void net_close(void)
{
    fn_atari_proc_disable();
    if (opened) { fn_close(session); opened = 0; }
}
