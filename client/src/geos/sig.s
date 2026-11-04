; sig.s -- GEOS on the Atari: what each place on display_gui.c's screen shows, as a 16-bit
; signature of what drew it, so a place is drawn again only when that changes. The signature
; rotates left and takes in a byte at a time, so any one byte that differs changes it.
;
; unsigned sig                    the signature being made
; unsigned char all               nonzero: every place is drawn (the whole screen is)
; void __fastcall__ mix(const unsigned char *p, unsigned char len)   p[0..len-1] into sig
; unsigned char __fastcall__ fresh(unsigned char place)   1 if sig isn't what the place shows
;                                                          (it does now), else 0
; unsigned char __fastcall__ fresh_buf(unsigned char place)  the same for buf..bp, its length first

        .export _sig, _all, _mix, _fresh, _fresh_buf
        .import _buf, _bp, popax
        .importzp ptr1, tmp1

PLACES  = 16                    ; display_gui.c's S_COUNT, or fewer

        .bss
_sig:   .res 2
_all:   .res 1
drawn:  .res 2 * PLACES

        .code

_mix:   sta tmp1                ; len
        jsr popax
        sta ptr1
        stx ptr1+1
        ldy #0
@next:  cpy tmp1
        beq @done
        lda _sig+1              ; rotate left
        asl a
        rol _sig
        rol _sig+1
        lda _sig
        eor (ptr1),y
        sta _sig
        iny
        bne @next
@done:  rts

_fresh_buf:
        pha                     ; place
        lda _bp
        sec
        sbc #<_buf
        sta _sig                ; the length, then the bytes
        ldx #0
        stx _sig+1
        pha
        lda #<_buf
        ldx #>_buf
        jsr pushax
        pla
        jsr _mix
        pla
        ; fall through
_fresh: asl a
        tay
        lda _all
        bne @new
        lda _sig
        cmp drawn,y
        bne @new
        lda _sig+1
        cmp drawn+1,y
        bne @new
        lda #0
        tax
        rts
@new:   lda _sig
        sta drawn,y
        lda _sig+1
        sta drawn+1,y
        lda #1
        ldx #0
        rts

        .import pushax
