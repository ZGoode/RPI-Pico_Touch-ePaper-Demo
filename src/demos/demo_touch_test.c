#include <stdio.h>

#include "board/board_config.h"
#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Touch test: shows the latest event with its display and raw controller
 * coordinates, a touch counter, and a marker at the finger position (a cross
 * while the finger is down, a hollow box where it lifted).
 *
 * The four squares in the corners of the content area are targets. Touching
 * one should put the marker on it, which makes this the quickest check that
 * the touch and display orientation agree (DISPLAY_ROTATION). The top squares
 * sit just below the header.
 */

#define BORDER_PX  2
#define CORNER_PX  16
#define CORNER_GAP 6
#define MARKER_ARM 12

static bool have_point;
static enum touch_event event;
static uint16_t px, py, raw_x, raw_y;
static unsigned int touches;

static const char *event_name(void)
{
    switch (event) {
    case TOUCH_PRESS:
        return "PRESS";
    case TOUCH_MOVE:
        return "MOVE";
    case TOUCH_HOLD:
        return "HOLD";
    default:
        return "RELEASE";
    }
}

static void draw_marker(void)
{
    int x = px;
    int y = py;

    if (event == TOUCH_RELEASE) {
        gfx_frame(x - 8, y - 8, 17, 17, 2);
    } else {
        gfx_fill_rect(x - MARKER_ARM, y - 1, 2 * MARKER_ARM + 1, 3);
        gfx_fill_rect(x - 1, y - MARKER_ARM, 3, 2 * MARKER_ARM + 1);
        gfx_fill_rect(x - 4, y - 4, 9, 9);
    }
}

static void enter(void)
{
    have_point = false;
    touches = 0;
}

static void draw(void)
{
    char line[40];
    int w = gfx_width();
    int h = gfx_height();
    int top = UI_HEADER_H + CORNER_GAP;

    /* Border and corner targets. */
    gfx_fill_rect(0, h - BORDER_PX, w, BORDER_PX);
    gfx_fill_rect(0, UI_HEADER_H, BORDER_PX, h - UI_HEADER_H);
    gfx_fill_rect(w - BORDER_PX, UI_HEADER_H, BORDER_PX, h - UI_HEADER_H);
    gfx_fill_rect(CORNER_GAP, top, CORNER_PX, CORNER_PX);
    gfx_fill_rect(w - CORNER_GAP - CORNER_PX, top, CORNER_PX, CORNER_PX);
    gfx_fill_rect(CORNER_GAP, h - CORNER_GAP - CORNER_PX, CORNER_PX, CORNER_PX);
    gfx_fill_rect(w - CORNER_GAP - CORNER_PX, h - CORNER_GAP - CORNER_PX, CORNER_PX, CORNER_PX);

    snprintf(line, sizeof(line), "SCREEN %dx%d  ROTATION %d", w, h, DISPLAY_ROTATION);
    gfx_text(30, 46, line, 1);

    snprintf(line, sizeof(line), "EVT: %s", have_point ? event_name() : "NONE");
    gfx_text(30, 62, line, 2);

    if (have_point) {
        snprintf(line, sizeof(line), "X:%3u Y:%3u", (unsigned)px, (unsigned)py);
        gfx_text(30, 88, line, 2);
        snprintf(line, sizeof(line), "RAW %3u,%3u", (unsigned)raw_x, (unsigned)raw_y);
        gfx_text(30, 114, line, 2);
    } else {
        gfx_text(30, 88, "X:--- Y:---", 2);
        gfx_text(30, 114, "RAW ---,---", 2);
        gfx_text(30, 170, "TOUCH THE PANEL", 1);
    }

    snprintf(line, sizeof(line), "TOUCHES: %u", touches);
    gfx_text(30, 140, line, 2);

    if (have_point) {
        draw_marker();
    }
}

static void touch(const struct touch_point *p)
{
    have_point = true;
    event = p->event;
    px = p->x;
    py = p->y;
    raw_x = p->raw_x;
    raw_y = p->raw_y;
    if (p->event == TOUCH_PRESS) {
        touches++;
    }
    ui_redraw();
}

const struct ui_screen demo_touch_test = {
    .title = "TOUCH TEST",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
