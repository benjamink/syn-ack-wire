#include "../platform.h"
#include "../common/game.h"
#include "keys.h"
#include "ui.h"

static unsigned char count;             /* the hivejack stepper */
static unsigned char sold;              /* what the first step settled on */
static unsigned char dirty;
static unsigned char title, yesno;

void ui_title_mode(unsigned char on) { title = on; }
void ui_ask_yes_no(unsigned char on) { yesno = on; }
void ui_reset(void) { count = 0; sold = 0; dirty = 1; }
unsigned char ui_dirty(void) { unsigned char d = dirty; dirty = 0; return d; }
unsigned char ui_count(void) { return count; }

static unsigned char one(unsigned char *keys, unsigned char k) { keys[0] = k; return 1; }

/* the most the stepper may reach: what is held, or for the swap what the rest and the supply allow, in pairs */
static unsigned char stepper_max(void)
{
    unsigned char m;
    if (!step) return pr[3];
    m = pr[3] > sold ? pr[3] - sold : 0;
    if (pr[4] < 127 && m / 2 > pr[4]) m = pr[4] * 2;
    return m & ~1;
}

static unsigned char hivejack(const struct layout *l, short x, short y, unsigned char button, unsigned char *keys)
{
    signed char b;
    unsigned char n = 0, t, m;
    if (button == UI_RIGHT) { count = 0; dirty = 1; return 0; }
    b = layout_btn_at(l, x, y);
    if (b == BTN_PLUS) {
        m = stepper_max();
        t = step ? 2 : 1;
        if (count + t <= m) { count += t; dirty = 1; }
    } else if (b == BTN_MINUS) {
        t = step ? 2 : 1;
        if (count >= t) { count -= t; dirty = 1; }
    } else if (b == BTN_OK) {
        for (t = 0; t < edit_len; ++t) keys[n++] = K_DELETE;
        if (count >= 10) keys[n++] = '0' + count / 10;
        keys[n++] = '0' + count % 10;
        keys[n++] = K_RETURN;
        if (!step) sold = count;
        count = 0; dirty = 1;
    }
    return n;
}

static unsigned char buy(const struct layout *l, short x, short y, unsigned char button, unsigned char *keys)
{
    signed char b, i;
    if (button == UI_RIGHT) return one(keys, step ? 'N' : K_DELETE);
    b = layout_btn_at(l, x, y);
    if (step) return b == BTN_YES ? one(keys, 'Y') : b == BTN_NO ? one(keys, 'N') : 0;
    if (b == BTN_BUY) return one(keys, K_RETURN);
    if (b == BTN_UNDO) return one(keys, K_DELETE);
    if (b == BTN_NULLROUTE) return (pr[3] & 1) ? one(keys, K_RETURN) : 0;
    for (i = 0; i < NSYN; ++i)
        if (layout_inside(&l->dlg_row[i], x, y)) return pr[4 + i] ? one(keys, syn_letter[i]) : 0;
    return 0;
}

static unsigned char pick_syndicate(const struct layout *l, short x, short y, unsigned char *keys)
{
    short n = layout_node_at(l, x, y);
    signed char s = layout_syn_at(l, x, y);
    unsigned char mask = pr_kind == 'K' ? pr[2] : pr[1];    /* a tie's options follow its context byte */
    if (n >= 0 && board[n] >= 2 && board[n] < 2 + NSYN) s = board[n] - 2;
    if (s >= 0 && (mask & (1 << s))) return one(keys, syn_letter[s]);
    return 0;
}

static unsigned char title_click(const struct layout *l, short x, short y, unsigned char button, unsigned char *keys)
{
    signed char b;
    if (button == UI_RIGHT) return one(keys, K_ESC);
    b = layout_btn_at(l, x, y);
    switch (b) {
    case BTN_JACK:  return edit_len ? one(keys, K_RETURN) : 0;
    case BTN_HINTS: return one(keys, K_OPTION);
    case BTN_SOUND: return one(keys, AK_SOUND);
    case BTN_HELP:  return one(keys, K_HELP);
    case BTN_QUIT:  return one(keys, K_ESC);
    }
    return 0;
}

unsigned char ui_click(const struct layout *l, short x, short y, unsigned char button, unsigned char *keys)
{
    signed char b;
    short n;
    if (button != UI_LEFT && button != UI_RIGHT) return 0;
    if (leaving || (title && yesno)) {          /* a Y/N question: only a yes is a yes */
        b = layout_btn_at(l, x, y);
        return one(keys, button == UI_LEFT && b == BTN_YES ? 'Y' : 'N');
    }
    if (title) return title_click(l, x, y, button, keys);
    if (!pr_kind || !link_up) return 0;
    switch (pr_kind) {
    case 'S':
        b = layout_btn_at(l, x, y);
        return button == UI_LEFT && b >= BTN_SEAT2 && b <= BTN_SEAT6 ? one(keys, '2' + (b - BTN_SEAT2)) : 0;
    case 'B': return buy(l, x, y, button, keys);
    case 'D': return hivejack(l, x, y, button, keys);
    case 'F': case 'K': return button == UI_LEFT ? pick_syndicate(l, x, y, keys) : 0;
    case 'P':
        if (button != UI_LEFT || turn != me) return 0;
        n = layout_node_at(l, x, y);
        b = layout_kit_at(l, x, y);
        if (b >= 0) return kit[(unsigned char)b] < RULE_ROWS * RULE_COLS ? one(keys, '1' + b) : 0;
        if (n >= 0)
            for (b = 0; b < RULE_KITSIZE; ++b)
                if (kit[(unsigned char)b] == n) return one(keys, '1' + b);
        return 0;
    }
    return 0;
}
