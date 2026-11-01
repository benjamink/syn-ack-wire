/* An Amiga raw key to a platform.h key; pure, so it is tested on the host. */
#ifndef AMIGA_KEYS_H
#define AMIGA_KEYS_H

#define AK_SOUND 0x03                   /* F3: handled by in_key, never reaches the game */

/* code: the IDCMP raw key code (bit 7 set is a key up), qualifier: IEQUALIFIER_*,
 * ascii: what the console device made of the key (0 if nothing). Returns 0 for a key the game ignores. */
unsigned char amiga_key_translate(unsigned short code, unsigned short qualifier, char ascii);

#endif
