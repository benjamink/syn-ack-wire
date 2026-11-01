/* The game's text screen on a 320-wide Amiga screen in Topaz 8: the MS-DOS display (src/msdos/display_text.c,
 * built with PC_SCREEN=pc_screen) fills pc_screen, and amiga_present() draws what changed. */
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "../platform.h"
#include "amiga.h"
#include "render.h"

unsigned short pc_screen[25 * 80];      /* the display's cells; it uses the first 40x24 */

struct Window *amiga_win;

static struct Screen *scr;
static struct RastPort *rp;
static unsigned short shown[R_ROWS * R_COLS];
static unsigned short top;              /* where row 0 starts: the 24 rows centred on the screen */

static struct TextAttr topaz = { (STRPTR)"topaz.font", 8, 0, FPF_ROMFONT };
static const UWORD colours[4] = { 0x001, 0x5F5, 0x0A0, 0xFF5 };   /* background, text, dim, bright */

static void draw_text(unsigned char row, unsigned char col, const char *s, unsigned char n, unsigned char inverse)
{
    SetAPen(rp, inverse ? 0 : 1);
    SetBPen(rp, inverse ? 1 : 0);
    Move(rp, col * 8, top + row * 8 + 6);
    Text(rp, (STRPTR)s, n);
}

void amiga_present(void)
{
    if (rp)
        amiga_render(pc_screen, shown, (ticks() >> 4) & 1, draw_text);
}

unsigned char amiga_display_open(void)
{
    struct NewScreen ns;
    struct NewWindow nw;
    unsigned short height = GfxBase->NormalDisplayRows;      /* 256 PAL, 200 NTSC */
    unsigned char *p;
    struct TextFont *font;

    for (p = (unsigned char *)&ns; p < (unsigned char *)(&ns + 1); ++p)
        *p = 0;
    for (p = (unsigned char *)&nw; p < (unsigned char *)(&nw + 1); ++p)
        *p = 0;
    ns.Width = 320;
    ns.Height = height;
    ns.Depth = 2;
    ns.DetailPen = 0;
    ns.BlockPen = 1;
    ns.Type = CUSTOMSCREEN;
    ns.Font = &topaz;
    ns.DefaultTitle = (UBYTE *)"SYN-ACK WIRE";
    scr = OpenScreen(&ns);
    if (!scr)
        return 0;
    LoadRGB4(&scr->ViewPort, (UWORD *)colours, 4);

    nw.Width = 320;
    nw.Height = height;
    nw.DetailPen = 0;
    nw.BlockPen = 1;
    nw.IDCMPFlags = IDCMP_RAWKEY;
    nw.Flags = BORDERLESS | BACKDROP | ACTIVATE | RMBTRAP | NOCAREREFRESH;
    nw.Screen = scr;
    nw.Type = CUSTOMSCREEN;
    amiga_win = OpenWindow(&nw);
    if (!amiga_win)
        return 0;
    ShowTitle(scr, FALSE);              /* the title bar would cover row 0 on a 200-line screen */
    rp = amiga_win->RPort;
    if ((font = OpenFont(&topaz)) != 0)
        SetFont(rp, font);
    SetDrMd(rp, JAM2);
    top = (height - R_ROWS * 8) / 2;
    return 1;
}

void amiga_display_close(void)
{
    rp = 0;
    if (amiga_win)
        CloseWindow(amiga_win);
    amiga_win = 0;
    if (scr)
        CloseScreen(scr);
    scr = 0;
}
