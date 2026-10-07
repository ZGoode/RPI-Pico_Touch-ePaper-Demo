#include "ui/menu.h"

#include <stdio.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Home screen: tiles grouped into pages of up to six (two columns by three
 * rows), with PREV / NEXT in the header.
 *
 * To add a demo: write its struct ui_screen in demos/, declare it in
 * demos/demos.h, add its .c file to CMakeLists.txt and add one line to the
 * table below, naming the page it belongs on (and raise PAGE_COUNT if it
 * starts a new page).
 */

struct tile {
    uint8_t page;                  /* 0-based home page */
    const char *label;             /* drawn at scale 2, at most 13 characters */
    const char *description;       /* drawn at scale 1 */
    const struct ui_screen *screen;
};

static const struct tile tiles[] = {
    /* Page 1: input */
    {0, "TOUCH TEST", "COORDINATES + MARKER", &demo_touch_test},
    {0, "NUMBER KEYPAD", "ENTER DIGITS", &demo_keypad},
    {0, "TEXT KEYBOARD", "ON-SCREEN QWERTY", &demo_keyboard},
    {0, "SCRIBBLE", "DRAW WITH A FINGER", &demo_scribble},
    {0, "GESTURES", "TAP, HOLD, SWIPE", &demo_gestures},
    {0, "LIST", "SCROLL, SELECT, CONFIRM", &demo_list},
    /* Page 2: apps */
    {1, "TIC-TAC-TOE", "1 OR 2 PLAYERS", &demo_tictactoe},
    {1, "HANDWRITING", "DRAW A CHARACTER, READ IT", &demo_handwriting},
    {1, "SETTINGS", "FAST, DEBOUNCE, HOLD, CLEAN", &demo_settings},
    {1, "SLEEP", "IDLE, TOUCH TO WAKE", &demo_sleep},
    {1, "SYSTEM INFO", "DISPLAY AND TOUCH", &demo_info},
    /* Page 3: diagnostics */
    {2, "TOUCH DIAG", "INT, I2C, RAW REPORT", &demo_diag},
    {2, "ACCURACY GRID", "NINE-POINT TAP TEST", &demo_touch_accuracy},
    {2, "REFRESH BENCH", "TIMED FULL REFRESHES", &demo_bench},
};

#define TILE_COUNT     ((int)(sizeof(tiles) / sizeof(tiles[0])))
#define TILES_PER_PAGE 6
#define PAGE_COUNT     3 /* highest .page above + 1 */

static int page; /* kept while visiting a demo, so HOME returns to it */

/* The tile in `slot` of home page `pg`, or NULL. */
static const struct tile *tile_at(int pg, int slot)
{
    int n = 0;

    for (int i = 0; i < TILE_COUNT; i++) {
        if (tiles[i].page == pg && n++ == slot) {
            return &tiles[i];
        }
    }
    return NULL;
}

static struct ui_box tile_box(int slot)
{
    return (struct ui_box){8 + (slot % 2) * 204, 42 + (slot / 2) * 64, 196, 56};
}

static void menu_draw(void)
{
    char buf[24];
    struct ui_box next = ui_header_box(0);
    struct ui_box prev = ui_header_box(1);

    if (touch_is_ready()) {
        snprintf(buf, sizeof(buf), "PAGE %d/%d", page + 1, PAGE_COUNT);
        gfx_text(140, 12, buf, 1);
    } else {
        gfx_text(140, 12, "TOUCH NOT FOUND", 1);
    }
    ui_button(&prev, "PREV", 2);
    ui_button(&next, "NEXT", 2);

    for (int slot = 0; slot < TILES_PER_PAGE; slot++) {
        const struct tile *t = tile_at(page, slot);

        if (!t) {
            break;
        }

        struct ui_box b = tile_box(slot);
        struct ui_box label = {b.x, b.y + 8, b.w, 20};
        struct ui_box description = {b.x, b.y + 34, b.w, 10};

        ui_frame(&b, 2);
        ui_text_center(&label, t->label, 2);
        ui_text_center(&description, t->description, 1);
    }
}

static void menu_touch(const struct touch_point *p)
{
    struct ui_box next = ui_header_box(0);
    struct ui_box prev = ui_header_box(1);

    if (p->event != TOUCH_PRESS) {
        return;
    }

    if (ui_hit(&next, p->x, p->y) || ui_hit(&prev, p->x, p->y)) {
        page = (page + (ui_hit(&next, p->x, p->y) ? 1 : PAGE_COUNT - 1)) % PAGE_COUNT;
        ui_redraw();
        return;
    }

    for (int slot = 0; slot < TILES_PER_PAGE; slot++) {
        const struct tile *t = tile_at(page, slot);
        struct ui_box b = tile_box(slot);

        if (t && ui_hit(&b, p->x, p->y)) {
            ui_goto(t->screen);
            return;
        }
    }
}

const struct ui_screen ui_menu = {
    .title = "DEMO MENU",
    .draw = menu_draw,
    .touch = menu_touch,
};
