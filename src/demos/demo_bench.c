#include <stdio.h>
#include <string.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Refresh benchmark: runs a fixed sequence of full-screen patterns, one full
 * e-paper refresh each, timing every refresh (the UI reports each completed
 * refresh through .refreshed, which starts the next one). Refreshes use the
 * update mode chosen in Settings, so run it once with FAST UPDATE on and once
 * off to compare. The results screen shows the times and leaves it to your
 * eyes to judge ghosting: faint remnants of earlier patterns on the results
 * page, after the white refresh, mean ghosting.
 */

enum pattern { P_WHITE, P_BLACK, P_CHECKER, P_STRIPES, P_TEXT, P_WHITE2 };
#define NSTEPS 6

static const char *const pattern_name[NSTEPS] = {"WHITE", "BLACK", "CHECKER",
                                                 "STRIPES", "TEXT", "WHITE"};

enum state { IDLE, RUNNING, DONE };

static enum state state;
static int step; /* pattern currently being refreshed */
static uint32_t ms[NSTEPS];
static const struct ui_box start_box = {108, 130, 200, 60};

static void enter(void)
{
    state = IDLE;
}

static void draw_pattern(int p)
{
    int w = gfx_width();
    int h = gfx_height();

    switch (p) {
    case P_BLACK:
        gfx_fill_rect(0, UI_HEADER_H, w, h - UI_HEADER_H);
        break;
    case P_CHECKER:
        for (int y = UI_HEADER_H; y < h; y += 16) {
            for (int x = 0; x < w; x += 16) {
                if ((((x / 16) + ((y - UI_HEADER_H) / 16))) & 1) {
                    gfx_fill_rect(x, y, 16, 16);
                }
            }
        }
        break;
    case P_STRIPES:
        for (int y = UI_HEADER_H; y < h; y += 8) {
            if (((y - UI_HEADER_H) / 8) & 1) {
                gfx_fill_rect(0, y, w, 8);
            }
        }
        break;
    case P_TEXT:
        for (int y = UI_HEADER_H + 2; y < h - 8; y += 10) {
            gfx_text(0, y, "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG 0123456789 "
                           "THE QUICK BROWN FOX", 1);
        }
        break;
    default: /* white: nothing to draw */
        break;
    }
}

static void draw_results(void)
{
    char buf[40];
    uint32_t min = ms[0];
    uint32_t max = ms[0];
    uint32_t sum = 0;
    struct ui_box again = ui_header_box(1);

    ui_button(&again, "AGAIN", 2);

    for (int i = 0; i < NSTEPS; i++) {
        snprintf(buf, sizeof(buf), "%d %-8s %5u MS", i + 1, pattern_name[i], (unsigned)ms[i]);
        gfx_text(8, 42 + i * 22, buf, 2);
        sum += ms[i];
        if (ms[i] < min) {
            min = ms[i];
        }
        if (ms[i] > max) {
            max = ms[i];
        }
    }
    snprintf(buf, sizeof(buf), "MIN %u AVG %u MAX %u MS", (unsigned)min, (unsigned)(sum / NSTEPS),
             (unsigned)max);
    gfx_text(8, 178, buf, 2);
    gfx_text(8, 204, "FAINT REMNANTS OF EARLIER PATTERNS ON THIS PAGE", 1);
    gfx_text(8, 216, "MEAN GHOSTING. TIMES ARE FULL REFRESHES ONLY.", 1);
}

static void draw(void)
{
    char buf[40];

    switch (state) {
    case IDLE:
        gfx_text(8, 44, "6 TIMED FULL REFRESHES", 2);
        gfx_text(8, 70, "WHITE, BLACK, CHECKER, STRIPES, TEXT, WHITE", 1);
        gfx_text(8, 90, "THE SCREEN FLASHES; EACH STEP TAKES SECONDS.", 1);
        gfx_text(8, 102, "WAIT FOR THE RESULTS PAGE.", 1);
        ui_button(&start_box, "START", 3);
        break;
    case RUNNING:
        draw_pattern(step);
        if (step == P_WHITE || step == P_WHITE2) {
            struct ui_box middle = {0, 110, gfx_width(), 14};

            snprintf(buf, sizeof(buf), "STEP %d/%d: WHITE", step + 1, NSTEPS);
            ui_text_center(&middle, buf, 2);
        }
        break;
    case DONE:
        draw_results();
        break;
    }
}

/* A refresh of the pattern drawn for `step` just finished. */
static void refreshed(uint32_t t)
{
    if (state != RUNNING) {
        return;
    }
    ms[step] = t;
    step++;
    if (step >= NSTEPS) {
        state = DONE;
    }
    ui_redraw();
}

static void touch(const struct touch_point *p)
{
    struct ui_box again = ui_header_box(1);

    if (p->event != TOUCH_PRESS) {
        return;
    }
    if (state == RUNNING) {
        return; /* let it finish; HOME still works */
    }
    if ((state == IDLE && ui_hit(&start_box, p->x, p->y)) ||
        (state == DONE && ui_hit(&again, p->x, p->y))) {
        state = RUNNING;
        step = 0;
        memset(ms, 0, sizeof(ms));
        ui_redraw();
    }
}

const struct ui_screen demo_bench = {
    .title = "REFRESH BENCH",
    .enter = enter,
    .draw = draw,
    .touch = touch,
    .refreshed = refreshed,
};
