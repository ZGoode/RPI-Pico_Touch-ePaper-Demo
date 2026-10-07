#include "display/gfx.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "board/board_config.h"
#include "display/epd.h"
#include "display/font5x7.h"

#if DISPLAY_ROTATION == 1 || DISPLAY_ROTATION == 3
#define LOGICAL_WIDTH  EPD_HEIGHT
#define LOGICAL_HEIGHT EPD_WIDTH
#elif DISPLAY_ROTATION == 0 || DISPLAY_ROTATION == 2
#define LOGICAL_WIDTH  EPD_WIDTH
#define LOGICAL_HEIGHT EPD_HEIGHT
#else
#error "DISPLAY_ROTATION must be 0, 1, 2 or 3"
#endif

static bool flipped;

void gfx_set_flip(bool flip)
{
    flipped = flip;
}

bool gfx_get_flip(void)
{
    return flipped;
}

int gfx_width(void)
{
    return LOGICAL_WIDTH;
}

int gfx_height(void)
{
    return LOGICAL_HEIGHT;
}

/* Sets one pixel black. Converts the logical position to the panel's native
 * portrait position according to DISPLAY_ROTATION; the touch layer applies
 * the inverse of this mapping (touch/touch.c). */
static void put_pixel(int lx, int ly)
{
    int x;
    int y;

    if (flipped) {
        lx = LOGICAL_WIDTH - 1 - lx;
        ly = LOGICAL_HEIGHT - 1 - ly;
    }

#if DISPLAY_ROTATION == 0
    x = lx;
    y = ly;
#elif DISPLAY_ROTATION == 1
    x = EPD_WIDTH - 1 - ly;
    y = lx;
#elif DISPLAY_ROTATION == 2
    x = EPD_WIDTH - 1 - lx;
    y = EPD_HEIGHT - 1 - ly;
#else
    x = ly;
    y = EPD_HEIGHT - 1 - lx;
#endif

    epd_framebuffer()[y * EPD_ROW_BYTES + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
}

void gfx_clear(void)
{
    memset(epd_framebuffer(), 0x00, EPD_FRAMEBUFFER_BYTES);
}

void gfx_fill_rect(int x, int y, int w, int h)
{
    int x1 = x + w;
    int y1 = y + h;

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x1 > LOGICAL_WIDTH) {
        x1 = LOGICAL_WIDTH;
    }
    if (y1 > LOGICAL_HEIGHT) {
        y1 = LOGICAL_HEIGHT;
    }

    for (int yy = y; yy < y1; yy++) {
        for (int xx = x; xx < x1; xx++) {
            put_pixel(xx, yy);
        }
    }
}

void gfx_frame(int x, int y, int w, int h, int thickness)
{
    gfx_fill_rect(x, y, w, thickness);
    gfx_fill_rect(x, y + h - thickness, w, thickness);
    gfx_fill_rect(x, y, thickness, h);
    gfx_fill_rect(x + w - thickness, y, thickness, h);
}

void gfx_line(int x0, int y0, int x1, int y1, int thickness)
{
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int half = thickness / 2;

    for (;;) {
        gfx_fill_rect(x0 - half, y0 - half, thickness, thickness);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void gfx_circle(int cx, int cy, int r, int thickness)
{
    int inner = r - thickness;

    for (int dy = -r; dy <= r; dy++) {
        int outer_x = (int)sqrtf((float)(r * r - dy * dy));

        if (inner > 0 && dy > -inner && dy < inner) {
            /* Only the two arcs at the left and right edges. */
            int inner_x = (int)sqrtf((float)(inner * inner - dy * dy));

            gfx_fill_rect(cx - outer_x, cy + dy, outer_x - inner_x, 1);
            gfx_fill_rect(cx + inner_x + 1, cy + dy, outer_x - inner_x, 1);
        } else {
            gfx_fill_rect(cx - outer_x, cy + dy, 2 * outer_x + 1, 1);
        }
    }
}

int gfx_text_width(const char *s, int scale)
{
    int n = (int)strlen(s);

    return n > 0 ? n * GFX_CELL_W * scale - scale : 0;
}

void gfx_text(int x, int y, const char *s, int scale)
{
    if (scale < 1) {
        scale = 1;
    }

    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;

        if (c < 32 || c > 126) {
            c = '?';
        }
        const uint8_t *glyph = gfx_font5x7[c - 32];

        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 8; row++) {
                if (glyph[col] & (1U << row)) {
                    gfx_fill_rect(x + col * scale, y + row * scale, scale, scale);
                }
            }
        }
        x += GFX_CELL_W * scale;
    }
}
