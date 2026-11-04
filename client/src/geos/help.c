/* GEOS on the Atari: the instruction pages, on the title or mid-game. They are a GEOS file on
 * the disk (tools/help_geos.py), a page a record, found by its class, so whatever the file is
 * called; a page is 960 screen codes, drawn in the Atari's ROM font (screen.s), as the Atari
 * shows it. The game goes on underneath, as the server doesn't wait: display_gui.c's gui_cover()
 * puts the screen into GEOS's background screen and points GEOS's drawing at it, so the game
 * keeps drawing there, unseen, while the pages are on the screen, and puts it back when they
 * close. A new turn's chord is the cue to go back.
 *
 * Keys: > or * (the right arrow) the next page, < or + (the left arrow) the one before, Esc,
 * Return, HELP or ? back. A click: on the left third the page before, the right third the next,
 * between them back. The last page read stays, so they open again where they were left. */
#include <string.h>
#include <geos.h>
#include "../platform.h"
#include "../display.h"

#define usedRecords (*(unsigned char *)0x8497)   /* the open VLIR file's records */

void __fastcall__ help_draw(const unsigned char *page);   /* screen.s */
void __fastcall__ gui_cover(unsigned char on);            /* display_gui.c */

unsigned char helping;                  /* display_gui.c's pointer turns pages meanwhile */
static unsigned char page[960], at;     /* the page read, and which (0: none) */
static char name[17];                   /* the file's, once found */
static const char nofile[] = "NO HELP FILE ON THIS DISK. ESC:EXIT";

/* Page n into page[], if there is one. */
static unsigned char load(unsigned char n)
{
    unsigned char ok = 0;
    if (!name[0])
        FindFTypes(name, APPL_DATA, 1, HELP_CLASS);
    if (name[0] && !OpenRecordFile(name)) {
        if (n <= usedRecords && !PointRecord(n - 1))
            ok = !ReadRecord((char *)page, sizeof page);
        CloseRecordFile();
    }
    return ok;
}

void help_open(void)
{
    const char *p;
    unsigned char *d;
    gui_cover(1);
    helping = 1;
    if (!at) {
        if (load(1)) {
            at = 1;
        } else {                        /* a blank page that says why */
            memset(page, 0, sizeof page);
            for (p = nofile, d = page; *p; )
                *d++ = *p++ - ' ';      /* ' '-'_' are screen codes 0-63 */
        }
    }
    help_draw(page);
}

unsigned char __fastcall__ help_key(unsigned char c)
{
    switch (c) {
    case '>':
    case '*':
        if (at && load(at + 1)) {
            ++at;
            help_draw(page);
        }
        break;
    case '<':
    case '+':
    case K_DELETE:                      /* GEOS's left arrow is its backspace */
        if (at > 1 && load(at - 1)) {
            --at;
            help_draw(page);
        }
        break;
    case K_ESC:
    case K_RETURN:
    case K_HELP:
    case '?':
        helping = 0;
        gui_cover(0);
        return 0;
    }
    return 1;
}
