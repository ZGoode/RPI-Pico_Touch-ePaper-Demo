#ifndef TOUCH_TOUCH_H_
#define TOUCH_TOUCH_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * Touch input as a stream of events in display coordinates.
 *
 * The layer reads samples from the controller driver (touch/tp370.c), maps
 * them to the rotated display coordinates used by display/gfx.h, and
 * condenses them into PRESS, MOVE, HOLD and RELEASE events:
 *
 *   PRESS    a finger touched the panel
 *   MOVE     the finger moved a few pixels from its last reported position
 *   HOLD     the finger rested in place for the hold time (once per touch)
 *   RELEASE  the finger lifted; reported at the last known position
 *
 * Events carry the time they were sampled, so timing stays correct even if
 * the application handles them late (for example after a display refresh).
 *
 * Usage: touch_init(), touch_set_display(), touch_start(callback), then call
 * touch_poll() often (at least every few milliseconds). The callback runs
 * from touch_poll().
 *
 * How sampling works: the controller raises INT while it has data. The INT
 * interrupt only marks that a new contact has started; touch_poll() then
 * waits for the controller to settle and reads a sample every 16 ms until the
 * finger is gone and INT is idle again. The controller is never read while
 * INT is idle.
 */

enum touch_event {
    TOUCH_PRESS,
    TOUCH_MOVE,
    TOUCH_HOLD,
    TOUCH_RELEASE,
};

struct touch_point {
    enum touch_event event;
    uint16_t x;     /* display coordinates */
    uint16_t y;
    uint16_t raw_x; /* controller coordinates, before rotation */
    uint16_t raw_y;
    uint32_t time_ms; /* when the sample was taken, in ms since power-up */
};

typedef void (*touch_callback_t)(const struct touch_point *point);

/* Initializes the touch controller. Returns 0 or a negative PICO_ERROR_* code. */
int touch_init(void);

/* True once touch_init() has succeeded. */
bool touch_is_ready(void);

/*
 * Describes the display the touch panel is mounted on: the native (rotation
 * 0) size in pixels and the rotation, using the same values as
 * DISPLAY_ROTATION in board/board_config.h.
 */
void touch_set_display(uint16_t native_width, uint16_t native_height, int rotation);

/* Starts delivering events to `callback`. */
void touch_start(touch_callback_t callback);

/* Samples the controller when due and delivers events. Call frequently. */
void touch_poll(void);

/*
 * Debounce time in milliseconds (0 = off): a finger that disappears for less
 * than this time is treated as still down, and a new PRESS is ignored for
 * this long after a RELEASE.
 */
void touch_set_debounce(uint16_t ms);
uint16_t touch_get_debounce(void);

/* Counters since power-up, for diagnostics screens. */
struct touch_stats {
    uint32_t int_edges;   /* INT asserted edges seen by the interrupt */
    uint32_t reads;       /* controller reads attempted */
    uint32_t read_errors; /* reads that failed on the bus */
    uint32_t unusable;    /* reads whose report held no usable touch */
    uint32_t events;      /* PRESS / MOVE / HOLD / RELEASE events produced */
};

void touch_get_stats(struct touch_stats *stats);

/* Controller name, or NULL if the controller was not found. */
const char *touch_name(void);

/* Copies the raw bytes of the last controller report; returns the length, 0 if none yet. */
int touch_last_report(uint8_t *buf, int max);

/* Time in milliseconds a finger must rest before HOLD is reported (0 = off). */
void touch_set_hold(uint16_t ms);
uint16_t touch_get_hold(void);

#endif /* TOUCH_TOUCH_H_ */
