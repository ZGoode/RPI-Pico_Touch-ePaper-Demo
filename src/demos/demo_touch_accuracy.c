#include <math.h>
#include <stdio.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Accuracy test: tap nine targets in turn. The error of each tap (reported
 * point minus target) is recorded, and the summary shows the average and
 * worst error and the mean offset. A consistent offset points at the
 * coordinate mapping or the panel alignment; scattered error points at the
 * finger or the panel. The first contact point (PRESS) of each tap is used.
 *
 * Target positions are laid out for the 416 x 240 landscape screen.
 */

#define TARGETS 9

static const int target_xs[3] = {40, 208, 376};
static const int target_ys[3] = {64, 137, 212};

static int step; /* targets completed; TARGETS when finished */
static int16_t err_x[TARGETS];
static int16_t err_y[TARGETS];

static int target_x(int i)
{
    return target_xs[i % 3];
}

static int target_y(int i)
{
    return target_ys[i / 3];
}

static void cross(int x, int y, int arm)
{
    gfx_fill_rect(x - arm, y - 1, 2 * arm + 1, 3);
    gfx_fill_rect(x - 1, y - arm, 3, 2 * arm + 1);
}

static void enter(void)
{
    step = 0;
}

static void draw_summary(void)
{
    char buf[48];
    int sum_dx = 0;
    int sum_dy = 0;
    int worst10 = 0;  /* tenths of a pixel */
    int total10 = 0;

    for (int i = 0; i < TARGETS; i++) {
        int e10 = (int)(sqrtf((float)(err_x[i] * err_x[i] + err_y[i] * err_y[i])) * 10.0f);

        total10 += e10;
        sum_dx += err_x[i];
        sum_dy += err_y[i];
        if (e10 > worst10) {
            worst10 = e10;
        }
    }

    snprintf(buf, sizeof(buf), "AVG ERROR %d.%d PX", total10 / TARGETS / 10,
             total10 / TARGETS % 10);
    gfx_text(70, 84, buf, 2);
    snprintf(buf, sizeof(buf), "WORST %d.%d PX", worst10 / 10, worst10 % 10);
    gfx_text(70, 102, buf, 2);
    snprintf(buf, sizeof(buf), "MEAN OFFSET %+d,%+d", sum_dx / TARGETS, sum_dy / TARGETS);
    gfx_text(70, 120, buf, 1);
}

static void draw(void)
{
    char buf[32];
    struct ui_box reset = ui_header_box(1);

    ui_button(&reset, "RESET", 2);

    if (step < TARGETS) {
        snprintf(buf, sizeof(buf), "TAP TARGET %d OF %d", step + 1, TARGETS);
        gfx_text(120, 38, buf, 1);
    }

    for (int i = 0; i < TARGETS; i++) {
        int x = target_x(i);
        int y = target_y(i);

        if (i < step) {
            /* Completed: small cross with its error beside it. */
            cross(x, y, 5);
            snprintf(buf, sizeof(buf), "%+d,%+d", err_x[i], err_y[i]);
            gfx_text(x > 300 ? x - 12 - gfx_text_width(buf, 1) : x + 12, y - 4, buf, 1);
        } else if (i == step) {
            cross(x, y, 14);
            gfx_circle(x, y, 10, 2);
        } else {
            gfx_fill_rect(x - 1, y - 1, 3, 3);
        }
    }

    if (step == TARGETS) {
        draw_summary();
    }
}

static void touch(const struct touch_point *p)
{
    struct ui_box reset = ui_header_box(1);

    if (p->event != TOUCH_PRESS) {
        return;
    }
    if (ui_hit(&reset, p->x, p->y)) {
        step = 0;
        ui_redraw();
        return;
    }
    if (step >= TARGETS || p->y < UI_HEADER_H) {
        return;
    }
    err_x[step] = (int)p->x - target_x(step);
    err_y[step] = (int)p->y - target_y(step);
    step++;
    ui_redraw();
}

const struct ui_screen demo_touch_accuracy = {
    .title = "ACCURACY GRID",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
