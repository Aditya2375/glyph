#include "harness.h"
#include "glyph.h"

#include <string.h>
#include <unistd.h>

int main(void) {
    /* screen: put, bounds, clear */
    struct screen s;
    screen_init(&s, 10, 5);
    screen_clear(&s, '.', GLYPH_WHITE, GLYPH_BLACK);
    screen_put(&s, 3, 2, 'X', GLYPH_RED, GLYPH_BLUE);
    screen_put(&s, -1, 0, 'B', GLYPH_RED, GLYPH_BLACK);   /* off grid: no crash */
    screen_put(&s, 99, 99, 'B', GLYPH_RED, GLYPH_BLACK);
    CHECK(s.back[2 * 10 + 3].ch == 'X', "put lands");
    CHECK(s.back[2 * 10 + 3].fg == GLYPH_RED, "fg stored");
    CHECK(s.back[0].ch == '.', "clear filled");
    screen_text(&s, 0, 0, "abc", GLYPH_GREEN, GLYPH_BLACK);
    CHECK(s.back[0].ch == 'a' && s.back[2].ch == 'c', "text spans");

    /* flush emits only changed cells */
    char buf[8192];
    FILE *f = fmemopen(buf, sizeof buf, "w");
    long first = screen_flush(&s, f);
    fclose(f);
    CHECK(first > 0, "first flush writes");
    f = fmemopen(buf, sizeof buf, "w");
    long second = screen_flush(&s, f);
    fclose(f);
    CHECK(second < first, "second flush writes only the reset");
    screen_put(&s, 0, 0, 'Z', GLYPH_WHITE, GLYPH_BLACK);
    f = fmemopen(buf, sizeof buf, "w");
    screen_flush(&s, f);
    fclose(f);
    CHECK(strstr(buf, "Z") != NULL, "changed cell emitted");

    /* input queue */
    struct input in = {0};
    CHECK(input_poll(&in) == GK_NONE, "empty queue");
    input_push(&in, 'a');
    input_push(&in, 'b');
    CHECK(input_poll(&in) == 'a' && input_poll(&in) == 'b' && input_poll(&in) == GK_NONE,
          "fifo order");
    for (int i = 0; i < 200; i++) input_push(&in, i);  /* overflow: drops, no crash */
    CHECK(input_poll(&in) != GK_NONE, "queue survives overflow");

    /* escape decoding needs a pipe */
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe");
    ssize_t wr = write(fds[1], "\033[A\033[Bx", 7);
    CHECK(wr == 7, "wrote escape bytes");
    struct input in2 = {0};
    input_read_fd(&in2, fds[0]);
    CHECK(input_poll(&in2) == GK_UP, "up arrow decoded");
    CHECK(input_poll(&in2) == GK_DOWN, "down arrow decoded");
    CHECK(input_poll(&in2) == 'x', "plain byte passes");
    close(fds[0]); close(fds[1]);

    /* physics */
    CHECK(box_overlap((struct box){0, 0, 4, 4}, (struct box){3, 3, 4, 4}) == 1, "overlap");
    CHECK(box_overlap((struct box){0, 0, 4, 4}, (struct box){4, 0, 4, 4}) == 0, "touching is not overlap");
    CHECK(box_overlap((struct box){0, 0, 2, 2}, (struct box){5, 5, 1, 1}) == 0, "apart");

    screen_free(&s);
    /* an escape sequence split across reads decodes as one key */
    {
        int fds[2];
        if (pipe(fds) != 0) { CHECK(0, "pipe"); return 1; }
        struct input in = {0};
        if (write(fds[1], "\033[", 2) != 2) { CHECK(0, "write"); return 1; }
        input_read_fd(&in, fds[0]);
        CHECK(input_poll(&in) == GK_NONE, "partial escape waits");
        if (write(fds[1], "A", 1) != 1) { CHECK(0, "write"); return 1; }
        input_read_fd(&in, fds[0]);
        CHECK(input_poll(&in) == GK_UP, "completed escape decodes to UP");
        close(fds[0]); close(fds[1]);
    }

    /* a full input queue drops keys but counts them */
    {
        struct input in = {0};
        for (int i = 0; i < 200; i++) input_push(&in, 'a' + (i % 26));
        CHECK(in.dropped == 200 - 63, "dropped keys are counted");
    }

    /* fps clamps to a sane range */
    {
        struct game g2;
        game_init(&g2, 8, 4, 5000);
        CHECK(g2.fps == 1000, "fps clamped high");
        screen_free(&g2.screen);
        game_init(&g2, 8, 4, 0);
        CHECK(g2.fps == 1, "fps clamped low");
        screen_free(&g2.screen);
    }

    TEST_RESULTS();
}
