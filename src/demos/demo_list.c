#include <stdio.h>
#include <string.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Scrolling list with paging, selection and a confirm dialog. Tap a row to
 * select it, UP / DN to page, DEL (shown while a row is selected) asks for
 * confirmation and then removes it. RESET in the header restores the list.
 */

#define ROWS    5
#define ROW_H   36
#define ROW_PIT 40

static const char *const names[] = {
    "KITCHEN SENSOR", "GARAGE DOOR", "FRONT PORCH", "BEDROOM LIGHT", "LIVING ROOM",
    "BACK GATE",      "WORKSHOP",    "ATTIC FAN",   "BASEMENT",      "GREENHOUSE",
    "DRIVEWAY",       "HALLWAY",     "OFFICE",      "PATIO",
};
#define N_ALL ((int)(sizeof(names) / sizeof(names[0])))

static uint8_t order[N_ALL]; /* indices into names[] still in the list */
static int count;
static int top;      /* index of the first visible row */
static int sel = -1; /* selected position in order[], or -1 */
static bool confirm;

static const struct ui_box up_box = {346, 40, 62, 52};
static const struct ui_box dn_box = {346, 98, 62, 52};
static const struct ui_box del_box = {346, 184, 62, 52};
static const struct ui_box yes_box = {50, 150, 140, 60};
static const struct ui_box no_box = {226, 150, 140, 60};

static struct ui_box row_box(int slot)
{
    return (struct ui_box){8, 40 + slot * ROW_PIT, 330, ROW_H};
}

static void reset_list(void)
{
    for (int i = 0; i < N_ALL; i++) {
        order[i] = i;
    }
    count = N_ALL;
    top = 0;
    sel = -1;
    confirm = false;
}

static void enter(void)
{
    if (count == 0) { /* first visit, or the list was emptied */
        reset_list();
    }
    confirm = false;
}

static int pages(void)
{
    return count ? (count + ROWS - 1) / ROWS : 1;
}

static void draw_confirm(void)
{
    struct ui_box title = {0, 50, gfx_width(), 24};
    struct ui_box name = {0, 90, gfx_width(), 16};

    ui_text_center(&title, "REMOVE THIS ITEM?", 3);
    ui_text_center(&name, names[order[sel]], 2);
    ui_button(&yes_box, "YES", 3);
    ui_button(&no_box, "NO", 3);
}

static void draw(void)
{
    char buf[16];
    struct ui_box reset = ui_header_box(1);

    ui_button(&reset, "RESET", 2);

    if (confirm && sel >= 0) {
        draw_confirm();
        return;
    }

    if (count == 0) {
        struct ui_box middle = {0, 100, gfx_width(), 30};

        ui_text_center(&middle, "LIST EMPTY - TAP RESET", 2);
        return;
    }

    for (int slot = 0; slot < ROWS; slot++) {
        int pos = top + slot;
        struct ui_box b = row_box(slot);

        if (pos >= count) {
            break;
        }
        ui_frame(&b, pos == sel ? 4 : 1);
        gfx_text(18, b.y + 11, names[order[pos]], 2);
        if (pos == sel) {
            gfx_text(b.x + b.w - 20, b.y + 11, ">", 2);
        }
    }

    ui_button(&up_box, "UP", 2);
    ui_button(&dn_box, "DN", 2);
    snprintf(buf, sizeof(buf), "%d/%d", top / ROWS + 1, pages());

    struct ui_box page_box = {346, 158, 62, 16};

    ui_text_center(&page_box, buf, 2);
    if (sel >= 0) {
        ui_button(&del_box, "DEL", 2);
    }
}

static void touch(const struct touch_point *p)
{
    if (p->event != TOUCH_PRESS) {
        return;
    }

    struct ui_box reset = ui_header_box(1);

    if (ui_hit(&reset, p->x, p->y)) {
        reset_list();
        ui_redraw();
        return;
    }

    if (confirm) {
        if (ui_hit(&yes_box, p->x, p->y) && sel >= 0) {
            memmove(&order[sel], &order[sel + 1], count - sel - 1);
            count--;
            sel = -1;
            if (top >= count && top > 0) {
                top -= ROWS;
            }
            confirm = false;
            ui_redraw();
        } else if (ui_hit(&no_box, p->x, p->y)) {
            confirm = false;
            ui_redraw();
        }
        return;
    }

    if (ui_hit(&up_box, p->x, p->y) && top > 0) {
        top -= ROWS;
        ui_redraw();
    } else if (ui_hit(&dn_box, p->x, p->y) && top + ROWS < count) {
        top += ROWS;
        ui_redraw();
    } else if (sel >= 0 && ui_hit(&del_box, p->x, p->y)) {
        confirm = true;
        ui_redraw();
    } else {
        for (int slot = 0; slot < ROWS; slot++) {
            struct ui_box b = row_box(slot);
            int pos = top + slot;

            if (pos < count && ui_hit(&b, p->x, p->y)) {
                sel = (sel == pos) ? -1 : pos;
                ui_redraw();
                return;
            }
        }
    }
}

const struct ui_screen demo_list = {
    .title = "LIST",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
