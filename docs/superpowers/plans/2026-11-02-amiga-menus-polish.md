# Amiga Client Step 3: Menus and Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline) to implement this plan task-by-task.

**Goal:** the Amiga client gets real instruction pages with the QR code, pull-down menus (Project, Game, Region), a remembered view/hints/sound, a cycling title, and a busy pointer.

**Architecture:** all of it is `client/src/amiga/` plus `client/tools/`; `common/` stays untouched. The instruction pages are the MS-DOS pages (`help/msdos/screen/HELP*.TXT`, one text source for the 40-column platforms) turned by a new `tools/help_amiga.py` into a generated header (`help_tab.h`: text lines and the QR module bitmap), compiled in, so no file lookup can fail on a floppy setup. Menus are Kickstart-1.3 Intuition `Menu`/`MenuItem` structures on the game window; a pick becomes the same key the keyboard types (or a call into the display), so game.c still decides what is legal. The Region menu is generated from `server/servers.fly.toml` by `tools/gen_regions.py` (a new `a` output) and only works from the title, where the dial has not happened yet.

**Tech Stack:** m68k-amigaos-gcc 15, Kickstart 1.3 API only (no gadtools), Python 3 tools, host-tested pure layers, the Amiberry rig for the screens.

**Spec:** `docs/superpowers/specs/2026-11-01-amiga-client-design.md` (step 3: "Menus and polish"; menus, Region, instructions with a QR bitmap, palette cycling, busy pointer; the Copper gradient is "optional later").

## Global Constraints

- Kickstart 1.3 API only; no clib2, gadtools, ReqTools, asl, datatypes; `-mcpu=68000 -msoft-float`.
- `client/src/common/` untouched. Names and terms from `theme.toml` only (help text pages and title art excepted).
- Keep the public default URL unchanged; tests dial only the local scratch server (`test/amiga_server.py`, port 6599).
- Report binary size changes (the sizes table in `client/README.md`).
- Rules from the earlier steps hold: ports 6599/65501/23499 checked free, no live server, stage files by name.

## Review Focus

1. Right mouse button: a menu pick never leaves a key half-queued; a right tap on a dialog still cancels (as before); menus are off while a window or the help is up and while the link is mid-exchange.
2. Region menu: choosing a server on the title dials that one next (and the prefs line is kept); nothing changes the server in play; an unreachable server leaves the title usable.
3. Help pages: first/last page edges, an over-long line or a missing page in the sources is a build error, the QR fits on PAL and NTSC without touching the prompt strip, Esc/click-out always returns to exactly the screen that was under it.
4. Palette cycling and the busy pointer: nothing is left cycling or busy after Esc-Y or a failed start; the cycle never touches the playfield's pens; chip RAM is the same before and after a second run.
5. Remembered settings: a missing, hand-edited or unknown prefs line never breaks start-up; saving never drops the server line.

---

### Task 1: The instruction pages and the QR code

**Files:**
- Create: `client/tools/help_amiga.py`, `client/tools/test_help_amiga.py`, `client/src/amiga/help_tab.h` (generated), `client/src/amiga/help.c`, `client/src/amiga/help.h`
- Modify: `client/src/amiga/display_gfx.c` (`help_open`/`help_key` move to help.c), `client/src/amiga/ui.c` + `ui.h` (`ui_help_mode`, clicks), `client/Makefile`, `client/README.md`

**Interfaces:**
- `help_amiga.py [--qr URL] OUT.h HELP01.TXT ...` writes `help_tab.h`: `#define HELP_PAGES n`, `static const char *const help_lines[HELP_PAGES][24]` with `{`..`}` reverse markers turned into byte 0x01/0x02, and for the QR page `HELP_QR_PAGE`, `HELP_QR_N` (modules a side) and `help_qr[]` (one bit a module, row-major, MSB first); the same checks as `help_msdos.py` (24 lines, 40 columns, numbering, `n/m` heading, `[QR]` only on lines 1–20).
- `help.h`: `void help_draw(unsigned char page)`; `unsigned char help_pages(void)`; `ui.h`: `void ui_help_mode(unsigned char on)`; with help up, `ui_click` returns `K_ESC` for a click on EXIT, `'>'`/`'<'` for NEXT/PREV (the keys `help_key` already takes), nothing else.

- [ ] **Step 1: Write the failing tests** `test_help_amiga.py`: a good page checks; a 41-character line, a 25th line, a missing page number and a heading `3/11` on page 2 each exit non-zero; the generated header for the 11 real pages declares 11 pages, and its QR is the one `qr.py` makes for the URL (compare module for module). `amiga_ui_test.c`: with `ui_help_mode(1)` a click on the NEXT/PREV/EXIT rects gives `'>'`, `'<'`, `K_ESC`, any other click gives nothing, with it off they give nothing; `amiga_layout_test.c`: the three help buttons lie inside the screen above the prompt strip, disjoint, PAL and NTSC.
- [ ] **Step 2: Run them** — Expected: FAIL (no tool, no symbols).
- [ ] **Step 3: Implement** `help_amiga.py` (reuse `help_msdos.py`'s checks and `qr.py`; `QR_URL` default `HTTPS://SYN-ACK-WIRE.FLY.DEV/AMIGA`), the `layout.c` help buttons, `ui.c` help mode, `help.c` drawing the page as a window over the screen (saving and restoring what was under it as the dialogs do) with the QR page drawn as pixel squares scaled to fit.
- [ ] **Step 4: Makefile** rule generating `help_tab.h` from `help/msdos/screen/HELP*.TXT`; `make amiga` and the host tests pass; rig: open the help on PAL and NTSC, screenshot each page, the QR page included.
- [ ] **Step 5: Commit** by name.

### Task 2: Menus and the Region list

**Files:**
- Create: `client/src/amiga/menus.c`, `client/src/amiga/menus.h`, `client/src/amiga/regions_tab.h` (generated), `client/test/amiga_menus_test.c`
- Modify: `client/tools/gen_regions.py` (an `a` output: the Amiga header), `client/src/amiga/gfx.c` (attach the strip, `RMBTRAP` off while it is on), `client/src/amiga/input.c` (`IDCMP_MENUPICKED`), `client/src/amiga/prefs.c`/`prefs_io.c`, `client/Makefile`, `client/README.md`

**Interfaces:**
- `menus.h`: `unsigned char menu_item_key(unsigned short menunum)` — pure: the menu number a pick carries to the key it stands for (`K_HELP`, `K_OPTION`, `AK_SOUND`, `AK_TABLE`, `K_ESC`, or `AK_REGION + n` for region n, 0 for nothing), tested on the host; `void menus_attach(void)`, `void menus_state(void)` called when the screen's state changes to enable/disable items (Region only on the title; everything but Quit/Help off during a window).
- `prefs.h`: `prefs_server` is still the source of the dialled address; `net_appkey(LOBBY_KEY)` serves the Region pick for this run and `amiga_prefs_save` keeps `SERVER=` as the pick.
- `regions_tab.h`: `REGION_HOST`, `NREGIONS`, `regions[]` as the desktop's.

- [ ] **Step 1: Failing tests** `amiga_menus_test.c`: every menu number the strip defines maps to its key; a number past the end, a sub-item that is a bar and `MENUNULL` map to 0; `AK_REGION + n` is `n` for each region. A `gen_regions.py a` test in `tools/test_gen_regions.py` (create if absent): one entry per `[[server]]` in `servers.fly.toml`.
- [ ] **Step 2: Run** — Expected: FAIL.
- [ ] **Step 3: Implement** the generator output, `menus.c` (Project: Help/Instructions, Quit; Game: Move Hints, Sound, Seats as table; Region: one item per server, only when the title shows; each with an Amiga-key shortcut), the `MENUPICKED` loop (every pick in a chain), the Region pick (sets the address for the next dial and updates the title's status line, saved on the next prefs write), the right-button cancel kept as `MENUPICKED` with `MENUNULL` while a window is up.
- [ ] **Step 4: Rig:** on the title choose a Region by keyboard shortcut (the guest's keys can send Amiga-key chords), jack in against the scratch server on its port by `USE_PREFS`-style override; a full keyed game still passes PAL/NTSC; screenshots of the menu strip open.
- [ ] **Step 5: Commit** by name.

### Task 3: Remembered settings

**Files:** `client/src/amiga/prefs.c`, `prefs_io.c`, `prefs.h`, `test/amiga_prefs_test.c`, `display_gfx.c`, `sound.c` or `input.c` (apply at start), `client/README.md`.

**Interfaces:** `prefs_format(handle, server, view, hints, sound, out)` and `prefs_view/hints/sound(...)` getters: lines `VIEW=TABLE|CARDS`, `HINTS=ON|OFF`, `SOUND=ON|OFF`; unknown or malformed lines are ignored and kept out on save only if they were not understood (as `SERVER=` already is).

- [ ] **Step 1: Failing tests:** each line parses; a missing, empty or hand-mangled line leaves the default (cards, hints off, sound on); the three round-trip through `prefs_format`; the `SERVER=` line survives a save.
- [ ] **Step 2: Run** — Expected: FAIL.
- [ ] **Step 3: Implement**; the view/hints/sound are saved when they change (at most one write a second, through the same protected path as the handle) and applied at start.
- [ ] **Step 4: Run** all host suites and a keyed PAL game. **Step 5: Commit.**

### Task 4: The title's neon and static

**Files:** `client/src/amiga/display_gfx.c` (title), `client/src/amiga/gfx.c` (`gfx_cycle`), `client/tools/amiga_mock.py` (the ramp's pens if they move), `client/README.md`

**Interfaces:** `void gfx_cycle(unsigned char first, unsigned char n, unsigned char step)` rotates `n` palette entries from `first` by one with `LoadRGB4` (a copy kept in RAM), called from `display_idle` on the title only, at most every 3 ticks; `display_title_update` stops it; leaving the title restores the palette.

- [ ] **Step 1: Failing test** (host): the rotation of a palette array by `step` is a pure function in `gfx`-free code (`palcycle.c`: `palcycle(unsigned short *rgb, n, step)`), tested for n = 1, 2, 7, a full turn returning the original, and never touching entries outside `[first, first+n)`.
- [ ] **Step 2: Run** — Expected: FAIL. **Step 3: Implement** it, the title's logo ramp pens cycle (the same ramp the title already uses), and a short static burst (a few random hatch rows of the logo area, redrawn by xor over two ticks) every few seconds.
- [ ] **Step 4: Rig:** screenshots of the title at three times differ in the ramp only; after jacking in the playfield colours are untouched (compare a pixel row of the grid before and after); `avail` free chip RAM equal after a second run. **Step 5: Commit.**

### Task 5: The busy pointer

**Files:** `client/src/amiga/pointer.c`, `net_fn.c`, `amiga.h`, `client/README.md`

**Interfaces:** `void pointer_busy(unsigned char on)`; `net_fn.c` calls it around the calls that can block for more than a moment (`net_open`, a read that waits, `net_close`), only when the game window exists (`amiga_win`).

- [ ] **Step 1: Failing host test:** `amiga_net_test.c` (the fake NIO): `pointer_busy` is called on then off, balanced, around `net_open`, including when it fails and when the read times out.
- [ ] **Step 2: Run** — Expected: FAIL. **Step 3: Implement** the busy sprite (an hourglass in chip RAM like the arrow; freed with it) and the calls. **Step 4: Rig:** a screenshot during the dial shows the busy pointer; a game still passes. **Step 5: Commit.**

### Task 6: Documentation and the floppy

**Files:** `client/README.md` (keys, menus, settings, the sizes table), `CLAUDE.md` if the build line changes, `client/tools/make-amiga-adf.sh` (nothing new to add: the pages are in the binary), mockups note for the Copper gradient.

- [ ] **Step 1:** README: the Amiga section (menus, help pages, remembered settings, what step 4 holds), the sizes row, `make amiga` size. **Step 2:** run every host suite and the six acceptance runs (keyed and mouse × PAL, NTSC, CUT). **Step 3:** build the ADF. **Step 4:** commit by name.

## Rulings carried into the plan

- Copper gradient: not built. The playfield covers the whole 5-plane screen in `PEN_BG0`, so a gradient on colour 0 would show nowhere; it comes back only with a layout that shows the backdrop. Recorded as a deferred item, not a silent skip.
- Help pages are compiled in (about 8 KB), not read from a file: a floppy's current directory is not reliable across 1.3 and 2.x launches.
- The Region menu works from the title only: a change in play would have to hang up and log off first, which the common code does only through its own Esc-Y flow.
