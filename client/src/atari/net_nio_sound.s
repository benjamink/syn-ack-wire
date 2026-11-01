; Sound restoration hook shared with sound.s (SIO stops the POKEY channels).
        .export net_hush, _net_hush, net_after, _nio_after
        .bss
net_hush:
_net_hush: .res 1
        .data
net_after: .addr noop
        .code
_nio_after: jmp (net_after)
noop:   rts
