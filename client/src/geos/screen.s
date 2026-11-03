; screen.s -- GEOS on the Atari: the Atari's 40x24 text layout (src/atari/display_text.c)
; drawn into GEOS's hi-res bitmap. The layout writes Atari screen codes into `shadow`,
; as it would into screen memory, and sets `screen_dirty`; screen_flush draws each cell
; that differs from `drawn` (what the bitmap shows) as 8 bytes of the font, inverted when
; the code's bit 7 is set. in_key() flushes, and so does display_message().
;
; GEOS's screen (geos-atari kernal/hw/displaylist.s): ANTIC mode F from $A038, 40 bytes a
; line for 200 lines, unbroken across the 4K boundary, which falls on a line. The 24 rows
; of 8 lines sit in the middle, 4 lines down. The font is the OS ROM's, copied at start-up
; from under GEOS: the ROM is switched in for the copy.
;
; void screen_init(void)
; void screen_flush(void)
; void screen_done(void)  GEOS's colours again, for its DeskTop

        .export _shadow, _font, _screen_dirty, _screen_init, _screen_flush, _screen_done
        .importzp ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, sreg

MouseOff = $C18D        ; GEOS jump table
SCREEN  = $A038 + 4 * 40
PORTB   = $D301         ; bit 0: OS ROM in; bit 7: self-test ROM out
NMIEN   = $D40E
COLPF1  = $D017         ; mode F: the pixels' luminance
COLPF2  = $D018         ; mode F: the background
COLBK   = $D01A         ; the border

        .bss
_shadow: .res 960
drawn:  .res 960        ; zero, like the cleared bitmap: code 0 is a space
_font:  .res 1024
_screen_dirty: .res 1

        .code

_screen_init:
        jsr MouseOff
        lda #0                  ; no interrupts while the ROM is in: its vectors are the OS's
        sta NMIEN
        php
        sei
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
        plp
        lda #$40
        sta NMIEN
        lda #$C0                ; the Atari client's green: dark behind, bright letters
        sta COLPF2
        lda #$0C
        sta COLPF1
        lda #0
        sta COLBK
        lda #<($A038)           ; the whole visible bitmap: 8000 bytes, 31 pages and 64
        sta ptr1
        lda #>($A038)
        sta ptr1+1
        lda #0
        ldx #31
        ldy #0
:       sta (ptr1),y
        iny
        bne :-
        inc ptr1+1
        dex
        bne :-
:       sta (ptr1),y
        iny
        cpy #64
        bne :-
        rts

_screen_done:
        lda #$9A                ; GEOS's own (geos-atari kernal/hw/displaylistinit.s)
        sta COLPF2
        lda #$94
        sta COLPF1
        lda #0
        sta COLBK
        rts

_screen_flush:
        lda _screen_dirty
        bne :+
        rts
:       lda #0
        sta _screen_dirty
        lda #<_shadow
        sta ptr1
        lda #>_shadow
        sta ptr1+1
        lda #<drawn
        sta ptr2
        lda #>drawn
        sta ptr2+1
        lda #<SCREEN
        sta ptr3
        lda #>SCREEN
        sta ptr3+1
        lda #24
        sta tmp2                ; rows left
@row:   ldy #0
@col:   lda (ptr1),y
        cmp (ptr2),y
        beq @same
        sta (ptr2),y
        jsr cell
@same:  iny
        cpy #40
        bne @col
        lda ptr1                ; next row: 40 more in the copies, 320 in the bitmap
        clc
        adc #40
        sta ptr1
        bcc :+
        inc ptr1+1
:       lda ptr2
        clc
        adc #40
        sta ptr2
        bcc :+
        inc ptr2+1
:       lda ptr3
        clc
        adc #<320
        sta ptr3
        lda ptr3+1
        adc #>320
        sta ptr3+1
        dec tmp2
        bne @row
        rts

; Draw screen code A in column Y of the row at ptr3. Keeps Y.
cell:   sty tmp3
        ldx #0
        asl a                   ; bit 7, inverse, into carry
        bcc :+
        dex                     ; X = $FF: invert
:       stx tmp1
        ldx #0
        stx ptr4+1
        asl a                   ; (code & $7F) * 8: the glyph's offset
        rol ptr4+1
        asl a
        rol ptr4+1
        adc #<_font             ; (carry clear: ptr4+1's top bits were 0)
        sta ptr4
        lda ptr4+1
        adc #>_font
        sta ptr4+1
        tya                     ; the cell's top line in the bitmap
        clc
        adc ptr3
        sta sreg
        lda ptr3+1
        adc #0
        sta sreg+1
        .repeat 7, k            ; lines 0-6: offsets up to 240
        ldy #k
        lda (ptr4),y
        eor tmp1
        ldy #40 * k
        sta (sreg),y
        .endrepeat
        ldy #7                  ; line 7: 280, past the page
        lda (ptr4),y
        eor tmp1
        inc sreg+1
        ldy #280 - 256
        sta (sreg),y
        ldy tmp3
        rts
