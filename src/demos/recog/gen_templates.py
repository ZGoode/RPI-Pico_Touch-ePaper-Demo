#!/usr/bin/env python3
"""
Generates recog_templates.c: the built-in handwriting templates.

Each character is one or more "skeletons" - a list of strokes, each stroke a
polyline in a 0..100 box (x right, y DOWN). The recognizer is stroke-order and
direction independent, so only the shape matters. Add or tweak a skeleton here,
re-run this script, rebuild.

    python3 gen_templates.py            # writes recog_templates.c next to this script
    python3 gen_templates.py --preview  # also writes templates_preview.png
"""
import math
import os
import sys


def arc(cx, cy, rx, ry, a0, a1, n=14):
    """Points on an ellipse arc. Angles in degrees, 0 = right, 90 = DOWN (y is
    down), so increasing angle runs clockwise on screen."""
    pts = []
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        pts.append((cx + rx * math.cos(a), cy + ry * math.sin(a)))
    return pts


def line(*p):
    return list(p)


# --------------------------------------------------------------------------
# kind: 'D' digit, 'L' letter, 'B' both (same shape is right for either)
# --------------------------------------------------------------------------
T = []  # (char, kind, [strokes])


def add(ch, kind, *strokes):
    T.append((ch, kind, [list(s) for s in strokes]))


# ---- digits ----
add('0', 'D', arc(50, 50, 30, 48, -90, 270, 20))
add('1', 'D', line((50, 0), (50, 100)))
add('1', 'D', line((24, 24), (50, 0), (50, 100)))
add('1', 'D', line((24, 24), (50, 0), (50, 100)), line((26, 100), (74, 100)))
add('2', 'D', arc(50, 28, 30, 28, 190, 400, 14) + [(22, 100), (82, 100)])
add('2', 'D', arc(50, 28, 30, 28, 190, 400, 14) + [(22, 100)], line((22, 100), (82, 100)))
add('3', 'D', arc(48, 26, 27, 25, 210, 450, 14) + arc(48, 75, 30, 27, 270, 510, 14))
add('4', 'D', line((68, 0), (10, 68), (90, 68)), line((68, 22), (68, 100)))
add('4', 'D', line((22, 0), (16, 62), (86, 62)), line((70, 0), (70, 100)))
add('5', 'D', line((76, 0), (28, 0), (25, 42)) + arc(45, 68, 32, 30, -120, 150, 14))
add('5', 'D', line((28, 0), (25, 42)) + arc(45, 68, 32, 30, -120, 150, 14), line((28, 0), (76, 0)))
add('6', 'D', line((72, 3), (45, 18), (25, 45), (20, 68)) + arc(50, 68, 30, 30, 180, 540, 18))
add('7', 'D', line((10, 0), (90, 0), (40, 100)))
add('7', 'D', line((10, 0), (90, 0), (40, 100)), line((30, 52), (72, 52)))
add('8', 'D', arc(50, 26, 24, 24, 90, 450, 16), arc(50, 74, 27, 26, 270, 630, 16))
add('8', 'D', arc(50, 26, 24, 24, -30, -270, 14) + arc(50, 74, 27, 26, 270, 640, 16))
add('9', 'D', arc(50, 30, 28, 28, 0, 360, 16), line((78, 30), (74, 68), (52, 96), (26, 92)))
add('9', 'D', arc(50, 30, 28, 28, 0, 360, 16), line((78, 30), (78, 100)))

# ---- letters (upper case) ----
add('A', 'L', line((5, 100), (50, 0), (95, 100)), line((25, 66), (75, 66)))
add('B', 'L', line((15, 0), (15, 100)),
    [(15, 0), (50, 0)] + arc(50, 25, 33, 25, -90, 90, 10) + [(15, 50)],
    [(15, 50), (52, 50)] + arc(52, 75, 36, 25, -90, 90, 10) + [(15, 100)])
add('C', 'L', arc(55, 50, 42, 50, 40, 320, 18))
add('D', 'L', line((15, 0), (15, 100)),
    [(15, 0), (35, 0)] + arc(35, 50, 55, 50, -90, 90, 12) + [(15, 100)])
add('E', 'L', line((85, 0), (15, 0), (15, 100), (85, 100)), line((15, 50), (70, 50)))
add('F', 'L', line((85, 0), (15, 0), (15, 100)), line((15, 48), (66, 48)))
add('G', 'L', arc(52, 50, 45, 50, 325, 20, 18) + [(95, 52), (58, 52)])
add('H', 'L', line((15, 0), (15, 100)), line((85, 0), (85, 100)), line((15, 50), (85, 50)))
add('I', 'L', line((50, 0), (50, 100)))
add('I', 'L', line((30, 0), (70, 0)), line((50, 0), (50, 100)), line((30, 100), (70, 100)))
add('J', 'L', [(66, 0), (66, 72)] + arc(41, 72, 25, 26, 0, 160, 8))
add('K', 'L', line((15, 0), (15, 100)), line((80, 0), (15, 56)), line((38, 40), (86, 100)))
add('K', 'L', line((15, 0), (15, 100)), line((80, 0), (15, 56), (86, 100)))
add('L', 'L', line((15, 0), (15, 100), (85, 100)))
add('M', 'L', line((10, 100), (10, 0), (50, 62), (90, 0), (90, 100)))
add('N', 'L', line((15, 100), (15, 0), (85, 100), (85, 0)))
add('O', 'L', arc(50, 50, 46, 50, -90, 270, 22))
add('P', 'L', line((15, 0), (15, 100)),
    [(15, 0), (50, 0)] + arc(50, 27, 36, 27, -90, 90, 10) + [(15, 54)])
add('Q', 'L', arc(48, 46, 42, 46, -90, 270, 20), line((58, 62), (92, 100)))
add('R', 'L', line((15, 0), (15, 100)),
    [(15, 0), (50, 0)] + arc(50, 27, 36, 27, -90, 90, 10) + [(15, 54)],
    line((46, 54), (86, 100)))
add('S', 'L', arc(50, 26, 30, 25, -30, -270, 12) + arc(50, 75, 33, 25, -90, 150, 12))
add('T', 'L', line((8, 0), (92, 0)), line((50, 0), (50, 100)))
add('U', 'L', [(15, 0), (15, 68)] + arc(50, 68, 35, 32, 180, 0, 12) + [(85, 0)])
add('V', 'L', line((8, 0), (50, 100), (92, 0)))
add('W', 'L', line((4, 0), (26, 100), (50, 38), (74, 100), (96, 0)))
add('X', 'L', line((10, 0), (90, 100)), line((90, 0), (10, 100)))
add('Y', 'L', line((10, 0), (50, 50), (90, 0)), line((50, 50), (50, 100)))
add('Z', 'L', line((10, 0), (90, 0), (10, 100), (90, 100)))


def q(v):
    return max(0, min(255, int(round(v))))


def emit(path):
    out = []
    out.append("/* GENERATED by gen_templates.py - edit the script, not this file. */\n")
    out.append('#include "demos/recog/recog.h"\n\n')
    out.append("/* Pool layout, per template: ch, kind ('D' digit / 'L' letter), stroke count,\n")
    out.append(" * then per stroke: point count, then x, y byte pairs (0..100 box, y down). */\n")
    out.append("const uint8_t recog_pool[] = {\n")
    n = 0
    for ch, kind, strokes in T:
        row = [ord(ch), ord(kind), len(strokes)]
        for s in strokes:
            row.append(len(s))
            for x, y in s:
                row += [q(x), q(y)]
        out.append("\t" + ", ".join(str(v) for v in row) + ",\n")
        n += 1
    out.append("};\n\n")
    out.append("const unsigned int recog_pool_len = sizeof(recog_pool);\n")
    out.append("const unsigned int recog_builtin_count = %d;\n" % n)
    with open(path, "w") as f:
        f.write("".join(out))
    return n


def preview(path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    cols = 12
    rows = (len(T) + cols - 1) // cols
    fig, axs = plt.subplots(rows, cols, figsize=(cols * 1.3, rows * 1.5))
    for ax in axs.flat:
        ax.axis("off")
    for ax, (ch, kind, strokes) in zip(axs.flat, T):
        for s in strokes:
            xs = [p[0] for p in s]
            ys = [p[1] for p in s]
            ax.plot(xs, ys, "k-", lw=1.5)
            ax.plot(xs[0], ys[0], "ro", ms=3)
        ax.set_xlim(-10, 110)
        ax.set_ylim(110, -10)
        ax.set_aspect("equal")
        ax.set_title("%s %s" % (ch, kind), fontsize=8)
    fig.tight_layout()
    fig.savefig(path, dpi=90)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    n = emit(os.path.join(here, "recog_templates.c"))
    print("wrote %d templates" % n)
    if "--preview" in sys.argv:
        preview(os.path.join(here, "templates_preview.png"))
        print("wrote preview")
