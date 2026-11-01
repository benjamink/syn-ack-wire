#include "layout.h"

static void set(struct rect *r, short x, short y, short w, short h)
{
    r->x = x; r->y = y; r->w = w; r->h = h;
}

/* The numbers are the approved mockups' (tools/amiga_mock.py: draw_grid_A, draw_side, seat_cards, prompt_strip and their NTSC
 * twins). PAL: 256 lines, cells 16x15, a card for every seat. NTSC: 200 lines, cells 16x13, two cards and a list. */
void layout_make(struct layout *l, unsigned short height)
{
    unsigned char i, ntsc = height < 256;
    unsigned char *z = (unsigned char *)l;
    unsigned short k;

    for (k = 0; k < sizeof *l; ++k)
        z[k] = 0;
    l->width = 320;
    l->height = ntsc ? 200 : 256;
    l->ntsc = ntsc;
    l->cell_w = 16;
    l->cell_h = ntsc ? 13 : 15;
    l->grid_x = 14;
    l->grid_y = ntsc ? 21 : 22;
    set(&l->header, 0, 0, 320, 11);
    set(&l->grid, 4, l->grid_y - 10, RULE_COLS * l->cell_w + 12, RULE_ROWS * l->cell_h + 12);
    if (ntsc) {
        set(&l->syn_panel, 218, 12, 98, 76);
        set(&l->kit_panel, 218, 90, 98, 50);
        set(&l->log, 0, 172, 320, 18);
        set(&l->prompt, 0, 190, 320, 10);
        set(&l->clock, 228, 192, 64, 6);
        l->seat_cards = 2;
        set(&l->seat[0], 4, 142, 102, 30);
        set(&l->seat[1], 108, 142, 102, 30);
        set(&l->others, 212, 142, 104, 30);
        set(&l->scroll_up, 304, 144, 9, 12);
        set(&l->scroll_down, 304, 158, 9, 12);
    } else {
        set(&l->syn_panel, 218, 12, 98, 80);
        set(&l->kit_panel, 218, 96, 98, 62);
        set(&l->log, 0, 228, 320, 18);
        set(&l->prompt, 0, 246, 320, 10);
        set(&l->clock, 228, 248, 64, 6);
        l->seat_cards = RULE_MAXPLAYERS;
        for (i = 0; i < RULE_MAXPLAYERS; ++i)
            set(&l->seat[i], 4 + (i % 3) * 105, 161 + (i / 3) * 33, 102, 31);
    }
    for (i = 0; i < NSYN; ++i)
        set(&l->syn_row[i], 222, (ntsc ? 24 : 26) + i * 9, 92, 9);
    for (i = 0; i < RULE_KITSIZE; ++i) {
        if (ntsc)
            set(&l->kit_chip[i], 223 + (i % 2) * 47, 102 + (i / 2) * 12, 44, 11);
        else
            set(&l->kit_chip[i], 223 + (i % 2) * 47, 109 + (i / 2) * 15, 44, 13);
    }
}

unsigned char layout_inside(const struct rect *r, short x, short y)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

short layout_node_at(const struct layout *l, short x, short y)
{
    short dx = x - l->grid_x, dy = y - l->grid_y;

    if (dx < 0 || dy < 0 || dx >= RULE_COLS * l->cell_w || dy >= RULE_ROWS * l->cell_h)
        return -1;
    return (dy / l->cell_h) * RULE_COLS + dx / l->cell_w;
}

void layout_node_rect(const struct layout *l, unsigned char node, struct rect *r)
{
    set(r, l->grid_x + (node % RULE_COLS) * l->cell_w, l->grid_y + (node / RULE_COLS) * l->cell_h, l->cell_w, l->cell_h);
}

signed char layout_syn_at(const struct layout *l, short x, short y)
{
    signed char i;
    for (i = 0; i < NSYN; ++i)
        if (layout_inside(&l->syn_row[i], x, y))
            return i;
    return -1;
}

signed char layout_kit_at(const struct layout *l, short x, short y)
{
    signed char i;
    for (i = 0; i < RULE_KITSIZE; ++i)
        if (layout_inside(&l->kit_chip[i], x, y))
            return i;
    return -1;
}

signed char layout_seat_at(const struct layout *l, short x, short y)
{
    signed char i;
    if (l->ntsc)
        return -1;
    for (i = 0; i < RULE_MAXPLAYERS; ++i)
        if (layout_inside(&l->seat[i], x, y))
            return i;
    return -1;
}

signed char layout_btn_at(const struct layout *l, short x, short y)
{
    signed char i;
    for (i = 0; i < UI_BTN_MAX; ++i)
        if (l->btn[i].w && layout_inside(&l->btn[i], x, y))
            return i;
    return -1;
}
