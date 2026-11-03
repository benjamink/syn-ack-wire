; frame.s -- GEOS on the Atari: our share of GEOS's frame interrupt (its NMI, the
; vertical blank). frame_init puts `frame` in front of whatever intTopVector held
; (GEOS's mouse, timers and the like), and frame_done puts it back.
;
; Once a frame: the tick count (ticks(), and net_sio.s's 4-second safety call),
; PROCEED and the chords. FujiNet's PROCEED (data waiting) reaches the PIA's CA1
; input, whose latch is PACTL bit 7. GEOS passes on no PIA interrupt, and its
; joystick driver reads PORTA every frame, which clears the latch, so this looks
; just before it does: GEOS calls intTopVector first, and the joystick from there.
;
; void frame_init(void)  (snd_init's, under GEOS)
; void frame_done(void)

        .export frame_init, _frame_done, geos_tick, _geos_tick
        .import net_ready, snd_vbi

intTopVector = $849D
PACTL   = $D302
NMIEN   = $D40E

        .bss
geos_tick:
_geos_tick: .res 1
old:    .res 2

        .code

frame_init:
        lda intTopVector
        ldx intTopVector+1
        sta old
        stx old+1
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

frame:  inc geos_tick
        bit PACTL               ; PROCEED latched since the last look?
        bpl :+
        lda #1
        sta net_ready
:       jsr snd_vbi
        jmp (old)
