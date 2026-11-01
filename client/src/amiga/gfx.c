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

#define FONT_H 8
#define BASELINE 6

struct layout amiga_layout;
struct Window *amiga_win;

static struct Screen *scr;
static struct RastPort *rp;
static struct TextFont *font;
static UWORD *checker;                  /* the dim pattern: the blitter reads it, so it sits in chip RAM */

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
    ns.Depth = 5;
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
    nw.IDCMPFlags = IDCMP_RAWKEY;
    nw.Flags = BORDERLESS | BACKDROP | ACTIVATE | RMBTRAP | NOCAREREFRESH;
    nw.Screen = scr;
    nw.Type = CUSTOMSCREEN;
    amiga_win = OpenWindow(&nw);
    if (!amiga_win) {
        gfx_close();
        return 0;
    }
    ShowTitle(scr, FALSE);              /* the title bar would cover row 0 on a 200-line screen */
    rp = amiga_win->RPort;
    if ((font = OpenFont(&topaz)) != 0)
        SetFont(rp, font);
    SetDrMd(rp, JAM2);
    checker = (UWORD *)AllocMem(4, MEMF_CHIP);
    if (checker) {
        checker[0] = 0xAAAA;
        checker[1] = 0x5555;
    }
    layout_make(&amiga_layout, h);
    *width = 320;
    *height = h;
    return 1;
}

void gfx_close(void)
{
    rp = 0;
    if (checker)
        FreeMem(checker, 4);
    checker = 0;
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

void gfx_hatch(const struct rect *r, unsigned char pen_a, unsigned char pen_b)
{
    short x, y;
    for (y = r->y; y < r->y + r->h; ++y)
        for (x = r->x; x < r->x + r->w; ++x) {
            SetAPen(rp, ((x + y) & 3) < 2 ? pen_a : pen_b);
            WritePixel(rp, x, y);
        }
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
