#ifndef GLYPH_H
#define GLYPH_H

#include <stdint.h>
#include <stdio.h>

/* glyph - a terminal game engine: fixed-timestep loop, double-buffered
   cell grid with colors, raw input, AABB physics, scene stack. */

enum {
    GLYPH_BLACK, GLYPH_RED, GLYPH_GREEN, GLYPH_YELLOW,
    GLYPH_BLUE, GLYPH_MAGENTA, GLYPH_CYAN, GLYPH_WHITE, GLYPH_DEFAULT
};

struct cell {
    char ch;
    uint8_t fg, bg;
};

/* The screen: a double-buffered grid. */
struct screen {
    int w, h;
    struct cell *front, *back;
};

void screen_init(struct screen *s, int w, int h);
void screen_free(struct screen *s);
void screen_clear(struct screen *s, char ch, uint8_t fg, uint8_t bg);
void screen_put(struct screen *s, int x, int y, char ch, uint8_t fg, uint8_t bg);
void screen_text(struct screen *s, int x, int y, const char *text, uint8_t fg, uint8_t bg);
/* write only changed cells to out (ANSI). Returns bytes written. */
long screen_flush(struct screen *s, FILE *out);
/* render the whole back buffer as plain text (testing). */
void screen_dump(const struct screen *s, FILE *out);

/* input: keys are bytes; arrows/escape decoded to these codes */
enum { GK_UP = 256, GK_DOWN, GK_LEFT, GK_RIGHT, GK_NONE = -1 };

struct input {
    int keys[64];       /* queue of key codes */
    int head, tail;
    unsigned long dropped;          /* keys lost to a full queue */
    unsigned char esc[8];           /* partial escape sequence across reads */
    int esc_len;
};

void input_push(struct input *in, int key);
int input_poll(struct input *in);             /* GK_NONE when empty */
/* decode available bytes from fd into the queue (non-blocking); an escape
   sequence split across reads is buffered until it completes. */
void input_read_fd(struct input *in, int fd);

/* the fixed-timestep loop */
struct game {
    struct screen screen;
    struct input input;
    int fps;            /* ticks per second */
    int running;
    long tick;
    void *state;        /* game-specific */
    void (*update)(struct game *g);
    void (*draw)(struct game *g);
};

/* fps is clamped to 1..1000; the loop uses a real accumulator. */
void game_init(struct game *g, int w, int h, int fps);
/* run headless for n ticks, feeding scripted keys, no sleeping/terminal */
void game_run_headless(struct game *g, int ticks, const int *script, int script_len);
/* run on the terminal: raw mode, real clock, ANSI rendering */
int game_run(struct game *g);

/* physics helpers */
struct box { int x, y, w, h; };
int box_overlap(struct box a, struct box b);

#endif
