; memset.s -- memset for the GEOS build, in place of cc65's geos-cbm.lib one.
; That one is GEOS's FillRam (dest, char what, len) under memset's name, but memset
; (ptr, int c, n) pushes its fill byte as an int: FillRam pops one byte too few, takes
; the int's high byte and the pointer's low byte as the destination ($xx00), and leaves
; a byte on the C stack. bzero and __bzero come along, so geos-cbm.lib's memset module
; (which also defines them, as ClearRam) is never linked.
;
; void* __fastcall__ memset (void* ptr, int c, size_t n);
; void* __fastcall__ __bzero (void* ptr, size_t n);
; void __fastcall__ bzero (void* ptr, size_t n);

        .export _memset, _bzero, ___bzero
        .import popax
        .importzp ptr1, ptr2, tmp1

_bzero:
___bzero:
        sta ptr2                ; n
        stx ptr2+1
        lda #0
        beq fill                ; always
_memset:
        sta ptr2
        stx ptr2+1
        jsr popax               ; c: its low byte
fill:   sta tmp1
        jsr popax               ; ptr, also the result
        sta ptr1
        stx ptr1+1
        pha
        lda tmp1
        ldy #0
        ldx ptr2+1              ; whole pages first
        beq @part
@page:  sta (ptr1),y
        iny
        bne @page
        inc ptr1+1
        dex
        bne @page
@part:  ldx ptr2                ; then the rest
        beq @done
@byte:  sta (ptr1),y
        iny
        dex
        bne @byte
@done:  lda ptr1+1              ; the result: ptr as it came
        sec
        sbc ptr2+1
        tax
        pla
        rts
