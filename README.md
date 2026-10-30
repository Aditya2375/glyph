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
  decode to `GK_UP/DOWN/LEFT/RIGHT`.
- **The loop**: fixed timestep - update, draw, flush, sleep for the rest
  of the frame.
- **Headless mode**: `game_run_headless` ticks the same update/draw code
  with scripted input and no terminal, which is how the test suite plays
  snake for real.

## Architecture and tests

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). `./tests/run-tests.sh`
runs 27 checks: engine primitives, and gameplay itself - steering,
no-reverse, eating and growth, wall death, quitting - plus a golden
rendered frame. CI runs the same script on every push.
