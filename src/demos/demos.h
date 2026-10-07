#ifndef DEMOS_DEMOS_H_
#define DEMOS_DEMOS_H_

#include "ui/ui.h"

/*
 * The demo screens. Each is a struct ui_screen (see ui/ui.h) defined in the
 * file of the same name and listed on the home screen in ui/menu.c.
 */

/* Input */

/* Live touch coordinates, event names and corner targets: checks that the
 * display and touch orientation agree. */
extern const struct ui_screen demo_touch_test;

/* Number keypad built from touch regions. */
extern const struct ui_screen demo_keypad;

/* On-screen QWERTY keyboard. */
extern const struct ui_screen demo_keyboard;

/* Free-hand drawing with a finger. */
extern const struct ui_screen demo_scribble;

/* Tap, double tap, hold and swipe recognition. */
extern const struct ui_screen demo_gestures;

/* Scrolling list with selection and a confirm dialog. */
extern const struct ui_screen demo_list;

/* Applications */

/* Tic-tac-toe for one or two players. */
extern const struct ui_screen demo_tictactoe;

/* Handwriting recognition (library in demos/recog). */
extern const struct ui_screen demo_handwriting;

/* Flip, pen, update mode, touch timing and panel clean. */
extern const struct ui_screen demo_settings;

/* Idle with the touch interrupt armed, touch to wake. */
extern const struct ui_screen demo_sleep;

/* Display and touch facts and the refresh counter. */
extern const struct ui_screen demo_info;

/* Diagnostics */

/* INT line, controller counters, raw report and I2C scan. */
extern const struct ui_screen demo_diag;

/* Nine tap targets with the measured error of each tap. */
extern const struct ui_screen demo_touch_accuracy;

/* Timed full refreshes in the selected update mode. */
extern const struct ui_screen demo_bench;

#endif /* DEMOS_DEMOS_H_ */
