#ifndef BOARD_STATUS_LED_H_
#define BOARD_STATUS_LED_H_

#include <stdbool.h>

/*
 * Diagnostic use of the Pico's on-board LED.
 *
 *   on while the panel is touched   the touch controller, its INT line and the
 *                                   touch events are working
 *   2 blinks, repeating             the e-paper panel failed to initialize
 *   3 blinks, once at start-up      the touch controller did not answer
 *   5 blinks after a screen change  a display refresh failed or timed out
 *                                   (check BUSY, RESET and the SPI wiring)
 *
 * The LED pin is the board's default LED (PICO_DEFAULT_LED_PIN). On boards
 * without one, including the Pico W and Pico 2 W whose LED is attached to
 * the wireless chip, these functions do nothing.
 */

void status_led_init(void);

void status_led_set(bool on);

/* Blinks `count` times, then pauses. Blocks for about a second. */
void status_led_blink(int count);

#endif /* BOARD_STATUS_LED_H_ */
