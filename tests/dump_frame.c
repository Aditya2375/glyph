#include "snake.h"

int main(void) {
    struct game g;
    struct snake s;
    snake_setup(&g, &s, 42);
    game_run_headless(&g, 5, NULL, 0);
    screen_dump(&g.screen, stdout);
    screen_free(&g.screen);
    return 0;
}
