#include <stdio.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Sleep and touch wake.
 *
 * What "sleep" means here: nothing in the application runs periodically, the
 * touch layer reads the controller only after the INT interrupt fires, and
 * the e-paper keeps its picture with no power. A touch anywhere wakes it; the
 * screen then reports how long it slept and how many INT edges arrived
 * meanwhile.
 *
 * The touch controller itself is not put to sleep: Pervasive's libraries
 * define no sleep protocol for it (only a release note that RESET resumes
 * it), and a controller held in reset could not wake the system on touch.
 */

enum state { AWAKE, ASLEEP };

static enum state state = AWAKE;

static uint32_t slept_at;
static uint32_t edges_at;
static uint32_t wakeups;
static uint32_t last_s, last_edges;
static int wake_x = -1;
static int wake_y;

static const struct ui_box sleep_box = {108, 150, 200, 60};

static void enter(void)
{
    state = AWAKE;
}

static void draw(void)
{
    char buf[40];

    if (state == ASLEEP) {
        struct ui_box a = {0, 66, gfx_width(), 32};
        struct ui_box b = {0, 118, gfx_width(), 14};
        struct ui_box c = {0, 150, gfx_width(), 8};

        ui_text_center(&a, "SLEEPING", 4);
        ui_text_center(&b, "TOUCH TO WAKE", 2);
        ui_text_center(&c, "TOUCH IRQ ARMED", 1);
        return;
    }

    gfx_text(8, 42, "SLEEP STOPS ALL PERIODIC WORK.", 1);
    gfx_text(8, 54, "THE TOUCH INTERRUPT STAYS ARMED. TOUCH ANYWHERE TO WAKE.", 1);

    snprintf(buf, sizeof(buf), "WAKE-UPS: %u", (unsigned)wakeups);
    gfx_text(8, 78, buf, 2);
    if (wakeups) {
        snprintf(buf, sizeof(buf), "SLEPT %u S, %u INT EDGES", (unsigned)last_s,
                 (unsigned)last_edges);
        gfx_text(8, 100, buf, 2);
        snprintf(buf, sizeof(buf), "WOKEN BY TOUCH AT %d,%d", wake_x, wake_y);
        gfx_text(8, 122, buf, 1);
    }

    ui_button(&sleep_box, "SLEEP NOW", 2);
}

static void touch(const struct touch_point *p)
{
    struct touch_stats st;

    if (p->event != TOUCH_PRESS) {
        return;
    }

    touch_get_stats(&st);

    if (state == AWAKE) {
        if (ui_hit(&sleep_box, p->x, p->y)) {
            edges_at = st.int_edges;
            slept_at = p->time_ms;
            state = ASLEEP;
            ui_redraw();
        }
        return;
    }

    /* Asleep: any touch wakes. */
    last_s = (p->time_ms - slept_at) / 1000;
    last_edges = st.int_edges - edges_at;
    wake_x = p->x;
    wake_y = p->y;
    wakeups++;
    state = AWAKE;
    ui_redraw();
}

const struct ui_screen demo_sleep = {
    .title = "SLEEP",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
