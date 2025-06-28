#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <err.h>

#define MAX_PATH_LEN 1024

int pipe_fd[2];
int max_parallel_proc_cnt;

int check_exec(char* file) {
    struct stat st;
    if (stat(file, &st) == -1) {
        err(1, "Error stat");
    }
    return (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH);
}

void acquire() {
    char buf[1];
    if (read(pipe_fd[0], buf, 1) != 1) {
        err(1, "Error reading from pipe");
    }
}

void release() {
    char buf[1] = {0};
    if (write(pipe_fd[1], buf, 1) != 1) {
        err(1, "Error writing to pipe");
    }
}

void run_watcher(char* file) {
    pid_t pid = fork();
    if (pid == -1) {
        err(1, "Error fork starting watcher");
    }
    if (pid == 0) {
        pid_t proc_pid = fork();
        if (proc_pid == -1) {
            err(1, "Error fork starting process");
        } else if (proc_pid == 0) {
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            if (execl(file, file, NULL) == -1) {
                err(1, "Error execl process");
            }
        } else {
            int status;
            waitpid(proc_pid, &status, 0);
            release();
            exit(0);
        }
    }
}

void walk_dir(char* cur_dir) {
    DIR* dir = opendir(cur_dir);
    if (dir == NULL) {
        err(1, "Error opening directory");
    }
    struct dirent* inode;
    while ((inode = readdir(dir)) != NULL) {
        if (strcmp(inode->d_name, ".") == 0 || strcmp(inode->d_name, "..") == 0) {
            continue;
        }
        char cur_path[MAX_PATH_LEN];
        snprintf(cur_path, MAX_PATH_LEN, "%s/%s", cur_dir, inode->d_name);
        if (inode->d_type == DT_DIR) {
            walk_dir(cur_path);
        } else {
            if (check_exec(cur_path) == 0) {
                return;
            }
            acquire();
            run_watcher(cur_path);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    max_parallel_proc_cnt = atoi(argv[1]);
    if (pipe(pipe_fd) == -1) {
        err(1, "Error creating pipe");
    }
    for (int i = 0; i < max_parallel_proc_cnt; ++i) {
        release();
    }
    walk_dir(".");
    while (wait(NULL) > 0);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return 0;
}