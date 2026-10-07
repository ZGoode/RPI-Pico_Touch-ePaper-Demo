#include "board/status_led.h"

#include "board/board_config.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define BLINK_ON_MS  150
#define BLINK_OFF_MS 150
#define BLINK_GAP_MS 600

void status_led_init(void)
{
#ifdef PIN_STATUS_LED
    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
    gpio_put(PIN_STATUS_LED, 0);
#endif
}

void status_led_set(bool on)
{
#ifdef PIN_STATUS_LED
    gpio_put(PIN_STATUS_LED, on);
#else
    (void)on;
#endif
}

void status_led_blink(int count)
{
    for (int i = 0; i < count; i++) {
        status_led_set(true);
        sleep_ms(BLINK_ON_MS);
        status_led_set(false);
        sleep_ms(BLINK_OFF_MS);
    }
    sleep_ms(BLINK_GAP_MS);
}
