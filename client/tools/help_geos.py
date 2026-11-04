#!/usr/bin/env python3
"""Make the GEOS client's help file: the instruction pages as one GEOS file, a page a record.

    help_geos.py [--qr URL] -o HELP.cvt --name NAME --class CLASS --parent APP [--info TEXT]
                 help/atari/screen/HELP01.TXT help/atari/screen/HELP02.TXT ...

The pages are the Atari's (help_disk.py checks them and turns each into 960 screen codes,
24 rows of 40, the QR code placed), so they look as they do on the Atari: src/geos/help.c
draws them in the Atari's ROM font. The file is a VLIR file in Convert's .cvt form (what
geos-atari's tools/mkramdisk.py puts on a GEOS disk): GEOS type APPL_DATA, record n-1 is
page n. The game finds it by its class (FindFTypes), so the file's name is only what the
DeskTop shows, at most 16 characters. A class is 12 characters (padded with spaces here) and
a version, V1.0; the file's parent is the game's class, the application the DeskTop opens it
with.
"""
import argparse
import datetime
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import help_disk                            # noqa: E402

BLOCK = 254                                 # a GEOS block's data, after its link
APPL_DATA, VLIR, USR = 7, 1, 0x83

# 24x21, a page with its corner folded and a question mark on it
ICON = """
.......................
.##############........
.#............##.......
.#............#.#......
.#............#..#.....
.#............#####....
.#.................#...
.#.......####......#...
.#......##..##.....#...
.#..........##.....#...
.#.........##......#...
.#........##.......#...
.#........##.......#...
.#.................#...
.#........##.......#...
.#........##.......#...
.#.................#...
.#.................#...
.###################...
.......................
.......................
"""


def icon_bytes():
    rows = [r for r in ICON.strip("\n").split("\n")]
    assert len(rows) == 21
    out = bytearray()
    for r in rows:
        bits = r.ljust(24, ".")
        for b in range(3):
            v = 0
            for x in range(8):
                v = v << 1 | (bits[b * 8 + x] == "#")
            out.append(v)
    return bytes(out)


def field(text, size, pad=b"\0"):
    raw = text.encode("ascii")
    if len(raw) > size:
        sys.exit(f"{text!r}: more than {size} characters")
    return raw.ljust(size, pad)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--qr", help="the URL for a page's [QR]")
    ap.add_argument("-o", dest="out", required=True)
    ap.add_argument("--name", required=True, help="the file's name, at most 16 characters")
    ap.add_argument("--class", dest="cls", required=True, help="its class, at most 12 characters")
    ap.add_argument("--parent", required=True, help="the game's class (its name), at most 12 characters")
    ap.add_argument("--info", default="", help="the DeskTop's info box text")
    ap.add_argument("pages", nargs="+")
    a = ap.parse_args()

    paths = sorted(Path(p) for p in a.pages)
    pages, errs = [], []
    for want, path in enumerate(paths, 1):
        scr, e, note = help_disk.load(path, want, len(paths), a.qr)
        pages.append(scr)
        errs += e
        if note:
            print(note)
    if errs:
        sys.exit("\n".join(errs))
    if len(pages) > 127:
        sys.exit("a VLIR file has at most 127 records")

    records = bytearray(BLOCK)              # per record: its blocks, and its last block's last byte
    data = bytearray()
    for n, scr in enumerate(pages):
        blocks = (len(scr) + BLOCK - 1) // BLOCK
        records[2 * n] = blocks
        records[2 * n + 1] = len(scr) - (blocks - 1) * BLOCK + 1
        data += scr.ljust(blocks * BLOCK, b"\0")

    now = datetime.datetime.now()
    entry = bytearray(30)
    entry[0] = USR
    entry[3:19] = field(a.name, 16, b"\xa0")
    entry[21] = VLIR
    entry[22] = APPL_DATA
    entry[23:28] = bytes([now.year % 100, now.month, now.day, now.hour, now.minute])
    size = 2 + len(data) // BLOCK           # the header and the record block, then the records
    entry[28:30] = bytes([size & 0xFF, size >> 8])
    first = (bytes(entry) + b"PRG formatted GEOS file V1.0").ljust(BLOCK, b"\0")

    header = bytearray(BLOCK)               # GEOS's header block, less its link: offsets 2 lower
    header[0:3] = bytes([3, 21, 0x80 | 63])
    header[3:66] = icon_bytes()
    header[66:69] = bytes([USR, APPL_DATA, VLIR])
    header[75:95] = field(field(a.cls, 12, b" ").decode() + "V1.0", 20)
    header[115:135] = field(field(a.parent, 12, b" ").decode() + "V1.0", 20)
    header[158:158 + 96] = field(a.info, 95) + b"\0"

    Path(a.out).write_bytes(first + header + records + data)
    print(f"{a.out}: {len(pages)} pages, {len(first + header + records + data)} bytes")


if __name__ == "__main__":
    main()
