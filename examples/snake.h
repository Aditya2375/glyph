#ifndef SNAKE_H
#define SNAKE_H

#include "glyph.h"

#define W 40
#define H 20
#define MAX_LEN 400

struct snake {
    int x[MAX_LEN], y[MAX_LEN];
    int len;
    int dx, dy;
    int food_x, food_y;
    int score;
    unsigned rng;
};

void snake_setup(struct game *g, struct snake *s, unsigned seed);

#endif
