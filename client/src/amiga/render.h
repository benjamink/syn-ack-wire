/* The text screen (40x24 cells, character then attribute, as the MS-DOS display writes them) to
 * runs of text for the Amiga to draw. Pure, so it is tested on the host. */
#ifndef AMIGA_RENDER_H
#define AMIGA_RENDER_H

#define R_COLS 40
#define R_ROWS 24

/* Draws n glyphs at (row, col), reversed if inverse. */
typedef void (*render_text)(unsigned char row, unsigned char col, const char *s, unsigned char n,
                            unsigned char inverse);

/* Draw every cell of `screen` that differs from `shown` (all zero at first), as runs along a row of
 * cells that look alike, and record them in `shown`. blink: 0 or 1, which half of the flash a cell
 * with the attribute's top bit is in. Returns the number of runs drawn. */
unsigned short amiga_render(const unsigned short *screen, unsigned short *shown, unsigned char blink,
                            render_text text);

/* The Topaz glyph for a code page 437 character. */
unsigned char amiga_glyph(unsigned char ch);

#endif
