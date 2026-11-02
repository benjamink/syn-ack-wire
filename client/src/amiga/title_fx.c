#include <string.h>
#include "../config.h"
#include "../platform.h"
#include "amiga.h"
#include "gfx.h"
#include "fxmath.h"
#include "title_fx.h"

#define SC_Y    66                      /* the picture's ticker strip: rows 66..83 are pen 0 */
#define SC_H    18
#define SCROLL_STEP 3                   /* pixels a beat */

static const char scroll_text[] = "   WELCOME TO " TH_TITLE "   TYPE A HANDLE AND PRESS RETURN TO " T_JACKIN "   ";

static unsigned char fill, next, letters;   /* the blank columns at the right, the next letter, the letters written */

void title_fx_paint(void)
{
    fill = 0;
    if (!amiga_layout.ntsc)             /* in the dark band above the floor */
        gfx_text(12, 194, "FUJINET RS-232", 14, PEN_STEEL, PEN_BG0);
}

/* The text flows left and a letter is written only where it comes in, at a height the sine gives it: the wave travels with
 * the text, and a beat costs one scroll. */
void title_fx_step(void)
{
    struct rect r;
    unsigned char len = sizeof scroll_text - 1;

    r.x = 0;
    r.y = SC_Y;
    r.w = 320;
    r.h = SC_H;
    gfx_scroll(&r, SCROLL_STEP, PEN_BG0);
    fill += SCROLL_STEP;
    while (fill >= 8) {
        fill -= 8;
        gfx_text(320 - fill - 8, 72 + fx_wave((unsigned char)(letters * 12)), &scroll_text[next], 1, PEN_WHITE, PEN_BG0);
        next = (next + 1) % len;
        ++letters;
    }
}
