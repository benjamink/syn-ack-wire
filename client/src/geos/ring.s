; The receive ring at $0400, the first page of GEOS's application RAM: page-aligned,
; so an 8-bit index wraps for free. The program starts after it, at $0500 (the
; Makefile's -S), so the ring costs nothing in the file.
        .export _rx_ring
_rx_ring = $0400
