; sound.s -- chords for board events, from the immediate VBI.
; SIO uses POKEY channels 3 and 4 as its serial clock, so chords use channels 1
; and 2 only, and nothing here writes 3, 4 or AUDCTL (SIO's $28 there leaves
; 1 and 2 on the 64 kHz clock). A chord's three notes share the two: channel 1
; holds the first (the lowest), channel 2 alternates the other two every frame,
; an arpeggio quick enough to hear as one chord. A note left on channel 4 made
; later SIO reads fail (FujiNet saw the same read retried after it had already
; sent the bytes). The immediate VBI runs during SIO too (net_sio.s masks IRQs,
; not NMIs), so the envelope keeps time, and net_sio.s's own SIO never touches
; channels 1 and 2: a chord sounds straight through a transfer. (The OS's SIO,
; which the instruction pages still use, turns them off; the next frame puts
; them back.)
;
; All chords are from B Locrian, 8-bit dividers on the 64 kHz clock:
; f = 63921 / (2 * (AUDF + 1)), within 10 cents from E3 to A4.
;
; void snd_init(void)
; void __fastcall__ play_chord(unsigned char n)
; void snd_toggle(void)

        .export _snd_init, _play_chord, _snd_toggle
.ifdef __GEOS__
        .export snd_vbi         ; src/geos/frame.s calls it once a frame
        .import frame_init
.endif

AUDF1   = $D200
AUDC1   = $D201
AUDF2   = $D202
AUDC2   = $D203
VVBLKI  = $0222         ; immediate VBI vector
SETVBV  = $E45C

PURE    = $A0           ; AUDC: pure tone; low nibble = volume
CHORD_HIVE = 2          ; platform.h

; note dividers
D3 = 217
E3 = 193
F3 = 182
G3 = 162
A3 = 144
B3 = 128
C4 = 121
D4 = 108
E4 = 96
F4 = 91
G4 = 81
A4 = 72

        .data
_snd_on: .byte 1

        .bss
vol:    .res 1          ; 0 = silent: the VBI leaves POKEY alone
pitch:  .res 3          ; the chord now sounding: the held note, the two alternating
rate:   .res 1          ;   starting volume << 4 | frames per volume step
next:   .res 1          ;   the chord after it, or $FF
tick:   .res 1          ; frames to the next volume step
prio:   .res 1          ; the chord number play_chord started
flip:   .res 1          ; which alternating note this frame: 0 or 1
oldvbi: .res 2

        .rodata
; per chord: 3 pitches (the first held, the other two alternating), starting
; volume << 4 | frames per volume step, next chord ($FF = none).
; Entries 0-6 are the chord numbers in platform.h; the rest only follow on.
chords: .byte D3, F3, A3, $62, $FF    ; 0 a spike: D minor, low, quiet and short
        .byte C4, E4, G4, $82, 7      ; 1 buy: C major...
        .byte C4, E4, G4, $A4, $FF    ; 2 new hive: C major
        .byte B3, D4, F4, $A4, $FF    ; 3 hivejack: B diminished
        .byte E3, G3, B3, $A6, $FF    ; 4 Black ICE: E minor, low
        .byte G3, B3, D4, $82, 8      ; 5 your turn: G major...
        .byte F3, A3, C4, $A3, 9      ; 6 null-route: F major...
        .byte C4, F4, A4, $82, $FF    ; 7 ...buy: up to F major, C held
        .byte C4, E4, G4, $82, $FF    ; 8 ...your turn: up to C major
        .byte C4, E4, G4, $A3, 10     ; 9 ...null-route: C major...
        .byte B3, F4, A4, $AF, $FF    ; 10 ...falls to B half-diminished (root, 5th, 7th)

        .code

_snd_init:
.ifdef __GEOS__
        jmp frame_init          ; GEOS's frame interrupt, not the OS's VBI
snd_vbi = vbi
.else
        lda VVBLKI
        sta oldvbi
        lda VVBLKI+1
        sta oldvbi+1
        lda #6                  ; immediate
        ldy #<vbi
        ldx #>vbi
        jmp SETVBV
.endif

; Turning sound on plays a chord, so the player hears that it is on.
_snd_toggle:
        lda _snd_on
        eor #1
        sta _snd_on
        lda #CHORD_HIVE
        ; fall through

; Start chord A, unless sound is off or a higher-numbered chord is still sounding.
_play_chord:
        ldx _snd_on
        beq done
        tax
        lda vol
        beq :+
        cpx prio
        bcc done
:       stx prio
        lda #0
        sta vol                 ; the VBI keeps out while the chord loads
        ; fall through

; Load chord X and start its envelope. Called with vol = 0, or from the VBI.
load:   txa
        asl a
        asl a
        sta tick
        txa
        adc tick                ; 5 bytes a chord
        tay
        ldx #0
:       lda chords,y
        sta pitch,x
        iny
        inx
        cpx #5
        bne :-
        lda rate
        lsr a
        lsr a
        lsr a
        lsr a
        tax                     ; the starting volume
        lda rate
        and #$0F
        sta rate
        sta tick
        stx vol                 ; last: now the VBI plays it
done:   rts

vbi:    lda vol
        beq @out
        dec tick
        bne @put
        lda rate
        sta tick
        dec vol
        bne @put
        ldx next                ; faded out: the next chord of a series
        bmi @put                ; none: write volume 0 once, then stay out
        jsr load
@put:   lda pitch               ; channel 1: the held note
        sta AUDF1
        lda flip                ; channel 2: the other two in turn
        eor #1
        sta flip
        tax
        lda pitch+1,x
        sta AUDF2
        lda vol
        ora #PURE
        sta AUDC1
        sta AUDC2
.ifdef __GEOS__
@out:   rts
.else
@out:   jmp (oldvbi)
.endif
