#include "consts.c"
#include <git2/oid.h>
#include <stdio.h>

void print_cwd(char *s) { printf(CYAN BOLD "%s" RESET, s); }

void print_git_branch(const char *s) {
    printf(MAGENTA BOLD GIT_BRANCH " %s" RESET, s);
}

int stash_cb(size_t _index, const char *_msg, const git_oid *_id,
             void *status) {
    // has stash
    (*(unsigned *)status) |= (1u << 3);
    // we don't need to know how many stashes there are
    return 114514;
}

void print_red_bold(const char *s) { printf(RED BOLD "%s" RESET, s); }

void print_git_info(unsigned map) {
    if (map == 0) {
        return;
    }
    char *git_info[] = {GIT_DELETED,      GIT_ADDED,        GIT_MODIFIED,
                        GIT_STASH,        GIT_UNTRACKED,    GIT_DIVERGE,
                        GIT_AHEAD_REMOTE, GIT_BEHIND_REMOTE};
    print_red_bold(" [");
    for (int i = 0; i < 8; i++) {
        if (map & (1u << i)) {
            print_red_bold(git_info[i]);
        }
    }
    print_red_bold("]");
}
void print_job_count(int cnt) {
    if (cnt != 0) {
        printf(BLUE "[%d] " RESET, cnt);
    }
}
void print_time(int time) {
    if (time < 3) {
        return;
    }
    if (time >= 3600) {
        printf(YELLOW BOLD "%dh" RESET, time / 3600);
        time %= 3600;
    }
    if (time >= 60) {
        printf(YELLOW BOLD "%dm" RESET, time / 60);
        time %= 60;
    }
    if (time != 0) {
        printf(YELLOW BOLD "%ds" RESET, time);
    }
    printf(" ");
}
void print_prompt(int exit_code) {
    if (exit_code == 0) {
        printf(GREEN BOLD PROMPT RESET);
    } else {
        printf(RED BOLD PROMPT RESET);
    }
}
