/* SIGINT mid-run: the child runs life on a pty, the parent sends SIGINT
   after the first frame. The engine must restore the terminal (teardown
   bytes reach the master) and die by SIGINT, not exit 0 or hang. */
#include "glyph.h"
#include "life.h"

#include <pty.h>
#include <signal.h>
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
        exit(game_run(&g));
    }

    char buf[65536];
    size_t total = 0;
    int sig_sent = 0;
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(master, &rfds);
        struct timeval tv = {.tv_sec = 5};
        if (select(master + 1, &rfds, NULL, NULL, &tv) <= 0) {
            fprintf(stderr, "signal test: timed out waiting for output\n");
            kill(pid, SIGKILL);
            return 1;
        }
        ssize_t n = read(master, buf + total, sizeof buf - total - 1);
        if (n <= 0) break;
        total += (size_t)n;
        buf[total] = '\0';
        if (!sig_sent && total > 512) {
            if (kill(pid, SIGINT) != 0) { perror("kill"); return 1; }
            sig_sent = 1;
        }
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGINT) {
        fprintf(stderr, "signal test: expected death by SIGINT (status %d)\n", status);
        return 1;
    }
    if (!strstr(buf, "\033[?25h")) {
        fprintf(stderr, "signal test: terminal never restored after SIGINT\n");
        return 1;
    }
    printf("SIGNAL TEST PASSED (terminal restored, died by SIGINT)\n");
    return 0;
}
