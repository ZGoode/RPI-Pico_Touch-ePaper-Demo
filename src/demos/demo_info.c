#include <stdio.h>

#include "board/board_config.h"
#include "demos/demos.h"
#include "display/epd.h"
#include "display/gfx.h"
#include "hardware/i2c.h"

/*
 * Static facts about the display and touch setup plus the refresh counter
 * (handy for judging how slow the current update mode really is). Shown as of
 * the last completed refresh.
 */

#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

static void draw(void)
{
    char buf[48];
    uint32_t count;
    uint32_t last_ms;

    ui_stats(&count, &last_ms);

    snprintf(buf, sizeof(buf), "SCREEN %dx%d ORIENT %d", gfx_width(), gfx_height(),
             ui_orientation());
    gfx_text(8, 44, buf, 2);
    snprintf(buf, sizeof(buf), "NATIVE PANEL %ux%u", (unsigned)EPD_WIDTH, (unsigned)EPD_HEIGHT);
    gfx_text(8, 68, buf, 2);
    snprintf(buf, sizeof(buf), "TOUCH: %s", touch_is_ready() ? touch_name() : "NOT FOUND");
    gfx_text(8, 92, buf, 2);
    snprintf(buf, sizeof(buf), "I2C BUS: I2C%u", i2c_hw_index(TOUCH_I2C));
    gfx_text(8, 116, buf, 2);
    gfx_text(8, 144, "SDA GP" STRINGIFY(PIN_TOUCH_SDA) " SCL GP" STRINGIFY(PIN_TOUCH_SCL)
                     " INT GP" STRINGIFY(PIN_TOUCH_INT) " RST GP" STRINGIFY(PIN_TOUCH_RESET), 1);
    snprintf(buf, sizeof(buf), "REFRESHES: %u  LAST: %u MS", (unsigned)count, (unsigned)last_ms);
    gfx_text(8, 164, buf, 2);
}

const struct ui_screen demo_info = {
    .title = "SYSTEM INFO",
    .draw = draw,
};
