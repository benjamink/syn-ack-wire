/* A mouse click becomes the keys the keyboard would type for the open prompt; game.c decides whether they are legal.
 * Pure: it reads the game state (game.h) and the layout, and never writes either. Tested on the host. */
#ifndef AMIGA_UI_H
#define AMIGA_UI_H

#include "layout.h"

#define UI_LEFT  1
#define UI_RIGHT 2
#define UI_KEYS_MAX 6                   /* a hivejack's two digits and Return, after up to two Deletes */

/* The keys for a press of `button` at (x, y), written to keys[]; returns how many (0: the click means nothing). */
unsigned char ui_click(const struct layout *l, short x, short y, unsigned char button, unsigned char *keys);

void ui_title_mode(unsigned char on);   /* the title screen is up: its buttons answer, not the playfield */
void ui_ask_yes_no(unsigned char on);   /* a Y/N question is on screen (the title's quit): YES is Y, any other click N */
void ui_reset(void);                    /* forget the hivejack stepper */
unsigned char ui_dirty(void);           /* the dialog changed (a stepper moved): draw it again; clears the flag */
unsigned char ui_count(void);           /* the hivejack stepper's count, for drawing it */

#endif
