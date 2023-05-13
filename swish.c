#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"
#include "swish_funcs.h"

#define CMD_LEN 512
#define PROMPT "@> "

int main(int argc, char **argv) {
    strvec_t tokens;
    strvec_init(&tokens);
    char cmd[CMD_LEN];

    printf("%s", PROMPT);
    while (fgets(cmd, CMD_LEN, stdin) != NULL) {
        // Need to remove trailing '\n' from cmd. There are fancier ways.
        int i = 0;
        while (cmd[i] != '\n') {
            i++;
        }
        cmd[i] = '\0';

        if (tokenize(cmd, &tokens) != 0) {
            printf("Failed to parse command\n");
            strvec_clear(&tokens);
            return 1;
        }
        if (tokens.length == 0) {
            printf("%s", PROMPT);
            continue;
        }

        if (strcmp(strvec_get(&tokens, 0), "exit") == 0) {
            strvec_clear(&tokens);
            break;
        }

        else if (strvec_find(&tokens, "|") == -1) {
            printf("Error: This simplified version of swish only supports piped commands\n");
        }

        else {
            // Assume this is a pipeline of programs to run
            run_pipelined_commands(&tokens);
        }

        strvec_clear(&tokens);
        printf("%s", PROMPT);
    }

    return 0;
}
