/* GEOS on the Atari: keys, the clock and quitting. Keys come from GEOS's own
 * keyboard queue (GetNextChar), which geos-atari's driver fills from the
 * keyboard interrupt: unshifted letters arrive in lowercase, Esc as GEOS's
 * left arrow, Shift-Delete as backspace, and OPTION held with a key sets its
 * bit 7 (the shortcut key, C= on a C64). OPTION and SELECT on their own are read
 * from CONSOL once a frame, as on the Atari: OPTION flips the move hints,
 * SELECT the sound. The ticks are frame.s's. There are no instruction pages
 * yet, so nothing returns K_HELP.
 *
 * in_key() also draws what changed on the screen (screen.s): the game loops
 * call it on every pass. */
#include "../platform.h"
#include "../display.h"

#define CONSOL  (*(volatile unsigned char *)0xD01F)  /* bit clear = key held */
#define PAL     (*(volatile unsigned char *)0xD014)  /* GTIA: low bits %0001 on PAL, %1111 on NTSC */

/* GEOS's key codes (its const.inc) */
#define G_RETURN    13
#define G_BACKSPACE 8
#define G_DELETE    29
#define G_LARROW    20                  /* geos-atari: Esc */

char GetNextChar(void);                 /* geos-cbm.lib */
void MouseUp(void);
void EnterDeskTop(void);
void screen_flush(void);                /* screen.s */
void screen_done(void);
void frame_done(void);                  /* frame.s */
extern volatile unsigned char geos_tick;

static unsigned char k, frame, held, pressed;

unsigned char in_key(void)
{
    screen_flush();
    if (frame != geos_tick) {
        frame = geos_tick;
        k = ~CONSOL & 6;                /* OPTION, SELECT */
        pressed = k & ~held;
        held = k;
        if (pressed & 2)
            snd_toggle();
        if (pressed & 4)
            return K_OPTION;
    }
    k = GetNextChar() & 0x7F;           /* OPTION with a key: the key */
    if (k >= 'a' && k <= 'z')
        return k - ('a' - 'A');
    switch (k) {
    case G_RETURN:
        return K_RETURN;
    case G_BACKSPACE:
    case G_DELETE:
        return K_DELETE;
    case G_LARROW:
        return K_ESC;
    }
    return k >= ' ' && k < 0x60 ? k : 0;    /* not GEOS's control codes: F1 is 1 */
}

unsigned char ticks(void)
{
    return geos_tick;
}

unsigned char ticks_per_sec(void)
{
    return PAL & 0x0E ? 60 : 50;
}

/* Back to GEOS's DeskTop, which redraws the screen. */
void platform_quit(void)
{
    frame_done();
    screen_done();
    MouseUp();
    EnterDeskTop();
}

/* The instruction pages, should a key ever ask for them: none on this disk yet. */
void help_open(void)
{
}

unsigned char __fastcall__ help_key(unsigned char c)
{
    (void)c;
    return 0;
}
