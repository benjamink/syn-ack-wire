/* The prefs file, S:SynAckWire.prefs: the handle, read at the start for the title and written when the player dials, and
 * an optional SERVER= address, dialled instead of the built-in server (and kept when the handle is written). */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include "../config.h"
#include "../platform.h"
#include "../common/game.h"
#include "prefs.h"

static char known[HANDLE_MAX + 1];       /* the handle in the file, as last read or written */
static char known_server[SERVER_MAX + 1];   /* its server address, "" for none */
static char picked[SERVER_MAX + 1];         /* the Region menu's choice this run, "" for none: it beats the file */
static char saved_server[SERVER_MAX + 1];   /* the address the file has */

/* No "insert volume" requester on the Workbench screen behind the game: a failed open just fails. */
static APTR quiet_dos(void)
{
    struct Process *me = (struct Process *)FindTask(0);
    APTR was = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    return was;
}

/* The lobby's AppKey 0 (the player's name) is here the saved handle, from rx_ring[2]; the lobby's own key (LOBBY_KEY, the
 * server it picked) is here the SERVER= address, which main.c dials after "N:"; any other key is none. */
unsigned char net_appkey(unsigned char key)
{
    char file[128];
    BPTR f;
    LONG got = 0;
    unsigned char n;
    APTR was;

    rx_ring[2] = 0;
    if (key != 0 && key != LOBBY_KEY)
        return 0;
    if (key != 0 && picked[0]) {
        n = (unsigned char)strlen(picked);
        memcpy(rx_ring + 2, picked, n + 1);
        return n;
    }
    was = quiet_dos();
    f = Open((UBYTE *)PREFS_FILE, MODE_OLDFILE);
    if (f) {
        got = Read(f, file, sizeof file);
        Close(f);
    }
    ((struct Process *)FindTask(0))->pr_WindowPtr = was;
    if (!f || got <= 0)
        return 0;
    prefs_server(file, (unsigned char)got, known_server);
    strcpy(saved_server, known_server);
    if (key != 0) {
        n = (unsigned char)strlen(known_server);
        memcpy(rx_ring + 2, known_server, n + 1);
        return n;
    }
    n = prefs_parse(file, (unsigned char)got, (char *)rx_ring + 2);
    memcpy(known, rx_ring + 2, n + 1);
    return n;
}

/* Remember the handle just played under. Best effort: a locked or full disk loses nothing but the memory. */
/* The Region menu's pick: dialled next, and written with the handle at the next jack-in. */
void amiga_set_server(const char *url)
{
    strncpy(picked, url, SERVER_MAX);
    picked[SERVER_MAX] = 0;
    strcpy(known_server, picked);
}

void amiga_prefs_save(void)
{
    char file[16 + HANDLE_MAX + SERVER_MAX + 2], name[9];
    unsigned char i;
    BPTR f;
    APTR was;

    if (!edit_len)
        return;
    for (i = 0; i < edit_len && i < HANDLE_MAX; ++i)
        name[i] = edit[i];
    name[i] = 0;
    i = prefs_format(name, known_server, file);
    file[i] = 0;
    prefs_parse(file, i, name);         /* as the file would give it back */
    if (strcmp(name, known) == 0 && strcmp(known_server, saved_server) == 0)
        return;                         /* nothing new: spare the floppy */
    was = quiet_dos();
    if ((f = Open((UBYTE *)PREFS_FILE, MODE_NEWFILE)) != 0) {
        Write(f, file, i);
        Close(f);
        strcpy(known, name);
        strcpy(saved_server, known_server);
    }
    ((struct Process *)FindTask(0))->pr_WindowPtr = was;
}
