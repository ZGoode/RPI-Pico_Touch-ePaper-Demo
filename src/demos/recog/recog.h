#ifndef RECOG_RECOG_H_
#define RECOG_RECOG_H_

/*
 * Tiny handwriting recognizer: digits 0-9 and capital letters A-Z.
 *
 * A point-cloud template matcher in the "$P" family (Vatavu, Anthony and
 * Wobbrock 2012): a drawing is resampled to RECOG_N points, scaled to a unit
 * box and centred; it is compared with stored templates by greedy
 * nearest-point matching. Stroke ORDER, stroke DIRECTION and where one stroke
 * ends and the next begins do not matter, so multi-stroke characters (E, 4, A)
 * work. The shape is NOT rotation invariant (a 6 is not a 9).
 *
 * Integer-only, using only the C standard library.
 *
 * Built-in templates are hand-authored skeletons (gen_templates.py). The
 * user can add templates at run time with recog_teach() (RAM only, lost at
 * reboot); they compete with the built-in ones and are the fix when a
 * particular handwriting is misread.
 */

#include <stdint.h>

#define RECOG_N 32 /* points per normalized shape */

/* One input sample. Any coordinate system (screen dots are fine); y may point
 * down. `id` numbers the strokes (0, 1, 2 ...): points with different ids are
 * never joined. */
struct recog_pt {
	int16_t x;
	int16_t y;
	uint8_t id;
};

/* A normalized shape: centred, unit box scaled to about +-500. */
struct recog_cloud {
	int16_t x[RECOG_N];
	int16_t y[RECOG_N];
};

/* Which character classes may be returned. */
#define RECOG_DIGITS  1u
#define RECOG_LETTERS 2u
#define RECOG_ALL     (RECOG_DIGITS | RECOG_LETTERS)

struct recog_cand {
	char ch;
	uint16_t dist; /* match distance in 1/1000 of the unit box; lower = better */
	uint8_t conf;  /* 0..100, a rough display figure derived from dist */
};

/* Normalize a drawing. Returns 0, or -1 if there is too little to go on (fewer
 * than 2 points or essentially no extent - a tap). */
int recog_normalize(const struct recog_pt *pts, int n, struct recog_cloud *out);

/* Rank the characters allowed by `mode` against `c`. Fills `out` with up to
 * `max` DISTINCT characters, best first; returns how many. Returns 0 only if
 * `mode` allows nothing.
 */
int recog_classify(const struct recog_cloud *c, unsigned int mode, struct recog_cand *out,
		   int max);

/* A best match worse than this distance is probably not a character at all. */
#define RECOG_REJECT_DIST 1500

/* Remember `c` as one more example of `ch` (digit or capital letter). Returns
 * the number of user templates now held. Oldest are dropped past RECOG_MAX_USER.
 */
#define RECOG_MAX_USER 64
int recog_teach(char ch, const struct recog_cloud *c);
int recog_user_count(void);
void recog_user_clear(void);

#endif /* RECOG_RECOG_H_ */
