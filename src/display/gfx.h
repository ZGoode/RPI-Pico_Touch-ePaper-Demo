#ifndef DISPLAY_GFX_H_
#define DISPLAY_GFX_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * Drawing primitives for the e-paper framebuffer.
 *
 * Coordinates are logical: the origin is the top-left corner of the picture
 * as the user sees it, after DISPLAY_ROTATION (board/board_config.h) has been
 * applied. Everything is black on a white background and is clipped to the
 * screen. Nothing reaches the panel until epd_refresh() is called.
 *
 * Text uses a built-in 5 x 7 pixel font (printable ASCII). Each character
 * occupies GFX_CELL_W x GFX_CELL_H pixels at scale 1 including spacing; the
 * glyph itself is GFX_GLYPH_H pixels high.
 */

#define GFX_CELL_W  6
#define GFX_CELL_H  8
#define GFX_GLYPH_H 7

/* Logical screen size. */
int gfx_width(void);
int gfx_height(void);

/*
 * Rotates the picture a further 180 degrees (on top of DISPLAY_ROTATION) for
 * everything drawn afterwards. The picture only changes on the panel at the
 * next refresh, so the touch layer must follow it once that refresh is done
 * (see ui/ui.c).
 */
void gfx_set_flip(bool flip);
bool gfx_get_flip(void);

/* Fills the whole screen with white. */
void gfx_clear(void);

/* Filled black rectangle. */
void gfx_fill_rect(int x, int y, int w, int h);

/* Rectangle outline of the given thickness, drawn inside the rectangle. */
void gfx_frame(int x, int y, int w, int h, int thickness);

/* Line of the given thickness. */
void gfx_line(int x0, int y0, int x1, int y1, int thickness);

/* Circle outline of the given thickness, drawn inside the radius. */
void gfx_circle(int cx, int cy, int r, int thickness);

/* Text with its top-left corner at (x, y), magnified by `scale`. */
void gfx_text(int x, int y, const char *s, int scale);

/* Width in pixels of `s` drawn at `scale`. */
int gfx_text_width(const char *s, int scale);

#endif /* DISPLAY_GFX_H_ */
