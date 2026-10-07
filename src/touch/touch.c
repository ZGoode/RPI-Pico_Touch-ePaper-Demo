#include "touch/touch.h"

#include <stdlib.h>

#include "pico/stdlib.h"
#include "touch/tp370.h"

/* A MOVE is only reported once the point has travelled this many pixels. */
#define MOVE_MIN_DOTS 3

/* A finger that strays further than this from its press point cannot HOLD. */
#define HOLD_TRAVEL_DOTS 12

/* After this many consecutive reports without a usable touch while a finger
 * is down, the finger is treated as lifted. */
#define STALE_RELEASE_SAMPLES 6

/* Settle time between the INT interrupt and the first read, and the sampling
 * interval while a finger is down. */
#define IRQ_SETTLE_MS    10
#define POLL_INTERVAL_MS 16

static bool ready;
static touch_callback_t callback;

/* Native display size and rotation used to map controller coordinates. */
static uint16_t native_w = TP370_RAW_X_MAX + 1;
static uint16_t native_h = TP370_RAW_Y_MAX + 1;
static int rotation;

static uint16_t debounce_ms;
static uint16_t hold_ms;

/* Sampling schedule. */
static volatile bool irq_pending; /* set by the INT interrupt */
static bool sampling;
static uint32_t next_sample_ms;

/* Contact state. */
static bool down;
static struct touch_point last;
static uint16_t press_x, press_y;
static uint32_t press_ms;
static uint32_t release_ms;
static bool released_once;
static bool hold_fired;
static bool hold_cancel;
static bool lost; /* finger missing; the release debounce is running */
static uint32_t lost_ms;
static uint8_t stale;

static struct touch_stats stats;

static uint32_t now_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

static void int_asserted_isr(void)
{
    irq_pending = true;
    stats.int_edges++;
}

/* Linear map of 0..in_max onto out_a..out_b. */
static int32_t scale(int32_t v, int32_t in_max, int32_t out_a, int32_t out_b)
{
    return v * (out_b - out_a) / in_max + out_a;
}

/*
 * Converts controller coordinates (native portrait, origin top-left) to
 * display coordinates. This is the inverse of the mapping in display/gfx.c;
 * both depend on the same rotation.
 */
static void map_point(uint16_t raw_x, uint16_t raw_y, uint16_t *x, uint16_t *y)
{
    int32_t rx = raw_x > TP370_RAW_X_MAX ? TP370_RAW_X_MAX : raw_x;
    int32_t ry = raw_y > TP370_RAW_Y_MAX ? TP370_RAW_Y_MAX : raw_y;
    int32_t w = native_w - 1;
    int32_t h = native_h - 1;

    switch (rotation) {
    case 1:
        *x = scale(ry, TP370_RAW_Y_MAX, 0, h);
        *y = scale(rx, TP370_RAW_X_MAX, w, 0);
        break;
    case 2:
        *x = scale(rx, TP370_RAW_X_MAX, w, 0);
        *y = scale(ry, TP370_RAW_Y_MAX, h, 0);
        break;
    case 3:
        *x = scale(ry, TP370_RAW_Y_MAX, h, 0);
        *y = scale(rx, TP370_RAW_X_MAX, 0, w);
        break;
    default:
        *x = scale(rx, TP370_RAW_X_MAX, 0, w);
        *y = scale(ry, TP370_RAW_Y_MAX, 0, h);
        break;
    }
}

/* Reads the controller once and returns true if that produced an event. */
static bool sample(struct touch_point *out)
{
    struct tp370_sample raw;
    bool finger = false;

    if (tp370_int_asserted()) {
        int ret = tp370_read(&raw);

        stats.reads++;
        if (ret == PICO_ERROR_NO_DATA) {
            stats.unusable++;
            /* Unusable report: the contact state stands, unless it keeps
             * happening, in which case the finger is treated as lifted. */
            if (!down || ++stale < STALE_RELEASE_SAMPLES) {
                return false;
            }
        } else if (ret < 0) {
            stats.read_errors++;
            return false;
        } else {
            finger = true;
        }
    }

    uint32_t now = now_ms();

    if (finger) {
        lost = false;

        if (!down && debounce_ms != 0 && released_once &&
            (now - release_ms) < debounce_ms) {
            return false; /* bounce right after a release, not a new touch */
        }

        struct touch_point p = {.raw_x = raw.x, .raw_y = raw.y, .time_ms = now};

        map_point(raw.x, raw.y, &p.x, &p.y);

        if (!down) {
            p.event = TOUCH_PRESS;
            press_ms = now;
            press_x = p.x;
            press_y = p.y;
            hold_fired = false;
            hold_cancel = false;
        } else {
            if (abs((int)p.x - press_x) > HOLD_TRAVEL_DOTS ||
                abs((int)p.y - press_y) > HOLD_TRAVEL_DOTS) {
                hold_cancel = true;
            }

            int dx = (int)p.x - (int)last.x;
            int dy = (int)p.y - (int)last.y;

            if (abs(dx) < MOVE_MIN_DOTS && abs(dy) < MOVE_MIN_DOTS) {
                if (hold_ms != 0 && !hold_fired && !hold_cancel &&
                    (now - press_ms) >= hold_ms) {
                    hold_fired = true;
                    p.event = TOUCH_HOLD;
                    *out = p; /* `last` is unchanged so MOVE tracking continues */
                    return true;
                }
                return false; /* finger resting */
            }
            p.event = TOUCH_MOVE;
        }

        down = true;
        stale = 0;
        last = p;
        *out = p;
        return true;
    }

    if (down) {
        /* The finger is gone. With debounce on, wait to see if it returns. */
        if (debounce_ms != 0) {
            if (!lost) {
                lost = true;
                lost_ms = now;
            }
            if ((now - lost_ms) < debounce_ms) {
                return false; /* still counts as down */
            }
        }

        down = false;
        lost = false;
        stale = 0;
        release_ms = now;
        released_once = true;
        *out = last;
        out->event = TOUCH_RELEASE;
        out->time_ms = now;
        return true;
    }

    return false;
}

int touch_init(void)
{
    int ret = tp370_init();

    ready = (ret == 0);
    return ret;
}

bool touch_is_ready(void)
{
    return ready;
}

void touch_set_display(uint16_t native_width, uint16_t native_height, int rot)
{
    native_w = native_width;
    native_h = native_height;
    rotation = rot;
}

void touch_start(touch_callback_t cb)
{
    if (!ready) {
        return;
    }
    callback = cb;
    tp370_int_enable(int_asserted_isr);
}

void touch_poll(void)
{
    if (!ready || callback == NULL) {
        return;
    }

    uint32_t now = now_ms();

    if (!sampling) {
        if (irq_pending) {
            /* A new contact: let the controller settle before reading. */
            irq_pending = false;
            sampling = true;
            next_sample_ms = now + IRQ_SETTLE_MS;
        } else if (tp370_int_asserted()) {
            /* INT was already asserted when polling started. */
            sampling = true;
            next_sample_ms = now;
        } else {
            return;
        }
    }

    if ((int32_t)(now - next_sample_ms) < 0) {
        return;
    }

    struct touch_point p;

    if (sample(&p)) {
        stats.events++;
        callback(&p);
    }

    next_sample_ms = now + POLL_INTERVAL_MS;
    if (!down && !tp370_int_asserted()) {
        sampling = false;
    }
}

void touch_set_debounce(uint16_t ms)
{
    debounce_ms = ms;
}

uint16_t touch_get_debounce(void)
{
    return debounce_ms;
}

void touch_set_hold(uint16_t ms)
{
    hold_ms = ms;
}

uint16_t touch_get_hold(void)
{
    return hold_ms;
}

void touch_get_stats(struct touch_stats *out)
{
    *out = stats;
}

const char *touch_name(void)
{
    return ready ? TP370_NAME : NULL;
}

int touch_last_report(uint8_t *buf, int max)
{
    return tp370_last_report(buf, max);
}
