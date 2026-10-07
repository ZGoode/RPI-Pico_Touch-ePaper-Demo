#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Scribble: draw with a finger. Strokes are kept as a point list and redrawn
 * from it, because the framebuffer is rebuilt for every refresh. The screen
 * refreshes when the finger lifts (a refresh mid-stroke would only show where
 * the finger was). CLEAR is in the header; the pen width is set on the
 * Settings screen.
 */

#define MAX_POINTS 500

struct point {
    uint16_t x;
    uint16_t y;
    uint8_t start; /* first point of a stroke */
};

static struct point points[MAX_POINTS];
static uint16_t point_count;
static bool pen_down; /* finger down inside the canvas */

static struct ui_box clear_box(void)
{
    return ui_header_box(1);
}

static void enter(void)
{
    pen_down = false;
}

static void add_point(const struct touch_point *p, bool start)
{
    if (point_count >= MAX_POINTS) {
        return;
    }
    points[point_count].x = p->x;
    points[point_count].y = p->y;
    points[point_count].start = start;
    point_count++;
}

static void draw(void)
{
    struct ui_box clear = clear_box();

    ui_button(&clear, "CLEAR", 2);

    if (point_count == 0) {
        struct ui_box middle = {0, 100, gfx_width(), 30};

        ui_text_center(&middle, "DRAW HERE", 2);
        return;
    }

    for (uint16_t i = 0; i < point_count; i++) {
        if (points[i].start || i == 0) {
            gfx_line(points[i].x, points[i].y, points[i].x, points[i].y, ui_settings.pen); /* dot */
        } else {
            gfx_line(points[i - 1].x, points[i - 1].y, points[i].x, points[i].y,
                     ui_settings.pen);
        }
    }

    if (point_count >= MAX_POINTS) {
        gfx_text(8, gfx_height() - 12, "FULL - TAP CLEAR", 1);
    }
}

static void touch(const struct touch_point *p)
{
    struct ui_box clear = clear_box();

    switch (p->event) {
    case TOUCH_PRESS:
        if (ui_hit(&clear, p->x, p->y)) {
            point_count = 0;
            pen_down = false;
            ui_redraw();
        } else if (p->y >= UI_HEADER_H) {
            pen_down = true;
            add_point(p, true);
        }
        break;
    case TOUCH_MOVE:
        if (pen_down && p->y >= UI_HEADER_H) {
            add_point(p, false);
        }
        break;
    case TOUCH_RELEASE:
        if (pen_down) {
            pen_down = false;
            ui_redraw();
        }
        break;
    default:
        break;
    }
}

const struct ui_screen demo_scribble = {
    .title = "SCRIBBLE",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
