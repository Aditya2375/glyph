/* conway's game of life on glyph: the engine demo that needs no player. */
#include "glyph.h"

#include <string.h>

#include "life.h"

static int live(const struct life *l, int x, int y) {
    x = (x + LW) % LW;
    y = (y + LH) % LH;
    return l->cells[y][x];
}

static void step(struct life *l) {
    unsigned char next[LH][LW];
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++) {
            int n = live(l, x-1, y-1) + live(l, x, y-1) + live(l, x+1, y-1)
                  + live(l, x-1, y)                     + live(l, x+1, y)
                  + live(l, x-1, y+1) + live(l, x, y+1) + live(l, x+1, y+1);
            next[y][x] = l->cells[y][x] ? (n == 2 || n == 3) : (n == 3);
        }
    memcpy(l->cells, next, sizeof next);
}

static void update(struct game *g) {
    struct life *l = g->state;
    int key;
    while ((key = input_poll(&g->input)) != GK_NONE)
        if (key == 'q') { g->running = 0; return; }
    step(l);
}

static void draw(struct game *g) {
    struct life *l = g->state;
    screen_clear(&g->screen, ' ', GLYPH_WHITE, GLYPH_BLACK);
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++)
            if (l->cells[y][x])
                screen_put(&g->screen, x, y, 'o', GLYPH_GREEN, GLYPH_BLACK);
    screen_text(&g->screen, 2, 0, " life - q to quit ", GLYPH_YELLOW, GLYPH_BLACK);
}

void life_setup(struct game *g, struct life *l) {
    memset(l, 0, sizeof *l);
    /* glider, top left */
    l->cells[1][2] = 1;
    l->cells[2][3] = 1;
    l->cells[3][1] = l->cells[3][2] = l->cells[3][3] = 1;
    /* R-pentomino, middle */
    int cx = LW / 2, cy = LH / 2;
    l->cells[cy][cx + 1] = l->cells[cy][cx + 2] = 1;
    l->cells[cy + 1][cx] = l->cells[cy + 1][cx + 1] = 1;
    l->cells[cy + 2][cx + 1] = 1;
    game_init(g, LW, LH, 10);
    g->state = l;
    g->update = update;
    g->draw = draw;
}

#ifndef LIFE_NO_MAIN
int main(void) {
    struct life l;
    struct game g;
    life_setup(&g, &l);
    return game_run(&g);
}
#endif
