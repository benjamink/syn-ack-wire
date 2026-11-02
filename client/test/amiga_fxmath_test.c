/* Host tests for src/amiga/fxmath.c: the title scroller's sine. */
#include <stdio.h>
#include "../src/amiga/fxmath.h"

static int ran, failures;
#define CHECK(c) do { if (!(c)) { ++failures; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define RUN(t) do { ++ran; printf("%s\n", #t); t(); } while (0)

static void test_wave_stays_in_range_and_turns(void)
{
    int p, lo = 0, hi = 0;
    for (p = 0; p < 256; ++p) {
        int w = fx_wave((unsigned char)p);
        if (w < lo) lo = w;
        if (w > hi) hi = w;
    }
    CHECK(lo == -4 && hi == 4);
    CHECK(fx_wave(0) == 0 && fx_wave(64) == 4 && fx_wave(128) == 0 && fx_wave(192) == -4);
}
int main(void)
{
    RUN(test_wave_stays_in_range_and_turns);
    printf("%d tests, %d failures\n", ran, failures);
    return failures != 0;
}
