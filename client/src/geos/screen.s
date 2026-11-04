; screen.s -- GEOS on the Atari: the OS ROM's font, copied from under GEOS for display_gui.c
; (the title art, the board, the currency sign).
;
; Interrupts: geos-atari's keyboard IRQ writes GEOS's r1H and clears its bank lock without
; saving either, so a key that lands while the kernal draws (PutChar works out a character's
; line from r1H) can write into the kernal itself. So the program runs with IRQs masked from
; here on, and in_key() lets one in where nothing is drawing (src/geos/input.c); POKEY holds
; a key's IRQ until then. The frame's NMI is unaffected. Our own SEIs all put the flag back
; as they found it (PHP/PLP).
;
; The ROM covers $C000-$FFFF while it is in, and geos-atari's display list is at $C0xx, in the RAM
; under it. Should ANTIC fetch from it then, it reads the ROM, may take a byte there for a jump and
; wander off into memory it never comes back from (on GEOS nothing reloads the display list each
; frame): the screen turns to dancing blocks, the player memory's. So the copy waits for the
; vertical blank, where ANTIC sits at its jump back to the top, and runs with ANTIC's DMA off,
; back on (GEOS's DMACTL, geos-atari kernal/hw/hw1b.s) in a vertical blank. A frame goes blank.
;
; void screen_init(void)  the font; IRQs masked

        .export _font, _screen_init
        .importzp ptr1, ptr2

PORTB   = $D301         ; bit 0: OS ROM in; bit 7: self-test ROM out
NMIEN   = $D40E
DMACTL  = $D400
VCOUNT  = $D40B
VBLANK  = 124           ; VCOUNT from here on: below the screen, PAL or NTSC
GEOS_DMACTL = %00111010 ; display list, single-line players, normal playfield

        .bss
_font:  .res 1024

        .code

_screen_init:
        sei
        lda #0                  ; no NMIs either while the ROM is in: its vectors are the OS's
        sta NMIEN
        jsr vblank
        lda #0
        sta DMACTL              ; ANTIC fetches nothing
        lda PORTB
        pha
        ora #$81                ; OS ROM in, self-test ROM out
        sta PORTB
        lda #<$E000             ; the ROM's font, internal (screen code) order
        sta ptr1
        lda #>$E000
        sta ptr1+1
        lda #<_font
        sta ptr2
        lda #>_font
        sta ptr2+1
        ldx #4
        ldy #0
:       lda (ptr1),y
        sta (ptr2),y
        iny
        bne :-
        inc ptr1+1
        inc ptr2+1
        dex
        bne :-
        pla
        sta PORTB
        jsr vblank
        lda #GEOS_DMACTL
        sta DMACTL
        lda #$40
        sta NMIEN
        rts                     ; IRQs stay masked

vblank: lda VCOUNT
        cmp #VBLANK
        bcc vblank
        rts
