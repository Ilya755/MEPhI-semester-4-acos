#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>

#define BUFFER_SIZE 1024
#define FILE_MAX_NAME 256

int logger_fd = -1;
sig_atomic_t rotate = 0;
char* log_filename = NULL;

void sched(int signum) {
    if (signum == SIGHUP) {
        rotate = 1;
    }
}

void create_handler() {
    struct sigaction sa;
    sa.sa_handler = sched;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);
}

void swap_logs(char* logger_name) {
    DIR* dir = opendir(".");
    if (!dir) {
        perror("Error opendir");
        exit(EXIT_FAILURE);
    }
    struct dirent* file;
    int mx_idx = 0;
    size_t base_len = strlen(logger_name);
    while ((file = readdir(dir)) != NULL) {
        char* name = file->d_name;
        if (strncmp(name, logger_name, base_len) != 0 || name[base_len] != '.') {
            continue;
        }
        char* num_ptr = name + base_len + 1;
        int num = atoi(num_ptr);
        if (num > 0 && num < INT_MAX) {
            if (num > mx_idx) {
                mx_idx = num;
            }
        }
    }
    closedir(dir);
    for (int i = mx_idx; i >= 1; --i) {
        char old_name[FILE_MAX_NAME];
        char new_name[FILE_MAX_NAME];
        snprintf(old_name, FILE_MAX_NAME, "%s.%d", logger_name, i);
        snprintf(new_name, FILE_MAX_NAME, "%s.%d", logger_name, i + 1);
        rename(old_name, new_name);
    }
    char new_name[FILE_MAX_NAME];
    snprintf(new_name, FILE_MAX_NAME, "%s.1", logger_name);
    rename(logger_name, new_name);
}

void logger() {
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (rotate) {
            rotate = 0;
            if (logger_fd >= 0) {
                close(logger_fd);
            }
            swap_logs(log_filename);
            logger_fd = open(log_filename, O_WRONLY | O_CREAT, 0644);
            if (logger_fd == -1) {
                perror("Failed to open new logfile after rotation");
                exit(EXIT_FAILURE);
            }
        }
        if (logger_fd == -1) {
            logger_fd = open(log_filename, O_WRONLY | O_CREAT, 0644);
            if (logger_fd == -1) {
                perror("Failed to open logfile");
                exit(EXIT_FAILURE);
            }
        }
        if (write(logger_fd, buffer, strlen(buffer)) == -1) {
            perror("Failed to write to logfile");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]) {
    log_filename = argv[1];
    create_handler();
    logger();
    if (logger_fd >= 0) {
        close(logger_fd);
    }
    return 0;
}