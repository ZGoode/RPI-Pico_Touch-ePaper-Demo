#include "display/epd.h"

#include <stdio.h>
#include <string.h>

#include "board/board_config.h"
#include "display/epd_hw.h"
#include "pico/stdlib.h"

/*
 * Temperature in degrees C reported to the controller, which selects the
 * waveform. The panel has no temperature sensor on this board, so this is a
 * fixed value; set it to the expected ambient temperature (0 - 50). Fast
 * update is only used between FAST_TEMP_MIN_C and FAST_TEMP_MAX_C.
 */
#define EPD_TEMPERATURE_C 25
#define FAST_TEMP_MIN_C   15
#define FAST_TEMP_MAX_C   30

_Static_assert(EPD_TEMPERATURE_C >= 0 && EPD_TEMPERATURE_C <= 50,
               "EPD_TEMPERATURE_C must be within 0 - 50");

/* UC8253 commands. */
#define CMD_PSR       0x00 /* panel setting; 0x0E = soft reset */
#define CMD_POWER_OFF 0x02
#define CMD_POWER_ON  0x04
#define CMD_DTM1      0x10 /* image data 1 */
#define CMD_REFRESH   0x12
#define CMD_DTM2      0x13 /* image data 2 */
#define CMD_CDI       0x50 /* VCOM and data interval */
#define CMD_TEMP_IN   0xE5 /* temperature input */
#define CMD_TEMP_ACT  0xE0 /* temperature activate */
#define CMD_OTP_READ  0xA2

#define PSR_SOFT_RESET 0x0E

/* Panel setting used when the OTP cannot be read, and the bits that select
 * the fast waveform. */
#define PSR0_DEFAULT  0x0F
#define PSR1_DEFAULT  0x89
#define PSR0_FAST_BIT 0x10
#define PSR1_FAST_BIT 0x02
#define TEMP_FAST_BIT 0x40

/* The factory panel setting is stored twice in OTP, at the same offset in two
 * banks. A bank is valid when its first byte is the marker. */
#define OTP_SPI_HZ       500000u
#define OTP_MARKER       0xA5
#define OTP_PSR_BANK0    0x0FB4
#define OTP_PSR_BANK1    0x1FB4
#define OTP_MARKER_BANK1 0x1000
#define OTP_COMMAND_GAP_MS 10

#define BUSY_IDLE_TIMEOUT_MS    3000
#define BUSY_REFRESH_TIMEOUT_MS 20000

static uint8_t framebuffer[EPD_FRAMEBUFFER_BYTES];

/* The image currently on the glass. Fast updates are computed from it. */
static uint8_t previous_plane[EPD_FRAMEBUFFER_BYTES];

static uint8_t panel_psr[2] = {PSR0_DEFAULT, PSR1_DEFAULT};

/* False until a refresh has completed, so the first update is global. */
static bool glass_known;

static void command1(uint8_t cmd, uint8_t value)
{
    epd_hw_command(cmd, &value, 1);
}

static bool temperature_allows_fast(void)
{
    return EPD_TEMPERATURE_C >= FAST_TEMP_MIN_C && EPD_TEMPERATURE_C <= FAST_TEMP_MAX_C;
}

/* ---- Factory settings -------------------------------------------------- */

static int read_otp_psr(uint8_t psr[2])
{
    uint8_t b;

    epd_hw_command(CMD_OTP_READ, NULL, 0);
    sleep_ms(OTP_COMMAND_GAP_MS);

    epd_hw_read_byte(&b); /* dummy byte */
    epd_hw_read_byte(&b); /* first OTP byte: marker of bank 0 if valid */

    bool bank1 = (b != OTP_MARKER);
    uint16_t psr_offset = bank1 ? OTP_PSR_BANK1 : OTP_PSR_BANK0;
    uint16_t next = 1; /* offset of the next byte to be read */

    if (bank1) {
        for (; next < OTP_MARKER_BANK1; next++) {
            epd_hw_read_byte(&b);
        }
        epd_hw_read_byte(&b);
        next++;
        if (b != OTP_MARKER) {
            return PICO_ERROR_GENERIC;
        }
    }

    for (; next < psr_offset; next++) {
        epd_hw_read_byte(&b);
    }
    epd_hw_read_byte(&psr[0]);
    epd_hw_read_byte(&psr[1]);
    return 0;
}

/* ---- Update sequence --------------------------------------------------- */

/* Soft-resets the controller and loads the temperature and panel settings. */
static int start_update(bool fast)
{
    uint8_t temperature = EPD_TEMPERATURE_C;
    uint8_t psr[2] = {panel_psr[0], panel_psr[1]};

    if (fast) {
        temperature |= TEMP_FAST_BIT;
        psr[0] |= PSR0_FAST_BIT;
        psr[1] |= PSR1_FAST_BIT;
    }

    command1(CMD_PSR, PSR_SOFT_RESET);
    int ret = epd_hw_wait_idle(BUSY_IDLE_TIMEOUT_MS);

    if (ret) {
        return ret;
    }
    command1(CMD_TEMP_IN, temperature);
    command1(CMD_TEMP_ACT, 0x02);
    epd_hw_command(CMD_PSR, psr, sizeof(psr));
    if (fast) {
        command1(CMD_CDI, 0x07);
    }
    return 0;
}

static void send_image(bool fast)
{
    if (fast) {
        /* DTM1 holds the image that is on the glass, DTM2 the new one; the
         * controller derives the waveform from the difference. */
        command1(CMD_CDI, 0x27);
        epd_hw_command(CMD_DTM1, previous_plane, EPD_FRAMEBUFFER_BYTES);
        epd_hw_command(CMD_DTM2, framebuffer, EPD_FRAMEBUFFER_BYTES);
        command1(CMD_CDI, 0x07);
    } else {
        /* Global update: DTM1 holds the new image, DTM2 is cleared. */
        epd_hw_command(CMD_DTM1, framebuffer, EPD_FRAMEBUFFER_BYTES);
        epd_hw_command_fill(CMD_DTM2, 0x00, EPD_FRAMEBUFFER_BYTES);
    }
}

static int run_waveform(void)
{
    int ret = epd_hw_wait_idle(BUSY_IDLE_TIMEOUT_MS);

    if (ret) {
        return ret;
    }
    epd_hw_command(CMD_POWER_ON, NULL, 0);
    ret = epd_hw_wait_idle(BUSY_IDLE_TIMEOUT_MS);
    if (ret) {
        return ret;
    }
    epd_hw_command(CMD_REFRESH, NULL, 0);
    ret = epd_hw_wait_idle(BUSY_REFRESH_TIMEOUT_MS);
    if (ret) {
        return ret;
    }
    epd_hw_command(CMD_POWER_OFF, NULL, 0);
    return epd_hw_wait_idle(BUSY_IDLE_TIMEOUT_MS);
}

/* ---- Public API -------------------------------------------------------- */

int epd_init(void)
{
    epd_hw_init();
    epd_hw_reset();

    /* The OTP is read at a reduced clock. */
    epd_hw_set_clock(OTP_SPI_HZ);
    int ret = read_otp_psr(panel_psr);
    epd_hw_set_clock(EPD_SPI_HZ);

    if (ret) {
        panel_psr[0] = PSR0_DEFAULT;
        panel_psr[1] = PSR1_DEFAULT;
        printf("epd: OTP panel setting not found, using defaults\n");
    }

    /* Reset again so the controller leaves OTP read mode. */
    epd_hw_reset();

    glass_known = false;
    return 0;
}

uint8_t *epd_framebuffer(void)
{
    return framebuffer;
}

int epd_refresh(enum epd_update_mode mode)
{
    bool fast = (mode == EPD_UPDATE_FAST) && glass_known && temperature_allows_fast();

    int ret = start_update(fast);

    if (ret == 0) {
        send_image(fast);
        ret = run_waveform();
    }
    if (ret) {
        printf("epd: refresh failed (%d)\n", ret);
        glass_known = false; /* the next update is global */
        return ret;
    }

    memcpy(previous_plane, framebuffer, sizeof(previous_plane));
    glass_known = true;
    return 0;
}

void epd_set_busy_hook(void (*hook)(void))
{
    epd_hw_set_busy_hook(hook);
}
