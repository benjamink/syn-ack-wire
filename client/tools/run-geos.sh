#!/bin/sh
# Run the GEOS client (a .cvt) under geos-atari on atari800, a PAL 130XE, with FujiNet-PC on NetSIO for the
# network and the GEOS disk. The disk is made here from the .cvt (geos-atari's tools/mkramdisk.py), with the
# program named DESK TOP, so GEOS starts straight into it; it sits in D2:, where geos-atari's D2 build
# (SIO_UNIT=2, as on the release's boot disk) looks for it, and atari800 loads that GEOS itself (-run).
#
#   tools/run-geos.sh [build/geos/synack.cvt]               emulator window (that .cvt if none is given)
#   HEADLESS=1 tools/run-geos.sh build/geos/synack-auto.cvt  no window, no sound
#   DUMPDIR=/tmp/g tools/run-geos.sh build/geos/synack-auto.cvt
#       headless; every BRK the autoplay build executes saves its text copy (DUMPDIR/scrNNN.bin, 960 bytes of
#       Atari screen codes: tools/screen2txt.py) and the bitmap (DUMPDIR/bmpNNN.bin, 24 rows of 320 lines)
#   BOOT=build/geos/syn-ack-wire-geos-boot.atr tools/run-geos.sh ...
#                       boot GEOS from that disk in D1:, as a player does, instead of -run
#   GEOS_ATARI=dir      the geos-atari checkout (default ../../geos-atari): its build/atari_130_sio_d2 GEOS
#                       (make -C $GEOS_ATARI DRIVE=siodrv_atari SIO_UNIT=2)
#   FUJINET_DIR=dir     an Atari FujiNet-PC; INSTANCE=n its ports: NetSIO 9997+n, web 8000+n (default 7)
#
# Start the server first (cd ../server && python3 serve.py --host 127.0.0.1 --port 6599) and build the client
# for it (make GEOS=... URL=N:TCP://localhost:6599/ LOBBY_KEY=0 .../synack-auto.cvt). atari800 runs on a scratch
# copy of ~/.atari800.cfg, which it would otherwise rewrite on exit.
set -e
CLIENT=$(cd "$(dirname "$0")/.." && pwd)
CVT=${1:-$CLIENT/build/geos/synack.cvt}
[ -f "$CVT" ] || { echo "no $CVT: make geos (or give a .cvt)" >&2; exit 1; }
CVT=$(realpath "$CVT")
GEOS_ATARI=$(realpath "${GEOS_ATARI:-$CLIENT/../../geos-atari}")
XEX=$GEOS_ATARI/build/atari_130_sio_d2/GEOS_ATARI_130XE_SIO.XEX
ATARI800=${ATARI800:-atari800}
FUJINET_DIR=${FUJINET_DIR:-$HOME/Nextcloud/Projects/computers/atari8/emulators/fujisan-portable/bin/fujinet-pc}
INSTANCE=${INSTANCE:-7}
NETSIO=$((9997 + INSTANCE))
WEB=$((8000 + INSTANCE))
[ -f "$XEX" ] || { echo "no $XEX: make -C $GEOS_ATARI DRIVE=siodrv_atari SIO_UNIT=2" >&2; exit 1; }
for p in $NETSIO $WEB; do
    if ss -ltnu | grep -q ":$p\b"; then
        echo "port $p is in use: pick another INSTANCE" >&2
        exit 1
    fi
done
WORK=$(mktemp -d "${TMPDIR:-/tmp}/geos-run.XXXXXX")
mkdir "$WORK/sd" "$WORK/cvt"

# the program as DESK TOP: the name is bytes 3-18 of the .cvt's directory entry, padded with $A0
python3 -I - "$CVT" "$WORK/cvt/desktop.cvt" <<'EOF'
import sys
d = bytearray(open(sys.argv[1], "rb").read())
d[3:19] = b"DESK TOP".ljust(16, b"\xa0")
open(sys.argv[2], "wb").write(d)
EOF
python3 "$GEOS_ATARI/tools/mkramdisk.py" --atr 717 -o "$WORK/sd/geos.atr" "$WORK/cvt/desktop.cvt" > "$WORK/mkdisk.log"

awk -v netsio=$NETSIO '/^\[/ { skip = /^\[Mount[0-9]+\]/; boip = /^\[BOIP\]/ }
     /^configenabled=/ { $0 = "configenabled=0" }
     /^boot_mode=/ { $0 = "boot_mode=0" }
     boip && /^port=/ { $0 = "port=" netsio }
     !skip' "$FUJINET_DIR/fnconfig.ini" > "$WORK/fnconfig.ini"
printf '\n[Mount2]\nhostslot=1\npath=/geos.atr\nmode=w\n' >> "$WORK/fnconfig.ini"
RUN="-run $XEX"
if [ -n "$BOOT" ]; then
    cp "$BOOT" "$WORK/sd/boot.atr"
    printf '\n[Mount1]\nhostslot=1\npath=/boot.atr\nmode=r\n' >> "$WORK/fnconfig.ini"
    RUN=
fi

(cd "$FUJINET_DIR" && while :; do
    rc=0
    stdbuf -oL ./fujinet -c "$WORK/fnconfig.ini" -s "$WORK/sd" -u "http://0.0.0.0:$WEB" || rc=$?
    [ $rc -eq 75 ] || break
done) > "$WORK/fujinet.log" 2>&1 &
FN=$!
trap 'pkill -P $FN 2>/dev/null; kill $FN 2>/dev/null; echo "logs: $WORK"' EXIT INT TERM

[ -f "$HOME/.atari800.cfg" ] && cp "$HOME/.atari800.cfg" "$WORK/atari800.cfg"
OPTS="-config $WORK/atari800.cfg -xe -pal -nobasic -netsio $NETSIO -nojoystick -no-kbdjoy1"
if [ -n "$DUMPDIR" ]; then
    mkdir -p "$DUMPDIR"
    shadow=$(sed -n 's/^al 00\([0-9A-F]*\) \._shadow$/\1/p' "${CVT%.cvt}.lbl")
    [ -n "$shadow" ] || { echo "no _shadow in ${CVT%.cvt}.lbl" >&2; exit 1; }
    end=$(printf '%X' $((0x$shadow + 959)))
    i=0
    while [ $i -lt 999 ]; do
        printf 'WRITE %s %s %s/scr%03d.bin\nWRITE A0D8 BED7 %s/bmp%03d.bin\nCONT\n' "$shadow" "$end" "$DUMPDIR" $i \
            "$DUMPDIR" $i
        i=$((i + 1))
    done > "$WORK/monitor.txt"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$ATARI800" $OPTS -bbrk $RUN \
        < "$WORK/monitor.txt" > "$WORK/atari800.log" 2>&1
elif [ -n "$HEADLESS" ]; then
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$ATARI800" $OPTS $RUN > "$WORK/atari800.log" 2>&1
else
    "$ATARI800" $OPTS $RUN
fi
