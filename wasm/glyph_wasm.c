/* Browser demo glue: run the real game loop one tick at a time under
   requestAnimationFrame and expose the screen buffer to JS. The native
   pieces left out are the raw-terminal shell and ANSI flush - see docs/. */
#include <emscripten.h>
#include <stdlib.h>

#include "../src/glyph.h"
#include "../examples/snake.h"

EMSCRIPTEN_KEEPALIVE struct game *glyph_snake_new(unsigned seed) {
    struct game *g = malloc(sizeof *g);
    struct snake *s = malloc(sizeof *s);
    if (!g || !s) { free(g); free(s); return NULL; }
    snake_setup(g, s, seed);
    return g;
}

EMSCRIPTEN_KEEPALIVE void glyph_snake_free(struct game *g) {
    if (!g) return;
    free(g->state);
    screen_free(&g->screen);
    free(g);
}

EMSCRIPTEN_KEEPALIVE void glyph_key(struct game *g, int key) { input_push(&g->input, key); }

/* One fixed-timestep tick: update then draw, like game_run_headless. */
EMSCRIPTEN_KEEPALIVE int glyph_tick(struct game *g) {
    g->update(g);
    g->draw(g);
    g->tick++;
    return g->running;
}

EMSCRIPTEN_KEEPALIVE int glyph_w(struct game *g) { return g->screen.w; }
EMSCRIPTEN_KEEPALIVE int glyph_h(struct game *g) { return g->screen.h; }

/* Pack cell i as (ch << 16) | (fg << 8) | bg for cheap JS reads. */
EMSCRIPTEN_KEEPALIVE int glyph_cell(struct game *g, int i) {
    struct cell c = g->screen.back[i];
    return ((unsigned char)c.ch << 16) | (c.fg << 8) | c.bg;
}
