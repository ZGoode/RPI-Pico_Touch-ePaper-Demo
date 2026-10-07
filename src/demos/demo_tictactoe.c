#include <stdio.h>
#include <string.h>

#include "demos/demos.h"
#include "display/gfx.h"

/*
 * Tic-tac-toe, one or two players. In one-player mode you are X and move
 * first; the CPU (O) takes a win, else blocks, else centre, corner, edge.
 * Turn-based play suits the slow e-paper: nothing happens between taps.
 */

#define BX   20 /* board origin */
#define BY   48
#define CELL 60

static char board[9];
static char turn;   /* 'X' or 'O' */
static char result; /* 0 playing, 'X', 'O' or 'D' (draw) */
static bool one_player = true;
static int score_x, score_o, score_d;

static const uint8_t lines[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
                                    {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

static const struct ui_box mode_box = {236, 124, 168, 44};
static const struct ui_box new_box = {236, 180, 168, 44};

static struct ui_box cell_box(int i)
{
    return (struct ui_box){BX + (i % 3) * CELL, BY + (i / 3) * CELL, CELL, CELL};
}

static char check_result(void)
{
    for (int l = 0; l < 8; l++) {
        char a = board[lines[l][0]];

        if (a && a == board[lines[l][1]] && a == board[lines[l][2]]) {
            return a;
        }
    }
    for (int i = 0; i < 9; i++) {
        if (!board[i]) {
            return 0;
        }
    }
    return 'D';
}

static void new_game(void)
{
    memset(board, 0, sizeof(board));
    turn = 'X';
    result = 0;
}

static void enter(void)
{
    if (turn == 0) {
        new_game();
    }
}

/* A cell that completes a line for `who`, or -1. */
static int completing_cell(char who)
{
    for (int l = 0; l < 8; l++) {
        int mine = 0;
        int empty = -1;

        for (int k = 0; k < 3; k++) {
            char c = board[lines[l][k]];

            if (c == who) {
                mine++;
            } else if (c == 0) {
                empty = lines[l][k];
            }
        }
        if (mine == 2 && empty >= 0) {
            return empty;
        }
    }
    return -1;
}

static int cpu_pick(void)
{
    static const uint8_t preference[9] = {4, 0, 2, 6, 8, 1, 3, 5, 7};
    int c = completing_cell('O');

    if (c < 0) {
        c = completing_cell('X');
    }
    for (int i = 0; c < 0 && i < 9; i++) {
        if (!board[preference[i]]) {
            c = preference[i];
        }
    }
    return c;
}

static void finish_if_over(void)
{
    result = check_result();
    if (result == 'X') {
        score_x++;
    } else if (result == 'O') {
        score_o++;
    } else if (result == 'D') {
        score_d++;
    }
}

static void draw_mark(int i)
{
    struct ui_box b = cell_box(i);
    int cx = b.x + CELL / 2;
    int cy = b.y + CELL / 2;

    if (board[i] == 'X') {
        gfx_line(cx - 17, cy - 17, cx + 17, cy + 17, 5);
        gfx_line(cx - 17, cy + 17, cx + 17, cy - 17, 5);
    } else if (board[i] == 'O') {
        gfx_circle(cx, cy, 20, 5);
    }
}

static void draw(void)
{
    char buf[24];
    const char *status;

    /* Grid. */
    gfx_fill_rect(BX + CELL - 1, BY, 3, 3 * CELL);
    gfx_fill_rect(BX + 2 * CELL - 1, BY, 3, 3 * CELL);
    gfx_fill_rect(BX, BY + CELL - 1, 3 * CELL, 3);
    gfx_fill_rect(BX, BY + 2 * CELL - 1, 3 * CELL, 3);
    for (int i = 0; i < 9; i++) {
        draw_mark(i);
    }

    if (result == 'X') {
        status = one_player ? "YOU WIN!" : "X WINS!";
    } else if (result == 'O') {
        status = one_player ? "CPU WINS" : "O WINS!";
    } else if (result == 'D') {
        status = "DRAW";
    } else if (one_player) {
        status = "YOUR MOVE";
    } else {
        status = turn == 'X' ? "X MOVE" : "O MOVE";
    }
    gfx_text(230, 52, status, 3);

    snprintf(buf, sizeof(buf), "X:%d O:%d D:%d", score_x, score_o, score_d);
    gfx_text(230, 92, buf, 2);

    ui_button(&mode_box, one_player ? "1 PLAYER" : "2 PLAYERS", 2);
    ui_button(&new_box, "NEW GAME", 2);
}

static void touch(const struct touch_point *p)
{
    if (p->event != TOUCH_PRESS) {
        return;
    }

    if (ui_hit(&new_box, p->x, p->y)) {
        new_game();
        ui_redraw();
        return;
    }
    if (ui_hit(&mode_box, p->x, p->y)) {
        one_player = !one_player;
        score_x = score_o = score_d = 0;
        new_game();
        ui_redraw();
        return;
    }
    if (result) {
        return; /* game over: NEW GAME to continue */
    }

    for (int i = 0; i < 9; i++) {
        struct ui_box b = cell_box(i);

        if (!ui_hit(&b, p->x, p->y) || board[i]) {
            continue;
        }
        board[i] = turn;
        finish_if_over();
        if (!result) {
            turn = turn == 'X' ? 'O' : 'X';
            if (one_player && turn == 'O') {
                board[cpu_pick()] = 'O';
                finish_if_over();
                if (!result) {
                    turn = 'X';
                }
            }
        }
        ui_redraw();
        return;
    }
}

const struct ui_screen demo_tictactoe = {
    .title = "TIC-TAC-TOE",
    .enter = enter,
    .draw = draw,
    .touch = touch,
};
