; net_sio.s -- the platform.h network calls for the Atari: FujiNet N1: over raw SIO.
; No N: CIO handler and no fujinet-lib. Every call returns A = 0 when fine,
; else the SIO status (128-255) or NET_CLOSED; X = 0.
;
; PROCEED: FujiNet pulls SIO pin 9 low when the N: device has data or news. It
; reaches the PIA's CA1 input. We can't just read the latched flag in PACTL,
; because the OS vertical blank reads PORTA for the joysticks every frame, and
; that clears it. So the CA1 interrupt is enabled and a 6-byte handler sets
; `ready`. net_poll only spends an SIO status call when `ready` is set, or every
; 4 seconds as a safety net for a missed pulse or a quiet disconnect.
;
; SIO is our own, not the OS's (derived from the GEOS port's disk driver): it
; talks to POKEY directly, polled with IRQs masked, always at 19200, so nothing
; that hooks SIOV (a U1MB's or Incognito's high-speed SIO) changes it, and it
; leaves POKEY channels 1 and 2 alone, so a chord sounds straight through a
; transfer. Channels 3 and 4 are its serial clock. The OS's VBI and our DLIs
; still run: at 19200 a byte takes about 930 cycles. A key or PROCEED during a
; transfer waits in its latch until the transfer ends. A command frame without
; an ACK is sent again (the first after a boot at high speed needs it, while
; FujiNet switches back); once acknowledged, a failed transfer is an error,
; never a resend, as the network device has already acted on it.

        .export _net_open, _net_poll, _net_send, _net_close, _net_appkey
        .export _rx_head, _rx_tail, _tx
        .import _rx_ring
        .importzp ptr1

RTCLOK  = $14           ; jiffy counter, low byte
POKMSK  = $10           ; IRQEN shadow
SSKCTL  = $0232         ; SKCTL shadow
AUDF3   = $D204
AUDF4   = $D206
AUDCTL  = $D208
SKREST  = $D20A
SERIO   = $D20D         ; SEROUT written, SERIN read
IRQEN   = $D20E         ; IRQST when read; its bits are 0 while latched
SKCTL   = $D20F         ; SKSTAT when read
PBCTL   = $D303         ; $34: the SIO COMMAND line low, $3C: high
WSYNC   = $D40A
VPRCED  = $0202         ; PROCEED interrupt vector
PORTA   = $D300         ; reading it clears the CA1 interrupt flag
PACTL   = $D302         ; bit 0 enables the CA1 (PROCEED) interrupt; bit 3 is the
                        ; cassette motor, so only ever OR into it
SAFETY  = 240           ; jiffies between status calls with no PROCEED (4 s; each is a
                        ; faint chirp of serial noise, and NO SIGNAL waits 20 s anyway)

NDEV    = $71           ; N1:
FUJI    = $70           ; the FujiNet device itself: AppKeys
NET_CLOSED = $FF
TRIES   = 13            ; command frames sent before giving up, as the OS
ACKWAIT = 15            ; frames to wait for an ACK, and between data bytes
KEYS    = $C0           ; IRQEN/IRQST bits: BREAK and the keyboard, kept on so a key
SERIN   = $20           ;   waits in its latch; a byte came in,
SEROR   = $10           ;   SEROUT wants the next,
SEROC   = $08           ;   the last has gone out
SIOERR  = $8A           ; any failure, as the OS's timeout

; command index -> SIO command, direction
OPEN    = 0
STATUS  = 1
READ    = 2
WRITE   = 3
CLOSE   = 4
AKOPEN  = 5             ; from here on, commands for FUJI
AKREAD  = 6

        .bss
_rx_head: .res 1        ; next byte the frame assembler takes
_rx_tail: .res 1        ; next byte SIO fills
_tx:    .res 16
stat:   .res 4          ; bytes waiting lo/hi, connected, error
n:      .res 1
ready:  .res 1          ; non-zero: FujiNet has something, or bytes were left behind
last:   .res 1          ; RTCLOK at the last status call
frame:  .res 4          ; the command frame: device, command, AUX1, AUX2
len:    .res 1          ; data bytes, 0 for 256 (the buffer is at ptr1)
mode:   .res 1          ; $80 send data, $40 receive it, 0 neither
sum:    .res 1          ; the frame's checksum so far
txbusy: .res 1          ; bit 7 set once SEROUT has a byte of this frame
ticks:  .res 1          ; frames left to wait
tlast:  .res 1          ; RTCLOK when last looked at
tries:  .res 1
want:   .res 1          ; the data's own checksum, then the long wait's count

        .data
akey:   .word 1         ; AppKey open block: creator (FujiNet), app (the lobby),
        .byte 1, 0, 0, 0 ;   key, mode (read), reserved

        .rodata
cmd:    .byte 'O', 'S', 'R', 'W', 'C', $DC, $DD
dir:    .byte $80, $40, $40, $80, $00, $80, $40

        .code

; unsigned char net_appkey(unsigned char key)
; Open the lobby's AppKey, then read it into the ring: a 2-byte length, then 64
; bytes. A missing key reads as length 0; older firmware fails the read instead.
_net_appkey:
        sta akey+3
        lda #6
        ldx #<akey
        ldy #AKOPEN
        jsr xfer_ak
        bne @none
        lda #2 + 64
        ldx #<_rx_ring
        ldy #AKREAD
        jsr xfer_ring
        bne @none
        lda _rx_ring            ; length, low byte: never over 64
        cmp #64 + 1
        bcc :+
@none:  lda #0
:       tay
        lda #0
        sta _rx_ring+2,y        ; a C string from rx_ring[2], empty if none
        tax
        tya
        rts

; unsigned char net_open(const char *url)
; The devicespec is sent as a 256-byte block; bytes after the NUL are ignored.
_net_open:
        sta ptr1
        stx ptr1+1
        ldy #<proceed
        sty VPRCED
        ldy #>proceed
        sty VPRCED+1
        lda PORTA               ; drop any stale PROCEED edge
        lda PACTL
        ora #1
        sta PACTL               ; CA1 interrupt on
        ldy #0
        sty _rx_head            ; anything left from an old connection is junk
        sty _rx_tail
        sty len                 ; 256 bytes
        sty frame+3             ; AUX2: translation 0, bytes pass untouched
        iny
        sty ready               ; Y = 1: check status on the first poll
        lda #12
        sta frame+2             ; AUX1: read + write
        ldx #OPEN
        jmp sio

; PROCEED handler. The OS IRQ code pushed A before jumping here.
proceed:
        lda PORTA               ; clear the PIA's interrupt flag
        lda #1
        sta ready
        pla
        rti

; unsigned char net_poll(void)
; Cheap unless PROCEED fired (or 4 seconds have passed): then one status call and,
; if bytes are waiting, one read of as many as fit before the ring's page end
; without overrunning unparsed bytes. Bytes left behind keep `ready` set.
_net_poll:
        lda ready
        bne @go
        lda RTCLOK
        sec
        sbc last
        cmp #SAFETY
        bcc ok
@go:    lda #0
        sta ready               ; a PROCEED from here on means "poll again"
        lda RTCLOK
        sta last
        lda #4
        ldx #<stat
        ldy #STATUS
        jsr xfer_hi
        bne done
        lda stat+1
        bne @cap                ; 256 or more waiting: take 255 now
        lda stat
        bne @have
        lda stat+2              ; nothing waiting: still connected?
        bne ok
        lda #NET_CLOSED
        rts                     ; X = 0 from sio
@cap:   lda #$FF
@have:  sta n
        lda _rx_head            ; free = head - tail - 1
        clc
        sbc _rx_tail
        cmp n
        bcs :+
        sta n
:       lda _rx_tail            ; room to page end = 256 - tail (tail 0: whole page)
        beq :+
        lda #0
        sec
        sbc _rx_tail
        cmp n
        bcs :+
        sta n
:       lda n
        beq @more               ; ring full: the assembler drains it first
        ldx _rx_tail
        ldy #READ
        jsr xfer_ring
        bne done
        lda _rx_tail
        clc
        adc n
        sta _rx_tail
        lda stat+1              ; all of it read?
        bne @more
        lda stat
        cmp n
        beq ok
@more:  inc ready               ; no: read the rest on the next poll
ok:     lda #0
        tax
done:   rts

; void net_close(void)
_net_close:
        ldx #CLOSE
        bne sio                 ; always

; unsigned char net_send(unsigned char len) -- sends tx[0..len-1]
_net_send:
        ldx #<_tx
        ldy #WRITE
        bne xfer                ; always

; A = length (1-255), X = buffer low byte, Y = command. xfer: buffer in tx's page,
; xfer_hi: in stat's page, xfer_ring: in the ring's page, xfer_ak: in akey's page.
; Length doubles as AUX1/2 (FujiNet ignores it for AppKeys).
xfer_ak:
        pha
        lda #>akey
        bne xfer_page           ; always
xfer_ring:
        pha
        lda #>_rx_ring
        bne xfer_page           ; always
xfer_hi:
        pha
        lda #>stat
        bne xfer_page           ; always
xfer:   pha
        lda #>_tx
xfer_page:
        sta ptr1+1
        stx ptr1
        pla
        sta len
        cpy #STATUS
        bne :+
        lda #0                  ; status takes no AUX length
:       sta frame+2
        lda #0
        sta frame+3
        tya
        tax

; X = command index. Returns A = 0 if fine, else SIOERR; X = 0.
sio:    lda #NDEV
        cpx #AKOPEN
        bcc :+
        lda #FUJI
:       sta frame               ; unit 1: the device byte as it is
        lda cmd,x
        sta frame+1
        lda dir,x
        sta mode
        php
        sei
        lda #$28                ; channel 3 at 1.79 MHz, joined to 4: 19200 baud
        sta AUDCTL              ; (channels 3 and 4 stay at volume 0: the OS's
        sta AUDF3               ; SIO leaves them so, and nothing else plays them)
        lda #0
        sta AUDF4
        lda #TRIES
        sta tries
@cmd:   jsr command
        bcc @acked
        dec tries
        bne @cmd
        beq @end                ; always (carry set)
@acked: bit mode
        bmi @write
        bvc @done               ; neither way: just COMPLETE
        jsr complete
        bcs @end
        sta sum                 ; A = 0: the checksum covers the data only
        tay
:       jsr getb
        bcs @end
        sta (ptr1),y
        iny
        cpy len
        bne :-
        lda sum
        sta want
        jsr getb
        bcs @end
        eor want
        bne @bad
        lda SKCTL               ; framing error or overrun? (bits 7, 6: 0 if so)
        and #$C0
        eor #$C0
        beq @end                ; no: fine (carry clear)
@bad:   sec
        bcs @end                ; always
@write: ldy #20                 ; at least 1 ms after the ACK
        jsr delay
        jsr sendstart
        ldy #0
:       lda (ptr1),y
        jsr putb
        iny
        cpy len
        bne :-
        jsr sendsum
        jsr ack
        bcs @end
@done:  jsr complete
@end:   lda #0
        bcc :+
        lda #SIOERR
:       tay
        lda SSKCTL              ; the serial port, IRQs and COMMAND as the OS left them
        sta SKCTL
        lda POKMSK
        sta IRQEN
        lda #$3C
        sta PBCTL
        plp
        ldx #0
        tya                     ; Z for the callers
        rts

; Send the command frame and wait for its ACK: carry clear if it came.
command:
        lda #$34                ; COMMAND low
        sta PBCTL
        ldy #14                 ; 750-1600 us before the frame
        jsr delay
        jsr sendstart
        ldy #0
:       lda frame,y
        jsr putb
        iny
        cpy #4
        bne :-
        jsr sendsum
        ldy #12                 ; 650-950 us after it
        jsr delay
        lda #$3C                ; COMMAND high
        sta PBCTL
ack:    jsr getb                ; (sendsum left us listening)
        bcs no
        cmp #'A'
        beq yes
no:     sec
        rts

; Wait up to 16 s for COMPLETE: carry clear and A = 0 if it came.
complete:
        lda #4
        sta want
:       lda #240
        jsr getbt
        bcc :+
        dec want
        bne :-
        rts                     ; carry set
:       cmp #'C'
        bne no
yes:    lda #0
        clc
        rts

; A byte into A, added to the checksum: carry clear, or set if none came in
; ACKWAIT frames (getbt: in A frames). Keeps Y.
getb:   lda #ACKWAIT
getbt:  sta ticks
        lda RTCLOK
        sta tlast
@wait:  lda IRQEN
        and #SERIN
        beq @got
        lda RTCLOK
        cmp tlast
        beq @wait
        sta tlast
        dec ticks
        bne @wait
        sec
        rts
@got:   ldx #KEYS|SERIN         ; let the latch take the next byte
        jsr irqs
        lda SERIO
addsum: pha                     ; add A to the checksum; keeps A, carry clear
        clc
        adc sum
        adc #0
        sta sum
        pla
        clc
        rts

; Send the checksum, wait for it to go out, then listen.
sendsum:
        lda sum
        jsr putb
:       lda IRQEN               ; the last byte in the shift register,
        and #SEROR
        bne :-
:       lda IRQEN               ; and out
        and #SEROC
        bne :-
        lda #$13                ; receive, clocked by channels 3 and 4
        sta SKCTL
        sta SKREST
        ldx #KEYS|SERIN
        bne irqs                ; always

; Send A, adding it to the checksum. Keeps Y.
putb:   jsr addsum
        bit txbusy              ; the frame's first byte goes straight out
        bpl @put
        pha
:       lda IRQEN               ; wait for SEROUT to take the next
        and #SEROR
        bne :-
        ldx #KEYS|SEROR|SEROC
        jsr irqs
        pla
@put:   sta SERIO
        sec
        ror txbusy
        rts

; Ready POKEY to send a frame; the checksum starts at 0.
sendstart:
        lda #$23                ; send, clocked by channel 4
        sta SKCTL
        sta SKREST
        lda #0
        sta sum
        sta txbusy
        ldx #KEYS|SEROR|SEROC
        ; fall through

; Re-arm POKEY's IRQ latches: IRQEN to the keys alone, then to X.
irqs:   lda #KEYS
        sta IRQEN
        stx IRQEN
        rts

; Wait Y scan lines.
delay:  sta WSYNC
        dey
        bne delay
        rts
