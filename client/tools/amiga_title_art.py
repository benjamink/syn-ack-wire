#!/usr/bin/env python3
"""Paint the Amiga client's title picture (320 x 256, 32 colours) and write it as src/amiga/title_art.c.

The picture is drawn straight into palette indices, with a 4x4 ordered dither between neighbouring pens of a ramp, as
a 90s Amiga artist would have in Deluxe Paint. The pens the title's buttons and text use keep the game's colours (the
palette of src/amiga/palette.h); the free ones (8-14, the logo's colour-cycling ramp, and 20-26, the sky) are the
picture's own. Run:  tools/amiga_title_art.py  [preview.png]   (needs Pillow and numpy; the output is committed, since
the font it paints the logo with differs from machine to machine.)"""
import os, re, sys, tomllib
import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
W, H = 320, 256
FONT = "/usr/share/fonts/gsfonts/NimbusSansNarrow-BoldOblique.otf"

TITLE = tomllib.load(open(os.path.join(ROOT, "theme", "theme.toml"), "rb"))["game"]["title"]

# the game's palette, read from the generated header so the two cannot differ
pal = [int(m, 16) for m in re.findall(r"0x([0-9A-F]{4}),", open(os.path.join(HERE, "..", "src", "amiga", "palette.h")).read())]
assert len(pal) == 32
FREE = {                                # the picture's own colours (0x0RGB)
    8: 0x0066, 9: 0x00A9, 10: 0x00DD, 11: 0x08FF, 12: 0x00DD, 13: 0x00A9, 14: 0x0066,      # the logo's ramp, a light that runs along it
    20: 0x0021, 21: 0x0053, 22: 0x0086, 23: 0x00B8, 24: 0x00F9, 25: 0x06FB, 26: 0x0CFD,    # phosphor green, dark to bright
}
pal = [FREE.get(i, c) for i, c in enumerate(pal)]
BG0, BG1, BG2, EDGE, EDGEHI, WHITE, DIM, STEEL, CYAN, AMBER, RED, MAG = 0, 4, 5, 6, 7, 3, 2, 29, 15, 16, 27, 31
RAMP = list(range(8, 15))
GREEN = [BG0, 20, 21, 22, 23, 24, 25, 26]

BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0
img = np.zeros((H, W), np.uint8)
ys, xs = np.mgrid[0:H, 0:W]


def ramp_pick(t, ramp):
    """t in 0..1 over the ramp -> a pen, dithered between the two nearest."""
    f = np.clip(t, 0, 0.9999) * (len(ramp) - 1)
    lo = f.astype(int)
    frac = f - lo
    hi = np.minimum(lo + 1, len(ramp) - 1)
    ra = np.array(ramp)
    return np.where(frac > BAYER[ys[:t.shape[0], :t.shape[1]] % 4, xs[:t.shape[0], :t.shape[1]] % 4], ra[hi], ra[lo])


rng = np.random.RandomState(1990)

# the rain: columns of falling phosphor with a bright head, behind the logo
SKY_END = 64
img[:SKY_END] = BG0
for x in range(0, W, 3):
    if rng.rand() < 0.8:
        head = rng.randint(6, SKY_END + 18)
        ln = rng.randint(8, 34)
        for k in range(ln):
            y = head - k
            if 0 <= y < SKY_END:
                lvl = 7 if k == 0 else max(1, 6 - k * 6 // ln)
                img[y, x] = GREEN[lvl]
                if k == 0 and x + 1 < W:
                    img[y, x + 1] = GREEN[3]
for _ in range(120):                                  # stray glyph specks in the dark between the columns
    x, y = rng.randint(0, W), rng.randint(0, SKY_END)
    if img[y, x] == BG0:
        img[y, x] = [20, 21][rng.randint(0, 2)]

# the ticker strip (the scroller runs in rows 66..83, which are pen 0), with tick marks along its rules
img[64] = EDGEHI
img[65] = EDGE
img[66:84] = BG0
img[84] = EDGE
img[85] = EDGEHI
for x in range(0, W, 8):
    img[63, x] = CYAN if x % 64 == 0 else EDGE
    img[86, x] = CYAN if x % 64 == 0 else EDGE

# below the strip: a board of circuit traces (the panels and buttons cover most of it), and stray stars of light
img[87:190] = BG0
for y in range(96, 190, 16):
    for x in range(8, W, 16):
        img[y, x] = BG2
for _ in range(34):                                   # a trace walks in straight runs and 45 degree bends and ends in a pad
    x, y = int(rng.randint(0, W)), int(rng.randint(90, 188))
    dx, dy = [(1, 0), (-1, 0), (0, 1), (0, -1)][rng.randint(0, 4)]
    for seg in range(rng.randint(3, 7)):
        for _ in range(rng.randint(6, 24)):
            if 0 <= x < W and 87 <= y < 190:
                img[y, x] = EDGE
            x += dx; y += dy
        dx, dy = [(dx, dy), (dy, dx), (-dy, -dx), (dx + dy, dy - dx) if dx and not dy else (dx, dy)][rng.randint(0, 3)]
        dx, dy = max(-1, min(1, dx)), max(-1, min(1, dy))
    if 1 <= x < W - 1 and 88 <= y < 189:
        img[y - 1:y + 2, x - 1:x + 2] = CYAN
        img[y, x] = BG0
for x in range(0, 24):                                # the board's edge connectors, left and right
    for k in range(0, 100, 6):
        img[96 + k, x] = EDGE if x < 18 else EDGEHI
        img[96 + k, W - 1 - x] = EDGE if x < 18 else EDGEHI

# PAL's bottom band: a wireframe globe on the horizon over a grid floor
HOR = 208
img[190:HOR] = BG0
for _ in range(30):
    x, y = rng.randint(0, W), rng.randint(191, HOR - 2)
    img[y, x] = [BG2, EDGE, DIM, STEEL][rng.randint(0, 4)]
img[HOR - 7:HOR] = ramp_pick(np.broadcast_to(((np.arange(7)[:, None] + 1) / 7.0), (7, W)), GREEN)
img[HOR:H] = BG1
GR, GCX, GCY = 19, 160, HOR + 1
rr2 = (xs - GCX) ** 2 + (ys - GCY) ** 2
globe = (rr2 <= GR * GR) & (ys < HOR)
img[globe] = BG0
for k in range(-3, 4):                                # meridians: ellipses of shrinking width
    ex = GR * abs(k) / 4.0
    if k == 0:
        img[globe & (np.abs(xs - GCX) < 1)] = 24
    else:
        ell = np.abs(((xs - GCX) / max(ex, 1)) ** 2 + ((ys - GCY) / float(GR)) ** 2 - 1) < 0.08
        img[globe & ell] = 23
for lat in (6, 12, 18):                               # parallels
    img[globe & (np.abs(ys - (HOR - lat)) < 1)] = 22
img[(np.abs(rr2 - GR * GR) < GR * 2) & (ys < HOR)] = 25        # the rim
for i in range(6):                                    # floor lines, closer together towards the horizon
    y = HOR + int((i / 6.0) ** 2 * (H - HOR))
    img[y, :] = CYAN if i < 3 else EDGEHI
for j in range(-7, 8):                                # and the lines that run to the horizon
    for y in range(HOR, H):
        x = int(160 + j * 56 * (y - HOR + 1) / (H - HOR))
        if 0 <= x < W:
            img[y, x] = CYAN
img[HOR, :] = 25

# HUD corner brackets round the whole picture
for (bx, by, sx, sy) in ((1, 1, 1, 1), (W - 2, 1, -1, 1), (1, H - 2, 1, -1), (W - 2, H - 2, -1, -1)):
    for k in range(12):
        img[by, bx + sx * k] = CYAN
        img[by + sy * k, bx] = CYAN

# the logo: big, slanted, a ramp for a fill (colour-cycled at run time), a white top light, a dark outline and a shadow
size = 60
while True:
    f = ImageFont.truetype(FONT, size)
    l, t_, rr, b = f.getbbox(TITLE)
    if rr - l <= 304 or size < 20:
        break
    size -= 1
m = Image.new("1", (W, H), 0)
dm = ImageDraw.Draw(m)
dm.fontmode = "1"
tx = (W - (rr - l)) // 2 - l
ty = 8 - t_
dm.text((tx, ty), TITLE, font=f, fill=1)
mask = np.array(m, bool)
for y0, h, dx in ((19, 2, -3), (39, 2, 3)):      # glitch: thin slices of the logo slip sideways
    mask[y0:y0 + h] = np.roll(mask[y0:y0 + h], dx, axis=1)
ys_in = np.where(mask.any(axis=1))[0]
top_y, bot_y = ys_in.min(), ys_in.max()


def grow(a, n):
    out = a.copy()
    for _ in range(n):
        o = out.copy()
        o[1:] |= out[:-1]; o[:-1] |= out[1:]; o[:, 1:] |= out[:, :-1]; o[:, :-1] |= out[:, 1:]
        o[1:, 1:] |= out[:-1, :-1]; o[:-1, :-1] |= out[1:, 1:]; o[1:, :-1] |= out[:-1, 1:]; o[:-1, 1:] |= out[1:, :-1]
        out = o
    return out


outline = grow(mask, 2)
shadow = np.roll(np.roll(grow(mask, 2), 3, axis=0), 2, axis=1)
img[shadow & ~outline] = BG0
img[outline & ~mask] = BG0
img[np.roll(mask, -2, axis=1) & ~mask] = MAG          # the colour fringes of a screen out of register
band = ((xs // 9 + ys // 5) % 7)                      # diagonal bands of the ramp: cycling runs a light along the letters
fill = np.array(RAMP)[band]
img[mask] = fill[mask]
edge_top = mask & ~np.roll(mask, 1, axis=0)         # the top light
edge_top2 = mask & np.roll(edge_top, 1, axis=0)
img[edge_top] = WHITE
img[edge_top2 & ~edge_top] = np.where((xs[edge_top2 & ~edge_top] % 2) == 0, WHITE, img[edge_top2 & ~edge_top])
low = mask & ~np.roll(mask, -1, axis=0)             # the bottom shade
img[low] = BG1

if len(sys.argv) > 1:
    rgb = np.array([[(c >> 8 & 15) * 17, (c >> 4 & 15) * 17, (c & 15) * 17] for c in pal], np.uint8)
    Image.fromarray(rgb[img]).resize((W * 3, H * 3), Image.NEAREST).save(sys.argv[1])

# planar, 5 planes of 40 bytes a row, then packed: a run of the same byte is (257 - n, byte), literals are (n - 1, bytes)
planes = []
for p in range(5):
    bits = ((img >> p) & 1).astype(np.uint8)
    planes.append(np.packbits(bits, axis=1).tobytes())
raw = b"".join(planes)


def pack(data):
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        run = 1
        while i + run < n and data[i + run] == data[i] and run < 128:
            run += 1
        if run >= 3:
            out += bytes([257 - run, data[i]])
            i += run
        else:
            j = i
            while j < n and j - i < 128 and not (j + 2 < n and data[j] == data[j + 1] == data[j + 2]):
                j += 1
            if j == i:
                j = i + 1
            out += bytes([j - i - 1]) + data[i:j]
            i = j
    return bytes(out)


packed = pack(raw)
dest = os.path.join(HERE, "..", "src", "amiga", "title_art.c")
with open(dest, "w") as o:
    o.write("/* GENERATED by tools/amiga_title_art.py: the title picture, 320 x 256 in 5 planes, packed (see gfx_art). Do not edit. */\n")
    o.write('#include "title_art.h"\n\n')
    o.write("const unsigned short title_art_palette[32] = {\n")
    for i in range(0, 32, 8):
        o.write("    " + ", ".join("0x%04X" % c for c in pal[i:i + 8]) + ",\n")
    o.write("};\n\n")
    o.write("const unsigned char title_art[%d] = {\n" % len(packed))
    for i in range(0, len(packed), 20):
        o.write("    " + ",".join(str(b) for b in packed[i:i + 20]) + ",\n")
    o.write("};\n\nconst unsigned short title_art_len = %d;\n" % len(packed))
print("title_art.c: %d bytes packed from %d" % (len(packed), len(raw)))
