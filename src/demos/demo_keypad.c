#include <stdio.h>
#include <string.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Number keypad built from touch regions. Each button is a
 * struct ui_box that is both drawn and hit-tested, so the picture and the
 * touch area cannot drift apart.
 *
 * Layout (416 x 240):
 *   left   entry field, digit counter, last ENTERed value and the ENTER button
 *   right  3 x 4 keys: 1 2 3 / 4 5 6 / 7 8 9 / CLR 0 DEL
 *
 * A key acts on PRESS. The panel needs a moment to show each change, but the
 * entry itself updates immediately, so typing ahead of the screen works.
 */

#define MAX_DIGITS 9 /* digits that fit the entry field at scale 3 */

#define KEY_CLR 9
#define KEY_DEL 11

static char entry[MAX_DIGITS + 1];
static unsigned int len;
static char last[MAX_DIGITS + 1];
static bool have_last;
static unsigned int entered;

static const char *const key_label[12] = {"1", "2", "3", "4",   "5", "6",
                                          "7", "8", "9", "CLR", "0", "DEL"};

static const struct ui_box field_box = {8, 40, 184, 48};
static const struct ui_box enter_box = {8, 176, 184, 56};

static struct ui_box key_box(int i)
{
    return (struct ui_box){200 + (i % 3) * 72, 40 + (i / 3) * 50, 68, 46};
}

static void enter(void)
{
    len = 0;
    entry[0] = '\0';
}

static void draw(void)
{
    char buf[24];

    /* Entry field with a cursor. */
    ui_frame(&field_box, 2);
    snprintf(buf, sizeof(buf), "%s%s", entry, len < MAX_DIGITS ? "_" : "");
    gfx_text(field_box.x + 8, field_box.y + (field_box.h - 3 * GFX_GLYPH_H) / 2, buf, 3);

    snprintf(buf, sizeof(buf), "DIGITS %u/%d", len, MAX_DIGITS);
    gfx_text(8, 94, buf, 1);

    gfx_text(8, 112, "LAST ENTERED:", 1);
    gfx_text(8, 124, have_last ? last : "---", 2);
    snprintf(buf, sizeof(buf), "ENTERED %u TIME%s", entered, entered == 1 ? "" : "S");
    gfx_text(8, 150, buf, 1);

    ui_button(&enter_box, "ENTER", 3);

    for (int i = 0; i < 12; i++) {
        struct ui_box b = key_box(i);

        ui_button(&b, key_label[i], 3);
    }
}

static void touch(const struct touch_point *p)
{
    if (p->event != TOUCH_PRESS) {
        return;
    }

    if (ui_hit(&enter_box, p->x, p->y)) {
        if (len == 0) {
            return;
        }
        memcpy(last, entry, sizeof(last));
        have_last = true;
        entered++;
        len = 0;
        entry[0] = '\0';
        ui_redraw();
        return;
    }

    for (int i = 0; i < 12; i++) {
        struct ui_box b = key_box(i);

        if (!ui_hit(&b, p->x, p->y)) {
            continue;
        }
        if (i == KEY_CLR) {
            len = 0;
        } else if (i == KEY_DEL) {
            if (len > 0) {
                len--;
            }
        } else if (len < MAX_DIGITS) {
            entry[len++] = key_label[i][0];
        }
        entry[len] = '\0';
        ui_redraw();
        return;
    }
}

const struct ui_screen demo_keypad = {
    .title = "NUMBER KEYPAD",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
