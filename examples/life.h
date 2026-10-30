#ifndef LIFE_H
#define LIFE_H

#include "glyph.h"

#define LW 60
#define LH 22

struct life {
    unsigned char cells[LH][LW];
};

/* seed with a glider and an R-pentomino; toroidal edges */
void life_setup(struct game *g, struct life *l);

#endif
