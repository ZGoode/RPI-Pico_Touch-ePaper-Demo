#include "touch/tp370.h"

#include <stdio.h>
#include <string.h>

#include "board/board_config.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define I2C_ADDR       0x38
#define I2C_TIMEOUT_US 5000

/* Start-up: RESET high 10 ms, low 10 ms, then 1 s before the first I2C access. */
#define RESET_PRE_MS    10
#define RESET_LOW_MS    10
#define RESET_SETTLE_MS 1000

/*
 * A report is read by writing register 0x00 and then reading nine bytes: a
 * three-byte header followed by one six-byte touch record.
 *   byte 3  bits 3:0 X[11:8]; bits 7:6 event
 *   byte 4  X[7:0]
 *   byte 5  bits 3:0 Y[11:8]; bits 7:4 touch ID, 0x0F when there is no touch
 *   byte 6  Y[7:0]
 */
#define REG_REPORT  0x00
#define REPORT_LEN  TP370_REPORT_LEN
#define OFF_XH      3
#define OFF_XL      4
#define OFF_YH      5
#define OFF_YL      6
#define ID_NO_TOUCH 0x0F

static void (*int_callback)(void);
static uint8_t last_report[REPORT_LEN];
static bool have_report;

static void gpio_irq_handler(uint gpio, uint32_t events)
{
    (void)events;
    if (gpio == PIN_TOUCH_INT && int_callback) {
        int_callback();
    }
}

int tp370_init(void)
{
    i2c_init(TOUCH_I2C, TOUCH_I2C_HZ);
    gpio_set_function(PIN_TOUCH_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_TOUCH_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_TOUCH_SDA);
    gpio_pull_up(PIN_TOUCH_SCL);

    gpio_init(PIN_TOUCH_INT);
    gpio_set_dir(PIN_TOUCH_INT, GPIO_IN);
    gpio_pull_up(PIN_TOUCH_INT);

    gpio_init(PIN_TOUCH_RESET);
    gpio_put(PIN_TOUCH_RESET, 1);
    gpio_set_dir(PIN_TOUCH_RESET, GPIO_OUT);

    sleep_ms(RESET_PRE_MS);
    gpio_put(PIN_TOUCH_RESET, 0);
    sleep_ms(RESET_LOW_MS);
    gpio_put(PIN_TOUCH_RESET, 1);
    sleep_ms(RESET_SETTLE_MS);

    /* The controller is present if it acknowledges its address. */
    uint8_t reg = REG_REPORT;
    int ret = i2c_write_timeout_us(TOUCH_I2C, I2C_ADDR, &reg, 1, false, I2C_TIMEOUT_US);

    if (ret != 1) {
        printf("tp370: no answer at I2C address 0x%02x - check SDA/SCL, pull-ups, "
               "power and RESET\n", I2C_ADDR);
        return ret < 0 ? ret : PICO_ERROR_GENERIC;
    }
    return 0;
}

bool tp370_int_asserted(void)
{
    return !gpio_get(PIN_TOUCH_INT);
}

void tp370_int_enable(void (*fn)(void))
{
    int_callback = fn;
    gpio_set_irq_enabled_with_callback(PIN_TOUCH_INT, GPIO_IRQ_EDGE_FALL, true,
                                       gpio_irq_handler);
}

int tp370_read(struct tp370_sample *sample)
{
    uint8_t reg = REG_REPORT;
    uint8_t d[REPORT_LEN];

    int ret = i2c_write_timeout_us(TOUCH_I2C, I2C_ADDR, &reg, 1, false, I2C_TIMEOUT_US);

    if (ret != 1) {
        return ret < 0 ? ret : PICO_ERROR_GENERIC;
    }
    ret = i2c_read_timeout_us(TOUCH_I2C, I2C_ADDR, d, sizeof(d), false, I2C_TIMEOUT_US);
    if (ret != (int)sizeof(d)) {
        return ret < 0 ? ret : PICO_ERROR_GENERIC;
    }

    memcpy(last_report, d, sizeof(last_report));
    have_report = true;

    /* A report without a touch ID is ignored; the release is detected from
     * INT returning to idle. */
    if ((d[OFF_YH] >> 4) >= ID_NO_TOUCH) {
        return PICO_ERROR_NO_DATA;
    }

    sample->x = (uint16_t)(((d[OFF_XH] & 0x0F) << 8) | d[OFF_XL]);
    sample->y = (uint16_t)(((d[OFF_YH] & 0x0F) << 8) | d[OFF_YL]);
    return 0;
}

int tp370_last_report(uint8_t *buf, int max)
{
    if (!have_report) {
        return 0;
    }
    int n = max < REPORT_LEN ? max : REPORT_LEN;

    memcpy(buf, last_report, n);
    return n;
}
