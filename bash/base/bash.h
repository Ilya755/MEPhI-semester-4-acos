#include "tokenizer.h"
#include <unistd.h>
#include <sys/wait.h>

void Exec(struct Tokenizer* tokenizer) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        size_t token_count = tokenizer->token_count;
        if (token_count == 0) {
            return;
        }
        char** argv = malloc((token_count + 1) * sizeof(char*));
        if (!argv) {
            perror("malloc failed");
            return;
        }
        struct Token* cur_token = tokenizer->head;
        for (size_t i = 0; i < token_count; ++i) {
            size_t token_len = cur_token->len;
            argv[i] = malloc(token_len + 1);
            if (!argv[i]) {
                perror("malloc failed");
                for (size_t j = 0; j < i; ++j) {
                    free(argv[j]);
                }
                free(argv);
                return;
            }
            memcpy(argv[i], cur_token->start, token_len);
            argv[i][token_len] = '\0';
            cur_token = cur_token->next;
        }
        argv[token_count] = NULL;
        if (execvp(argv[0], argv) == -1) {
            fprintf(stdout, "Command not found\n");
            for (size_t i = 0; i < token_count; ++i) {
                free(argv[i]);
            }
            free(argv);
        }
    } else {
        wait(NULL);
    }
}
