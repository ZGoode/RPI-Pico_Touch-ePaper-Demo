#ifndef DISPLAY_EPD_HW_H_
#define DISPLAY_EPD_HW_H_

#include <stddef.h>
#include <stdint.h>

/*
 * Pin and bus access for the e-paper panel. This layer knows the SPI framing
 * and the RESET and BUSY lines but nothing about the controller's commands;
 * those live in epd.c. Pin numbers come from board/board_config.h.
 *
 * Every command is sent inside one chip-select assertion: the command byte
 * with DC low followed by its data bytes with DC high. A full image plane
 * (12480 bytes, about 13 ms at 8 MHz) is sent as a single blocking transfer,
 * so no DMA is needed.
 */

/* Configures the SPI bus and the CS, DC, RESET and BUSY pins. */
void epd_hw_init(void);

/* Sends `cmd`, then `len` bytes from `data` (len may be 0). */
void epd_hw_command(uint8_t cmd, const uint8_t *data, size_t len);

/* Sends `cmd`, then `len` copies of `value`. */
void epd_hw_command_fill(uint8_t cmd, uint8_t value, size_t len);

/* Changes the SPI clock. */
void epd_hw_set_clock(uint32_t hz);

/*
 * Reads one byte from the controller in its own chip-select frame (DC high).
 * The data line is shared with MOSI, so the MOSI output is released while the
 * byte is clocked in and restored afterwards.
 */
void epd_hw_read_byte(uint8_t *value);

/* Hardware reset pulse with the timing of Pervasive's COG start-up flow. */
void epd_hw_reset(void);

/*
 * Waits until BUSY goes high (idle). Returns 0, or PICO_ERROR_TIMEOUT after
 * `timeout_ms`. A short settle time is waited first because BUSY can still
 * read idle for a moment after a command.
 */
int epd_hw_wait_idle(uint32_t timeout_ms);

/*
 * Function called about once per millisecond while waiting for BUSY, so the
 * application can keep servicing touch input during a refresh. The hook must
 * not draw or start another display operation. NULL disables it.
 */
void epd_hw_set_busy_hook(void (*hook)(void));

#endif /* DISPLAY_EPD_HW_H_ */
