/* What the Amiga files share. The window and the libraries belong to display_text.c / start.c. */
#ifndef AMIGA_H
#define AMIGA_H

struct Window;
extern struct Window *amiga_win;        /* the game window; NULL until start.c has opened it */

void amiga_present(void);               /* draw what changed since the last call */
unsigned char amiga_input_open(void);   /* console.device and the vblank counter; 0 on failure */
void amiga_input_close(void);           /* undo it, safe if never opened */

#endif
