/* The screens as a GEOS application: a menu bar, framed panels with shadows on a
 * dithered desktop, in GEOS's colours, drawn with the GEOS kernal (Rectangle, FrameRectangle,
 * InvertRectangle, PutChar).
 *
 * The title: one window, the Atari's title art in its own font, then REGION, STATUS,
 * HANDLE and the move hints, and the bar the playfield has, for what to do.
 *
 * The playfield:
 *   y   0-12   menu bar; the region and the link on the right
 *      14-97   board panel: 12x8 cells of 12x8 pixels  |  HIVES: letter, nodes, name,
 *              (hives inverse, in the Atari's font)    |  price and crypto left while buying
 *     101-159  NETRUNNER  ACCT  crypto per syndicate, every seat; the turn's row inverse
 *     162-197  three log lines, then the bar: what the prompt is about on the left, the
 *              question on the right, the seconds left at the end
 *
 * When what the prompt is about and the question don't fit side by side (a hivejack's),
 * the first goes on the log's last line, inverse, and the log shows two lines until the
 * prompt closes.
 *
 * Text is GEOS's system font (BSW 9). The board, the row numbers and the currency sign
 * are the Atari ROM's font (screen.s copies it), as a GEOS font built here, so a glyph
 * can sit at any x. Inverse areas are drawn plain, then inverted. */
#include <string.h>
#include <geos.h>
#include "../config.h"
#include "../platform.h"
#include "../display.h"
#include "../common/proto.h"
#include "../common/proto_tab.h"
#include "../common/game.h"

#include "../atari/title_art.h"        /* title_art[], TITLE_END */

extern unsigned char font[1024];        /* screen.s: the ROM's; the yen goes at '\' */
void screen_init(void);                 /* screen.s: the font, IRQs masked */


#define P_WHITE  0                      /* GEOS's patterns */
#define P_BLACK  1
#define P_DITHER 2

#define BASE    6                       /* both fonts: the baseline, from the top */
#define BITMAP  ((unsigned char *)0xA038)   /* GEOS's screen: 40 bytes a line */

/* the board */
#define BX      14
#define BY      25
#define CW      12
#define CH      8
/* HIVES */
#define HX      166                     /* the panel's left */
#define H_SIZE  188                     /* right edge of the node count */
#define H_NAME  191
#define H_PRICE 292                     /* right edges */
#define H_LEFT  312
/* seats */
#define SY      101
#define S_NAME  10
#define S_ACCT  112                     /* right edge */
#define S_COIN  124                     /* the first crypto column; 27 apart */
/* log and bar */
#define LY      162
#define BAR_TOP 187
#define BAR_R   293                     /* the question's right edge; the clock after */
#define CLOCK_R 312
/* the title */
#define ART_Y   17                      /* the art's first row (the Atari's row 1) */
#define T_Y     (ART_Y + 8 * (TITLE_END - 1) + 4)   /* REGION; STATUS, HANDLE 10 apart */
#define T_X     40                      /* the labels */
#define T_VAL   96                      /* what they say */

unsigned char romfont[8 + 2 * 97 + 8 * 64];   /* (draw.s draws with it) */
static unsigned char gui_ready, gui_on, on_title;
static unsigned char hold, missed;     /* a menu is open: drawing waits; something did */
static unsigned char menu = 0xFF, item = 0xFF, was_down;   /* the menu's open (0), its item */
static unsigned char i, j, n, v;
static unsigned char half;
static const char *const mixed[NSYN] = SYN_MIXED;

/* ---------------------------------------------------------------- drawing */

/* draw.s: areas, characters, text put together in buf from bp and drawn at the pen, px, in
 * `bold` (0 or SET_BOLD), clipped `clip` lines below its top (GEOS draws a character's whole box,
 * so it would wipe what's below). Bit 7 on a character in buf draws it the other way round from
 * what's around it (inverse in a white area, plain in the bar while it asks); '\' is the yen. */
extern unsigned px;
extern unsigned char bold, clip, buf[48];
#define bp (*(unsigned char **)0x78)    /* GEOS's application register a6 */
void box(unsigned char top, unsigned char bot, unsigned left, unsigned right, unsigned char pat);
void invert(unsigned char top, unsigned char bot, unsigned left, unsigned right);
void panel(unsigned char top, unsigned char bot, unsigned left, unsigned right);
void rom(unsigned char c, unsigned char y, unsigned x);     /* one of the Atari's glyphs */
unsigned width(const unsigned char *s, unsigned char len);
void put(unsigned char c);
void text(const char *s);
void textn(const unsigned char *s, unsigned char len);
void number(unsigned value, unsigned char money);           /* money: the yen, and "00" after */
void node_name(unsigned char node);
void syn_letters(unsigned char mask);
void out(const unsigned char *s, unsigned char len, unsigned char y);   /* at the pen, top y */
void show(unsigned x, unsigned char y);                     /* buf at x */
void show_right(unsigned x, unsigned char y);               /* buf ending at x */
void say(const char *s, unsigned x, unsigned char y);

/* The fixed parts of a screen, as tables (draw.s draws them). */
struct label {
    const char *s;
    unsigned x;
    unsigned char y, how;               /* GEOS's style (SET_BOLD), and: */
};
#define L_RIGHT 1                       /* ending at x */
#define L_INV   2                       /* the other way round */
struct area {
    unsigned char top, bot;
    unsigned left, right;
};
void labels(const struct label *p);     /* to one with s 0 */
void panels(const struct area *p);      /* to one with top 0xFF */

/* ---------------------------------------------------------------- set-up */

/* A GEOS font of the Atari's glyphs $20-$5F: 8 wide, 8 high, a byte a glyph per line. */
static void gui_init(void)
{
    static const unsigned char yen[8] = { 0x00, 0x66, 0x66, 0x3C, 0x7E, 0x18, 0x7E, 0x18 };
    unsigned *idx = (unsigned *)(romfont + 8);
    unsigned char *data = romfont + 8 + 2 * 97;
    gui_ready = 1;
    screen_init();
    memcpy(font + 0x3C * 8, yen, 8);    /* "\" -> yen */
    romfont[0] = BASE;
    romfont[1] = 64;                    /* bytes a line */
    romfont[3] = 8;                     /* height */
    romfont[4] = 8;                     /* the index, from the header */
    romfont[6] = 8 + 2 * 97;            /* the data */
    for (i = 0; i <= 96; ++i)
        idx[i] = (i < 64 ? i : 64) * 8;
    for (i = 0; i < 8; ++i)
        for (j = 0; j < 64; ++j)
            data[i * 64 + j] = font[j * 8 + i];
    half = ticks_per_sec() >> 1;
}

/* Draw now? Not while a menu is open: then remember to redraw. */
static unsigned char live(void)
{
    if (hold) {
        missed = 1;
        return 0;
    }
    return gui_on;
}

/* What the places drawn often show, as a signature of what drew them (sig.s): a place is
 * drawn again only when that changes, or when everything is (`all`). */
enum { S_REGION, S_STATUS, S_HANDLE, S_HINTS, S_SOUND, S_BAR, S_CLOCK, S_HEAD, S_HIVE };   /* 15 */
extern unsigned sig;
extern unsigned char all;
void __fastcall__ mix(const unsigned char *p, unsigned char len);
unsigned char __fastcall__ fresh(unsigned char place);
unsigned char __fastcall__ fresh_buf(unsigned char place);

/* what a menu kept from being drawn */
static unsigned char missed_node[(NODES + 7) / 8], missed_seat, missed_more;
#define M_LOG   1
#define M_LINK  2


/* ---------------------------------------------------------------- the bar and the log */

static unsigned char info[48], info_n, ask[48], ask_n;
static unsigned char borrowed;          /* the prompt's subject is on the log's last line */

/* The bar (its clock, and the subject on the log's last line) is inverse only while a question
 * waits for the player: a prompt, Esc's Y/N, the title's handle, or a message that asks (a '?'). */
static unsigned char asking;            /* set where the question comes or goes */

/* An arrow 7 wide at the log panel's right, its point up or down. */
static void arrow(unsigned char y, unsigned char up)
{
    for (j = 0; j < 4; ++j)
        box(y + (up ? j : 3 - j), y + (up ? j : 3 - j), 306 - j, 306 + j, P_BLACK);
}

/* The log's lines from the history, log_back lines back, then the arrows that step
 * through it, and while back, how far. */
static void log_lines(void)
{
    const unsigned char *p;
    unsigned char back = log_back + 2 - borrowed;
    box(LY + 1, BAR_TOP - 1, 4, 314, P_WHITE);
    for (i = 0; i < 3 - borrowed; ++i) {
        p = log_line(back - i);
        if (p) {
            px = 8;
            out(p + 1, p[0], LY + 1 + 8 * i);
        }
    }
    if (borrowed) {
        px = 8;
        out(info, info_n, LY + 17);
        if (asking)
            invert(LY + 16, BAR_TOP - 1, 4, 314);
    }
    arrow(LY + 3, 1);
    arrow(LY + 11, 0);
    if (log_back) {
        bp = buf;
        put('^');
        number(log_back, 0);
        bold = SET_BOLD;
        show_right(298, LY + 1);
        bold = 0;
    }
}

static unsigned char secs;              /* the clock's */

static void clock_at(void)
{
    sig = secs;
    if (asking)
        sig |= 0x8000;
    if (!fresh(S_CLOCK))
        return;
    box(BAR_TOP, BAR_TOP + 9, BAR_R + 1, 314, P_WHITE);
    if (secs) {
        bp = buf;
        number(secs, 0);
        bold = SET_BOLD;
        show_right(CLOCK_R, BAR_TOP + 1);
        bold = 0;
    }
    if (asking)
        invert(BAR_TOP, BAR_TOP + 9, BAR_R + 1, 314);
}

/* The bar from info[] and ask[], and the log's lines if the room changes. */
static void bar(void)
{
    unsigned wi, wa;
    unsigned char b;
    sig = info_n << 8 | ask_n;
    if (asking)
        sig |= 0x8000;              /* info_n < 48 */
    mix(info, info_n);
    mix(ask, ask_n);
    b = fresh(S_BAR);
    if (!pr_kind)                       /* no question: no seconds left */
        secs = 0;
    clock_at();
    if (!b)
        return;
    wi = width(info, info_n);
    wa = width(ask, ask_n);
    b = info_n && ask_n && 8 + wi + 8 + wa > BAR_R;
    if (b != borrowed) {
        borrowed = b;
        log_lines();
    } else if (b) {
        log_lines();                    /* the subject changed */
    }
    box(BAR_TOP, BAR_TOP + 9, 4, BAR_R, P_WHITE);
    if (!b) {
        px = 8;
        out(info, info_n, BAR_TOP + 1);
    }
    px = BAR_R - wa;
    out(ask, ask_n, BAR_TOP + 1);
    if (asking)
        invert(BAR_TOP, BAR_TOP + 9, 4, BAR_R);
}

static void set_info(void)
{
    info_n = bp - buf;
    memcpy(info, buf, info_n);
}

static void set_ask(const char *s)
{
    bp = buf;
    text(complaint ? complaint : s);
}

static void ask_done(void)
{
    ask_n = bp - buf;
    memcpy(ask, buf, ask_n);
}

void display_log(void)                  /* the history has it already */
{
    if (live())
        log_lines();
    else
        missed_more |= M_LOG;
}

void display_log_view(void)
{
    display_log();
}

/* ---------------------------------------------------------------- the board */

/* The last move blinks, inverse and back each half second, for BLINKS halves. */
#define BLINKS 10
static unsigned char blink = NONE, blink_t, flips, lit;

static unsigned char target(unsigned char node)
{
    if (!hints)
        return 0;
    for (n = 0; n < RULE_KITSIZE; ++n)
        if (kit[n] == node)
            return '1' + n;
    return 0;
}

static void last_line(void)
{
    box(25 + 63, 25 + 63 + 8, HX + 1, 314, P_WHITE);
    if (last_node == NONE)
        return;
    bp = buf;
    text("LAST: ");
    node_name(last_node);
    show(HX + 5, 25 + 63);
}

void display_node(unsigned char node)
{
    unsigned char r, c = node, inv = 0;
    unsigned x;
    unsigned char y;
    if (!live()) {
        missed_node[node >> 3] |= 1 << (node & 7);
        return;
    }
    for (r = 0; c >= RULE_COLS; ++r)
        c -= RULE_COLS;
    x = BX + c * CW;
    y = BY + r * CH;
    v = board[node];
    if (v == CELL_EMPTY) {
        v = target(node);
        if (!v)
            v = GLYPH_EMPTY;
    } else if (v == CELL_SPIKE) {
        v = GLYPH_SPIKE;
    } else {
        v = syn_letter[v - 2];
        inv = 1;
    }
    if (node == last_node) {
        if (blink != node) {
            blink = node;
            blink_t = ticks();
            flips = 0;
            lit = 1;
            last_line();
        }
        if (flips < BLINKS)
            inv ^= lit;
    } else if (node == blink) {
        blink = NONE;
    }
    box(y, y + CH - 1, x, x + CW - 1, P_WHITE);
    rom(v, y, x + 2);
    if (inv)
        invert(y, y + CH - 1, x, x + CW - 1);
}

void display_idle(void)
{
    if (blink == NONE || flips == BLINKS || (unsigned char)(ticks() - blink_t) < half)
        return;
    blink_t += half;
    lit ^= 1;
    ++flips;
    display_node(blink);
}

/* ---------------------------------------------------------------- hives and seats */

void display_hives(void)
{
    unsigned char y;
    unsigned char buying = pr_kind == 'B';
    if (!live())
        return;
    sig = buying;
    if (fresh(S_HEAD)) {
        box(15, 23, 240, 314, P_WHITE);
        if (buying) {                   /* plain: bold, the two would touch */
        bp = buf;
        text("PRICE");
        show_right(H_PRICE, 15);
        bp = buf;
        text("LEFT");
        show_right(H_LEFT, 15);
        }
    }
    for (j = 0; j < NSYN; ++j) {
        sig = hive_size[j];             /* the price follows from it */
        if (buying)
            sig |= (pr[11 + j] | 0x80) << 8;
        if (!fresh(S_HIVE + j))
            continue;
        y = 25 + 9 * j;
        box(y, y + 8, HX + 1, 314, P_WHITE);
        bold = SET_BOLD;
        bp = buf;
        put(syn_letter[j]);
        show(HX + 3, y);
        bold = 0;
        bp = buf;
        n = hive_size[j];
        if (n)
            number(n, 0);
        else
            put('-');
        show_right(H_SIZE, y);
        if (n >= RULE_SAFESIZE)         /* Black ICE */
            invert(y - 1, y + 7, H_SIZE - width(buf, bp - buf) - 1, H_SIZE);
        say(mixed[j], H_NAME, y);
        if (buying && pr[4 + j]) {      /* pr: timeout, cash[2], flags, price[7], supply[7] */
            bp = buf;
            number(pr[4 + j], 1);
            show_right(H_PRICE, y);
            bp = buf;
            number(pr[11 + j], 0);
            show_right(H_LEFT, y);
        }
    }
    bold = 0;
}

void display_seat(unsigned char seat)
{
    unsigned char y;
    if (seat >= SEATS)
        return;
    if (!live()) {
        missed_seat |= 1 << seat;
        return;
    }
    y = SY + 10 + 8 * seat;
    clip = 6;                           /* the next row starts at y + 7 */
    box(y - 1, y + 6, 4, 314, P_WHITE);
    if (seat == me)
        say(">", 4, y);
    n = seat_kind[seat];
    if (n == SEAT_EMPTY) {
        say("-", S_NAME, y);
        clip = 7;
        return;
    }
    bp = buf;
    for (j = 0; j < 8 && seat_name[seat][j] != ' '; ++j)
        put(seat_name[seat][j]);
    if (n == SEAT_AWAY)
        put('?');
    else if (n == SEAT_BOT)
        put('*');
    show(S_NAME, y);
    bp = buf;
    number(cash[seat], 1);
    show_right(S_ACCT, y);
    for (j = 0; j < NSYN; ++j) {
        bp = buf;
        n = coins[seat][j];
        if (n)
            number(n, 0);
        else
            put('.');
        px = S_COIN + 27 * j + 3 - width(buf, bp - buf) / 2;
        out(buf, bp - buf, y);
    }
    if (seat == turn)
        invert(y - 1, y + 6, 4, 314);
    clip = 7;
}

/* ---------------------------------------------------------------- prompts */

static void kit_info(void)
{
    bp = buf;
    text(T_EXPLOIT_P);
    for (j = 0; j < RULE_KITSIZE; ++j) {
        v = kit[j];
        put(' ');
        put('1' + j);
        put(':');
        if (v == NONE) {
            text("--");
        } else {
            node_name(v & 0x7F);
            if (v & 0x80) {             /* can't run now: the other way round */
                bp[-1] |= 0x80;
                bp[-2] |= 0x80;
            }
        }
    }
    set_info();
}

void display_prompt(void)
{
    asking = pr_kind | leaving;
    if (leaving) {                      /* Esc: the question waits; the prompt comes back after */
        bp = buf;
        text(T_LOGOFF " NOW? Y/N");
        ask_done();
        if (live())
            bar();
        return;
    }
    if (pr_kind == 'P' || (pr_kind == 0 && turn != NONE))
        kit_info();
    else if (pr_kind != 'S' && pr_kind != 0)
        info_n = 0;                     /* in the lobby the bar keeps the last result */
    bp = buf;
    switch (pr_kind) {
    case 0:
        if (turn < SEATS && turn != me) {
            text("WAITING FOR ");
            for (j = 0; j < 8 && seat_name[turn][j] != ' '; ++j)
                put(seat_name[turn][j]);
        }
        break;
    case 'S':
        set_ask("START WITH HOW MANY SEATS? 2-6");
        break;
    case 'P':
        set_ask("PRESS 1-6");           /* the kit is beside it */
        break;
    case 'F':                           /* pr[1] = available */
        text(T_INFILTRATION "! " T_BROKER " TO ");
        syn_letters(pr[1]);
        set_info();
        set_ask("PRESS A SYNDICATE LETTER");
        break;
    case 'K':                           /* pr[1] = ctx, pr[2] = options */
        text(pr[1] ? "TIE! ABSORB WHICH NEXT? " : "TIE! WHICH SURVIVES? ");
        syn_letters(pr[2]);
        set_info();
        set_ask("PRESS A SYNDICATE LETTER");
        break;
    case 'D':                           /* pr: absorbed, absorber, held, supply */
        text(T_HIVEJACK ": ");
        put(syn_letter[pr[2]]);
        text(" TAKES ");
        put(syn_letter[pr[1]]);
        text(". YOU HOLD ");
        number(pr[3], 0);
        put(' ');
        put(syn_letter[pr[1]]);
        set_info();
        set_ask(step ? T_SWAP " 2 FOR 1, HOW MANY? " : T_CASHOUT " HOW MANY? ");
        if (!complaint)
            textn(edit, edit_len);
        break;
    case 'B':                           /* pr: cash[2], flags, price[7], supply[7] */
        if (!(step && !edit_len)) {     /* nothing bought, at the end: just the question */
            text("BUY UP TO 3 " T_COIN ": ");
            textn(edit, edit_len);
            set_info();
        }
        set_ask(step ? T_NULLROUTE " AND END THE GAME? Y/N" : "LETTERS, DELETE, RETURN TO BUY");
        break;
    }
    ask_done();
    if (live())
        bar();
}

void display_clock(unsigned char s)
{
    if (!pr_kind)
        return;
    secs = s;
    if (live())
        clock_at();
}

void display_message(const char *s)
{
    asking = strchr(s, '?') != 0;
    bp = buf;
    text(s);
    ask_done();
    if (live())
        bar();
}

void display_game_over(void)
{
    bp = buf;
    if (winners & (winners - 1))        /* a tie: the names need the room */
        text("TOP " T_PLAYER_P ":");
    else
        text("GAME OVER. TOP " T_PLAYER ":");
    for (i = 0; i < SEATS; ++i)
        if (winners >> i & 1) {
            put(' ');
            for (j = 0; j < 8 && seat_name[i][j] != ' '; ++j)
                put(seat_name[i][j]);
        }
    set_info();
    if (live())
        bar();
}

/* ---------------------------------------------------------------- the menu bar */

void display_link(void)
{
    if (on_title)                       /* the title has its own REGION and STATUS */
        return;
    if (!live()) {
        missed_more |= M_LINK;
        return;
    }
    box(1, 11, 160, 318, P_WHITE);
    bp = buf;
    textn(region, region_len);
    put(':');
    text(link_up ? "CONNECTED" : "NO SIGNAL");
    show_right(315, 2);
    if (!link_up)
        invert(1, 11, px - width(buf, bp - buf) - 2, 316);
}

static const struct label menu_labels[] = {
    { "file", 6, 2, 0 },
    { 0 }
};

/* ---------------------------------------------------------------- the title */

/* The art: the Atari's screen codes, a cell a byte across, 8 lines down, from row 1. */
static void art(void)
{
    const unsigned char *p, *g;
    unsigned char *d;
    for (p = title_art; *p != 0xFF; p += 3 + p[2])
        for (i = 0; i < p[2]; ++i) {
            v = p[3 + i];
            if (!v)
                continue;
            g = font + (v & 0x7F) * 8;
            n = v & 0x80 ? 0xFF : 0;
            d = BITMAP + (ART_Y + 8 * (p[0] - 1)) * 40 + p[1] + i;
            for (j = 0; j < 8; ++j, d += 40)
                *d = g[j] ^ n;
        }
}

/* A value, if it changed: white behind it first, so the last one leaves nothing. */
static void field(unsigned char place, unsigned char y)
{
    if (!fresh_buf(place))
        return;
    box(y - 1, y + 8, T_VAL, 312, P_WHITE);   /* from y - 1: an inverse character's top */
    show(T_VAL, y);
}

extern unsigned char snd_on;            /* sound.s */

/* ON or OFF, the other way round */
static void on_off(unsigned char on)
{
    text(on ? "ON" : "OFF");
    bp[-1] |= 0x80;
    bp[-2] |= 0x80;
    if (!on)
        bp[-3] |= 0x80;
}

/* Each line that changes, drawn over in place. */
void display_title_update(void)
{
    if (!live() || !on_title)
        return;
    bp = buf;
    textn(region, region_len);
    field(S_REGION, T_Y);
    bp = buf;
    if (status_len)
        textn(status_text, status_len - 1);
    else if (status_code == NO_CARRIER)
        text("NO CARRIER");
    field(S_STATUS, T_Y + 10);
    bp = buf;
    textn(edit, edit_len);
    put(' ' | 0x80);                    /* the cursor */
    field(S_HANDLE, T_Y + 20);
    bp = buf;
    text("MOVE HINTS ");
    on_off(hints);
    field(S_HINTS, T_Y + 33);
    bp = buf;
    text("SOUND ");
    on_off(snd_on);
    field(S_SOUND, T_Y + 43);
}

static const struct label title_labels[] = {
    { "REGION", T_X, T_Y, SET_BOLD },
    { "STATUS", T_X, T_Y + 10, SET_BOLD },
    { "HANDLE", T_X, T_Y + 20, SET_BOLD },
    { "OPTION", T_X, T_Y + 33, L_INV }, /* the keys; not bold, which would overhang the inverse */
    { "SELECT", T_X, T_Y + 43, L_INV },
    { 0 }
};

static void title_paint(void)
{
    panel(14, 197, 3, 315);
    art();
    labels(title_labels);
    display_title_update();
}

static const struct area field_panels[] = {
    { 14, 97, 3, BX + RULE_COLS * CW + 2 },     /* the board */
    { 14, 97, HX, 315 },                        /* hives */
    { SY, SY + 58, 3, 315 },                    /* seats */
    { LY, 197, 3, 315 },                        /* log and bar */
    { 0xFF }
};
static const struct label field_labels[] = {
    { T_HIVE_P, HX + 4, 15, SET_BOLD },
    { T_PLAYER, S_NAME, SY + 1, SET_BOLD },
    { T_ACCOUNT, S_ACCT, SY + 1, SET_BOLD | L_RIGHT },
    { 0 }
};

/* ---------------------------------------------------------------- the whole screen */

/* Everything, from the game state and the log and bar kept here. */
static void repaint(void)
{
    all = 1;
    box(0, 199, 0, 319, P_DITHER);
    box(0, 12, 0, 319, P_WHITE);        /* the menu bar */
    FrameRectangle(0xFF);
    labels(menu_labels);
    if (on_title) {
        title_paint();
        bar();
        all = 0;
        return;
    }
    display_link();
    panels(field_panels);
    labels(field_labels);
    /* the board */
    for (j = 0; j < RULE_COLS; ++j) {
        bp = buf;
        put('A' + j);
        show(BX + CW * j + (CW - width(buf, 1)) / 2, 15);
    }
    for (j = 0; j < RULE_ROWS; ++j)
        rom('0' + j, BY + CH * j, 4);
    for (j = 0; j < NODES; ++j)
        display_node(j);
    display_hives();
    last_line();
    /* seats */
    bold = SET_BOLD;
    for (j = 0; j < NSYN; ++j) {
        bp = buf;
        put(syn_letter[j]);
        px = S_COIN + 27 * j + 3 - width(buf, 1) / 2;
        out(buf, 1, SY + 1);
    }
    bold = 0;
    for (i = 0; i < SEATS; ++i)
        display_seat(i);
    /* log and bar */
    log_lines();
    bar();
    all = 0;
}

/* After a menu: the screen is as it was before it, so what changed meanwhile, drawn now. */
static void refresh(void)
{
    unsigned char k;                    /* not i, j or n: the drawing uses those */
    if (on_title) {
        display_title_update();
    } else {
        if (missed_more & M_LINK)
            display_link();
        display_hives();
        for (k = 0; k < SEATS; ++k)
            if (missed_seat >> k & 1)
                display_seat(k);
        for (k = 0; k < NODES; ++k)
            if (missed_node[k >> 3] & 1 << (k & 7))
                display_node(k);
        if (missed_more & M_LOG)
            log_lines();
    }
    memset(missed_node, 0, sizeof missed_node);
    missed_seat = missed_more = 0;
    bar();
}

/* The screen ours, with the pointer, and everything drawn. GEOS's colours stay, the DeskTop's. */
static void take(void)
{
    gui_on = 1;
    hold = 0;
    menu = item = 0xFF;
    dispBufferOn = ST_WR_FORE;
    MouseUp();
    UseSystemFont();
    repaint();
}

void display_title_screen(void)
{
    if (!gui_ready)
        gui_init();
    on_title = asking = 1;              /* a handle to type */
    secs = 0;                           /* no question here, so no seconds left */
    log_count = log_back = 0;           /* a new game, a new log */
    info_n = borrowed = 0;
    bp = buf;
    text(refused == REFUSED_LATE ? "TOO LATE: THE LAST SEAT WAS TAKEN"
         : refused == REFUSED_HANDLE ? "THAT HANDLE IS TAKEN" : "TYPE A HANDLE, THEN RETURN");
    ask_done();
    take();
}

void display_playfield(void)
{
    on_title = gui_on = 0;              /* the bar's text first, not drawn yet */
    info_n = ask_n = borrowed = 0;
    display_prompt();
    take();
}


/* ---------------------------------------------------------------- the menu */

/* One menu, file, drawn here, GEOS-like, and read from the pointer by gui_poll(): a click on its
 * title opens it, a click on an item chooses it, a click anywhere else or a key closes it. The
 * game goes on while it's open; what it would draw waits, and is drawn after, over what the menu
 * covered, put back. Move hints and sound are the title screen's (OPTION, SELECT). */
#define M_TITLE 31                      /* the title's right end */
#define M_RIGHT 99                      /* the widest item, 30 to spare (BSW 9: "Quit to DeskTop" 69) */
#define M_BOT   35                      /* two items, 10 apart from 14 */

extern const unsigned char *key_queue;  /* input.c: keys to give the game, one a call */

static const struct label menu_items[] = {
    { TM_LOGOFF, 12, 15, 0 },
    { "Esc", M_RIGHT - 4, 15, L_RIGHT },
    { "Quit to DeskTop", 12, 25, 0 },
    { 0 }
};
static const unsigned char quit_keys[] = { K_ESC, 'Y', K_ESC, 'Y', 0 };  /* log off, then quit */
static const unsigned char logoff_keys[] = { K_ESC, 0 };   /* the Y/N is the player's */

static unsigned char saved[13 * 37];    /* what the menu covers: 13 bytes by 37 lines */
#define SV_BYTES 13
#define SV_ROWS  37

static void close_menu(void)
{
    unsigned char *p = saved;
    if (menu == 0xFF)
        return;
    menu = item = 0xFF;
    hold = 0;
    for (i = 0; i < SV_ROWS; ++i, p += SV_BYTES)
        memcpy(BITMAP + (1 + i) * 40, p, SV_BYTES);
    if (missed) {
        missed = 0;
        refresh();
    }
}

static void open_menu(void)
{
    unsigned char *p = saved;
    menu = 0;
    item = 0xFF;
    hold = 1;
    missed = 0;
    for (i = 0; i < SV_ROWS; ++i, p += SV_BYTES)
        memcpy(p, BITMAP + (1 + i) * 40, SV_BYTES);
    invert(1, 11, 1, M_TITLE - 1);
    panel(12, M_BOT, 0, M_RIGHT);
    labels(menu_items);
}

/* The item k's line, highlighted or back */
static void flip(unsigned char k)
{
    invert(14 + 10 * k, 23 + 10 * k, 1, M_RIGHT - 1);
}

/* The pointer and a key (0 if none) from in_key(): what the game should take as the key. */
unsigned char __fastcall__ gui_poll(unsigned char k, unsigned x, unsigned char y, unsigned char down)
{
    unsigned char click = down && !was_down, m;
    was_down = down;
    if (!gui_on)
        return k;
    if (menu != 0xFF) {
        m = 0xFF;                       /* the item under the pointer */
        if (x && x < M_RIGHT && y >= 14 && y < M_BOT - 1)
            m = (y - 14) / 10;
        if (m != item) {
            if (item != 0xFF)
                flip(item);
            if (m != 0xFF)
                flip(m);
            item = m;
        }
        if (k || click)
            close_menu();
        if (click && m == 0) {          /* Log off */
            if (!on_title)              /* not jacked in yet: nothing to leave */
                key_queue = logoff_keys;
        } else if (click && m == 1) {   /* Quit to DeskTop */
            key_queue = on_title ? quit_keys + 2 : quit_keys;
        }
        return 0;
    }
    if (click && x >= 298 && y > LY && y <= LY + 16)    /* the log's arrows */
        return y <= LY + 8 ? K_UP : K_DOWN;
    if (click && y < 12 && x < M_TITLE)
        open_menu();
    return k;
}
