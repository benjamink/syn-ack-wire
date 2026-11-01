/* The player's handle, kept in S:SynAckWire.prefs as one line, HANDLE=NAME. The format is pure and tested on the host;
 * the file itself is prefs_io.c's. */
#ifndef AMIGA_PREFS_H
#define AMIGA_PREFS_H

#define PREFS_FILE "S:SynAckWire.prefs"
#define HANDLE_MAX 8

/* The handle from the file's `len` bytes, upcased, only what the title lets a player type (above a space, up to Z),
 * at most HANDLE_MAX, NUL-ended in `handle` (9 bytes). Returns its length; 0 for no HANDLE line. */
unsigned char prefs_parse(const char *file, unsigned char len, char *handle);

/* The file for `handle` (NUL-ended, filtered the same way) in `out` (at least 17 bytes); returns its length. */
unsigned char prefs_format(const char *handle, char *out);

#endif
