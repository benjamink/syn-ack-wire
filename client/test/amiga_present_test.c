/* Host tests for src/amiga/render.c: which cells of the 40x24 text screen get drawn, and with which pens.
 *   make test-amiga-present */
#include <stdio.h>
#include <string.h>
#include "../src/amiga/render.h"

static int failures, ran;
#define CHECK(c) do { if (!(c)) { printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
#define RUN(t) do { ++ran; reset(); printf("%s\n", #t); t(); } while (0)

#define NORMAL 0x07
#define INV    0x70
#define FLASH  0xF0

static unsigned short screen[R_ROWS * R_COLS], shown[R_ROWS * R_COLS];
static struct { unsigned char row, col, n, inverse; char s[41]; } calls[64];
static int ncalls;

static void fake_text(unsigned char row, unsigned char col, const char *s, unsigned char n, unsigned char inverse)
{
    if (ncalls < 64) {
        calls[ncalls].row = row; calls[ncalls].col = col; calls[ncalls].n = n; calls[ncalls].inverse = inverse;
        memcpy(calls[ncalls].s, s, n); calls[ncalls].s[n] = 0;
    }
    ++ncalls;
}

static void reset(void)
{
    int i;
    for (i = 0; i < R_ROWS * R_COLS; ++i)
        screen[i] = NORMAL << 8 | ' ';
    memset(shown, 0, sizeof shown);
    amiga_render(screen, shown, 0, fake_text);      /* the first full draw */
    ncalls = 0;
}

static void test_only_changed_cells_drawn(void)
{
    screen[5 * R_COLS + 3] = NORMAL << 8 | 'A';
    screen[9 * R_COLS + 10] = NORMAL << 8 | 'B';
    screen[20 * R_COLS + 39] = NORMAL << 8 | 'C';
    amiga_render(screen, shown, 0, fake_text);
    CHECK(ncalls == 3);
    CHECK(calls[0].row == 5 && calls[0].col == 3 && calls[0].s[0] == 'A');
    CHECK(calls[2].row == 20 && calls[2].col == 39);
    ncalls = 0;
    amiga_render(screen, shown, 0, fake_text);      /* nothing changed since */
    CHECK(ncalls == 0);
}
static void test_a_run_is_one_call(void)
{
    int i;
    for (i = 0; i < 5; ++i)
        screen[2 * R_COLS + 4 + i] = NORMAL << 8 | ('A' + i);
    amiga_render(screen, shown, 0, fake_text);
    CHECK(ncalls == 1);
    CHECK(calls[0].n == 5 && strcmp(calls[0].s, "ABCDE") == 0);
}
static void test_inverse_swaps_pens(void)
{
    screen[0] = INV << 8 | 'X';
    screen[1] = NORMAL << 8 | 'Y';
    amiga_render(screen, shown, 0, fake_text);
    CHECK(ncalls == 2);                             /* the pens differ, so two runs */
    CHECK(calls[0].inverse == 1 && calls[1].inverse == 0);
}
static void test_only_the_game_area_drawn(void)
{
    screen[23 * R_COLS + 39] = NORMAL << 8 | 'Z';   /* the last cell of the last row */
    amiga_render(screen, shown, 0, fake_text);
    CHECK(ncalls == 1 && calls[0].row == 23 && calls[0].col == 39);
}
static void test_blink_alternates(void)
{
    screen[7] = FLASH << 8 | 'F';
    amiga_render(screen, shown, 0, fake_text);
    CHECK(ncalls == 1 && calls[0].inverse == 1);    /* phase 0: reverse */
    ncalls = 0;
    amiga_render(screen, shown, 1, fake_text);
    CHECK(ncalls == 1 && calls[0].inverse == 0);    /* phase 1: normal again */
    ncalls = 0;
    amiga_render(screen, shown, 1, fake_text);
    CHECK(ncalls == 0);
}
static void test_cp437_glyphs(void)
{
    CHECK(amiga_glyph('A') == 'A');
    CHECK(amiga_glyph(0x9D) == 0xA5);               /* the yen the server sends as "\" */
    CHECK(amiga_glyph(0xDB) == '#');                /* a full block */
    CHECK(amiga_glyph(0xC4) == '-');                /* a horizontal line */
    CHECK(amiga_glyph(0xB3) == '|');
    CHECK(amiga_glyph(0xC5) == '+');
    CHECK(amiga_glyph(0xFF) == '?');
    CHECK(amiga_glyph(0x00) == ' ');
}

int main(void)
{
    RUN(test_only_changed_cells_drawn); RUN(test_a_run_is_one_call); RUN(test_inverse_swaps_pens);
    RUN(test_only_the_game_area_drawn); RUN(test_blink_alternates); RUN(test_cp437_glyphs);
    printf("%d tests, %d failures\n", ran, failures);
    return failures != 0;
}
