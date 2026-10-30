#include "harness.h"
#include "life.h"

#include <string.h>

int main(void) {
    struct game g;
    struct life l;

    /* a lone glider translates by (1,1) every 4 ticks */
    life_setup(&g, &l);
    memset(l.cells, 0, sizeof l.cells);
    l.cells[1][2] = 1;
    l.cells[2][3] = 1;
    l.cells[3][1] = l.cells[3][2] = l.cells[3][3] = 1;
    game_run_headless(&g, 4, NULL, 0);
    int live_cells = 0, minx = LW, miny = LH;
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++)
            if (l.cells[y][x]) { live_cells++; if (x < minx) minx = x; if (y < miny) miny = y; }
    CHECK(live_cells == 5, "glider stays five cells");
    CHECK(minx == 2 && miny == 2, "glider translated by (1,1) after 4 ticks");

    /* still life: a block never changes */
    life_setup(&g, &l);
    memset(l.cells, 0, sizeof l.cells);
    l.cells[5][5] = l.cells[5][6] = l.cells[6][5] = l.cells[6][6] = 1;
    game_run_headless(&g, 6, NULL, 0);
    CHECK(l.cells[5][5] && l.cells[5][6] && l.cells[6][5] && l.cells[6][6], "block is stable");
    int others = 0;
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++)
            if (l.cells[y][x]) others++;
    CHECK(others == 4, "block spawns nothing");

    /* q quits */
    life_setup(&g, &l);
    int q[] = {'q'};
    game_run_headless(&g, 1, q, 1);
    CHECK(g.running == 0, "q exits life");

    screen_free(&g.screen);
    TEST_RESULTS();
}
