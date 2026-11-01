/* The handle's file, S:SynAckWire.prefs: read at the start for the title, written when the player dials. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include "../platform.h"
#include "../common/game.h"
#include "prefs.h"

static char known[HANDLE_MAX + 1];       /* the handle in the file, as last read or written */

/* No "insert volume" requester on the Workbench screen behind the game: a failed open just fails. */
static APTR quiet_dos(void)
{
    struct Process *me = (struct Process *)FindTask(0);
    APTR was = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    return was;
}

/* The lobby's AppKey 0 (the player's name) is here the saved handle, from rx_ring[2]; any other key is none. */
unsigned char net_appkey(unsigned char key)
{
    char file[48];
    BPTR f;
    LONG got = 0;
    unsigned char n;
    APTR was;

    rx_ring[2] = 0;
    if (key != 0)
        return 0;
    was = quiet_dos();
    f = Open((UBYTE *)PREFS_FILE, MODE_OLDFILE);
    if (f) {
        got = Read(f, file, sizeof file);
        Close(f);
    }
    ((struct Process *)FindTask(0))->pr_WindowPtr = was;
    if (!f)
        return 0;
    if (got <= 0)
        return 0;
    n = prefs_parse(file, (unsigned char)got, (char *)rx_ring + 2);
    memcpy(known, rx_ring + 2, n + 1);
    return n;
}

/* Remember the handle just played under. Best effort: a locked or full disk loses nothing but the memory. */
void amiga_prefs_save(void)
{
    char file[24], name[9];
    unsigned char i;
    BPTR f;
    APTR was;

    if (!edit_len)
        return;
    for (i = 0; i < edit_len && i < HANDLE_MAX; ++i)
        name[i] = edit[i];
    name[i] = 0;
    i = prefs_format(name, file);
    file[i] = 0;
    prefs_parse(file, i, name);         /* as the file would give it back */
    if (strcmp(name, known) == 0)
        return;                         /* nothing new: spare the floppy */
    was = quiet_dos();
    if ((f = Open((UBYTE *)PREFS_FILE, MODE_NEWFILE)) != 0) {
        Write(f, file, i);
        Close(f);
        strcpy(known, name);
    }
    ((struct Process *)FindTask(0))->pr_WindowPtr = was;
}
