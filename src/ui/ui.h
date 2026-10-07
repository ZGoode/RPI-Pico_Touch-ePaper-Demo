#ifndef UI_UI_H_
#define UI_UI_H_

#include <stdbool.h>
#include <stdint.h>

#include "display/epd.h"
#include "touch/touch.h"

/*
 * Screen manager and small widget helpers shared by the demos.
 *
 * The UI shows one screen at a time. A screen is a set of callbacks:
 *
 *   enter      the screen has just become active; reset its state here
 *   draw       draw the screen into the framebuffer (display/gfx.h)
 *   touch      handle a touch event (touch/touch.h)
 *   refreshed  the panel has finished showing the screen (optional)
 *
 * Screens never talk to the panel themselves. When something changes they
 * call ui_redraw(); the UI then clears the framebuffer, draws the header and
 * the screen's draw callback, and refreshes the panel. A refresh takes a
 * while, so touch events that arrive meanwhile are queued, and any number of
 * redraw requests made before the next refresh are served by one refresh.
 *
 * Every screen except the home screen gets a HOME button in its header that
 * returns to the home screen. Screens that are not the home screen may use
 * header slots 1 and 2 (see ui_header_box()) for their own buttons.
 *
 * All callbacks run on the main thread from ui_run(); no locking is needed.
 */

/* Height of the header band; screen content starts below it. */
#define UI_HEADER_H 34

struct ui_box {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

struct ui_screen {
    const char *title; /* shown in the header */
    void (*enter)(void);
    void (*draw)(void);
    void (*touch)(const struct touch_point *point);
    void (*refreshed)(uint32_t refresh_ms);
};

/*
 * User-adjustable settings (changed on the Settings screen, read by other
 * screens). Call ui_apply_settings() after changing them.
 */
struct ui_settings {
    bool flip;                  /* picture rotated a further 180 degrees */
    bool fast_update;           /* fast update for screen refreshes; off = global */
    uint16_t touch_debounce_ms; /* 0 = off; see touch_set_debounce() */
    uint16_t touch_hold_ms;     /* 0 = off; see touch_set_hold() */
    uint8_t auto_clean;         /* bit 0: panel clean at power-up;
                                 * bit 1: global update when returning to the home screen */
    uint8_t pen;                /* scribble pen width in pixels: 1, 3 or 5 */
};
extern struct ui_settings ui_settings;

/* Pushes ui_settings to the display and touch layers. */
void ui_apply_settings(void);

/* Runs the UI starting at `home`, which is also where HOME returns. Never returns. */
void ui_run(const struct ui_screen *home);

/* Makes `screen` the active screen and redraws. The rest of the current
 * finger contact is not delivered to the new screen. */
void ui_goto(const struct ui_screen *screen);

/* Requests a refresh of the current screen. */
void ui_redraw(void);

/*
 * Panel clean: a few full black / white global updates to wipe ghosting
 * (takes tens of seconds), then the current screen is redrawn. Touch input is
 * ignored meanwhile.
 */
void ui_clean_panel(void);

/* Number of screen refreshes so far and how long the last one took. */
void ui_stats(uint32_t *count, uint32_t *last_ms);

/* Touch rotation matching what is currently on the glass. */
int ui_orientation(void);

/* Milliseconds since power-up. */
uint32_t ui_millis(void);

/* ---- Widgets ---- */

/* Box of header button `slot`: 0 is HOME, 1 and 2 sit to its left. */
struct ui_box ui_header_box(int slot);

/* Rectangle outline. */
void ui_frame(const struct ui_box *box, int thickness);

/* Text centred in `box`. */
void ui_text_center(const struct ui_box *box, const char *text, int scale);

/* Outlined box with a centred label. */
void ui_button(const struct ui_box *box, const char *label, int scale);

/* True if the point is inside the box. */
bool ui_hit(const struct ui_box *box, int x, int y);

#endif /* UI_UI_H_ */
