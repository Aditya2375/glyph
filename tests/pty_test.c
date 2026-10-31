/* The real interactive loop on a pty: the child runs life on a terminal
   it believes in, the parent watches frames arrive and sends q. Verifies
   raw-mode setup, rendering to a tty, clean exit, and terminal restore. */
#include "glyph.h"
#include "life.h"

#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    struct winsize ws = {.ws_col = LW, .ws_row = LH + 1};
    int master;
    pid_t pid = forkpty(&master, NULL, NULL, &ws);
    if (pid < 0) { perror("forkpty"); return 1; }
    if (pid == 0) {
        struct game g;
        struct life l;
        life_setup(&g, &l);
        exit(game_run(&g));   /* not _exit: let stdio flush the teardown */
    }

    char buf[65536];
    size_t total = 0;
    int quit_sent = 0;
    /* read until a frame has been drawn, send q, then drain to EOF so the
       teardown bytes (cursor restore) are captured too */
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(master, &rfds);
        struct timeval tv = {.tv_sec = 5};
        if (select(master + 1, &rfds, NULL, NULL, &tv) <= 0) {
            fprintf(stderr, "pty test: timed out waiting for output\n");
            return 1;
        }
        ssize_t n = read(master, buf + total, sizeof buf - total - 1);
        if (n <= 0) break;                          /* child closed the pty */
        total += (size_t)n;
        buf[total] = '\0';
        if (!quit_sent && total > 512) {            /* first frame drawn */
            if (write(master, "q", 1) != 1) { perror("write"); return 1; }
            quit_sent = 1;
        }
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "pty test: game exited badly (status %d)\n", status);
        return 1;
    }
    if (!strstr(buf, "\033[?25l")) {
        fprintf(stderr, "pty test: never hid the cursor\n");
        return 1;
    }
    if (!strstr(buf, "\033[?25h")) {
        fprintf(stderr, "pty test: never restored the cursor\n");
        return 1;
    }
    printf("PTY TEST PASSED (%zu bytes of terminal output)\n", total);
    return 0;
}
