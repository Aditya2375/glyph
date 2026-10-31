#include "glyph.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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
    int cur_fg = -1, cur_bg = -1;   /* -1: no attribute sequence emitted */
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
    if (cur_fg != -1)               /* only reset when colors were set */
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
    if (next == in->head) { in->dropped++; return; }   /* full: drop, but count it */
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
#include <sys/ioctl.h>
#include <sys/time.h>

/* feed one byte through the escape-sequence state machine */
static void input_byte(struct input *in, unsigned char b) {
    if (in->esc_len == 0) {
        if (b == 27) { in->esc[in->esc_len++] = b; return; }
        input_push(in, b);
        return;
    }
    if (in->esc_len == 1) {
        if (b == '[') { in->esc[in->esc_len++] = b; return; }
        input_push(in, 27);        /* a lone ESC, then this byte */
        in->esc_len = 0;
        if (b == 27) in->esc[in->esc_len++] = b;
        else input_push(in, b);
        return;
    }
    /* esc_len == 2: expecting the final byte of an arrow */
    int key = GK_NONE;
    switch (b) {
    case 'A': key = GK_UP; break;
    case 'B': key = GK_DOWN; break;
    case 'C': key = GK_RIGHT; break;
    case 'D': key = GK_LEFT; break;
    }
    if (key != GK_NONE) {
        input_push(in, key);
    } else {
        input_push(in, 27);
        input_push(in, '[');
        input_push(in, b);
    }
    in->esc_len = 0;
}

void input_read_fd(struct input *in, int fd) {
    unsigned char buf[64];
    ssize_t n = read(fd, buf, sizeof buf);
    if (n <= 0) return;
    for (ssize_t i = 0; i < n; i++) input_byte(in, buf[i]);
}

void game_init(struct game *g, int w, int h, int fps) {
    memset(g, 0, sizeof *g);
    screen_init(&g->screen, w, h);
    if (fps < 1) fps = 1;
    if (fps > 1000) fps = 1000;
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

static long now_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

static volatile sig_atomic_t got_quit;
static void on_quit(int sig) { got_quit = sig; }

int game_run(struct game *g) {
    struct winsize ws;
    if (tcgetattr(0, &(struct termios){0}) != 0) {
        fprintf(stderr, "glyph: stdin is not a terminal\n");
        return 1;
    }
    if (ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0 &&
        ((int)ws.ws_col < g->screen.w || (int)ws.ws_row < g->screen.h)) {
        fprintf(stderr, "glyph: terminal is %dx%d, need at least %dx%d\n",
                ws.ws_col, ws.ws_row, g->screen.w, g->screen.h);
        return 1;
    }
    struct termios saved, raw;
    tcgetattr(0, &saved);
    raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &raw);
    /* no SA_RESTART: a signal interrupts sleeps and reads so the loop can
       unwind promptly instead of hanging in usleep */
    struct sigaction sa = { .sa_handler = on_quit };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    got_quit = 0;
    printf("\033[?25l\033[2J");   /* hide cursor, clear */
    fflush(stdout);

    /* fixed timestep with an accumulator: updates run at exactly fps,
       rendering happens once per iteration, slow frames catch up in
       bounded steps instead of drifting or spiraling */
    const long frame_us = 1000000L / g->fps;
    long last = now_us();
    long acc = 0;
    while (g->running && !got_quit) {
        long now = now_us();
        acc += now - last;
        last = now;
        if (acc > 8 * frame_us) acc = 8 * frame_us;
        input_read_fd(&g->input, 0);
        while (acc >= frame_us && g->running) {
            g->update(g);
            g->tick++;
            acc -= frame_us;
        }
        g->draw(g);
        screen_flush(&g->screen, stdout);
        fflush(stdout);
        /* sleep the remainder of the frame instead of polling */
        if (g->running && acc < frame_us)
            usleep((useconds_t)(frame_us - acc));
    }

    printf("\033[?25h\033[0m\033[2J");
    tcsetattr(0, TCSAFLUSH, &saved);
    fflush(stdout);
    if (got_quit) {
        /* restore done above; re-raise so the exit status is honest */
        signal(got_quit, SIG_DFL);
        raise(got_quit);
    }
    return 0;
}

int box_overlap(struct box a, struct box b) {
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}
