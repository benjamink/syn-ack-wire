#!/bin/bash
# A floppy image to try the game on a real Amiga: build/amiga/syn-ack-wire-amiga.adf (880 KB, OFS, not bootable).
# It holds the game and its icon, the Start script (with an icon, run by IconX) and the FujiNet NIO device and loader
# for the Workbench in FUJINET_NIO_ADF_WB (wb13, wb204, wb31 or wb32; default wb204). The game is built for the
# public server (the default URL) unless URL=N:TCP://host:port/ is given.
set -eu
cd "$(dirname "$0")/.."
WS=${FUJINET_NIO_WS:-$HOME/dev/fujinet-nio-workspace}
PKG=$WS/build/amiga-artifacts/${FUJINET_NIO_ADF_WB:-wb204}/NIO
for f in "$PKG/fujinet-nio.device" "$PKG/fujinet-load-resident"; do
    [ -f "$f" ] || { echo "missing $f" >&2; exit 1; }
done
command -v xdftool >/dev/null || { echo "needs xdftool (amitools)" >&2; exit 1; }
make -s amiga ${URL:+URL="$URL"}
D=build/amiga/adf
ADF=build/amiga/syn-ack-wire-amiga.adf
mkdir -p $D
cat > $D/Start <<'S'
.KEY dummy
; Load the FujiNet NIO device once per boot, then play. Run it from this disk: Execute SYNACK:Start
SYNACK:nio/fujinet-load-resident SYNACK:nio/fujinet-nio.device fujinet-nio.device
SYNACK:syn-ack-wire-amiga
S
python3 tools/make_icon.py tool $D/game.info
python3 tools/make_icon.py script $D/Start.info
rm -f $ADF
xdftool $ADF format SYNACK ofs >/dev/null
xdftool $ADF makedir nio >/dev/null
xdftool $ADF write build/amiga/syn-ack-wire-amiga syn-ack-wire-amiga >/dev/null
xdftool $ADF write $D/game.info syn-ack-wire-amiga.info >/dev/null
xdftool $ADF write $D/Start Start >/dev/null
xdftool $ADF write $D/Start.info Start.info >/dev/null
xdftool $ADF write "$PKG/fujinet-load-resident" nio/fujinet-load-resident >/dev/null
xdftool $ADF write "$PKG/fujinet-nio.device" nio/fujinet-nio.device >/dev/null
xdftool $ADF list
echo "wrote $ADF"
