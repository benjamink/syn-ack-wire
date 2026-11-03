; Read the lobby's AppKey file through NIO FileDevice, using the idle ring as
; scratch. The call ABI is unchanged; no legacy compatibility code is linked.
        .export _net_appkey
        .import _net_nio_init, _fn_raw_call, _rx_ring
        .import pusha, pushax
SCRATCH = _rx_ring + 80
        .bss
key:    .res 1
count:  .res 1
reply:  .res 3                  ; fn_raw_response_t: status, payload length
        .rodata
request:
        .byte 1,31,0,"persist:///FujiNet/00010100.key"
        .byte 0,0,0,0,64,0
hex:    .byte "0123456789abcdef"
        .code
_net_appkey:
        sta key
        lda #0
        sta count
        jsr _net_nio_init
        beq :+
        jmp empty
:
        ldx #39
copy_request:
        lda request,x
        sta SCRATCH,x
        dex
        bpl copy_request
        lda key
        lsr a
        lsr a
        lsr a
        lsr a
        tax
        lda hex,x
        sta SCRATCH+28
        lda key
        and #15
        tax
        lda hex,x
        sta SCRATCH+29
        lda #$fe                ; FileDevice
        jsr pusha
        lda #3                  ; File Read
        jsr pusha
        lda #<SCRATCH
        ldx #>SCRATCH
        jsr pushax
        lda #40
        ldx #0
        jsr pushax
        lda #<SCRATCH
        ldx #>SCRATCH
        jsr pushax
        lda #74
        ldx #0
        jsr pushax
        lda #<reply
        ldx #>reply
        jsr _fn_raw_call
        bne finish
        lda reply
        bne finish
        lda reply+2
        bne have_header
        lda reply+1
        cmp #10
        bcc finish
have_header:
        lda SCRATCH
        cmp #1
        bne finish
        lda SCRATCH+4
        ora SCRATCH+5
        ora SCRATCH+6
        ora SCRATCH+7
        ora SCRATCH+9
        bne finish
        lda SCRATCH+8
        cmp #65
        bcs finish
        sta count
        clc
        adc #10
        ldx reply+2
        bne copy_value
        cmp reply+1
        bcc copy_value
        beq copy_value
        lda #0
        sta count
        beq finish
copy_value:
        ldx count
        beq finish
        dex
copy_loop:
        lda SCRATCH+10,x
        sta _rx_ring+2,x
        dex
        bpl copy_loop
finish:
empty:
        ldx count
        lda #0
        sta _rx_ring+2,x
        txa
        ldx #0
        rts
