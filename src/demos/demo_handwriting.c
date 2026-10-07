#include <stdio.h>
#include <string.h>

#include "demos/demos.h"
#include "demos/recog/recog.h"
#include "display/gfx.h"

/*
 * Handwriting: write ONE character (digit or capital letter) in the box, tap
 * READ, and it is recognised and appended to the text line - the same idea as
 * Scribble, with the recognizer in demos/recog.
 *
 *   box        Write here; ink appears when the finger lifts (the e-paper
 *              refresh is slow). Several strokes are fine (E, 4, A ...):
 *              finish the whole character, then READ.
 *   READ       Recognise the ink, append the best guess, clear the ink.
 *   ERASE      Clear the ink only (hold: forget everything taught, below).
 *   DEL        Delete the last character (hold: clear all text).
 *   SPACE      Append a space.
 *   mode       Header button: digits only / letters only / both. Restricting
 *              the set avoids look-alikes (0 O, 1 I, 5 S, 2 Z, 8 B).
 *   3 guesses  Under the text: the best three. TAP the right one to fix a
 *              wrong pick - and the drawing is remembered as an example of
 *              that character, so the same handwriting is read correctly
 *              next time (kept in RAM until reset).
 */

#define MAX_INK  420
#define MAX_TEXT 9

static const struct ui_box inkbox = {8, 40, 204, 192};
static const struct ui_box textbox = {220, 50, 188, 34};
static const struct ui_box read_box = {220, 152, 92, 34};
static const struct ui_box erase_box = {316, 152, 92, 34};
static const struct ui_box del_box = {220, 192, 92, 34};
static const struct ui_box space_box = {316, 192, 92, 34};

static struct ui_box cand_box(int i)
{
    return (struct ui_box){220 + i * 64, 88, 60, 34};
}

static struct recog_pt ink[MAX_INK];
static int ink_count;
static uint8_t next_id;
static bool pen_down;

static unsigned int mode = RECOG_ALL;
static char text[MAX_TEXT + 1];
static int text_len;

static struct recog_cand cand[3];
static int cand_count;
static struct recog_cloud last_cloud;
static bool have_cloud;
static bool appended; /* the last READ put its best guess on the text line */
static const char *msg;

static bool del_down;
static bool del_consumed;
static bool erase_down;
static bool erase_consumed;

static const char *mode_label(void)
{
    return mode == RECOG_DIGITS ? "123" : (mode == RECOG_LETTERS ? "ABC" : "ABC123");
}

static void enter(void)
{
    pen_down = false;
    del_down = false;
    erase_down = false;
}

static void clear_ink(void)
{
    ink_count = 0;
    next_id = 0;
    pen_down = false;
}

static void add_point(int x, int y)
{
    int x0 = inkbox.x + 3;
    int x1 = inkbox.x + inkbox.w - 4;
    int y0 = inkbox.y + 3;
    int y1 = inkbox.y + inkbox.h - 4;

    x = x < x0 ? x0 : (x > x1 ? x1 : x);
    y = y < y0 ? y0 : (y > y1 ? y1 : y);
    if (ink_count >= MAX_INK) {
        return;
    }
    ink[ink_count].x = (int16_t)x;
    ink[ink_count].y = (int16_t)y;
    ink[ink_count].id = (uint8_t)(next_id - 1);
    ink_count++;
}

static void append_char(char ch)
{
    if (text_len < MAX_TEXT) {
        text[text_len++] = ch;
        text[text_len] = '\0';
    } else {
        msg = "TEXT FULL";
    }
}

static void do_read(void)
{
    struct recog_cloud c;

    msg = NULL;
    if (ink_count < 2 || recog_normalize(ink, ink_count, &c) != 0) {
        msg = "WRITE A CHARACTER FIRST";
        clear_ink();
        ui_redraw();
        return;
    }

    cand_count = recog_classify(&c, mode, cand, 3);
    last_cloud = c;
    have_cloud = cand_count > 0;
    appended = false;
    clear_ink();

    if (cand_count > 0 && cand[0].dist <= RECOG_REJECT_DIST) {
        append_char(cand[0].ch);
        appended = (msg == NULL);
    } else {
        msg = "UNSURE - TAP A GUESS OR REDO";
    }
    ui_redraw();
}

static void pick_candidate(int i)
{
    char ch = cand[i].ch;

    if (have_cloud) {
        (void)recog_teach(ch, &last_cloud); /* this handwriting is now an example of ch */
    }
    msg = NULL;
    if (appended && text_len > 0) {
        text[text_len - 1] = ch; /* fix the wrong pick */
    } else {
        append_char(ch);
        appended = (msg == NULL);
    }
    ui_redraw();
}

static void draw_ink(void)
{
    int thick = ui_settings.pen > 3 ? 3 : ui_settings.pen;

    for (int i = 0; i < ink_count; i++) {
        if (i == 0 || ink[i].id != ink[i - 1].id) {
            gfx_line(ink[i].x, ink[i].y, ink[i].x, ink[i].y, thick); /* dot */
        } else {
            gfx_line(ink[i - 1].x, ink[i - 1].y, ink[i].x, ink[i].y, thick);
        }
    }
}

static void draw(void)
{
    char buf[24];
    struct ui_box mode_box = ui_header_box(1);

    ui_button(&mode_box, mode_label(), 2);

    /* Writing box. */
    ui_frame(&inkbox, 2);
    if (ink_count == 0) {
        struct ui_box l1 = {inkbox.x, inkbox.y + 80, inkbox.w, 12};
        struct ui_box l2 = {inkbox.x, inkbox.y + 96, inkbox.w, 12};

        ui_text_center(&l1, "WRITE ONE CHARACTER", 1);
        ui_text_center(&l2, "THEN TAP READ", 1);
    } else {
        draw_ink();
    }

    /* Text line. */
    gfx_text(220, 40, "TEXT", 1);
    snprintf(buf, sizeof(buf), "TAUGHT %d", recog_user_count());
    gfx_text(408 - gfx_text_width(buf, 1), 40, buf, 1);
    ui_frame(&textbox, 2);
    snprintf(buf, sizeof(buf), "%s%s", text, text_len < MAX_TEXT ? "_" : "");
    gfx_text(textbox.x + 6, textbox.y + 7, buf, 3);

    /* Guesses. */
    for (int i = 0; i < cand_count; i++) {
        struct ui_box b = cand_box(i);
        struct ui_box cb = {b.x, b.y + 3, b.w, 22};
        bool current = appended && text_len > 0 && text[text_len - 1] == cand[i].ch;
        char s[2] = {cand[i].ch == ' ' ? '_' : cand[i].ch, '\0'};

        ui_frame(&b, current ? 4 : 2);
        ui_text_center(&cb, s, 3);
        snprintf(buf, sizeof(buf), "%u%%", cand[i].conf);
        gfx_text(b.x + (b.w - gfx_text_width(buf, 1)) / 2, b.y + 25, buf, 1);
    }

    /* Hints and status. */
    gfx_text(220, 126, msg ? msg : "TAP A GUESS TO FIX + TEACH", 1);
    gfx_text(220, 134, "HOLD DEL: CLEAR TEXT", 1);
    gfx_text(220, 142, "HOLD ERASE: FORGET TAUGHT", 1);

    ui_button(&read_box, "READ", 2);
    ui_button(&erase_box, "ERASE", 2);
    ui_button(&del_box, "DEL", 2);
    ui_button(&space_box, "SPACE", 2);
}

static void touch(const struct touch_point *p)
{
    struct ui_box mode_box = ui_header_box(1);

    switch (p->event) {
    case TOUCH_PRESS:
        del_down = false;
        erase_down = false;

        if (ui_hit(&mode_box, p->x, p->y)) {
            mode = (mode == RECOG_DIGITS) ? RECOG_LETTERS
                                          : (mode == RECOG_LETTERS ? RECOG_ALL : RECOG_DIGITS);
            cand_count = 0; /* the guesses belonged to the old mode */
            have_cloud = false;
            ui_redraw();
        } else if (ui_hit(&inkbox, p->x, p->y)) {
            if (ink_count == 0) {
                cand_count = 0; /* a new character: the old guesses no longer apply */
                have_cloud = false;
                msg = NULL;
            }
            if (ink_count < MAX_INK) {
                next_id++;
                pen_down = true;
                add_point(p->x, p->y);
            }
        } else if (ui_hit(&read_box, p->x, p->y)) {
            do_read();
        } else if (ui_hit(&erase_box, p->x, p->y)) {
            erase_down = true;
            erase_consumed = false;
        } else if (ui_hit(&del_box, p->x, p->y)) {
            del_down = true;
            del_consumed = false;
        } else if (ui_hit(&space_box, p->x, p->y)) {
            msg = NULL;
            append_char(' ');
            appended = false;
            ui_redraw();
        } else {
            for (int i = 0; i < cand_count; i++) {
                struct ui_box b = cand_box(i);

                if (ui_hit(&b, p->x, p->y)) {
                    pick_candidate(i);
                    break;
                }
            }
        }
        break;

    case TOUCH_MOVE:
        if (pen_down) {
            add_point(p->x, p->y);
        }
        break;

    case TOUCH_HOLD:
        if (del_down && !del_consumed) {
            del_consumed = true;
            text_len = 0;
            text[0] = '\0';
            appended = false;
            msg = NULL;
            ui_redraw();
        } else if (erase_down && !erase_consumed) {
            erase_consumed = true;
            recog_user_clear();
            msg = "FORGOT ALL TAUGHT EXAMPLES";
            ui_redraw();
        }
        break;

    case TOUCH_RELEASE:
        if (pen_down) {
            pen_down = false;
            ui_redraw();
        }
        if (del_down) {
            del_down = false;
            if (!del_consumed && text_len > 0) {
                text[--text_len] = '\0';
                appended = false;
                msg = NULL;
                ui_redraw();
            }
        }
        if (erase_down) {
            erase_down = false;
            if (!erase_consumed) {
                clear_ink();
                msg = NULL;
                ui_redraw();
            }
        }
        break;
    }
}

const struct ui_screen demo_handwriting = {
    .title = "HANDWRITING",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
