#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Gesture detector: tap, double tap, long press and swipe in four
 * directions. A gesture is classified when the finger lifts, from the PRESS
 * position and time and the RELEASE. The exception is a long press, which
 * the touch layer reports as a HOLD event while the finger is still down
 * (see touch_set_hold(); Settings changes the hold time). With HOLD turned
 * off, a long press is recognised by its duration at release instead.
 */

#define TAP_TRAVEL_MAX 12 /* pixels of movement still counted as staying put */
#define SWIPE_MIN      40 /* pixels of travel for a swipe */
#define LONG_PRESS_MS  600
#define DOUBLE_TAP_MS  400
#define DOUBLE_TAP_GAP 30 /* max distance between the two taps */
#define HISTORY        5

enum gesture { G_NONE, G_TAP, G_DOUBLE, G_LONG, G_SWIPE, G_DRAG };

static bool down;
static bool held; /* HOLD already reported for this contact */
static uint32_t press_ms;
static int x0, y0, lx, ly;
static int travel;

static enum gesture last_gesture = G_NONE;
static int last_dx, last_dy, last_duration;
static char last_name[16] = "TRY A GESTURE";
static char history[HISTORY][32];
static int history_count;
static unsigned int counts[5]; /* tap, double, long, swipe, drag */

static bool have_tap;
static uint32_t tap_ms;
static int tap_x, tap_y;

static void enter(void)
{
    down = false;
    held = false;
}

static void record(enum gesture g, const char *name, int dx, int dy, int duration)
{
    last_gesture = g;
    snprintf(last_name, sizeof(last_name), "%s", name);
    last_dx = dx;
    last_dy = dy;
    last_duration = duration;
    counts[g - 1]++;

    for (int i = HISTORY - 1; i > 0; i--) {
        memcpy(history[i], history[i - 1], sizeof(history[i]));
    }
    snprintf(history[0], sizeof(history[0]), "%-11s %4d,%4d %4dMS", name, dx, dy, duration);
    if (history_count < HISTORY) {
        history_count++;
    }
}

static void classify(uint32_t release_ms)
{
    int duration = (int)(release_ms - press_ms);
    int dx = lx - x0;
    int dy = ly - y0;
    unsigned int long_ms = touch_get_hold() ? touch_get_hold() : LONG_PRESS_MS;

    if (travel >= SWIPE_MIN) {
        const char *name;

        if (abs(dx) >= abs(dy)) {
            name = dx < 0 ? "SWIPE LEFT" : "SWIPE RIGHT";
        } else {
            name = dy < 0 ? "SWIPE UP" : "SWIPE DOWN";
        }
        record(G_SWIPE, name, dx, dy, duration);
        have_tap = false;
    } else if (travel > TAP_TRAVEL_MAX) {
        record(G_DRAG, "SHORT DRAG", dx, dy, duration);
        have_tap = false;
    } else if ((unsigned int)duration >= long_ms) {
        record(G_LONG, "LONG PRESS", dx, dy, duration);
        have_tap = false;
    } else if (have_tap && (release_ms - tap_ms) <= DOUBLE_TAP_MS &&
               abs(x0 - tap_x) <= DOUBLE_TAP_GAP && abs(y0 - tap_y) <= DOUBLE_TAP_GAP) {
        record(G_DOUBLE, "DOUBLE TAP", dx, dy, duration);
        have_tap = false;
    } else {
        record(G_TAP, "TAP", dx, dy, duration);
        have_tap = true;
        tap_ms = release_ms;
        tap_x = x0;
        tap_y = y0;
    }
}

/* Arrow about 60 pixels long centred on (cx, cy), pointing along the unit
 * step (ux, uy). */
static void arrow(int cx, int cy, int ux, int uy)
{
    int tip_x = cx + ux * 30;
    int tip_y = cy + uy * 30;
    int tail_x = cx - ux * 30;
    int tail_y = cy - uy * 30;
    int perp_x = -uy;
    int perp_y = ux;

    gfx_line(tail_x, tail_y, tip_x, tip_y, 5);
    gfx_line(tip_x, tip_y, tip_x - ux * 16 + perp_x * 14, tip_y - uy * 16 + perp_y * 14, 4);
    gfx_line(tip_x, tip_y, tip_x - ux * 16 - perp_x * 14, tip_y - uy * 16 - perp_y * 14, 4);
}

static void draw(void)
{
    char buf[64];
    struct ui_box name = {0, 50, gfx_width(), 22};
    int cx = 330;
    int cy = 170;

    if (touch_get_hold()) {
        snprintf(buf, sizeof(buf), "TAP  DOUBLE TAP  HOLD %u MS  SWIPE",
                 (unsigned)touch_get_hold());
    } else {
        snprintf(buf, sizeof(buf), "TAP  DOUBLE TAP  HOLD %d MS (AT RELEASE)  SWIPE",
                 LONG_PRESS_MS);
    }
    gfx_text(8, 38, buf, 1);
    ui_text_center(&name, last_name, 3);

    if (last_gesture != G_NONE) {
        snprintf(buf, sizeof(buf), "DX %+d DY %+d", last_dx, last_dy);
        gfx_text(8, 84, buf, 2);
        snprintf(buf, sizeof(buf), "TIME %d MS", last_duration);
        gfx_text(8, 102, buf, 2);
    }

    for (int i = 0; i < history_count; i++) {
        gfx_text(8, 130 + i * 12, history[i], 1);
    }
    snprintf(buf, sizeof(buf), "TAP %u DBL %u LONG %u SWIPE %u DRAG %u", counts[0], counts[1],
             counts[2], counts[3], counts[4]);
    gfx_text(8, 212, buf, 1);

    switch (last_gesture) {
    case G_TAP:
        gfx_circle(cx, cy, 20, 3);
        break;
    case G_DOUBLE:
        gfx_circle(cx, cy, 24, 3);
        gfx_circle(cx, cy, 12, 3);
        break;
    case G_LONG:
        gfx_circle(cx, cy, 20, 20);
        break;
    case G_SWIPE:
        if (abs(last_dx) >= abs(last_dy)) {
            arrow(cx, cy, last_dx < 0 ? -1 : 1, 0);
        } else {
            arrow(cx, cy, 0, last_dy < 0 ? -1 : 1);
        }
        break;
    default:
        break;
    }
}

static void touch(const struct touch_point *p)
{
    switch (p->event) {
    case TOUCH_PRESS:
        if (p->y < UI_HEADER_H) {
            return;
        }
        down = true;
        held = false;
        press_ms = p->time_ms;
        x0 = lx = p->x;
        y0 = ly = p->y;
        travel = 0;
        break;
    case TOUCH_MOVE:
        if (down) {
            lx = p->x;
            ly = p->y;

            int d = abs(lx - x0) > abs(ly - y0) ? abs(lx - x0) : abs(ly - y0);

            if (d > travel) {
                travel = d;
            }
        }
        break;
    case TOUCH_HOLD:
        if (down && !held) {
            held = true;
            record(G_LONG, "LONG PRESS", lx - x0, ly - y0, (int)(p->time_ms - press_ms));
            have_tap = false;
            ui_redraw(); /* shown while the finger is still down */
        }
        break;
    case TOUCH_RELEASE:
        if (down) {
            down = false;
            if (held) {
                held = false; /* already reported when the hold fired */
            } else {
                classify(p->time_ms);
                ui_redraw();
            }
        }
        break;
    }
}

const struct ui_screen demo_gestures = {
    .title = "GESTURES",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
