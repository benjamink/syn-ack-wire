/* GEOS on the Atari: keys, the clock and quitting. Keys come from GEOS's own
 * keyboard queue (GetNextChar), which geos-atari's driver fills from the
 * keyboard interrupt: unshifted letters arrive in lowercase, Esc as GEOS's
 * left arrow, Shift-Delete as backspace, and OPTION held with a key sets its
 * bit 7 (the shortcut key, C= on a C64). OPTION and SELECT on their own are read
 * from CONSOL once a frame, as on the Atari: OPTION flips the move hints,
 * SELECT the sound. The ticks are frame.s's. There are no instruction pages
 * yet, so nothing returns K_HELP.
 *
 * The game loops call in_key() on every pass. It is where the keyboard's IRQ
 * may come in: the program runs with IRQs masked (screen.s says why), and a
 * key held back meanwhile arrives here. It passes the key and GEOS's pointer
 * to display_gui.c's menu, which may take them, or give a key of their own;
 * a menu item that needs more than one (Quit: Esc, Y, then Esc, Y on the title)
 * queues them in key_queue. */
#include "../platform.h"
#include "../display.h"

#define CONSOL  (*(volatile unsigned char *)0xD01F)  /* bit clear = key held */
#define PAL     (*(volatile unsigned char *)0xD014)  /* GTIA: low bits %0001 on PAL, %1111 on NTSC */

/* GEOS's key codes (its const.inc) */
#define G_RETURN    13
#define G_BACKSPACE 8
#define G_DELETE    29
#define G_LARROW    20                  /* geos-atari: Esc */
#define G_UP        16                  /* geos-atari: CTRL+- and CTRL+=, the arrows */
#define G_DOWN      17

char GetNextChar(void);                 /* geos-cbm.lib */
void MouseUp(void);
void EnterDeskTop(void);
unsigned char __fastcall__ gui_poll(unsigned char k, unsigned x, unsigned char y, unsigned char down);
#define mouseXPos   (*(volatile unsigned *)0x3A)    /* GEOS's pointer */
#define mouseYPos   (*(volatile unsigned char *)0x3C)
#define TRIG0       (*(volatile unsigned char *)0xD010)  /* the fire button: 0 = down. Not GEOS's
                                           mouseData: its driver sets that only on a change */
void frame_done(void);                  /* frame.s */
extern volatile unsigned char geos_tick;

static unsigned char k, frame, held, pressed;
const unsigned char *key_queue;         /* display_gui.c's menu: keys for the game, one a call */

static unsigned char key(void)
{
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
    case G_UP:
        return K_UP;
    case G_DOWN:
        return K_DOWN;
    }
    return k >= ' ' && k < 0x60 ? k : 0;    /* not GEOS's control codes: F1 is 1 */
}

unsigned char in_key(void)
{
    __asm__ ("cli");                    /* a key waiting in POKEY: its IRQ now */
    __asm__ ("nop");
    __asm__ ("sei");
    if (frame != geos_tick) {
        frame = geos_tick;
        k = ~CONSOL & 6;                /* OPTION, SELECT */
        pressed = k & ~held;
        held = k;
        if (pressed & 2) {
            snd_toggle();
            display_title_update();     /* the title shows it */
        }
        if (pressed & 4)
            return K_OPTION;
    }
    if (key_queue) {
        k = *key_queue++;
        if (!*key_queue)
            key_queue = 0;
        return k;
    }
    return gui_poll(key(), mouseXPos, mouseYPos, !(TRIG0 & 1));
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
    MouseUp();
    __asm__ ("cli");                    /* the DeskTop's keyboard */
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
