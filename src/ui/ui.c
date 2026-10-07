#include "ui/ui.h"

#include <stdio.h>

#include "board/board_config.h"
#include "board/status_led.h"
#include "display/gfx.h"
#include "pico/stdlib.h"

/* The demo layouts are drawn for a landscape screen. */
_Static_assert(DISPLAY_ROTATION == 1 || DISPLAY_ROTATION == 3,
               "The demo screens need a landscape DISPLAY_ROTATION (1 or 3)");

/* Panel clean: a number of global updates alternating black and white that
 * always ends on white.
 *   CLEAN_FLASHES       the Settings "CLEAN PANEL" button
 *   BOOT_CLEAN_FLASHES  at power-up (the first update is global anyway)
 */
#define CLEAN_FLASHES      4
#define BOOT_CLEAN_FLASHES 1

#define EVENT_QUEUE_SIZE 8

static const struct ui_screen *home_screen;
static const struct ui_screen *current;

/* Touch events received but not yet handled. When the queue is full the
 * oldest event is dropped. */
static struct touch_point queue[EVENT_QUEUE_SIZE];
static unsigned int queue_head;
static unsigned int queue_count;

/* Ignore the rest of the contact that changed screens, so the new screen
 * does not see a RELEASE (or MOVE) without a PRESS. */
static bool swallow;

struct ui_settings ui_settings = {
    .flip = false,
    .fast_update = true,
    .touch_debounce_ms = 40,
    .touch_hold_ms = 600,
    .auto_clean = 3, /* power-up and home */
    .pen = 3,
};

static bool redraw_requested;
static bool global_next; /* the next refresh is global (returning home) */
static bool clean_requested;
static int clean_flashes = CLEAN_FLASHES;
static bool cleaning;
static uint32_t refresh_count;
static uint32_t refresh_last_ms;

/* The touch rotation follows what is physically on the glass, not what has
 * merely been requested: a flip only takes effect for touch once the flipped
 * picture has finished refreshing (taps during that refresh still land on the
 * old picture). */
static int glass_orientation = DISPLAY_ROTATION;

uint32_t ui_millis(void)
{
    return to_ms_since_boot(get_absolute_time());
}

void ui_apply_settings(void)
{
    gfx_set_flip(ui_settings.flip);
    touch_set_debounce(ui_settings.touch_debounce_ms);
    touch_set_hold(ui_settings.touch_hold_ms);
}

int ui_orientation(void)
{
    return glass_orientation;
}

/* ---- Touch event queue ------------------------------------------------- */

static void queue_push(const struct touch_point *p)
{
    if (queue_count == EVENT_QUEUE_SIZE) {
        queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
        queue_count--;
    }
    queue[(queue_head + queue_count) % EVENT_QUEUE_SIZE] = *p;
    queue_count++;
}

static bool queue_pop(struct touch_point *p)
{
    if (queue_count == 0) {
        return false;
    }
    *p = queue[queue_head];
    queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
    queue_count--;
    return true;
}

/* Called from touch_poll(), possibly while a refresh is in progress. */
static void on_touch(const struct touch_point *p)
{
    /* The LED follows the finger, even during a refresh. */
    if (p->event == TOUCH_PRESS) {
        status_led_set(true);
    } else if (p->event == TOUCH_RELEASE) {
        status_led_set(false);
    }

    if (cleaning) {
        /* Panel clean in progress: drop the touch and the rest of its contact. */
        swallow = (p->event != TOUCH_RELEASE);
        return;
    }
    queue_push(p);
}

/* ---- Screens ----------------------------------------------------------- */

static void enter_screen(const struct ui_screen *s)
{
    current = s;
    if (s->enter) {
        s->enter();
    }
    redraw_requested = true;
}

void ui_goto(const struct ui_screen *screen)
{
    swallow = true;
    if (screen == home_screen && current != home_screen && (ui_settings.auto_clean & 2)) {
        global_next = true; /* mini clean: this refresh is global */
    }
    enter_screen(screen);
}

void ui_redraw(void)
{
    redraw_requested = true;
}

void ui_clean_panel(void)
{
    if (clean_requested || cleaning) {
        return;
    }
    clean_flashes = CLEAN_FLASHES;
    clean_requested = true;
}

void ui_stats(uint32_t *count, uint32_t *last_ms)
{
    *count = refresh_count;
    *last_ms = refresh_last_ms;
}

static void dispatch(const struct touch_point *p)
{
    if (p->event == TOUCH_PRESS) {
        swallow = false;
    }
    if (swallow) {
        if (p->event == TOUCH_RELEASE) {
            swallow = false;
        }
        return;
    }

    if (current != home_screen && p->event == TOUCH_PRESS) {
        struct ui_box home_button = ui_header_box(0);

        if (ui_hit(&home_button, p->x, p->y)) {
            ui_goto(home_screen);
            return;
        }
    }
    if (current->touch) {
        current->touch(p);
    }
}

/* ---- Rendering --------------------------------------------------------- */

static void draw_header(const struct ui_screen *s)
{
    gfx_text(8, 10, s->title, 2);
    gfx_fill_rect(0, UI_HEADER_H - 2, gfx_width(), 2);
    if (s != home_screen) {
        struct ui_box home_button = ui_header_box(0);

        ui_button(&home_button, "HOME", 2);
    }
}

static void clean_panel(void)
{
    cleaning = true;
    for (int i = 0; i < clean_flashes; i++) {
        bool black = ((clean_flashes - i) % 2) == 0; /* counted from the end */

        gfx_clear();
        if (black) {
            gfx_fill_rect(0, 0, gfx_width(), gfx_height());
        }
        if (epd_refresh(EPD_UPDATE_GLOBAL) != 0) {
            break;
        }
    }
    cleaning = false;
}

static void render(void)
{
    redraw_requested = false;

    if (clean_requested) {
        clean_requested = false;
        clean_panel();
    }

    enum epd_update_mode mode = ui_settings.fast_update ? EPD_UPDATE_FAST : EPD_UPDATE_GLOBAL;

    if (global_next) {
        global_next = false;
        mode = EPD_UPDATE_GLOBAL;
    }

    gfx_clear();
    draw_header(current);
    if (current->draw) {
        current->draw();
    }
    bool built_flip = gfx_get_flip();

    uint32_t start = ui_millis();
    int ret = epd_refresh(mode);

    refresh_last_ms = ui_millis() - start;
    refresh_count++;
    if (ret != 0) {
        status_led_blink(5);
        return;
    }

    /* The picture that is now on the glass decides the touch rotation. */
    int want = (DISPLAY_ROTATION + (built_flip ? 2 : 0)) & 3;

    if (want != glass_orientation) {
        glass_orientation = want;
        touch_set_display(EPD_WIDTH, EPD_HEIGHT, glass_orientation);
    }
    if (current->refreshed) {
        current->refreshed(refresh_last_ms);
    }
}

void ui_run(const struct ui_screen *home)
{
    home_screen = home;
    ui_apply_settings();

    if (touch_is_ready()) {
        touch_start(on_touch);
        /* Keep sampling touch while the panel is refreshing. */
        epd_set_busy_hook(touch_poll);
    }

    if (ui_settings.auto_clean & 1) {
        clean_flashes = BOOT_CLEAN_FLASHES;
        clean_requested = true; /* clean first; the home screen is drawn right after it */
    }
    enter_screen(home);

    for (;;) {
        struct touch_point p;

        touch_poll();

        /* Handle every queued event before drawing so that the redraw
         * requests they make share one refresh. */
        while (queue_pop(&p)) {
            dispatch(&p);
        }

        if (redraw_requested || clean_requested) {
            render();
        } else {
            sleep_ms(1);
        }
    }
}

/* ---- Widgets ----------------------------------------------------------- */

struct ui_box ui_header_box(int slot)
{
    int w = gfx_width();

    if (slot <= 0) {
        return (struct ui_box){w - 72, 3, 68, 28};
    }
    return (struct ui_box){w - 72 - 8 - 84 - (slot - 1) * 92, 3, 84, 28};
}

void ui_frame(const struct ui_box *box, int thickness)
{
    gfx_frame(box->x, box->y, box->w, box->h, thickness);
}

void ui_text_center(const struct ui_box *box, const char *text, int scale)
{
    int x = box->x + (box->w - gfx_text_width(text, scale)) / 2;
    int y = box->y + (box->h - GFX_GLYPH_H * scale) / 2;

    gfx_text(x, y, text, scale);
}

void ui_button(const struct ui_box *box, const char *label, int scale)
{
    ui_frame(box, 2);
    ui_text_center(box, label, scale);
}

bool ui_hit(const struct ui_box *box, int x, int y)
{
    return x >= box->x && x < box->x + box->w && y >= box->y && y < box->y + box->h;
}
