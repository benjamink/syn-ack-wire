/* GEOS on the Atari: the region panel (REGION_PICK). START on the title, or a click on its REGION, hangs
 * up (main.c) and opens it: a framed panel over the title with each region the servers run
 * (regions_tab.h, from server/servers.fly.toml by tools/gen_regions.py), its pace and the seats taken
 * at its table out of the seats there, as the FujiNet lobby's PLAYERS column has them. Those come
 * from the servers' web page, GET /SEATS ("CHIBA-GRID-7 2 6", a line each), read with N1 as HTTP
 * while nothing else is on it. 1-4 or a click picks a region (the title dials it), R or Refresh
 * reads the seats again, Esc or Cancel keeps the one there was. Drawing behind it waits
 * (display_gui.c's gui_hold); the title is drawn again after. */
#include <string.h>
#include <geos.h>
#include "../platform.h"
#include "../display.h"
#include "../common/game.h"
#include "regions_tab.h"

/* draw.s */
extern unsigned char bold, buf[48];
#define bp (*(unsigned char **)0x78)
void box(unsigned char top, unsigned char bot, unsigned left, unsigned right, unsigned char pat);
void invert(unsigned char top, unsigned char bot, unsigned left, unsigned right);
void panel(unsigned char top, unsigned char bot, unsigned left, unsigned right);
void put(unsigned char c);
void number(unsigned value, unsigned char money);
void show(unsigned x, unsigned char y);
void show_right(unsigned x, unsigned char y);
void say(const char *s, unsigned x, unsigned char y);
/* display_gui.c */
void __fastcall__ gui_hold(unsigned char on);
extern unsigned gui_x;
extern unsigned char gui_y;
#define K_CLICK 0x04                    /* gui_poll(), while held: a click at gui_x, gui_y */
/* net_sio.s, ring.s */
extern unsigned char rx_ring[256], rx_tail;

#define LEFT    40
#define RIGHT   280
#define TOP     26
#define ROW0    (TOP + 16)              /* region n's line: ROW0 + 11 n */
#define BTN     (ROW0 + 11 * NREGIONS + 4)
#define BOT     (BTN + 16)

static const char *const name[NREGIONS] = REGION_NAMES;
static const char *const pace[NREGIONS] = REGION_PACES;
static const char *const url[NREGIONS] = REGION_URLS;
static unsigned char cur, taken[NREGIONS], seats[NREGIONS];   /* seats 0: not known */

const char *region_url(void)
{
    return url[cur];
}

/* A number from p on into *v; where it stops. */
static unsigned char *num(unsigned char *p, unsigned char *v)
{
    *v = 0;
    while (*p >= '0' && *p <= '9')
        *v = *v * 10 + *p++ - '0';
    return p;
}

/* GET /SEATS, a few seconds at most, into taken[] and seats[]. */
static void fetch(void)
{
    unsigned char t, i, n, *p;
    memset(seats, 0, sizeof seats);
    if (!net_open(REGION_SEATS)) {
        t = ticks();
        while (!net_poll() && (unsigned char)(ticks() - t) < 250)   /* NET_CLOSED: all read */
            ;
    }
    net_close();
    rx_ring[rx_tail] = 0;               /* the answer from rx_ring[0], a C string */
    for (p = rx_ring; *p; ) {
        for (i = 0; i < NREGIONS; ++i) {
            n = strlen(name[i]);
            if (!memcmp(p, name[i], n) && p[n] == ' ')
                num(num(p + n + 1, &taken[i]) + 1, &seats[i]);
        }
        while (*p && *p++ != '\n')
            ;
    }
}

/* Region i's line: its number, name, pace and seats; the one picked the other way round. */
static void line(unsigned char i)
{
    unsigned char y = ROW0 + 11 * i;
    box(y - 1, y + 8, LEFT + 3, RIGHT - 3, 0);
    bp = buf;
    put('1' + i);
    show(LEFT + 7, y);
    say(name[i], LEFT + 20, y);
    say(pace[i], LEFT + 112, y);
    bp = buf;
    if (seats[i]) {
        number(taken[i], 0);
        put('/');
        number(seats[i], 0);
    } else {
        put('-');
    }
    show_right(RIGHT - 8, y);
    if (i == cur)
        invert(y - 1, y + 8, LEFT + 3, RIGHT - 3);
}

static void lines(void)
{
    unsigned char i;
    for (i = 0; i < NREGIONS; ++i)
        line(i);
}

/* A button: a frame and its word, at x. */
static void button(const char *s, unsigned x)
{
    panel(BTN, BTN + 11, x, x + 60);
    say(s, x + 12, BTN + 2);
}

void __fastcall__ region_pick(unsigned char (*key)(void))
{
    unsigned char k;
    gui_hold(1);
    panel(TOP, BOT, LEFT, RIGHT);
    bold = SET_BOLD;
    say("REGION", LEFT + 20, TOP + 3);
    say("PACE", LEFT + 112, TOP + 3);
    say("SEATS", RIGHT - 38, TOP + 3);
    bold = 0;
    button("Refresh", LEFT + 20);
    button("Cancel", RIGHT - 80);
    lines();                            /* the seats not known yet: - */
    fetch();
    lines();
    for (;;) {
        k = key();
        if (k == K_CLICK) {
            k = 0;
            if (gui_x > LEFT && gui_x < RIGHT && gui_y >= ROW0 - 1 && gui_y < ROW0 - 1 + 11 * NREGIONS)
                k = '1' + (gui_y - ROW0 + 1) / 11;
            else if (gui_y >= BTN && gui_y <= BTN + 11)
                k = gui_x < 160 ? 'R' : K_ESC;
        }
        if (k == 'R') {
            fetch();
            lines();
        } else if (k == K_ESC) {
            break;
        } else if (k >= '1' && k < '1' + NREGIONS) {
            cur = k - '1';
            region_len = strlen(name[cur]);
            memcpy(region, name[cur], region_len);   /* the title shows it while it dials */
            break;
        }
    }
    gui_hold(0);
}
