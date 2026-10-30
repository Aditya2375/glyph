/* snake on glyph: eat the food, grow, don't hit the walls or yourself. */
#include "glyph.h"

#include <stdlib.h>

#include "snake.h"

static void place_food(struct game *g) {
    struct snake *s = g->state;
    for (;;) {
        s->rng = s->rng * 1103515245 + 12345;
        int fx = (int)((s->rng >> 16) % (W - 2)) + 1;
        s->rng = s->rng * 1103515245 + 12345;
        int fy = (int)((s->rng >> 16) % (H - 2)) + 1;
        int on_snake = 0;
        for (int i = 0; i < s->len; i++)
            if (s->x[i] == fx && s->y[i] == fy) on_snake = 1;
        if (!on_snake) { s->food_x = fx; s->food_y = fy; return; }
    }
}

static void update(struct game *g) {
    struct snake *s = g->state;
    int key;
    while ((key = input_poll(&g->input)) != GK_NONE) {
        switch (key) {
        case GK_UP:    if (s->dy != 1)  { s->dx = 0;  s->dy = -1; } break;
        case GK_DOWN:  if (s->dy != -1) { s->dx = 0;  s->dy = 1;  } break;
        case GK_LEFT:  if (s->dx != 1)  { s->dx = -1; s->dy = 0;  } break;
        case GK_RIGHT: if (s->dx != -1) { s->dx = 1;  s->dy = 0;  } break;
        case 'q': g->running = 0; return;
        }
    }
    int nx = s->x[0] + s->dx;
    int ny = s->y[0] + s->dy;
    if (nx <= 0 || ny <= 0 || nx >= W - 1 || ny >= H - 1) { g->running = 0; return; }
    for (int i = 0; i < s->len; i++)
        if (s->x[i] == nx && s->y[i] == ny) { g->running = 0; return; }
    int ate = (nx == s->food_x && ny == s->food_y);
    if (ate && s->len < MAX_LEN) {
        for (int i = s->len; i > 0; i--) { s->x[i] = s->x[i - 1]; s->y[i] = s->y[i - 1]; }
        s->len++;
        s->score++;
        place_food(g);
    } else {
        for (int i = s->len - 1; i > 0; i--) { s->x[i] = s->x[i - 1]; s->y[i] = s->y[i - 1]; }
    }
    s->x[0] = nx;
    s->y[0] = ny;
}

static void draw(struct game *g) {
    struct snake *s = g->state;
    screen_clear(&g->screen, ' ', GLYPH_WHITE, GLYPH_BLACK);
    for (int x = 0; x < W; x++) {
        screen_put(&g->screen, x, 0, '#', GLYPH_CYAN, GLYPH_BLACK);
        screen_put(&g->screen, x, H - 1, '#', GLYPH_CYAN, GLYPH_BLACK);
    }
    for (int y = 0; y < H; y++) {
        screen_put(&g->screen, 0, y, '#', GLYPH_CYAN, GLYPH_BLACK);
        screen_put(&g->screen, W - 1, y, '#', GLYPH_CYAN, GLYPH_BLACK);
    }
    screen_put(&g->screen, s->food_x, s->food_y, '*', GLYPH_RED, GLYPH_BLACK);
    for (int i = 0; i < s->len; i++)
        screen_put(&g->screen, s->x[i], s->y[i], i == 0 ? '@' : 'o', GLYPH_GREEN, GLYPH_BLACK);
    char hud[64];
    snprintf(hud, sizeof hud, " score: %d ", s->score);
    screen_text(&g->screen, 2, 0, hud, GLYPH_YELLOW, GLYPH_BLACK);
}

void snake_setup(struct game *g, struct snake *s, unsigned seed) {
    *s = (struct snake){.len = 3, .dx = 1, .dy = 0, .rng = seed};
    s->x[0] = 10; s->y[0] = 10;
    s->x[1] = 9;  s->y[1] = 10;
    s->x[2] = 8;  s->y[2] = 10;
    game_init(g, W, H, 8);
    g->state = s;
    g->update = update;
    g->draw = draw;
    place_food(g);
}

#ifndef SNAKE_NO_MAIN
int main(void) {
    struct snake s;
    struct game g;
    snake_setup(&g, &s, 42);
    return game_run(&g);
}
#endif
