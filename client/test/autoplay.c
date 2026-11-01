/* Test hooks for an unattended client (make test builds build/atari/synack-auto.xex):
 * auto_key() stands in for in_key(): it types a handle, then presses keys a
 * player might press for each prompt, wrong ones included, so the local checks
 * and the server's refusals both get exercised. When a new prompt opens, and
 * every DUMP_TICKS, it executes BRK; atari800 -bbrk stops in its monitor there, and tools/run-test.sh feeds
 * the monitor commands that save screen memory to a file. On a PC (make pc-apple2) it calls pc_dump()
 * instead, which saves the screen array; on the NES, where a BRK would only go to the IRQ handler, it
 * calls test/nes_dump.c's, which the emulator watches (tools/nes.lua). */
#include "../src/platform.h"
#include "../src/common/game.h"

#ifndef DUMP_TICKS
#define DUMP_TICKS 240             /* 0: no screen dumps */
#endif
#ifndef KEY_TICKS
#define KEY_TICKS 20               /* between keys: a third of a second at 60 ticks a second */
#endif

static unsigned char seed = 0x5A, last, prev, shown = 0xFF, typed, paged;  /* 0xFF: dump the title too */
static unsigned elapsed;
#ifndef AUTO_HANDLE
#define AUTO_HANDLE "ATARI"
#endif
static const char name[] = AUTO_HANDLE "\r";
/* first, the help pages: open, page to the end (extra presses are ignored), back one, leave */
static const unsigned char paging[] = { K_HELP, '>', '>', '>', '>', '>', '>', '>', '>', '<', K_ESC, 0 };

#if defined(__CC65__) && !defined(__NES__)
#define DUMP() do { __asm__ ("brk"); __asm__ ("nop"); } while (0)
#else
void pc_dump(void);                         /* test/pc_harness.c, test/nes_dump.c */
#define DUMP() pc_dump()
#endif

static unsigned char rnd(void)
{
    seed = (seed << 1) ^ ((seed & 0x80) ? 0x1D : 0);   /* 8-bit LFSR */
    return seed;
}

#ifdef AUTOCLICK
unsigned char amiga_autoclick(unsigned char key);   /* test/amiga_autoclick.c: click the widget that gives `key`; 0 if none */
#endif

/* the key a player might press for the open prompt */
static unsigned char pick(unsigned char r)
{
    switch (pr_kind) {
    case 'S':
        return '3';
    case 'P':
        return '1' + (r & 7);                       /* 7 and 8 are ignored */
    case 'F':
    case 'K':
        return syn_letter[r % NSYN];
    case 'D':
#ifdef __AMIGA__
        if (complaint)                              /* refused: clear the count, or wait out the server's minute */
            return K_DELETE;
#endif
        return (r & 3) ? '0' + (r & 3) : K_RETURN;
    case 'B':
        if (r < 40)
            return K_DELETE;
        if (r < 100)
            return K_RETURN;
        if (r < 120)
            return 'N';
        return syn_letter[r % NSYN];
    }
    return 0;
}

unsigned char auto_key(void)
{
    unsigned char t = ticks(), k;
#ifdef AUTOCLICK
    if ((k = in_key()) != 0)                        /* the keys a click queued come out one at a time */
        return k;
#endif
    elapsed += (unsigned char)(t - prev);
    prev = t;
    if (!status_len && !status_code && shown == 0xFF)
        return 0;                                   /* the title, until the server answers: its status shows */
    if (DUMP_TICKS && (elapsed >= DUMP_TICKS || pr_kind != shown)) {
        elapsed = 0;
        shown = pr_kind;
        DUMP();
    }
#ifndef AUTOCLICK
    in_key();
#endif
    if ((unsigned char)(t - last) < KEY_TICKS)
        return 0;
    last = t;
    if (paging[paged]) {                            /* a dump before each key shows each page */
        if (DUMP_TICKS)
            DUMP();
#ifdef AUTOCLICK
        if (paging[paged] == K_HELP && amiga_autoclick(K_HELP)) {      /* the HELP button */
            ++paged;
            return 0;
        }
#endif
        return paging[paged++];
    }
    if (name[typed]) {                              /* title: the handle, then Return */
#ifdef AUTOCLICK
        if (name[typed] == '\r' && amiga_autoclick('\r')) {   /* Return is the JACK-IN button */
            ++typed;
            return 0;
        }
#endif
        return name[typed++];
    }
    k = pick(rnd());
#ifdef AUTOCLICK
    if (k && amiga_autoclick(k))
        return 0;                                   /* the click queued it (or them) */
#endif
    return k;
}
