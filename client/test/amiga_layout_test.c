/* Host tests for src/amiga/layout.c: rectangles for PAL and NTSC, and which widget a pixel is on.
 *   make test-amiga-layout */
#include <stdio.h>
#include <string.h>
#include "../src/amiga/layout.h"

static int failures, ran;
#define CHECK(c) do { if (!(c)) { printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
#define RUN(t) do { ++ran; printf("%s\n", #t); t(); } while (0)

static const unsigned short heights[2] = { 256, 200 };

static int inside_screen(const struct layout *l, const struct rect *r)
{
    return r->w == 0 || (r->x >= 0 && r->y >= 0 && r->x + r->w <= l->width && r->y + r->h <= l->height);
}
static int overlap(const struct rect *a, const struct rect *b)
{
    return a->w && b->w && a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

static void test_every_rect_within_screen(void)
{
    int k, i, n;
    struct layout l;
    for (k = 0; k < 2; ++k) {
        layout_make(&l, heights[k]);
        CHECK(l.width == 320 && l.height == heights[k]);
        CHECK(inside_screen(&l, &l.header) && inside_screen(&l, &l.grid) && inside_screen(&l, &l.syn_panel));
        CHECK(inside_screen(&l, &l.kit_panel) && inside_screen(&l, &l.log) && inside_screen(&l, &l.prompt));
        CHECK(inside_screen(&l, &l.clock) && inside_screen(&l, &l.others));
        for (i = 0; i < NSYN; ++i) CHECK(inside_screen(&l, &l.syn_row[i]));
        for (i = 0; i < RULE_KITSIZE; ++i) CHECK(inside_screen(&l, &l.kit_chip[i]));
        for (i = 0; i < RULE_MAXPLAYERS; ++i) CHECK(inside_screen(&l, &l.seat[i]));
        for (n = 0; n < RULE_ROWS * RULE_COLS; ++n) {
            struct rect r;
            layout_node_rect(&l, n, &r);
            CHECK(inside_screen(&l, &r));
        }
    }
}
static void test_no_overlap_between_panels(void)
{
    int k, i, j;
    struct layout l;
    for (k = 0; k < 2; ++k) {
        const struct rect *p[16];
        int n = 0;
        layout_make(&l, heights[k]);
        p[n++] = &l.header; p[n++] = &l.grid; p[n++] = &l.syn_panel; p[n++] = &l.kit_panel; p[n++] = &l.log;
        p[n++] = &l.prompt;
        for (i = 0; i < l.seat_cards; ++i) p[n++] = &l.seat[i];
        if (l.ntsc) p[n++] = &l.others;
        for (i = 0; i < n; ++i)
            for (j = i + 1; j < n; ++j)
                CHECK(!overlap(p[i], p[j]));
    }
}
static void test_node_roundtrip(void)
{
    int k, n;
    struct layout l;
    for (k = 0; k < 2; ++k) {
        layout_make(&l, heights[k]);
        for (n = 0; n < RULE_ROWS * RULE_COLS; ++n) {
            struct rect r;
            layout_node_rect(&l, n, &r);
            CHECK(layout_node_at(&l, r.x + r.w / 2, r.y + r.h / 2) == n);
        }
    }
}
static void test_node_borders_half_open(void)
{
    int k;
    struct layout l;
    for (k = 0; k < 2; ++k) {
        struct rect a, b;
        layout_make(&l, heights[k]);
        layout_node_rect(&l, 0, &a);
        layout_node_rect(&l, 1, &b);
        CHECK(layout_node_at(&l, a.x, a.y) == 0);
        CHECK(layout_node_at(&l, a.x + a.w - 1, a.y + a.h - 1) == 0);
        CHECK(layout_node_at(&l, b.x, a.y) == 1);                       /* the next pixel is the neighbour */
        CHECK(layout_node_at(&l, a.x - 1, a.y) == -1);
        CHECK(layout_node_at(&l, a.x, a.y - 1) == -1);
        layout_node_rect(&l, RULE_ROWS * RULE_COLS - 1, &b);
        CHECK(layout_node_at(&l, b.x + b.w, b.y) == -1);
        CHECK(layout_node_at(&l, b.x, b.y + b.h) == -1);
    }
}
static void test_gaps_are_not_nodes(void)
{
    struct layout l;
    int k;
    for (k = 0; k < 2; ++k) {
        layout_make(&l, heights[k]);
        CHECK(layout_node_at(&l, l.grid_x - 4, l.grid_y + 3) == -1);    /* the row numbers' column */
        CHECK(layout_node_at(&l, l.grid_x + 3, l.grid.y + 2) == -1);    /* the column letters' row, above the nodes */
        CHECK(layout_node_at(&l, -1, -1) == -1);
        CHECK(layout_node_at(&l, 320, 255) == -1);
        CHECK(layout_node_at(&l, 10000, 10000) == -1);
    }
}
static void test_syn_and_kit_hits_roundtrip(void)
{
    struct layout l;
    int k, i;
    for (k = 0; k < 2; ++k) {
        layout_make(&l, heights[k]);
        for (i = 0; i < NSYN; ++i)
            CHECK(layout_syn_at(&l, l.syn_row[i].x + 2, l.syn_row[i].y + 2) == i);
        for (i = 0; i < RULE_KITSIZE; ++i)
            CHECK(layout_kit_at(&l, l.kit_chip[i].x + 2, l.kit_chip[i].y + 2) == i);
        CHECK(layout_syn_at(&l, 0, 0) == -1 && layout_kit_at(&l, 0, 0) == -1);
    }
}
static void test_ntsc_shows_me_and_mover(void)
{
    struct layout l, p;
    layout_make(&l, 200);
    layout_make(&p, 256);
    CHECK(l.ntsc && !p.ntsc);
    CHECK(l.seat_cards == 2 && p.seat_cards == RULE_MAXPLAYERS);
    CHECK(l.others.w > 0 && l.others.y >= l.grid.y + l.grid.h);        /* the list is below the grid */
    CHECK(l.scroll_up.w > 0 && l.scroll_down.w > 0);
    CHECK(layout_seat_at(&l, l.seat[0].x + 3, l.seat[0].y + 3) == -1);   /* NTSC cards are not tied to seat numbers */
    CHECK(layout_seat_at(&p, p.seat[4].x + 3, p.seat[4].y + 3) == 4);
}
static void test_height_clamps(void)
{
    struct layout l;
    int n;
    layout_make(&l, 150);
    CHECK(l.ntsc && l.height <= 200);
    for (n = 0; n < RULE_ROWS * RULE_COLS; ++n) {
        struct rect r;
        layout_node_rect(&l, n, &r);
        CHECK(inside_screen(&l, &r));
    }
    layout_make(&l, 300);
    CHECK(!l.ntsc && l.height == 256);
}
static void test_buttons_empty_until_filled(void)
{
    struct layout l;
    layout_make(&l, 256);
    CHECK(layout_btn_at(&l, 160, 100) == -1);                           /* nothing there: a click finds no button */
    l.btn[BTN_BUY].x = 10; l.btn[BTN_BUY].y = 20; l.btn[BTN_BUY].w = 30; l.btn[BTN_BUY].h = 16;
    CHECK(layout_btn_at(&l, 10, 20) == BTN_BUY);
    CHECK(layout_btn_at(&l, 39, 35) == BTN_BUY);
    CHECK(layout_btn_at(&l, 40, 35) == -1);
}

int main(void)
{
    RUN(test_every_rect_within_screen); RUN(test_no_overlap_between_panels); RUN(test_node_roundtrip);
    RUN(test_node_borders_half_open); RUN(test_gaps_are_not_nodes); RUN(test_syn_and_kit_hits_roundtrip);
    RUN(test_ntsc_shows_me_and_mover); RUN(test_height_clamps); RUN(test_buttons_empty_until_filled);
    printf("%d tests, %d failures\n", ran, failures);
    return failures != 0;
}
