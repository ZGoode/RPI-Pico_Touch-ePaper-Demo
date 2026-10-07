#include "display/epd_hw.h"

#include "board/board_config.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

/* BUSY_N can still read high for a moment after a command is accepted. */
#define BUSY_SETTLE_MS 5

static void (*busy_hook)(void);

static void output_pin(uint pin, bool initial)
{
    gpio_init(pin);
    gpio_put(pin, initial); /* set the level before enabling the driver */
    gpio_set_dir(pin, GPIO_OUT);
}

void epd_hw_init(void)
{
    spi_init(EPD_SPI, EPD_SPI_HZ);
    spi_set_format(EPD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_EPD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_EPD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_EPD_MISO, GPIO_FUNC_SPI);

    output_pin(PIN_EPD_CS, true);
    output_pin(PIN_EPD_DC, false);
    output_pin(PIN_EPD_RESET, true);

    gpio_init(PIN_EPD_BUSY);
    gpio_set_dir(PIN_EPD_BUSY, GPIO_IN);
    gpio_disable_pulls(PIN_EPD_BUSY);
}

void epd_hw_command(uint8_t cmd, const uint8_t *data, size_t len)
{
    gpio_put(PIN_EPD_DC, 0);
    gpio_put(PIN_EPD_CS, 0);
    spi_write_blocking(EPD_SPI, &cmd, 1);
    if (len != 0) {
        gpio_put(PIN_EPD_DC, 1);
        spi_write_blocking(EPD_SPI, data, len);
    }
    gpio_put(PIN_EPD_CS, 1);
}

void epd_hw_command_fill(uint8_t cmd, uint8_t value, size_t len)
{
    uint8_t chunk[64];

    for (size_t i = 0; i < sizeof(chunk); i++) {
        chunk[i] = value;
    }

    gpio_put(PIN_EPD_DC, 0);
    gpio_put(PIN_EPD_CS, 0);
    spi_write_blocking(EPD_SPI, &cmd, 1);
    gpio_put(PIN_EPD_DC, 1);
    while (len > 0) {
        size_t n = len < sizeof(chunk) ? len : sizeof(chunk);

        spi_write_blocking(EPD_SPI, chunk, n);
        len -= n;
    }
    gpio_put(PIN_EPD_CS, 1);
}

void epd_hw_set_clock(uint32_t hz)
{
    spi_set_baudrate(EPD_SPI, hz);
}

void epd_hw_read_byte(uint8_t *value)
{
    gpio_put(PIN_EPD_DC, 1);
    gpio_put(PIN_EPD_CS, 0);

    /* Take MOSI off the shared SDI line. The SPI block keeps generating the
     * clock and samples MISO, which is wired to the same line. */
    gpio_set_function(PIN_EPD_MOSI, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_EPD_MOSI, GPIO_IN);
    spi_read_blocking(EPD_SPI, 0x00, value, 1);
    gpio_set_function(PIN_EPD_MOSI, GPIO_FUNC_SPI);

    gpio_put(PIN_EPD_CS, 1);
}

void epd_hw_reset(void)
{
    /* RESET high 5 ms, low 10 ms, high 10 ms before the first command. */
    sleep_ms(5);
    gpio_put(PIN_EPD_RESET, 1);
    sleep_ms(5);
    gpio_put(PIN_EPD_RESET, 0);
    sleep_ms(10);
    gpio_put(PIN_EPD_RESET, 1);
    sleep_ms(10);
}

int epd_hw_wait_idle(uint32_t timeout_ms)
{
    sleep_ms(BUSY_SETTLE_MS);

    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

    while (!gpio_get(PIN_EPD_BUSY)) {
        if (time_reached(deadline)) {
            return PICO_ERROR_TIMEOUT;
        }
        if (busy_hook) {
            busy_hook();
        }
        sleep_ms(1);
    }
    return 0;
}

void epd_hw_set_busy_hook(void (*hook)(void))
{
    busy_hook = hook;
}
