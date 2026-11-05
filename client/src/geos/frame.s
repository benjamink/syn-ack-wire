; frame.s -- GEOS on the Atari: our share of GEOS's frame interrupt (its NMI, the
; vertical blank). frame_init puts `frame` in front of whatever intTopVector held
; (GEOS's mouse, timers and the like), and frame_done puts it back.
;
; Once a frame: our tick count (net_sio.s's 4-second safety call), PROCEED and the
; chords. geos-atari's NMI counts every frame (jiffyCounter) but skips intTopVector
; while its kernal's bank is in (interrupt_lock): during its drawing, disk reads and
; such calls as GetNextChar. A loop of those loses most of our ticks (a minute's turn
; clock under the instruction pages showed 54 s left after 19). So ticks() reads
; GEOS's count, its address taken from the INC in GEOS's NMI handler (STA NMIRES, then
; INC jiffyCounter), and ours only should that not be there. FujiNet's PROCEED (data waiting) reaches the PIA's CA1
; input, whose latch is PACTL bit 7. GEOS passes on no PIA interrupt, and its
; joystick driver reads PORTA every frame, which clears the latch, so this looks
; just before it does: GEOS calls intTopVector first, and the joystick from there.
;
; void frame_init(void)  (snd_init's, under GEOS)
; void frame_done(void)
; unsigned char ticks(void)  frames, a free-running byte

        .export frame_init, _frame_done, geos_tick, _geos_tick, _ticks
        .import net_ready, snd_vbi
        .importzp ptr1

NMI_VECTOR = $FFFA      ; RAM under GEOS: its NMI handler

intTopVector = $849D
PACTL   = $D302
NMIEN   = $D40E

        .bss
geos_tick:
_geos_tick: .res 1

        .code

frame_init:
        lda NMI_VECTOR
        sta ptr1
        lda NMI_VECTOR+1
        sta ptr1+1
        ldy #3
        lda (ptr1),y
        cmp #$EE                ; INC abs: its operand is jiffyCounter
        bne :+
        iny
        lda (ptr1),y
        sta count
        iny
        lda (ptr1),y
        sta count+1
:       lda intTopVector
        ldx intTopVector+1
        sta old
        stx old+1               ; (in the JMP itself: a JMP ($xxFF) would read the wrong high byte)
        lda #<frame
        ldx #>frame
        bne set                 ; always

_frame_done:
        lda old
        ldx old+1
set:    ldy #0
        sty NMIEN               ; no frame interrupt halfway through the vector
        sta intTopVector
        stx intTopVector+1
        ldy #$40
        sty NMIEN
        rts

_ticks: .byte $AD               ; LDA: GEOS's count, or geos_tick
count:  .word geos_tick
        ldx #0
        rts

frame:  inc geos_tick
        bit PACTL               ; PROCEED latched since the last look?
        bpl :+
        lda #1
        sta net_ready
:       jsr snd_vbi
        .byte $4C               ; JMP: on to what intTopVector held before
old:    .word 0
