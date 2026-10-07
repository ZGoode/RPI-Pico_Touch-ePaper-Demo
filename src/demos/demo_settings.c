#include <stdio.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Settings: toggles and radio groups plus a few buttons, all writing to
 * ui_settings (read by the other screens). Two pages; "MORE >" and "< BACK"
 * (bottom left) switch between them.
 *
 * Page 1
 *   FLIP DISPLAY   Picture rotated a further 180 degrees. The picture flips
 *                  on the next refresh; touch follows once that picture is
 *                  on the glass (see ui/ui.c), so taps during the refresh
 *                  still hit the old one.
 *   PEN SIZE       Scribble and handwriting pen width.
 *
 * Page 2
 *   FAST UPDATE    Fast-update flow. OFF = the slower global update, which
 *                  also clears ghosting. Applies to the very next refresh.
 *   DEBOUNCE MS    Touch debounce (0 = off); see touch_set_debounce().
 *   HOLD MS        Long-press time (0 = off): a HOLD event is sent while the
 *                  finger is still down; see touch_set_hold().
 *   CLEAN PANEL    Four global black / white flashes to wipe ghosting, then
 *                  the screen is redrawn.
 *   AUTO CLEAN     OFF / BOOT (quick clean at power-up) / HOME (the refresh
 *                  when returning to the home menu is a global one) / BOTH.
 */

#define ROW_Y(i) (40 + (i) * 32)

static uint8_t page; /* 0 or 1 */

static const uint8_t pen_sizes[3] = {1, 3, 5};
static const uint16_t debounce_values[4] = {0, 40, 80, 120};
static const uint16_t hold_values[4] = {0, 400, 600, 800};
static const char *const auto_labels[4] = {"OFF", "BOOT", "HOME", "BOTH"}; /* auto_clean = index */

static struct ui_box toggle_box(int row)
{
    return (struct ui_box){330, ROW_Y(row), 76, 28};
}

static struct ui_box radio_box(int row, int i)
{
    return (struct ui_box){270 + i * 46, ROW_Y(row), 40, 28};
}

/* Four-way value picker on `row` (debounce, hold). */
static struct ui_box value_box(int row, int i)
{
    return (struct ui_box){206 + i * 52, ROW_Y(row), 48, 28};
}

static struct ui_box auto_box(int i)
{
    return (struct ui_box){140 + i * 66, ROW_Y(4), 62, 28};
}

static const struct ui_box nav_box = {8, ROW_Y(5), 150, 28};
static const struct ui_box clean_box = {8, ROW_Y(3), 200, 28};

/* Touch area of a toggle row: the switch plus some margin to its left. */
static struct ui_box toggle_hit_box(int row)
{
    struct ui_box b = toggle_box(row);

    return (struct ui_box){b.x - 40, b.y - 2, b.w + 40, b.h + 4}; /* rows are 32 apart */
}

static void draw_toggle(int row, const char *label, bool on)
{
    struct ui_box b = toggle_box(row);

    gfx_text(8, b.y + 7, label, 2);
    gfx_text(b.x - 30, b.y + 10, on ? "ON" : "OFF", 1);
    ui_frame(&b, 2);

    /* Knob: hollow at the left when off, solid at the right when on. */
    if (on) {
        gfx_fill_rect(b.x + b.w - 34, b.y + 4, 30, b.h - 8);
    } else {
        gfx_frame(b.x + 4, b.y + 4, 30, b.h - 8, 2);
    }
}

static void draw_page0(void)
{
    char buf[16];

    draw_toggle(0, "FLIP DISPLAY", ui_settings.flip);

    gfx_text(8, ROW_Y(1) + 7, "PEN SIZE", 2);
    for (int i = 0; i < 3; i++) {
        struct ui_box b = radio_box(1, i);
        bool selected = ui_settings.pen == pen_sizes[i];

        snprintf(buf, sizeof(buf), "%u", pen_sizes[i]);
        ui_frame(&b, selected ? 4 : 2);
        ui_text_center(&b, buf, 2);
    }

    ui_button(&nav_box, "MORE >", 2);
}

static void draw_values(int row, const char *label, const uint16_t *values, uint16_t current)
{
    char buf[8];

    gfx_text(8, ROW_Y(row) + 7, label, 2);
    for (int i = 0; i < 4; i++) {
        struct ui_box b = value_box(row, i);

        snprintf(buf, sizeof(buf), "%u", (unsigned)values[i]);
        ui_frame(&b, current == values[i] ? 4 : 2);
        ui_text_center(&b, buf, 2);
    }
}

static void draw_page1(void)
{
    char buf[56];
    uint32_t count;
    uint32_t ms;

    draw_toggle(0, "FAST UPDATE", ui_settings.fast_update);
    draw_values(1, "DEBOUNCE MS", debounce_values, ui_settings.touch_debounce_ms);
    draw_values(2, "HOLD MS", hold_values, ui_settings.touch_hold_ms);

    ui_button(&clean_box, "CLEAN PANEL", 2);
    gfx_text(clean_box.x + clean_box.w + 10, clean_box.y + 10, "4 FULL FLASHES", 1);

    ui_stats(&count, &ms);
    snprintf(buf, sizeof(buf), "PREVIOUS REFRESH: %u MS (#%u)", (unsigned)ms, (unsigned)count);
    gfx_text(nav_box.x + nav_box.w + 8, ROW_Y(5) + 10, buf, 1);

    gfx_text(8, ROW_Y(4) + 7, "AUTO CLEAN", 2);
    for (int i = 0; i < 4; i++) {
        struct ui_box b = auto_box(i);

        ui_frame(&b, ui_settings.auto_clean == i ? 4 : 2);
        ui_text_center(&b, auto_labels[i], 2);
    }

    ui_button(&nav_box, "< BACK", 2);
}

static void draw(void)
{
    if (page == 0) {
        draw_page0();
    } else {
        draw_page1();
    }
}

static void touch_page0(const struct touch_point *p)
{
    struct ui_box flip = toggle_hit_box(0);

    if (ui_hit(&flip, p->x, p->y)) {
        ui_settings.flip = !ui_settings.flip;
        ui_apply_settings();
        ui_redraw();
        return;
    }

    for (int i = 0; i < 3; i++) {
        struct ui_box b = radio_box(1, i);

        if (ui_hit(&b, p->x, p->y)) {
            ui_settings.pen = pen_sizes[i];
            ui_redraw();
            return;
        }
    }

    if (ui_hit(&nav_box, p->x, p->y)) {
        page = 1;
        ui_redraw();
    }
}

static void touch_page1(const struct touch_point *p)
{
    struct ui_box fast = toggle_hit_box(0);

    if (ui_hit(&fast, p->x, p->y)) {
        ui_settings.fast_update = !ui_settings.fast_update;
        ui_redraw();
        return;
    }

    for (int i = 0; i < 4; i++) {
        struct ui_box d = value_box(1, i);

        if (ui_hit(&d, p->x, p->y)) {
            ui_settings.touch_debounce_ms = debounce_values[i];
            ui_apply_settings(); /* effective immediately */
            ui_redraw();
            return;
        }
    }

    for (int i = 0; i < 4; i++) {
        struct ui_box d = value_box(2, i);

        if (ui_hit(&d, p->x, p->y)) {
            ui_settings.touch_hold_ms = hold_values[i];
            ui_apply_settings(); /* effective immediately */
            ui_redraw();
            return;
        }
    }

    for (int i = 0; i < 4; i++) {
        struct ui_box d = auto_box(i);

        if (ui_hit(&d, p->x, p->y)) {
            ui_settings.auto_clean = i;
            ui_redraw();
            return;
        }
    }

    if (ui_hit(&clean_box, p->x, p->y)) {
        ui_clean_panel(); /* ignored if one is already running */
        return;
    }

    if (ui_hit(&nav_box, p->x, p->y)) {
        page = 0;
        ui_redraw();
    }
}

static void touch(const struct touch_point *p)
{
    if (p->event != TOUCH_PRESS) {
        return;
    }
    if (page == 0) {
        touch_page0(p);
    } else {
        touch_page1(p);
    }
}

static void enter(void)
{
    page = 0;
}

const struct ui_screen demo_settings = {
    .title = "SETTINGS",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
