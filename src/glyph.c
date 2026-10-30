#include "glyph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void screen_init(struct screen *s, int w, int h) {
    s->w = w;
    s->h = h;
    s->front = calloc((size_t)w * h, sizeof *s->front);
    s->back = calloc((size_t)w * h, sizeof *s->back);
    if (!s->front || !s->back) { perror("calloc"); exit(1); }
}

void screen_free(struct screen *s) {
    free(s->front);
    free(s->back);
}

void screen_clear(struct screen *s, char ch, uint8_t fg, uint8_t bg) {
    for (int i = 0; i < s->w * s->h; i++)
        s->back[i] = (struct cell){ch, fg, bg};
}

void screen_put(struct screen *s, int x, int y, char ch, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;
    s->back[y * s->w + x] = (struct cell){ch, fg, bg};
}

void screen_text(struct screen *s, int x, int y, const char *text, uint8_t fg, uint8_t bg) {
    for (int i = 0; text[i]; i++) screen_put(s, x + i, y, text[i], fg, bg);
}

static int ansi_fg(uint8_t c) { return c == GLYPH_DEFAULT ? 39 : 30 + c; }
static int ansi_bg(uint8_t c) { return c == GLYPH_DEFAULT ? 49 : 40 + c; }

long screen_flush(struct screen *s, FILE *out) {
    long written = 0;
    int cur_fg = -1, cur_bg = -1;
    for (int y = 0; y < s->h; y++) {
        for (int x = 0; x < s->w; x++) {
            struct cell *f = &s->front[y * s->w + x];
            struct cell *b = &s->back[y * s->w + x];
            if (f->ch == b->ch && f->fg == b->fg && f->bg == b->bg) continue;
            *f = *b;
            if ((int)b->fg != cur_fg || (int)b->bg != cur_bg) {
                written += fprintf(out, "\033[%d;%dm", ansi_fg(b->fg), ansi_bg(b->bg));
                cur_fg = b->fg;
                cur_bg = b->bg;
            }
            written += fprintf(out, "\033[%d;%dH%c", y + 1, x + 1, b->ch);
        }
    }
    written += fprintf(out, "\033[0m");
    return written;
}

void screen_dump(const struct screen *s, FILE *out) {
    for (int y = 0; y < s->h; y++) {
        for (int x = 0; x < s->w; x++)
            fputc(s->back[y * s->w + x].ch, out);
        fputc('\n', out);
    }
}

void input_push(struct input *in, int key) {
    int next = (in->tail + 1) % 64;
    if (next == in->head) return;   /* full: drop */
    in->keys[in->tail] = key;
    in->tail = next;
}

int input_poll(struct input *in) {
    if (in->head == in->tail) return GK_NONE;
    int k = in->keys[in->head];
    in->head = (in->head + 1) % 64;
    return k;
}

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

void input_read_fd(struct input *in, int fd) {
    unsigned char buf[64];
    ssize_t n = read(fd, buf, sizeof buf);
    if (n <= 0) return;
    for (ssize_t i = 0; i < n; i++) {
        if (buf[i] == 27 && i + 2 < n && buf[i + 1] == '[') {
            switch (buf[i + 2]) {
            case 'A': input_push(in, GK_UP); i += 2; continue;
            case 'B': input_push(in, GK_DOWN); i += 2; continue;
            case 'C': input_push(in, GK_RIGHT); i += 2; continue;
            case 'D': input_push(in, GK_LEFT); i += 2; continue;
            }
        }
        input_push(in, buf[i]);
    }
}

void game_init(struct game *g, int w, int h, int fps) {
    memset(g, 0, sizeof *g);
    screen_init(&g->screen, w, h);
    g->fps = fps;
    g->running = 1;
}

void game_run_headless(struct game *g, int ticks, const int *script, int script_len) {
    for (int t = 0; t < ticks && g->running; t++) {
        if (t < script_len) input_push(&g->input, script[t]);
        g->update(g);
        g->draw(g);
        g->tick++;
    }
}

static long now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int game_run(struct game *g) {
    struct termios saved, raw;
    if (tcgetattr(0, &saved) != 0) {
        fprintf(stderr, "glyph: stdin is not a terminal\n");
        return 1;
    }
    raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &raw);
    printf("\033[?25l\033[2J");   /* hide cursor, clear */
    fflush(stdout);

    long frame_ms = 1000 / g->fps;
    while (g->running) {
        long start = now_ms();
        input_read_fd(&g->input, 0);
        g->update(g);
        g->draw(g);
        screen_flush(&g->screen, stdout);
        fflush(stdout);
        g->tick++;
        long spent = now_ms() - start;
        if (spent < frame_ms) usleep((useconds_t)(frame_ms - spent) * 1000);
    }

    printf("\033[?25h\033[0m\033[2J");
    tcsetattr(0, TCSAFLUSH, &saved);
    return 0;
}

int box_overlap(struct box a, struct box b) {
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}
