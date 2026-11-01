#include "render.h"

#define BG_MASK  0x70                   /* the background bits of the attribute: reverse video */
#define BLINK    0x80

unsigned char amiga_glyph(unsigned char ch)
{
    if (ch < 0x20)
        return ' ';
    if (ch < 0x80)
        return ch;
    if (ch == 0x9D)
        return 0xA5;                    /* yen */
    if (ch >= 0xB3 && ch <= 0xDF) {     /* box drawing and blocks: lines, joints, else solid */
        switch (ch) {
        case 0xB3: case 0xBA: return '|';
        case 0xC4: case 0xCD: return '-';
        case 0xB4: case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9: case 0xBB: case 0xBC: case 0xBD:
        case 0xBE: case 0xBF: case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC5: case 0xC6: case 0xC7:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCE: case 0xCF: case 0xD0: case 0xD1:
        case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7: case 0xD8: case 0xD9: case 0xDA:
            return '+';
        default: return '#';
        }
    }
    if (ch >= 0xB0 && ch <= 0xB2)
        return '#';
    return '?';
}

unsigned short amiga_render(const unsigned short *screen, unsigned short *shown, unsigned char blink,
                            render_text text)
{
    unsigned short runs = 0;
    unsigned char row, col, end, inv, n;
    char buf[R_COLS];
    unsigned short i, want[R_COLS];

    for (row = 0; row < R_ROWS; ++row) {
        const unsigned short *s = screen + row * R_COLS;
        unsigned short *d = shown + row * R_COLS;

        for (col = 0; col < R_COLS; ++col) {
            unsigned char attr = s[col] >> 8;
            inv = (attr & BG_MASK) != 0;
            if (attr & BLINK)
                inv ^= blink;
            want[col] = (unsigned short)(inv + 1) << 8 | amiga_glyph((unsigned char)s[col]);
        }
        for (col = 0; col < R_COLS; ++col) {
            if (want[col] == d[col])
                continue;
            inv = (want[col] >> 8) - 1;
            for (end = col, n = 0; end < R_COLS && want[end] != d[end] && (want[end] >> 8) - 1 == inv; ++end)
                buf[n++] = (char)want[end];
            text(row, col, buf, n, inv);
            for (i = col; i < end; ++i)
                d[i] = want[i];
            col = end - 1;
            ++runs;
        }
    }
    return runs;
}
