#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

/*
 * Hardware configuration for the touch + fast-update e-paper example.
 *
 * Every board-specific setting lives in this file: pin assignments, the SPI
 * and I2C instances, bus speeds and the display rotation. To run the example
 * with different wiring or on another Pico-based board, change the values
 * here; no other file refers to a pin number.
 *
 * Hardware
 *   Display  Pervasive Displays E2370PS0C1: 3.70" 240 x 416 black/white
 *            e-paper, UC8253c driver, embedded fast update.
 *   Touch    Pervasive Displays TP370PGH01 capacitive overlay: I2C
 *            controller at address 0x38, active-low INT and RESET.
 *
 * All signals are 3.3 V.
 */

#include "hardware/i2c.h"
#include "hardware/spi.h"

/* ---- E-paper: SPI0 plus four control lines ---------------------------- */

#define EPD_SPI       spi0
#define EPD_SPI_HZ    8000000u /* Pervasive's standard clock for this COG */

#define PIN_EPD_SCK   18 /* must be an SPI0 SCK pin */
#define PIN_EPD_MOSI  19 /* must be an SPI0 TX pin; panel SDI */
#define PIN_EPD_MISO  16 /* must be an SPI0 RX pin; wired to the same SDI line */
#define PIN_EPD_CS    17 /* driven as a plain GPIO, active low */
#define PIN_EPD_DC    20 /* low = command byte, high = data */
#define PIN_EPD_RESET 21 /* panel RST_N, active low */
#define PIN_EPD_BUSY  22 /* panel BUSY_N, low while the controller is busy */

/*
 * The panel has a single bidirectional data line (SDI). MOSI and MISO are
 * both connected to it: MOSI drives commands and image data, and MISO reads
 * the factory panel settings from the controller's OTP memory at start-up.
 */

/* ---- Touch: I2C0 plus INT and RESET ----------------------------------- */

#define TOUCH_I2C       i2c0
#define TOUCH_I2C_HZ    100000u

#define PIN_TOUCH_SDA   4 /* must be an I2C0 SDA pin */
#define PIN_TOUCH_SCL   5 /* must be an I2C0 SCL pin */
#define PIN_TOUCH_INT   6 /* active low: the controller has a report to read */
#define PIN_TOUCH_RESET 7 /* active low */

/* ---- Diagnostic LED --------------------------------------------------- */

/* The board's own LED (see board/status_led.h). Not defined on boards that
 * have no GPIO-driven LED, such as the Pico W and Pico 2 W. */
#ifdef PICO_DEFAULT_LED_PIN
#define PIN_STATUS_LED PICO_DEFAULT_LED_PIN
#endif

/* ---- Display orientation ---------------------------------------------- */

/*
 * Rotation applied to the panel's native portrait framebuffer (240 x 416),
 * shared by drawing and touch so both use the same logical coordinates:
 *   0  native portrait
 *   1  rotated 90 degrees: landscape, 416 x 240
 *   2  rotated 180 degrees
 *   3  rotated 270 degrees: landscape, 416 x 240, upside down relative to 1
 *
 * The demo screens are laid out for landscape, so use 1 or 3. If the picture
 * and touch are upside down, switch between the two.
 */
#define DISPLAY_ROTATION 1

#endif /* BOARD_CONFIG_H_ */
