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
    int pdx[8], pdy[8];     /* pending turns, applied one per tick */
    int pq_head, pq_tail;
    int food_x, food_y;
    int score;
    int dead;
    unsigned rng;
};

void snake_setup(struct game *g, struct snake *s, unsigned seed);

#endif
