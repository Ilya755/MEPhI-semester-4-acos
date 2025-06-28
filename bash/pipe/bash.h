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

bool syntax_error(char** argv, size_t idx, char* infile_name,
                    char* outfile_name, bool infile_before,
                    bool outfile_before) {
    if (infile_before || outfile_before) {
        do_free_exit(argv, idx, "Syntax error",
                     infile_name, outfile_name);
        return true;
    }
    return false;
}

void Exec(struct Tokenizer* tokenizer) {
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
    bool infile_before = false, outfile_before = false,
            pipe_before = false;
    char* infile_name = NULL;
    char* outfile_name = NULL;
    int outfile_block = -1;
    size_t token_count = 0, block_count = 0;
    for (size_t i = 0; i < words_count; ++i) {
        size_t token_len = cur_token->len;
        switch (cur_token->type) {
            case TT_INFILE:
                if (syntax_error(argv, token_count, infile_name,
                                           outfile_name, infile_before,
                                           outfile_before)) {
                    return;
                }
                if (block_count != 0) {
                    do_free_exit(argv, token_count, "Syntax error",
                                       infile_name, outfile_name);
                    return;
                }
                ++infile_count;
                infile_before = true;
                break;
            case TT_OUTFILE:
                if (syntax_error(argv, token_count, infile_name,
                                    outfile_name, infile_before,
                                    outfile_before)) {
                    return;
                }
                if (outfile_block == -1) {
                    outfile_block = block_count;
                }
                ++outfile_count;
                outfile_before = true;
                break;
            case TT_PIPE:
                if (pipe_before) {
                    do_free_exit(argv, token_count, "Syntax error",
                                       infile_name, outfile_name);
                    return;
                }
                pipe_before = true;
                ++block_count;
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
                pipe_before = false;
                break;
        }
        cur_token = cur_token->next;
    }
    if ((outfile_block != -1 && outfile_block != block_count) ||
        (infile_before || outfile_before) ||
        (infile_count > 1 || outfile_count > 1)) {
        do_free_exit(argv, token_count, "Syntax error",
                     infile_name, outfile_name);
        return;
    }
    argv[token_count] = NULL;
    if (block_count == 0) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            do_free_exit(argv, token_count + 1, NULL,
                            infile_name, outfile_name);
            return;
        } else if (pid == 0) {
            if (outfile_name) {
                int fd = open(outfile_name, O_WRONLY | O_CREAT
                                | O_TRUNC, 0644);
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
            do_free_exit(argv, token_count + 1, NULL,
                            infile_name, outfile_name);
        }
    } else {
        int pipefd[2];
        int prev_pipe_read_end = -1;
        cur_token = tokenizer->head;
        for (size_t i = 0; i < block_count + 1; ++i) {
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                do_free_exit(argv, token_count + 1, NULL,
                             infile_name, outfile_name);
                return;
            }
            do_free(argv, token_count);
            argv = malloc((words_count + 1) * sizeof(char*));
            if (!argv) {
                perror("malloc failed");
                if (infile_name) {
                    free(infile_name);
                }
                if (outfile_name) {
                    free(outfile_name);
                }
                return;
            }
            token_count = 0;
            if (cur_token->type == TT_PIPE) {
                cur_token = cur_token->next;
            }
            while (cur_token != NULL && cur_token->type != TT_PIPE) {
                if (cur_token->type == TT_INFILE) {
                    infile_before = true;
                } else if (cur_token->type == TT_OUTFILE) {
                    outfile_before = true;
                } else if (cur_token->type == TT_WORD) {
                    if (!infile_before && !outfile_before) {
                        size_t token_len = cur_token->len;
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
                }
                cur_token = cur_token->next;
            }
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork failed");
                do_free_exit(argv, token_count, NULL,
                                infile_name, outfile_name);
                return;
            } else if (pid == 0) {
                argv[token_count] = NULL;
                if (i > 0) {
                    dup2(prev_pipe_read_end, STDIN_FILENO);
                    close(prev_pipe_read_end);
                }
                if (i < block_count) {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);
                }
                if (i == 0 && infile_name) {
                    int fd = open(infile_name, O_RDONLY);
                    if (fd == -1) {
                        if (prev_pipe_read_end != -1) {
                            close(prev_pipe_read_end);
                        }
                        dup2(dup_out, STDOUT_FILENO);
                        do_free_exit(argv, token_count + 1, "I/O error",
                                     infile_name, outfile_name);
                        return;
                    }
                    if (dup2(fd, STDIN_FILENO) == -1) {
                        close(fd);
                        do_free_exit(argv, token_count + 1, "I/O error",
                                     infile_name, outfile_name);
                        return;
                    }
                    close(fd);
                    free(infile_name);
                }
                if (i == block_count && outfile_name) {
                    int fd = open(outfile_name, O_WRONLY |
                                    O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        if (prev_pipe_read_end != -1) {
                            close(prev_pipe_read_end);
                        }
                        do_free_exit(argv, token_count + 1, "I/O error",
                                     NULL, outfile_name);
                        return;
                    }
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        close(fd);
                        do_free_exit(argv, token_count + 1, "I/O error",
                                     NULL, outfile_name);
                        return;
                    }
                    close(fd);
                    free(outfile_name);
                }
                if (execvp(argv[0], argv) == -1) {
                    fprintf(stdout, "Command not found\n");
                    if (i == 0) {
                        do_free_exit(argv, token_count + 1, NULL,
                                     infile_name, outfile_name);
                    } else if (i == block_count) {
                        do_free(argv, token_count + 1);
                    } else {
                        do_free(argv, token_count + 1);
                        free(outfile_name);
                    }
                    return;
                }
            } else {
                if (i > 0) {
                    close(prev_pipe_read_end);
                }
                prev_pipe_read_end = pipefd[0];
                close(pipefd[1]);
                wait(NULL);
            }
        }
        do_free_exit(argv, token_count, NULL, infile_name, outfile_name);
    }
}