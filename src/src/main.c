#include <stdio.h>

#include "board/board_config.h"
#include "board/status_led.h"
#include "display/epd.h"
#include "pico/stdlib.h"
#include "touch/touch.h"
#include "ui/menu.h"
#include "ui/ui.h"

/*
 * Touch + fast-update e-paper example for the Raspberry Pi Pico.
 *
 * Hardware access is in display/ (e-paper panel) and touch/ (touch
 * controller); the screen manager and home menu are in ui/; the individual
 * demonstrations are in demos/. Pin assignments are in board/board_config.h.
 */

int main(void)
{
    stdio_init_all();
    status_led_init();

    if (epd_init() != 0) {
        for (;;) {
            printf("E-paper initialization failed\n");
            status_led_blink(2);
        }
    }

    /* Without a touch controller the menu is shown but cannot be operated. */
    if (touch_init() != 0) {
        printf("Touch controller not available\n");
        status_led_blink(3);
    }
    touch_set_display(EPD_WIDTH, EPD_HEIGHT, DISPLAY_ROTATION);

    ui_run(&ui_menu);
    return 0;
}
