#include <stdio.h>
#include <string.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * On-screen text keyboard (upper case, digits and space). Layout (416 x 240):
 *
 *   text field            y 36..62
 *   status line           y 64
 *   1 2 3 4 5 6 7 8 9 0
 *   Q W E R T Y U I O P
 *    A S D F G H J K L
 *     Z X C V B N M  DEL
 *   SPACE        CLR  ENTER
 *
 * Keys act on PRESS. The text updates at once and the screen follows each
 * refresh. ENTER stores the line and clears the field.
 */

#define MAX_TEXT 30
#define KEY_H    30
#define KEY_W    39
#define KEY_PIT  41

enum special { K_CHAR, K_DEL, K_SPACE, K_CLR, K_ENTER };

struct key {
    struct ui_box box;
    char label[6];
    enum special kind;
    char ch;
};

static struct key keys[48];
static int key_count;

static char text[MAX_TEXT + 1];
static uint8_t len;
static char last[MAX_TEXT + 1];
static bool have_last;

static void add_key(int x, int y, int w, const char *label, enum special kind, char ch)
{
    struct key *k = &keys[key_count++];

    k->box = (struct ui_box){x, y, w, KEY_H};
    snprintf(k->label, sizeof(k->label), "%s", label);
    k->kind = kind;
    k->ch = ch;
}

static void add_row(const char *chars, int x0, int y)
{
    for (int i = 0; chars[i]; i++) {
        char label[2] = {chars[i], 0};

        add_key(x0 + i * KEY_PIT, y, KEY_W, label, K_CHAR, chars[i]);
    }
}

static void build_keys(void)
{
    key_count = 0;
    add_row("1234567890", 3, 72);
    add_row("QWERTYUIOP", 3, 104);
    add_row("ASDFGHJKL", 23, 136);
    add_row("ZXCVBNM", 43, 168);
    add_key(336, 168, 76, "DEL", K_DEL, 0);
    add_key(3, 200, 230, "SPACE", K_SPACE, ' ');
    add_key(237, 200, 84, "CLR", K_CLR, 0);
    add_key(325, 200, 88, "ENTER", K_ENTER, 0);
}

static void enter(void)
{
    if (key_count == 0) {
        build_keys();
    }
    len = 0;
    text[0] = '\0';
}

static void draw(void)
{
    char buf[40];
    struct ui_box field = {4, 36, 408, 26};

    ui_frame(&field, 2);
    snprintf(buf, sizeof(buf), "%s%s", text, len < MAX_TEXT ? "_" : "");
    gfx_text(10, 42, buf, 2);

    if (have_last) {
        snprintf(buf, sizeof(buf), "LAST: %s", last);
    } else {
        snprintf(buf, sizeof(buf), "%u/%d CHARACTERS", len, MAX_TEXT);
    }
    gfx_text(6, 64, buf, 1);

    for (int i = 0; i < key_count; i++) {
        ui_button(&keys[i].box, keys[i].label, 2);
    }
}

static void touch(const struct touch_point *p)
{
    if (p->event != TOUCH_PRESS) {
        return;
    }

    for (int i = 0; i < key_count; i++) {
        struct key *k = &keys[i];

        if (!ui_hit(&k->box, p->x, p->y)) {
            continue;
        }

        switch (k->kind) {
        case K_CHAR:
        case K_SPACE:
            if (len < MAX_TEXT) {
                text[len++] = k->ch;
            }
            break;
        case K_DEL:
            if (len > 0) {
                len--;
            }
            break;
        case K_CLR:
            len = 0;
            break;
        case K_ENTER:
            if (len == 0) {
                return;
            }
            text[len] = '\0';
            memcpy(last, text, sizeof(last));
            have_last = true;
            len = 0;
            break;
        }
        text[len] = '\0';
        ui_redraw();
        return;
    }
}

const struct ui_screen demo_keyboard = {
    .title = "TEXT KEYBOARD",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
