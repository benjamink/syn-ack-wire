; Minimal soft65c02 entry: initialise the cc65 software stack and run one case.
        .export init_harness, end_init_harness
        .import _test_func
        .importzp c_sp
        .segment "CODE"
init_harness:
        lda #$00
        sta c_sp
        lda #$80
        sta c_sp+1
        jsr _test_func
end_init_harness:
        brk
