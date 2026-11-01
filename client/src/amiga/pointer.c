/* The mouse pointer: the mockups' arrow as a Kickstart 1.3 sprite image (two bitplanes, so colours 1-3 of the sprite's
 * registers 17-19 in palette.h). The data is built into chip RAM, which the sprite DMA reads, when the window opens. */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "amiga.h"

#define ROWS 14

/* X: the outline (sprite colour 1), O: the fill (2) */
static const char *const arrow[ROWS] = {
    "X.........", "XX........", "XOX.......", "XOOX......", "XOOOX.....", "XOOOOX....", "XOOOOOX...",
    "XOOOOOOX..", "XOOOOXXXX.", "XOXOOX....", "XX.XOOX...", "X..XOOX...", "....XOOX..", "....XXXX..",
};

static UWORD *image;
#define IMAGE_BYTES ((2 + ROWS * 2 + 2) * 2)

void amiga_pointer_set(struct Window *w)
{
    unsigned char r, c;
    if (!image)
        image = (UWORD *)AllocMem(IMAGE_BYTES, MEMF_CHIP | MEMF_CLEAR);
    if (!image)
        return;                         /* the system's pointer stays */
    for (r = 0; r < ROWS; ++r)
        for (c = 0; arrow[r][c]; ++c) {
            if (arrow[r][c] == 'X')
                image[2 + r * 2] |= 0x8000 >> c;
            else if (arrow[r][c] == 'O')
                image[2 + r * 2 + 1] |= 0x8000 >> c;
        }
    SetPointer(w, image, ROWS, 16, 0, 0);
}

void amiga_pointer_clear(struct Window *w)
{
    if (w)
        ClearPointer(w);
    if (image)
        FreeMem(image, IMAGE_BYTES);
    image = 0;
}
