# glyph

A terminal game engine in C: fixed-timestep loop, double-buffered cell
graphics with colors, raw input with arrow-key decoding, AABB collision
helpers, and a headless mode that runs the whole game without a terminal
so gameplay is unit-testable. Ships with a full snake game.

```text
$ ./build/snake
########################################
#            oo@        *              #
########################################
```

## Quick start

```bash
make
./build/snake        # arrows to steer, q to quit
```

## Using the engine

```c
#include "glyph.h"

static void update(struct game *g) {
    int key;
    while ((key = input_poll(&g->input)) != GK_NONE) { /* ... */ }
    /* advance the world */
}

static void draw(struct game *g) {
    screen_clear(&g->screen, ' ', GLYPH_WHITE, GLYPH_BLACK);
    screen_put(&g->screen, x, y, '@', GLYPH_GREEN, GLYPH_BLACK);
}

struct game g;
game_init(&g, 40, 20, 60);   /* width, height, ticks/sec */
g.update = update;
g.draw = draw;
game_run(&g);                /* raw terminal, real clock */
```

- **Double buffering**: draw into the back buffer; `screen_flush` writes
  only the cells that changed, positioned with ANSI escapes.
- **Input**: a bounded queue fed from non-blocking reads; escape sequences
  decode to `GK_UP/DOWN/LEFT/RIGHT`, even when a sequence arrives split
  across two reads. Dropped keys on a full queue are counted.
- **The loop**: fixed timestep with a real accumulator - updates run at
  exactly the requested fps (clamped to 1..1000), rendering happens once
  per pass, and a slow frame catches up in bounded steps instead of
  drifting or busy-spinning.
- **Headless mode**: `game_run_headless` ticks the same update/draw code
  with scripted input and no terminal, which is how the test suite plays
  snake for real.

## Architecture and tests

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). `./tests/run-tests.sh`
runs 45 checks: engine primitives, and gameplay itself - steering,
same-tick double turns, tail-vacation moves, eating and growth, wall
death, the game-over screen, quitting - plus a golden rendered frame.
Two games ship as examples: snake and a Game of Life. CI runs the same
script on every push.

## Tested / not tested

Tested here: 23 engine unit checks (screen, diff flush, input queue with
drop counting, escape decoding including sequences split across reads,
fps clamping, collision), 17 headless gameplay checks that drive snake
for real (steering, blocked reverse, same-tick double turns, tail
vacation, eating/growth, wall death, the game-over screen flow,
quitting), 5 Game of Life checks (glider translation, still lifes), and
a byte-exact golden rendered frame. The real terminal loop is exercised
on a pty: it renders frames and exits cleanly on q.

Not tested: window resize mid-game, non-ANSI terminals, and long
interactive sessions by a human (logic is headless-verified; ANSI output
is verified as text and on a pty, not by eye on every emulator).
