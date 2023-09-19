#include <git2.h>
#include <git2/buffer.h>
#include <git2/diff.h>
#include <git2/global.h>
#include <git2/net.h>
#include <git2/repository.h>
#include <git2/stash.h>
#include <git2/status.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "str.c"
#include "util.c"

int main(int argc, char *argv[]) {
    // Arg parsing
    if (argc == 2 && strcmp(argv[1], "init") == 0) {
        printf("%s", SHELL_SETUP);
        return 0;
    }
    if (argc != 4) {
        perror("bad arguments, \"init\" is the only option you should use\n");
        return 1;
    }
    int exit_status = strtol(argv[1], NULL, 10);
    int time_taken = strtol(argv[2], NULL, 10);
    int job_count = strtol(argv[3], NULL, 10);

    char cwd[PATH_MAX];
    unsigned accessible = 0;
    if (getcwd(cwd, PATH_MAX) == NULL) {
        perror("failed to get the current working directory\n");
        return 1;
    }
    accessible = access(cwd, R_OK) == 0 && access(cwd, W_OK) == 0;

    // splitting the path by "/"
    unsigned size;
    if (strcmp(cwd, "/") != 0) {
        size = count_occur(cwd, 0x2F);
    } else {
        size = 0;
    }
    char *elements[size];
    strsplit(elements, cwd, "/");

    git_libgit2_init();
    git_repository *repo;

    char *prepend = "/";   // str before the truncated dirs
    unsigned level = size; // how many levels of directory to show

    char *username = getlogin();
    if (username == NULL) {
        perror("failed to get the current username\n");
        return 1;
    }
    if (strcmp("home", elements[0]) == 0 &&
        strcmp(username, elements[1]) == 0) {
        // cwd is under home
        if (size == 2) {
            prepend = "~";
            level = 0;
        } else {
            prepend = "~/";
            level = size - 2;
        }
        if (size > (2 + LEVELS_DISPLAYED)) {
            level = size - 2;
        }
    }
    if (level > LEVELS_DISPLAYED) {
        level = LEVELS_DISPLAYED;
        prepend = "";
    }

    unsigned show_git_info = 0u;
    // Assuming /, /*. /*/* cannot be git repos themselves
    for (int i = size; i > 2; i--) {
        // check from cwd to /*/*/*
        char *path = str_path_clamp(elements, i);
        if (git_repository_open(&repo, path) == 0) {
            // Is a git repo
            show_git_info = 1;
            level = size - i;
            prepend = malloc((strlen(elements[i - 1]) + 3) * sizeof(char));
            prepend[0] = '\0';
            strcat(prepend, elements[i - 1]);
            if (level > LEVELS_DISPLAYED) {
                // we are deep inside a git repo
                level = LEVELS_DISPLAYED;
                strcat(prepend, ": ");
            } else if (level == 0) {
                // Do nothing, we are at root
            } else {
                strcat(prepend, "/");
            }
            break;
        }
    }

    // print the whole cwd info
    char *tail = str_path_tail(elements, level, size);
    printf("\n");
    print_cwd(prepend);
    print_cwd(tail);
    if (!accessible) {
        printf(LOCKED_SYMBOL);
    }
    if (!show_git_info) {
        goto PRINT_SECOND_ROW;
    } else {
        free(prepend);
    }
    printf(" on ");
    // get branch name
    git_reference *head_ref;
    if (git_repository_head(&head_ref, repo) != 0) {
        perror("failed to get head ref\n");
        git_repository_free(repo);
        return 1;
    }
    const char *branch_name = git_reference_shorthand(head_ref);
    print_git_branch(branch_name);
    // get info about the staging area
    git_status_list *status;
    git_status_options opt = GIT_STATUS_OPTIONS_INIT;
    opt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;
    if (git_status_list_new(&status, repo, &opt) != 0) {
        perror("failed to get status list\n");
        git_repository_free(repo);
        return 1;
    }
    size_t entry_count = git_status_list_entrycount(status);
    // bitmap
    unsigned git_status = 0;
    for (size_t i = 0; i < entry_count; i++) {
        const git_status_entry *entry = git_status_byindex(status, i);
        if (entry->index_to_workdir == NULL) {
            // GIT_ADDED "+"
            if (entry->status == GIT_STATUS_INDEX_NEW) {
                git_status |= (1u << 1);
            }
        } else {
            git_delta_t index_status = entry->index_to_workdir->status;
            git_status_t status = entry->status;
            if (status == GIT_STATUS_WT_DELETED &&
                index_status == GIT_DELTA_DELETED) {
                // GIT_DELETED "✘"
                git_status |= 1u;
            } else if (index_status == GIT_DELTA_MODIFIED) {
                // GIT_MODIFIED "!"
                git_status |= (1u << 2);
            } else if (status == GIT_STATUS_WT_NEW &&
                       index_status == GIT_DELTA_UNTRACKED) {
                // GIT_UNTRACKED "?"
                git_status |= (1u << 4);
            }
        }
    }
    // get stash info
    int e = git_stash_foreach(repo, stash_cb, &git_status);
    if (e != 0 && e != 114514) {
        perror("failed to get stash info\n");
        git_repository_free(repo);
        git_status_list_free(status);
        return 1;
    }
    git_status_list_free(status);

    // Get remote info
    git_oid local_oid, remote_oid;
    size_t ahead, behind;
    git_strarray remotes;
    if (git_remote_list(&remotes, repo) != 0) {
        perror("failed to get remotes\n");
        git_repository_free(repo);
        return 1;
    }
    // string to query oid for local head (refs/heads/main)
    char *local_name = malloc(
        (strlen("refs/heads/") + strlen(branch_name) + 1) * sizeof(char));
    local_name[0] = '\0';
    strcat(local_name, "refs/heads/");
    strcat(local_name, branch_name);
    if (git_reference_name_to_id(&local_oid, repo, local_name) != 0) {
        perror("failed to get id for local branch\n");
        git_repository_free(repo);
        free(local_name);
        return 1;
    }

    // string to query oid for remote name for this branch
    // (refs/remotes/origin/main)
    git_buf buf = GIT_BUF_INIT;
    if (git_branch_upstream_remote(&buf, repo, local_name) != 0) {
        goto PRINT_GIT_STATUS;
    }
    free(local_name);
    char *remote_name = malloc(
        (strlen("refs/remotes/") + strlen(buf.ptr) + strlen(branch_name) + 2) *
        sizeof(char));
    remote_name[0] = '\0';
    strcat(remote_name, "refs/remotes/");
    strcat(remote_name, buf.ptr);
    strcat(remote_name, "/");
    strcat(remote_name, branch_name);
    if (git_reference_name_to_id(&remote_oid, repo, remote_name) != 0) {
        perror("failed to get id for remote branch\n");
        git_repository_free(repo);
        free(remote_name);
        return 1;
    }
    free(remote_name);

    if (git_graph_ahead_behind(&ahead, &behind, repo, &local_oid,
                               &remote_oid) != 0) {
        perror("failed to determine the relationship between local and "
               "remote branch\n");
    }
    if (ahead > 0 && behind > 0) {
        // diverged
        git_status |= (1u << 5);
    } else if (ahead > 0) {
        // ahead
        git_status |= (1u << 6);
    } else if (behind > 0) {
        // behind
        git_status |= (1u << 7);
    }

PRINT_GIT_STATUS:
    print_git_info(git_status);
    git_repository_free(repo);
PRINT_SECOND_ROW:
    printf("\n");
    print_job_count(job_count);
    print_time(time_taken);
    print_prompt(exit_status);

    git_libgit2_shutdown();
    return 0;
}
