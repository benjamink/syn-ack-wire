/* The title's scroller, under the picture's logo: the text runs left on the blitter and a letter is written only where it comes in. */
#ifndef AMIGA_TITLE_FX_H
#define AMIGA_TITLE_FX_H

void title_fx_paint(void);              /* once the title is drawn: the scroller starts over */
void title_fx_step(void);               /* one beat */

#endif
