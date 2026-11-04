#!/bin/sh
# Play the GEOS client as a player would: atari800 (a PAL 130XE) boots the two disks `make geos-atr` makes, served by
# FujiNet-PC over NetSIO: the boot disk in D1: (GEOS), the GEOS disk in D2: (the file selector as its DeskTop, and
# SYN-ACK WIRE). Pick SYN-ACK WIRE in GEOS's file dialog, then the title asks for a handle; the GEOS screen comes up
# once you join.
#
# The pointer: the PC's mouse stands in for the joystick in port 1 (-mouse joy: moving it pushes the stick, its
# button is fire); MOUSESPEED=1-9 (default 3). atari800 keeps that out of its config file, so it's set here. Its
# keyboard joystick works too: the numeric keypad's 8, 4, 6 and 5, Right Ctrl for fire.
#
#   tools/run-geos-disks.sh                        build/geos/syn-ack-wire-geos-boot.atr and syn-ack-wire-geos.atr
#   tools/run-geos-disks.sh BOOT.atr GEOS.atr      other disks
#   FUJINET_DIR=dir     an Atari FujiNet-PC; INSTANCE=n its ports: NetSIO 9997+n, web 8000+n (default 7)
#   HSIO=n              FujiNet's high-speed SIO index for the boot (default 3, about 87 kbaud; -1 off, 19200):
#                       the boot disk's MyPicoDos loads GEOS at that speed (15 s at 3, 27 s at 8, 58 s off). 0
#                       stalls the boot in atari800. GEOS's own disk driver and the game's network stay at 19200.
#
# The disks are copied, so the built ones stay as they are (GEOS writes to D2:). The disks dial the public server
# unless built otherwise (make geos-atr URL=N:TCP://localhost:6599/ LOBBY_KEY=0, after make clean of build/geos).
# atari800 runs on a scratch copy of ~/.atari800.cfg, which it would otherwise rewrite on exit.
set -e
CLIENT=$(cd "$(dirname "$0")/.." && pwd)
BOOTATR=${1:-$CLIENT/build/geos/syn-ack-wire-geos-boot.atr}
GEOSATR=${2:-$CLIENT/build/geos/syn-ack-wire-geos.atr}
for f in "$BOOTATR" "$GEOSATR"; do
    [ -f "$f" ] || { echo "no $f: make geos-atr GEOS_ATARI=<geos-atari dir>" >&2; exit 1; }
done
ATARI800=${ATARI800:-atari800}
FUJINET_DIR=${FUJINET_DIR:-$HOME/Nextcloud/Projects/computers/atari8/emulators/fujisan-portable/bin/fujinet-pc}
INSTANCE=${INSTANCE:-7}
NETSIO=$((9997 + INSTANCE))
WEB=$((8000 + INSTANCE))
for p in $NETSIO $WEB; do
    if ss -ltnu | grep -q ":$p\b"; then
        echo "port $p is in use: pick another INSTANCE" >&2
        exit 1
    fi
done
WORK=$(mktemp -d "${TMPDIR:-/tmp}/geos-disks.XXXXXX")
mkdir "$WORK/sd"
cp "$BOOTATR" "$WORK/sd/boot.atr"
cp "$GEOSATR" "$WORK/sd/geos.atr"

awk -v netsio=$NETSIO -v hsio="${HSIO:-3}" '/^\[/ { skip = /^\[Mount[0-9]+\]/; boip = /^\[BOIP\]/ }
     /^configenabled=/ { $0 = "configenabled=0" }
     /^boot_mode=/ { $0 = "boot_mode=0" }
     /^hsioindex=/ { $0 = "hsioindex=" hsio }
     boip && /^port=/ { $0 = "port=" netsio }
     !skip' "$FUJINET_DIR/fnconfig.ini" > "$WORK/fnconfig.ini"
printf '\n[Mount1]\nhostslot=1\npath=/boot.atr\nmode=r\n\n[Mount2]\nhostslot=1\npath=/geos.atr\nmode=w\n' \
    >> "$WORK/fnconfig.ini"

(cd "$FUJINET_DIR" && while :; do
    rc=0
    stdbuf -oL ./fujinet -c "$WORK/fnconfig.ini" -s "$WORK/sd" -u "http://0.0.0.0:$WEB" || rc=$?
    [ $rc -eq 75 ] || break
done) > "$WORK/fujinet.log" 2>&1 &
FN=$!
trap 'pkill -P $FN 2>/dev/null; kill $FN 2>/dev/null; echo "logs: $WORK"' EXIT INT TERM

[ -f "$HOME/.atari800.cfg" ] && cp "$HOME/.atari800.cfg" "$WORK/atari800.cfg"
"$ATARI800" -config "$WORK/atari800.cfg" -xe -pal -nobasic -netsio $NETSIO -mouse joy -mousespeed "${MOUSESPEED:-3}"
