#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "amiga.h"
#include "gfx.h"

#ifndef GFX_DEPTH
#define GFX_DEPTH 5                      /* 32 colours */
#endif
#define FONT_H 8
#define BASELINE 6

struct layout amiga_layout;
struct Window *amiga_win;

static struct Screen *scr;
static struct RastPort *rp;
static struct TextFont *font;
static UWORD *checker;                  /* the dim pattern, then the hatch's: the blitter reads them, so they sit in chip RAM */
static UWORD *stripes;

static struct BitMap aside;             /* gfx_save's copy */
static struct RastPort aside_rp;
static struct rect aside_r;
static unsigned char aside_planes;

static struct TextAttr topaz = { (STRPTR)"topaz.font", FONT_H, 0, FPF_ROMFONT };

unsigned char gfx_open(unsigned short *width, unsigned short *height)
{
    struct NewScreen ns;
    struct NewWindow nw;
    unsigned char *p;
    unsigned short h = GfxBase->NormalDisplayRows;           /* 256 PAL, 200 NTSC */

    for (p = (unsigned char *)&ns; p < (unsigned char *)(&ns + 1); ++p)
        *p = 0;
    for (p = (unsigned char *)&nw; p < (unsigned char *)(&nw + 1); ++p)
        *p = 0;
    ns.Width = 320;
    ns.Height = h;
    ns.Depth = GFX_DEPTH;
    ns.DetailPen = 0;
    ns.BlockPen = 1;
    ns.Type = CUSTOMSCREEN;
    ns.Font = &topaz;
    ns.DefaultTitle = (UBYTE *)"SYN-ACK WIRE";
    scr = OpenScreen(&ns);
    if (!scr)
        return 0;
    LoadRGB4(&scr->ViewPort, (UWORD *)amiga_palette, 32);

    nw.Width = 320;
    nw.Height = h;
    nw.DetailPen = 0;
    nw.BlockPen = 1;
    nw.IDCMPFlags = IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS | IDCMP_MENUPICK | IDCMP_MENUVERIFY;
    nw.Flags = BORDERLESS | BACKDROP | ACTIVATE | RMBTRAP | NOCAREREFRESH;
    nw.Screen = scr;
    nw.Type = CUSTOMSCREEN;
    amiga_win = OpenWindow(&nw);
    if (!amiga_win) {
        gfx_close();
        return 0;
    }
    amiga_pointer_set(amiga_win);
    ShowTitle(scr, FALSE);              /* the title bar would cover row 0 on a 200-line screen */
    rp = amiga_win->RPort;
    if ((font = OpenFont(&topaz)) != 0)
        SetFont(rp, font);
    SetDrMd(rp, JAM2);
    checker = (UWORD *)AllocMem(12, MEMF_CHIP);
    if (checker) {
        checker[0] = 0xAAAA;
        checker[1] = 0x5555;
        stripes = checker + 2;          /* four rows, each shifted a pixel: diagonals, 4 pixels apart */
        stripes[0] = 0x8888;
        stripes[1] = 0x4444;
        stripes[2] = 0x2222;
        stripes[3] = 0x1111;
    }
    layout_make(&amiga_layout, h);
    *width = 320;
    *height = h;
    return 1;
}

void gfx_close(void)
{
    gfx_restore();
    rp = 0;
    if (checker)
        FreeMem(checker, 12);
    checker = stripes = 0;
    menus_close();
    amiga_pointer_clear(amiga_win);
    if (amiga_win)
        CloseWindow(amiga_win);
    amiga_win = 0;
    if (font)
        CloseFont(font);
    font = 0;
    if (scr)
        CloseScreen(scr);
    scr = 0;
}

void gfx_rect(const struct rect *r, unsigned char pen)
{
    if (r->w <= 0 || r->h <= 0)
        return;
    SetAPen(rp, pen);
    RectFill(rp, r->x, r->y, r->x + r->w - 1, r->y + r->h - 1);
}

static void line(short x0, short y0, short x1, short y1, unsigned char pen)
{
    SetAPen(rp, pen);
    Move(rp, x0, y0);
    Draw(rp, x1, y1);
}

void gfx_frame(const struct rect *r, unsigned char pen)
{
    short x1 = r->x + r->w - 1, y1 = r->y + r->h - 1;
    if (r->w < 2 || r->h < 2)
        return;
    line(r->x, r->y, x1, r->y, pen);
    line(x1, r->y, x1, y1, pen);
    line(x1, y1, r->x, y1, pen);
    line(r->x, y1, r->x, r->y, pen);
}

void gfx_bevel(const struct rect *r, unsigned char fill, unsigned char hi, unsigned char lo)
{
    short x1 = r->x + r->w - 1, y1 = r->y + r->h - 1;
    gfx_rect(r, fill);
    if (r->w < 2 || r->h < 2)
        return;
    line(r->x, y1, r->x, r->y, hi);
    line(r->x, r->y, x1, r->y, hi);
    line(x1, r->y + 1, x1, y1, lo);
    line(x1, y1, r->x + 1, y1, lo);
}

void gfx_text(short x, short y, const char *s, unsigned char n, unsigned char pen, unsigned char bg)
{
    SetAPen(rp, pen);
    SetBPen(rp, bg);
    Move(rp, x, y + BASELINE);
    Text(rp, (STRPTR)s, n);
}

void gfx_text_right(short right, short y, const char *s, unsigned char n, unsigned char pen, unsigned char bg)
{
    gfx_text(right - n * 8, y, s, n, pen, bg);
}

void gfx_glyph(short x, short y, unsigned char ch, unsigned char scale, unsigned char pen)
{
    ULONG loc;
    unsigned short off, w, row, bit, bitpos;
    short start;
    const UBYTE *data;

    if (!font || ch < font->tf_LoChar || ch > font->tf_HiChar)
        return;
    loc = ((const ULONG *)font->tf_CharLoc)[ch - font->tf_LoChar];
    off = loc >> 16;
    w = loc & 0xFFFF;
    SetAPen(rp, pen);
    for (row = 0; row < font->tf_YSize; ++row) {
        data = (const UBYTE *)font->tf_CharData + row * font->tf_Modulo;
        start = -1;
        for (bit = 0; bit <= w; ++bit) {            /* runs of lit pixels, one RectFill each */
            bitpos = off + bit;
            if (bit < w && (data[bitpos >> 3] & (0x80 >> (bitpos & 7)))) {
                if (start < 0)
                    start = bit;
            } else if (start >= 0) {
                RectFill(rp, x + start * scale, y + row * scale, x + bit * scale - 1, y + row * scale + scale - 1);
                start = -1;
            }
        }
    }
}

void gfx_hatch(const struct rect *r, unsigned char pen_a, unsigned char pen_b)
{
    if (!stripes || r->w <= 0 || r->h <= 0)
        return;
    SetAPen(rp, pen_a);
    SetBPen(rp, pen_b);
    SetAfPt(rp, stripes, 2);            /* 4 rows of 16 pixels */
    RectFill(rp, r->x, r->y, r->x + r->w - 1, r->y + r->h - 1);
    SetAfPt(rp, 0, 0);
}

void gfx_dim(const struct rect *r, unsigned char pen)
{
    if (!checker || r->w <= 0 || r->h <= 0)
        return;
    SetAPen(rp, pen);
    SetDrMd(rp, JAM1);
    SetAfPt(rp, checker, 1);            /* 2 rows of 16 pixels: alternate dots */
    RectFill(rp, r->x, r->y, r->x + r->w - 1, r->y + r->h - 1);
    SetAfPt(rp, 0, 0);
    SetDrMd(rp, JAM2);
}

/* what start.c calls: the screen and window, before and after the game */
unsigned char amiga_display_open(void)
{
    unsigned short w, h;
    return gfx_open(&w, &h);
}

void amiga_display_close(void)
{
    gfx_close();
}

/* A dialog opens over the playfield and closes again many times a game: copying what it covers aside and back is two
 * blits, where drawing the playfield again is some fifteen hundred graphics calls. */
unsigned char gfx_save(const struct rect *r)
{
    unsigned char i;
    if (!rp || aside_planes)
        return 0;
    InitBitMap(&aside, GFX_DEPTH, r->w, r->h);
    for (i = 0; i < GFX_DEPTH; ++i) {
        aside.Planes[i] = (PLANEPTR)AllocRaster(r->w, r->h);
        if (!aside.Planes[i])
            break;
        ++aside_planes;
    }
    if (aside_planes != GFX_DEPTH) {
        while (aside_planes)
            FreeRaster(aside.Planes[--aside_planes], r->w, r->h);
        return 0;
    }
    aside_r = *r;
    InitRastPort(&aside_rp);
    aside_rp.BitMap = &aside;
    ClipBlit(rp, r->x, r->y, &aside_rp, 0, 0, r->w, r->h, 0xC0);
    return 1;
}

void gfx_discard(void)
{
    while (aside_planes)
        FreeRaster(aside.Planes[--aside_planes], aside_r.w, aside_r.h);
}

void gfx_restore(void)
{
    if (!aside_planes)
        return;
    if (rp)
        ClipBlit(&aside_rp, 0, 0, rp, aside_r.x, aside_r.y, aside_r.w, aside_r.h, 0xC0);
    while (aside_planes)
        FreeRaster(aside.Planes[--aside_planes], aside_r.w, aside_r.h);
}
