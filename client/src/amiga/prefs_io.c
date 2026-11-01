/* The handle's file, S:SynAckWire.prefs: read at the start for the title, written when the player dials. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "../platform.h"
#include "../common/game.h"
#include "prefs.h"

/* The lobby's AppKey 0 (the player's name) is here the saved handle, from rx_ring[2]; any other key is none. */
unsigned char net_appkey(unsigned char key)
{
    char file[48];
    BPTR f;
    LONG got = 0;
    unsigned char n;

    rx_ring[2] = 0;
    if (key != 0 || !(f = Open((UBYTE *)PREFS_FILE, MODE_OLDFILE)))
        return 0;
    got = Read(f, file, sizeof file);
    Close(f);
    if (got <= 0)
        return 0;
    n = prefs_parse(file, (unsigned char)got, (char *)rx_ring + 2);
    return n;
}

/* Remember the handle just played under. Best effort: a locked or full disk loses nothing but the memory. */
void amiga_prefs_save(void)
{
    char file[24], name[9];
    unsigned char i;
    BPTR f;

    if (!edit_len)
        return;
    for (i = 0; i < edit_len && i < HANDLE_MAX; ++i)
        name[i] = edit[i];
    name[i] = 0;
    i = prefs_format(name, file);
    if ((f = Open((UBYTE *)PREFS_FILE, MODE_NEWFILE)) != 0) {
        Write(f, file, i);
        Close(f);
    }
}
