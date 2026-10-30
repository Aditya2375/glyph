#include "harness.h"
#include "snake.h"

static void run(struct game *g, struct snake *s, int ticks, const int *script, int n) {
    snake_setup(g, s, 42);
    game_run_headless(g, ticks, script, n);
}

int main(void) {
    struct game g;
    struct snake s;

    /* moves right by default */
    run(&g, &s, 3, NULL, 0);
    CHECK(s.len == 3, "still 3 long");
    CHECK(s.x[0] == 13 && s.y[0] == 10, "head moved right 3");
    CHECK(s.x[1] == 12 && s.x[2] == 11, "body follows");
    screen_free(&g.screen);

    /* arrow keys steer */
    int script[] = {GK_UP, GK_LEFT};
    run(&g, &s, 3, script, 2);
    CHECK(s.x[0] == 8 && s.y[0] == 9, "up then left");
    screen_free(&g.screen);

    /* reversing into yourself is blocked */
    int rev[] = {GK_LEFT};   /* facing right; left would be suicide */
    run(&g, &s, 2, rev, 1);
    CHECK(s.x[0] == 12 && g.running == 1, "no instant reverse");
    screen_free(&g.screen);

    /* eating grows and scores: play toward food deterministically */
    snake_setup(&g, &s, 42);
    /* drive the snake onto the food with a computed path */
    int path[256], n = 0;
    for (int guard = 0; guard < 300 && s.score == 0; guard++) {
        if (s.food_x > s.x[0] && s.dx != -1) path[n++] = GK_RIGHT;
        else if (s.food_x < s.x[0] && s.dx != 1) path[n++] = GK_LEFT;
        else if (s.food_y > s.y[0] && s.dy != -1) path[n++] = GK_DOWN;
        else if (s.food_y < s.y[0] && s.dy != 1) path[n++] = GK_UP;
        else path[n++] = GK_DOWN;  /* reposition */
        if (n >= 250) break;
        /* simulate one tick to update the path online */
        game_run_headless(&g, 1, path + n - 1, 1);
        n = 0;
        game_run_headless(&g, 0, NULL, 0);
        path[n] = 0;
        /* note: path stepping done tick-by-tick */
    }
    CHECK(s.score == 1, "ate the food");
    CHECK(s.len == 4, "grew to 4");
    screen_free(&g.screen);

    /* wall kills */
    snake_setup(&g, &s, 42);
    game_run_headless(&g, 40, NULL, 0);   /* run right into the wall */
    CHECK(g.running == 0, "wall ends the game");
    screen_free(&g.screen);

    /* q quits */
    snake_setup(&g, &s, 42);
    int quit[] = {'q'};
    game_run_headless(&g, 1, quit, 1);
    CHECK(g.running == 0, "q quits");
    screen_free(&g.screen);

    TEST_RESULTS();
}
