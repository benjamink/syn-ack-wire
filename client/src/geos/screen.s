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
; void __fastcall__ help_draw(const unsigned char *page)
;       an instruction page (help.c), 24 rows of 40 screen codes, onto GEOS's screen from line
;       PAGE_Y: a cell a byte across and 8 lines down, bit 7 inverse, as the Atari shows it; the
;       lines above and below it white. Straight into the screen: GEOS draws into its background
;       screen meanwhile

        .export _font, _screen_init, _help_draw
        .importzp ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4

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

BITMAP  = $A038         ; GEOS's screen, 40 bytes a line
PAGE_Y  = 4             ; 192 lines of page, 4 above and below

_help_draw:
        sta ptr1                ; the page
        stx ptr1+1
        lda #0
        ldy #PAGE_Y * 40        ; white above and below
:       sta BITMAP - 1,y
        sta BITMAP + (PAGE_Y + 192) * 40 - 1,y
        dey
        bne :-
        lda #<(BITMAP + PAGE_Y * 40)
        sta ptr2                ; the row's first line
        lda #>(BITMAP + PAGE_Y * 40)
        sta ptr2+1
        lda #24
        sta tmp3                ; rows left
@row:   ldy #0
@cell:  sty tmp4                ; the column
        lda (ptr1),y
        tax
        and #$7F                ; the glyph: _font + 8 * code
        sta ptr3
        lda #0
        asl ptr3
        rol a
        asl ptr3
        rol a
        asl ptr3
        rol a
        adc #>_font             ; (C = 0: the rol took a 0)
        sta ptr3+1
        lda ptr3
        adc #<_font
        sta ptr3
        bcc :+
        inc ptr3+1
:       txa
        and #$80                ; inverse: every line the other way round
        beq :+
        lda #$FF
:       sta tmp1
        lda ptr2                ; the cell's first line
        clc
        adc tmp4
        sta ptr4
        lda ptr2+1
        adc #0
        sta ptr4+1
        ldy #0
@line:  lda (ptr3),y
        eor tmp1
        sty tmp2
        ldy #0
        sta (ptr4),y
        lda ptr4
        clc
        adc #40
        sta ptr4
        bcc :+
        inc ptr4+1
:       ldy tmp2
        iny
        cpy #8
        bne @line
        ldy tmp4
        iny
        cpy #40
        bne @cell
        lda ptr1                ; the next row: 40 codes on, 8 lines down
        clc
        adc #40
        sta ptr1
        bcc :+
        inc ptr1+1
:       lda ptr2
        clc
        adc #<320
        sta ptr2
        lda ptr2+1
        adc #>320
        sta ptr2+1
        dec tmp3
        bne @row
        rts

vblank: lda VCOUNT
        cmp #VBLANK
        bcc vblank
        rts
