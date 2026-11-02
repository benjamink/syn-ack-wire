/* What the Amiga files share. The window and the libraries belong to display_text.c / start.c. */
#ifndef AMIGA_H
#define AMIGA_H

struct Window;
extern struct Window *amiga_win;        /* the game window; NULL until start.c has opened it */

void amiga_pointer_set(struct Window *w);    /* the game's arrow for this window */
void amiga_pointer_clear(struct Window *w);  /* the system's back, and the image freed */
void dialog_sync(void);                 /* open, redraw, change or close the dialog the game state calls for */
void dialog_quit_ask(unsigned char on); /* the title's quit question, over the dimmed title */
unsigned char amiga_snd_on(void);       /* the sound switch, for the menu's tick */
unsigned char amiga_table_view(void);   /* the seats are shown as a table, for the menu's tick */
void menus_update(void);                /* the menu strip follows the screen: off under a window, Region only on the title */
void menus_refresh(void);               /* the ticks, just before the menu opens */
unsigned short menus_next(unsigned short code);   /* the next pick of a chain, or MENUNULL */
void menus_region(unsigned char n);     /* the title's Region menu: the next dial goes there */
unsigned char amiga_in_play(void);      /* the playfield is up (else the title) */
unsigned char amiga_helping(void);      /* the instructions are up */
unsigned char dialog_quit_open(void);   /* the title's quit question is up */
void amiga_set_server(const char *url); /* the server the next dial uses, kept with the handle (prefs_io.c) */
void menus_close(void);                 /* take the strip off the window */
void amiga_toggle_table(void);          /* F4 or a click on the seats: cards or table */
void dialog_forget(void);               /* a full repaint is coming: drop what the window saved */
void display_kit_stale(void);           /* the kit panel must be drawn again at the next prompt */
unsigned char amiga_scroll_others(signed char d);   /* NTSC: the seat list up (-1) or down (1); 1 if it took the click */
unsigned char dialog_up(void);          /* a dialog is open: the playfield's parts keep their hands off the screen */
void display_repaint(void);             /* the whole playfield again from the game state */
unsigned char amiga_display_open(void); /* the screen and window; 0 on failure, with nothing left open */
void amiga_display_close(void);         /* close the window then the screen, whichever opened */
void amiga_click(unsigned char button, short x, short y);   /* a press at window pixel (x, y), as the mouse sends it */
unsigned char amiga_input_open(void);   /* console.device and the vblank counter; 0 on failure */
void amiga_input_close(void);           /* undo it, safe if never opened */

#endif
