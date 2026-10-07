#ifndef DISPLAY_EPD_H_
#define DISPLAY_EPD_H_

#include <stdint.h>

/*
 * Driver for the Pervasive Displays E2370PS0C1 e-paper panel (UC8253c
 * controller, 240 x 416 pixels, black/white).
 *
 * The application draws into a framebuffer in RAM and then calls
 * epd_refresh() to show it. The framebuffer uses the panel's native portrait
 * orientation: one bit per pixel, 1 = black, row-major, most significant bit
 * first, EPD_ROW_BYTES bytes per row. Draw through display/gfx.h rather than
 * writing the buffer directly; it applies DISPLAY_ROTATION.
 *
 * Update modes
 *   EPD_UPDATE_FAST    The controller is given the previous and the new image
 *                      and runs the panel's fast waveform. It is the quickest
 *                      mode and has little flashing, but it leaves faint
 *                      ghosting that builds up over repeated updates.
 *   EPD_UPDATE_GLOBAL  Full waveform that flashes the whole panel and clears
 *                      ghosting. Slower.
 *
 * Both modes update the entire screen; the controller is not given a partial
 * window. A global update is used instead of a fast one when the content of
 * the glass is unknown (the first refresh after power-up, or after a failed
 * refresh) or when the configured temperature is outside the range in which
 * fast update is specified (15 - 30 C).
 *
 * epd_refresh() blocks for the duration of the update (about a second or
 * more); see epd_set_busy_hook() for keeping other work alive meanwhile.
 */

#define EPD_WIDTH             240
#define EPD_HEIGHT            416
#define EPD_ROW_BYTES         (EPD_WIDTH / 8)
#define EPD_FRAMEBUFFER_BYTES (EPD_ROW_BYTES * EPD_HEIGHT)

enum epd_update_mode {
    EPD_UPDATE_FAST,
    EPD_UPDATE_GLOBAL,
};

/*
 * Initializes the bus, resets the panel and reads its factory settings from
 * the controller's OTP memory. Returns 0 or a negative PICO_ERROR_* code.
 * A failed OTP read is not fatal; built-in default settings are used.
 */
int epd_init(void);

/* The framebuffer that epd_refresh() sends to the panel. */
uint8_t *epd_framebuffer(void);

/* Shows the framebuffer. Returns 0 or a negative PICO_ERROR_* code. */
int epd_refresh(enum epd_update_mode mode);

/*
 * Registers a function that is called about once per millisecond while a
 * refresh is waiting for the panel. The hook can poll touch input but must
 * not draw or call back into this driver. NULL removes it.
 */
void epd_set_busy_hook(void (*hook)(void));

#endif /* DISPLAY_EPD_H_ */
