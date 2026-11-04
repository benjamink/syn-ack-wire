/* GEOS on the Atari: the instruction pages, from the title screen. They are a GEOS file on the
 * disk (tools/help_geos.py), a page a record, found by its class, so whatever the file is
 * called; a page is 960 screen codes, drawn in the Atari's ROM font (screen.s), as the Atari
 * shows it. Nothing else draws while they're up (display_gui.c holds it back, and draws the
 * screen again after them), and the title's loop goes on reading the network meanwhile.
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
void box(unsigned char top, unsigned char bot, unsigned left, unsigned right, unsigned char pat);  /* draw.s */

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

static void show(void)
{
    box(0, 3, 0, 319, 0);               /* white above and below the page */
    box(196, 199, 0, 319, 0);
    help_draw(page);
}

void help_open(void)
{
    const char *p;
    unsigned char *d;
    helping = 1;
    gui_cover(1);
    if (!at) {
        if (load(1)) {
            at = 1;
        } else {                        /* a blank page that says why */
            memset(page, 0, sizeof page);
            for (p = nofile, d = page; *p; )
                *d++ = *p++ - ' ';      /* ' '-'_' are screen codes 0-63 */
        }
    }
    show();
}

unsigned char __fastcall__ help_key(unsigned char c)
{
    switch (c) {
    case '>':
    case '*':
        if (at && load(at + 1)) {
            ++at;
            show();
        }
        break;
    case '<':
    case '+':
    case K_DELETE:                      /* GEOS's left arrow is its backspace */
        if (at > 1 && load(at - 1)) {
            --at;
            show();
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
