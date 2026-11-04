; draw.s -- GEOS on the Atari: display_gui.c's drawing helpers, in assembly for the room (cc65's
; C for them was about twice the size). Their C prototypes are unchanged (display_gui.c lists them):
; the arguments come on cc65's stack, the last in A/X (__fastcall__).
;
; Areas are GEOS's r2L (top), r2H (bottom), r3 (left), r4 (right). Text is put together in buf from
; bp (zero page $78, the first of GEOS's application registers, after cc65's own), a character with
; bit 7 drawn the other way round; it is drawn at the pen, px, in GEOS's system font with the style
; in `bold`, clipped `clip` lines below its top (GEOS draws a character's whole box, 9 lines, which
; would wipe the top of the row below); '\' is the yen, from the Atari's font (romfont).
;
; geos-atari's InvertRectangle gets one area wrong (kernal/graph/line.s, __InvertLineDo has no case
; for it): both ends in one byte, the left one not on its first pixel, inverts to that byte's end and
; the right end's mask in the next byte too. From the byte's first pixel it's right, so that one is
; two: from there to the right end, then from there to just before the left end, which inverts that
; part back.

        .export _box, _invert, _panel, _rom, _width, _put, _text, _textn, _number, _node_name
        .export _syn_letters, _out, _show, _show_right, _say, _labels, _panels
        .export _px, _bold, _clip, _buf
        .import _romfont, _syn_letter
        .import popa, popax
        .importzp ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4

; GEOS (cc65's libsrc/geos-cbm jumptab.inc and geossym.inc; geos-atari's are the same)
r0L     = $02
r0H     = $03
r1H     = $05
r2L     = $06
r2H     = $07
r3L     = $08
r3H     = $09
r4L     = $0a
r4H     = $0b
r11L    = $18
r11H    = $19
currentMode  = $2e
windowBottom = $34
Rectangle       = $c124
FrameRectangle  = $c127
InvertRectangle = $c12a
SetPattern      = $c139
PutChar         = $c145
UseSystemFont   = $c14b
GetCharWidth    = $c1c9
LoadCharSet     = $c1cc

bp      = $78                   ; display_gui.c: #define bp
BASE    = 6                     ; both fonts: the baseline, from the top
P_WHITE = 0
P_BLACK = 1
RULE_COLS = 12
NSYN    = 7

        .bss
_buf:   .res 48
_px:    .res 2
_bold:  .res 1
ty:     .res 1                  ; the text's top
olen:   .res 1
ochar:  .res 1
x0:     .res 2
wsum:   .res 2
pt:     .res 1                  ; a panel's top, bottom, left, right
pb:     .res 1
pl:     .res 2
pr:     .res 2

        .data
_clip:  .byte 7

        .code

; ---------------------------------------------------------------- areas

; void box(unsigned char top, unsigned char bot, unsigned left, unsigned right, unsigned char pat)
_box:   pha
        jsr area
        pla
        jsr SetPattern
        jmp Rectangle

; the area from the stack: right in A/X here, then left, bottom, top
area4:  sta r4L
        stx r4H
        jmp area3
area:   jsr popax
        sta r4L
        stx r4H
area3:  jsr popax
        sta r3L
        stx r3H
        jsr popa
        sta r2H
        jsr popa
        sta r2L
        rts

; void invert(unsigned char top, unsigned char bot, unsigned left, unsigned right)
_invert:
        jsr area4
invr:   lda r3L
        and #7
        beq @go
        lda r4L                 ; right + 1 in the same byte as left?
        clc
        adc #1
        tax
        lda r4H
        adc #0
        cmp r3H
        bne @go
        txa
        eor r3L
        and #$F8
        bne @go
        lda r3L
        pha
        and #$F8
        sta r3L
        jsr InvertRectangle     ; (keeps r2L, r3, r4)
        pla
        sec
        sbc #1
        sta r4L
        lda r3H
        sta r4H
@go:    jmp InvertRectangle

; void panel(unsigned char top, unsigned char bot, unsigned left, unsigned right): white, framed,
; a 2-pixel shadow right and below
_panel: jsr area4
panelr: lda r2L
        sta pt
        lda r2H
        sta pb
        lda r3L
        sta pl
        lda r3H
        sta pl+1
        lda r4L
        sta pr
        lda r4H
        sta pr+1
        lda #P_BLACK
        jsr SetPattern
        ldx pb                  ; below: bot+1..bot+2, left+2..right+2
        inx
        stx r2L
        inx
        stx r2H
        lda pl
        clc
        adc #2
        sta r3L
        lda pl+1
        adc #0
        sta r3H
        lda pr
        clc
        adc #2
        sta r4L
        lda pr+1
        adc #0
        sta r4H
        jsr Rectangle
        lda pt                  ; right: top+2..bot+2, right+1..right+2
        clc
        adc #2
        sta r2L
        lda pr
        clc
        adc #1
        sta r3L
        lda pr+1
        adc #0
        sta r3H
        jsr Rectangle
        lda pt
        sta r2L
        lda pb
        sta r2H
        lda pl
        sta r3L
        lda pl+1
        sta r3H
        lda pr
        sta r4L
        lda pr+1
        sta r4H
        lda #P_WHITE
        jsr SetPattern
        jsr Rectangle
        lda #$FF
        jmp FrameRectangle

; ---------------------------------------------------------------- characters

; One character, A, at the pen, top ty; the pen moves on.
glyph:  pha
        lda _bold
        sta currentMode
        lda _px
        sta r11L
        lda _px+1
        sta r11H
        lda ty
        clc
        adc #BASE
        sta r1H
        pla
        cmp #$5C           ; the yen ('\\')
        bne @sys
        pha
        jsr romset
        pla
        jsr PutChar
        jsr UseSystemFont
        jmp @done
@sys:   jsr PutChar
@done:  lda r11L
        sta _px
        lda r11H
        sta _px+1
        rts

romset: lda #<_romfont
        sta r0L
        lda #>_romfont
        sta r0H
        jmp LoadCharSet

; void rom(unsigned char c, unsigned char y, unsigned x): one of the Atari's glyphs
_rom:   sta r11L
        stx r11H
        jsr popa                ; y
        tax
        clc
        adc #7
        sta windowBottom
        txa
        clc
        adc #BASE
        sta r1H
        jsr popa                ; c
        pha
        lda #0
        sta currentMode
        jsr romset
        pla
        jsr PutChar
        jsr UseSystemFont
        lda #199
        sta windowBottom
        rts

; The width of character A, as drawn (bold one wider).
cw:     cmp #$5C           ; the yen ('\\')
        bne @sys
        lda #8
        bne @bold
@sys:   jsr GetCharWidth
@bold:  ldx _bold
        beq @x
        clc
        adc #1
@x:     rts

; The width of olen characters from ptr4, into wsum (A/X).
widthp: lda #0
        sta wsum
        sta wsum+1
@next:  lda olen
        beq @done
        dec olen
        ldy #0
        lda (ptr4),y
        inc ptr4
        bne :+
        inc ptr4+1
:       and #$7F
        jsr cw
        clc
        adc wsum
        sta wsum
        bcc @next
        inc wsum+1
        bne @next
@done:  lda wsum
        ldx wsum+1
        rts

; unsigned width(const unsigned char *s, unsigned char len)
_width: sta olen
        jsr popax
        sta ptr4
        stx ptr4+1
        jmp widthp

; ---------------------------------------------------------------- putting text together

; void put(unsigned char c): at bp, while buf has room
_put:   tax
        lda bp
        cmp #<(_buf + 48)
        lda bp+1
        sbc #>(_buf + 48)
        bcs @full
        txa
        ldy #0
        sta (bp),y
        inc bp
        bne @full
        inc bp+1
@full:  rts

; void text(const char *s)
_text:  sta ptr1
        stx ptr1+1
@next:  ldy #0
        lda (ptr1),y
        beq @done
        jsr _put
        inc ptr1
        bne @next
        inc ptr1+1
        bne @next
@done:  rts

; void textn(const unsigned char *s, unsigned char len)
_textn: sta tmp1
        jsr popax
        sta ptr1
        stx ptr1+1
@next:  lda tmp1
        beq @done
        dec tmp1
        ldy #0
        lda (ptr1),y
        jsr _put
        inc ptr1
        bne @next
        inc ptr1+1
        bne @next
@done:  rts

; void number(unsigned value, unsigned char money): decimal, no leading zeros; money (in hundreds):
; the yen first and "00" after, unless it's 0
_number:
        sta tmp4                ; money
        jsr popax
        sta ptr1                ; the value
        stx ptr1+1
        lda tmp4
        beq :+
        lda #$5C           ; the yen ('\\')
        jsr _put
:       lda #0
        sta tmp2                ; digits put
        ldy #0                  ; the power of ten
@pow:   lda #'0'
        sta tmp1
@sub:   lda ptr1
        cmp pow_lo,y
        lda ptr1+1
        sbc pow_hi,y
        bcc @digit
        lda ptr1
        sbc pow_lo,y            ; (carry set)
        sta ptr1
        lda ptr1+1
        sbc pow_hi,y
        sta ptr1+1
        inc tmp1
        bne @sub
@digit: lda tmp1
        ldx tmp2
        bne @put
        cpy #4
        beq @put
        cmp #'0'
        beq @nput
@put:   sty tmp3
        jsr _put
        ldy tmp3
        inc tmp2
@nput:  iny
        cpy #5
        bne @pow
        lda tmp4
        beq @done
        lda tmp2                ; money, not just "0": "00" after
        cmp #1
        bne @00
        lda tmp1
        cmp #'0'
        beq @done
@00:    lda #'0'
        jsr _put
        lda #'0'
        jmp _put
@done:  rts

pow_lo: .byte <10000, <1000, <100, <10, <1
pow_hi: .byte >10000, >1000, >100, >10, >1

; void node_name(unsigned char node): its column letter and row digit
_node_name:
        ldx #'0'
@row:   cmp #RULE_COLS
        bcc @col
        sbc #RULE_COLS
        inx
        bne @row
@col:   stx tmp1
        clc
        adc #'A'
        jsr _put
        lda tmp1
        jmp _put

; void syn_letters(unsigned char mask): each syndicate's letter in it, the other way round, and a space
_syn_letters:
        sta tmp4
        ldx #0
@next:  lsr tmp4
        bcc @skip
        stx tmp3
        lda _syn_letter,x
        ora #$80
        jsr _put
        lda #' '
        jsr _put
        ldx tmp3
@skip:  inx
        cpx #NSYN
        bne @next
        rts

; ---------------------------------------------------------------- drawing text

; olen characters from ptr4 at the pen, top ty
outp:   lda ty
        clc
        adc _clip
        sta windowBottom
@next:  lda olen
        beq @done
        dec olen
        ldy #0
        lda (ptr4),y
        inc ptr4
        bne :+
        inc ptr4+1
:       sta ochar
        lda _px
        sta x0
        lda _px+1
        sta x0+1
        lda ochar
        and #$7F
        jsr glyph
        bit ochar
        bpl @next
        ldx ty                  ; the other way round: ty-1..ty+7, x0..px-1
        dex
        stx r2L
        txa
        clc
        adc #8
        sta r2H
        lda x0
        sta r3L
        lda x0+1
        sta r3H
        lda _px
        sec
        sbc #1
        sta r4L
        lda _px+1
        sbc #0
        sta r4H
        jsr invr
        jmp @next
@done:  lda #199                ; GEOS's own text (a Panic's box) unclipped
        sta windowBottom
        rts

; void out(const unsigned char *s, unsigned char len, unsigned char y)
_out:   sta ty
        jsr popa
        sta olen
        jsr popax
        sta ptr4
        stx ptr4+1
        jmp outp

; buf..bp into ptr4, olen
bufp:   lda #<_buf
        sta ptr4
        lda #>_buf
        sta ptr4+1
        lda bp
        sec
        sbc #<_buf
        sta olen
        rts

; void show(unsigned x, unsigned char y): buf at x
_show:  sta ty
        jsr popax
setpx:  sta _px
        stx _px+1
        jsr bufp
        jmp outp

; void show_right(unsigned x, unsigned char y): buf ending at x
_show_right:
        sta ty
        jsr popax
showr:  sta x0                  ; (x in A/X, ty set)
        stx x0+1
        jsr bufp
        jsr widthp
        lda x0
        sec
        sbc wsum
        pha
        lda x0+1
        sbc wsum+1
        tax
        pla
        jmp setpx

; void say(const char *s, unsigned x, unsigned char y): s, at x
_say:   sta ty
        jsr popax
        sta x0
        stx x0+1
        jsr popax
        pha
        lda #<_buf
        sta bp
        lda #>_buf
        sta bp+1
        pla
        jsr _text
        lda x0
        ldx x0+1
        jmp setpx

; ---------------------------------------------------------------- the fixed parts of a screen

; void labels(const struct label *p): display_gui.c's struct label { const char *s; unsigned x;
; unsigned char y, how; }, to one with s 0. how: GEOS's style bits (SET_BOLD...), and L_RIGHT (1)
; ending at x, L_INV (2) the other way round.
L_RIGHT = 1
L_INV   = 2
_labels:
        sta ptr3
        stx ptr3+1
@next:  ldy #1
        lda (ptr3),y
        beq @done               ; (no string is in page 0)
        tax
        dey
        lda (ptr3),y
        pha
        ldy #4
        lda (ptr3),y
        sta ty
        iny
        lda (ptr3),y
        sta tmp4
        and #$F8
        sta _bold
        lda #<_buf
        sta bp
        lda #>_buf
        sta bp+1
        pla
        jsr _text               ; (keeps ptr3)
        lda tmp4
        and #L_INV
        beq @x
        lda bp
        sec
        sbc #<_buf
        sta tmp1                ; its length
        ldy #0
@inv:   cpy tmp1
        beq @x
        lda _buf,y
        ora #$80
        sta _buf,y
        iny
        bne @inv
@x:     ldy #2
        lda (ptr3),y
        pha
        iny
        lda (ptr3),y
        tax
        pla
        lsr tmp4                ; L_RIGHT
        bcc @left
        jsr showr
        jmp @step
@left:  jsr setpx
@step:  lda ptr3
        clc
        adc #6
        sta ptr3
        bcc @next
        inc ptr3+1
        bne @next
@done:  lda #0
        sta _bold
        rts

; void panels(const struct area *p): display_gui.c's struct area { unsigned char top, bot;
; unsigned left, right; }, to one with top $FF
_panels:
        sta ptr3
        stx ptr3+1
@next:  ldy #0
        lda (ptr3),y
        cmp #$FF
        beq @done
        ldx #0
@copy:  lda (ptr3),y            ; into r2L, r2H, r3, r4
        sta r2L,x
        inx
        iny
        cpy #6
        bne @copy
        jsr panelr
        lda ptr3
        clc
        adc #6
        sta ptr3
        bcc @next
        inc ptr3+1
        bne @next
@done:  rts
