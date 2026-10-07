#include <stdio.h>

#include "board/board_config.h"
#include "demos/demos.h"
#include "display/gfx.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

/*
 * Touch diagnostics: live INT level, controller counters, the last raw
 * report, the last contact's sample rate, and an I2C scan of the touch bus
 * (which should show the touch controller at 0x38). The e-paper is slow, so
 * values are a snapshot taken at each refresh: the screen refreshes when a
 * finger lifts, and on SCAN.
 */

#define SCAN_FIRST 0x08
#define SCAN_LAST  0x77
#define MAX_FOUND  24

static uint8_t found[MAX_FOUND];
static int found_count;
static bool scanned;

static bool have_event;
static enum touch_event last_event;
static uint16_t last_x, last_y, last_raw_x, last_raw_y;

static uint32_t press_ms;
static uint32_t press_reads;
static bool have_contact;
static uint32_t contact_samples, contact_ms;

/* Probes every address with a one-byte read, as the Pico SDK's bus scan
 * example does; a device that acknowledges is listed. Addresses reserved by
 * the I2C specification are skipped. */
static void scan(void)
{
    found_count = 0;
    for (int addr = SCAN_FIRST; addr <= SCAN_LAST; addr++) {
        uint8_t dummy;

        if (i2c_read_timeout_us(TOUCH_I2C, addr, &dummy, 1, false, 5000) >= 0 &&
            found_count < MAX_FOUND) {
            found[found_count++] = addr;
        }
    }
    scanned = true;
}

static void enter(void)
{
    scan();
}

static const char *event_name(void)
{
    switch (last_event) {
    case TOUCH_PRESS:
        return "PRESS";
    case TOUCH_MOVE:
        return "MOVE";
    case TOUCH_RELEASE:
        return "RELEASE";
    case TOUCH_HOLD:
        return "HOLD";
    default:
        return "NONE";
    }
}

/* Board knowledge, only used to label the scan. */
static const char *label_for(uint8_t addr)
{
    if (addr == 0x38) {
        const char *name = touch_name();

        return name ? name : "TOUCH";
    }
    return "?";
}

static void draw(void)
{
    char buf[64];
    struct touch_stats st;
    uint8_t report[16];
    struct ui_box scan_box = ui_header_box(1);

    ui_button(&scan_box, "SCAN", 2);

    snprintf(buf, sizeof(buf), "INT GP%d: %s", PIN_TOUCH_INT,
             gpio_get(PIN_TOUCH_INT) ? "IDLE" : "ASSERTED");
    gfx_text(8, 40, buf, 2);

    touch_get_stats(&st);
    snprintf(buf, sizeof(buf), "INT EDGES %u  READS %u  EVENTS %u", (unsigned)st.int_edges,
             (unsigned)st.reads, (unsigned)st.events);
    gfx_text(8, 62, buf, 1);
    snprintf(buf, sizeof(buf), "READ ERRORS %u  UNUSABLE REPORTS %u", (unsigned)st.read_errors,
             (unsigned)st.unusable);
    gfx_text(8, 74, buf, 1);

    if (have_event) {
        snprintf(buf, sizeof(buf), "LAST: %s  LOGICAL %u,%u  RAW %u,%u", event_name(),
                 (unsigned)last_x, (unsigned)last_y, (unsigned)last_raw_x, (unsigned)last_raw_y);
    } else {
        snprintf(buf, sizeof(buf), "LAST: NO TOUCH YET");
    }
    gfx_text(8, 90, buf, 1);

    int n = touch_last_report(report, sizeof(report));

    if (n > 0) {
        int o = snprintf(buf, sizeof(buf), "REPORT:");

        for (int i = 0; i < n && o < (int)sizeof(buf) - 4; i++) {
            o += snprintf(buf + o, sizeof(buf) - o, " %02X", report[i]);
        }
    } else {
        snprintf(buf, sizeof(buf), "REPORT: NONE YET");
    }
    gfx_text(8, 102, buf, 1);

    if (have_contact && contact_ms > 0) {
        snprintf(buf, sizeof(buf), "LAST CONTACT: %u SAMPLES IN %u MS = %u/S",
                 (unsigned)contact_samples, (unsigned)contact_ms,
                 (unsigned)(contact_samples * 1000 / contact_ms));
    } else {
        snprintf(buf, sizeof(buf), "LAST CONTACT: -");
    }
    gfx_text(8, 114, buf, 1);

    snprintf(buf, sizeof(buf), "I2C SCAN: I2C%u", i2c_hw_index(TOUCH_I2C));
    gfx_text(8, 136, buf, 2);

    if (!scanned || found_count == 0) {
        gfx_text(8, 160, "NOTHING FOUND", 2);
        return;
    }
    for (int i = 0; i < found_count && i < 3; i++) {
        snprintf(buf, sizeof(buf), "0x%02X  %s", found[i], label_for(found[i]));
        gfx_text(8, 160 + i * 20, buf, 2);
    }
    if (found_count > 3) {
        snprintf(buf, sizeof(buf), "+%d MORE", found_count - 3);
        gfx_text(8, 220, buf, 1);
    }
}

static void touch(const struct touch_point *p)
{
    struct ui_box scan_box = ui_header_box(1);
    struct touch_stats st;

    if (p->event == TOUCH_PRESS && ui_hit(&scan_box, p->x, p->y)) {
        scan();
        ui_redraw();
        return;
    }

    have_event = true;
    last_event = p->event;
    last_x = p->x;
    last_y = p->y;
    last_raw_x = p->raw_x;
    last_raw_y = p->raw_y;

    touch_get_stats(&st);
    if (p->event == TOUCH_PRESS) {
        press_ms = p->time_ms;
        press_reads = st.reads;
    } else if (p->event == TOUCH_RELEASE) {
        contact_ms = p->time_ms - press_ms;
        contact_samples = st.reads - press_reads;
        have_contact = true;
        ui_redraw();
    }
}

const struct ui_screen demo_diag = {
    .title = "TOUCH DIAG",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
