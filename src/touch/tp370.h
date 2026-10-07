#ifndef TOUCH_TP370_H_
#define TOUCH_TP370_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * Controller driver for the Pervasive Displays TP370PGH01 capacitive touch
 * overlay for the 3.70" panel. It reports a single touch point in the
 * panel's native portrait coordinates, with the origin at the top-left corner
 * of the native (rotation 0) picture. touch/touch.c turns these samples into
 * touch events in the display's rotated coordinates.
 *
 * Interface: I2C at address 0x38, an active-low INT line and an active-low
 * RESET line (see board/board_config.h). INT is asserted while the controller
 * has a report to deliver. The controller must only be read while INT is
 * asserted.
 */

/* Highest raw coordinates the controller reports. */
#define TP370_RAW_X_MAX 239
#define TP370_RAW_Y_MAX 415

/* Controller name. */
#define TP370_NAME "TP370PGH01"

/* Length of a controller report in bytes. */
#define TP370_REPORT_LEN 9

struct tp370_sample {
    uint16_t x;
    uint16_t y;
};

/*
 * Configures I2C, INT and RESET, resets the controller and checks that it
 * answers on the bus. Returns 0 or a negative PICO_ERROR_* code.
 */
int tp370_init(void);

/* True while INT is asserted. */
bool tp370_int_asserted(void);

/*
 * Calls `fn` from the GPIO interrupt handler each time INT becomes asserted.
 * `fn` runs in interrupt context and must be short.
 */
void tp370_int_enable(void (*fn)(void));

/*
 * Reads the current report. Returns 0 and fills `sample` if a finger is
 * present, PICO_ERROR_NO_DATA if the report contains no usable touch, or
 * another negative PICO_ERROR_* code on an I2C error.
 */
int tp370_read(struct tp370_sample *sample);

/*
 * Copies the bytes of the most recent report into `buf` (at most `max`).
 * Returns the number of bytes copied, 0 if nothing has been read yet.
 */
int tp370_last_report(uint8_t *buf, int max);

#endif /* TOUCH_TP370_H_ */
