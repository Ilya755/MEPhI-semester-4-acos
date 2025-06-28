#include "tokenizer.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

void do_free(char** argv, size_t idx) {
    for (size_t j = 0; j < idx; ++j) {
        free(argv[j]);
    }
    free(argv);
}

void do_free_exit(char** argv, size_t idx, char* err_msg,
                  char* infile_name, char* outfile_name) {
    if (err_msg) {
        fprintf(stdout, "%s\n", err_msg);
    }
    do_free(argv, idx);
    if (infile_name) {
        free(infile_name);
    }
    if (outfile_name) {
        free(outfile_name);
    }
}

bool syntax_error(char** argv, size_t idx, char* infile_name, char* outfile_name,
                  bool infile_before, bool outfile_before) {
    if (infile_before || outfile_before) {
        do_free_exit(argv, idx, "Syntax error",
                     infile_name, outfile_name);
        return true;
    }
    return false;
}

void Exec(struct Tokenizer* tokenizer) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        size_t words_count = tokenizer->token_count;
        if (words_count == 0) {
            return;
        }
        int dup_out = dup(STDOUT_FILENO);
        char** argv = malloc((words_count + 1) * sizeof(char*));
        if (!argv) {
            perror("malloc failed");
            return;
        }
        struct Token* cur_token = tokenizer->head;
        size_t infile_count = 0, outfile_count = 0;
        bool infile_before = false, outfile_before = false;
        char* infile_name = NULL;
        char* outfile_name = NULL;
        size_t token_count = 0;
        for (size_t i = 0; i < words_count; ++i) {
            size_t token_len = cur_token->len;
            switch (cur_token->type) {
                case TT_INFILE:
                    if (syntax_error(argv, token_count, infile_name, outfile_name,
                                     infile_before, outfile_before)) {
                        return;
                    }
                    ++infile_count;
                    infile_before = true;
                    break;
                case TT_OUTFILE:
                    if (syntax_error(argv, token_count, infile_name, outfile_name,
                                     infile_before, outfile_before)) {
                        return;
                    }
                    ++outfile_count;
                    outfile_before = true;
                    break;
                case TT_WORD:
                    if (infile_before) {
                        if (infile_name) {
                            free(infile_name);
                        }
                        infile_name = malloc((token_len + 1) * sizeof(char));
                        if (!infile_name) {
                            perror("malloc failed");
                            do_free_exit(argv, token_count, NULL,
                                         NULL, outfile_name);
                            return;
                        }
                        memcpy(infile_name, cur_token->start, token_len);
                        infile_name[token_len] = '\0';
                    } else if (outfile_before) {
                        if (outfile_name) {
                            free(outfile_name);
                        }
                        outfile_name = malloc((token_len + 1) * sizeof(char));
                        if (!outfile_name) {
                            perror("malloc failed");
                            do_free_exit(argv, token_count, NULL,
                                         infile_name, NULL);
                            return;
                        }
                        memcpy(outfile_name, cur_token->start, token_len);
                        outfile_name[token_len] = '\0';
                    } else {
                        argv[token_count] = malloc((token_len + 1) * sizeof(char));
                        if (!argv[token_count]) {
                            perror("malloc failed");
                            do_free_exit(argv, token_count, NULL,
                                         infile_name, outfile_name);
                            return;
                        }
                        memcpy(argv[token_count], cur_token->start, token_len);
                        argv[token_count][token_len] = '\0';
                        ++token_count;
                    }
                    infile_before = false;
                    outfile_before = false;
                    break;
            }
            cur_token = cur_token->next;
        }
        if (infile_before || outfile_before) {
            do_free_exit(argv, token_count, "Syntax error",
                         infile_name, outfile_name);
            return;
        }
        argv[token_count] = NULL;
        if (infile_count > 1 || outfile_count > 1) {
            do_free_exit(argv, token_count + 1, "Syntax error",
                         infile_name, outfile_name);
            return;
        }
        if (outfile_name) {
            int fd = open(outfile_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                do_free_exit(argv, token_count + 1, "I/O error",
                             infile_name, outfile_name);
                return;
            }
            if (fd != STDOUT_FILENO && dup2(fd, STDOUT_FILENO) == -1) {
                close(fd);
                do_free_exit(argv, token_count + 1, "I/O error",
                             infile_name, outfile_name);
                return;
            }
            close(fd);
            free(outfile_name);
        }
        if (infile_name) {
            int fd = open(infile_name, O_RDONLY);
            if (fd == -1) {
                dup2(dup_out, STDOUT_FILENO);
                do_free_exit(argv, token_count + 1, "I/O error",
                             infile_name, NULL);
                return;
            }
            if (fd != STDIN_FILENO && dup2(fd, STDIN_FILENO) == -1) {
                close(fd);
                do_free_exit(argv, token_count + 1, "I/O error",
                             infile_name, NULL);
                return;
            }
            close(fd);
            free(infile_name);
        }
        if (execvp(argv[0], argv) == -1) {
            fprintf(stdout, "Command not found\n");
            do_free(argv, token_count + 1);
            return;
        }
    } else {
        wait(NULL);
    }
}