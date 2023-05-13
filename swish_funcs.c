#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"
#include "swish_funcs.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipes: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipes, int in_idx, int out_idx) {
    // redirect process input/output with dup2 to the appropriate pipe end (only if necessary)
    if (in_idx > -1) {  // only redirect STDIN if the command's input is supposed to be redirected
        if (dup2(pipes[in_idx], STDIN_FILENO) == -1) {  // redirect command's standard input to read from the pipe instead
            perror("dup2");
            return -1;
        }
    }
    if (out_idx > -1) {  // only redirect STDOUT if the command's output is supposed to be redirected
        if (dup2(pipes[out_idx], STDOUT_FILENO) == -1) {  // redirect command's standard output to write to the pipe instead
            perror("dup2");
            return -1;
        }
    }
    // run and execute the command, check for errors
    if (run_command(tokens) == -1) {
        printf("run_command error");
        if (in_idx > -1) {
            if (close(pipes[in_idx]) == -1) {
                perror("close1111");
            }
        }
        if (out_idx > -1) {
            if (close(pipes[out_idx]) == -1) {
                perror("close2222");
            }
        }
        return -1;
    }
    // close read/write of pipes we actually used
    int ret_val = 0;
    if (in_idx > -1) {
        if (close(pipes[in_idx]) == -1) {
            perror("close");
            close(pipes[in_idx]);
            ret_val = 1;
        }
    }
    if (out_idx > -1) {
        if (close(pipes[out_idx]) == -1) {
            perror("close");
            close(pipes[out_idx]);
            ret_val = 1;
        }
    }
    return ret_val;
}

// does most of the heavy lifting: processes tokens, forks children, closes most of the pipes, etc.
int run_pipelined_commands(strvec_t *tokens) {
    // count # pipes in tokens and allocate pipes array appropriately
    int num_pipes = strvec_num_occurrences(tokens, "|");
    int *pipe_fds = malloc(2 * sizeof(int) * num_pipes);  // allocate the pipes
    if (pipe_fds == NULL) {  // error check malloc
        fprintf(stderr, "malloc failed\n");
        return -1;
    }
    // set up all pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fds + (2 * i)) == -1) {
            perror("pipe");
            for (int j = 0; j < i; j++) {  // if error, close all created pipes
                if (close(pipe_fds[2*j]) == -1) {
                    free(pipe_fds);
                    return -1;  // nested error
                }
                if (close(pipe_fds[2*j + 1]) == -1) {
                    free(pipe_fds);
                    return -1;  // nested error
                }
            }
            free(pipe_fds);
            return -1;
        }
    }
    // command forking loop
    for (int i = num_pipes; i >= 0; i--) {  // loop "backwards" through the commands
        // values to pass to run_piped_command()
        int in_idx = 2 * (i - 1);  // index of the read end of the input pipe
        int out_idx = (2 * (i + 1)) - 1;  // index of the write end of the output pipe

        if (i == 0) {  // if first command, set in_idx = -1 to indicate that it shouldn't redirect its input
            in_idx = -1;
        } else if (i == num_pipes) {  // if last command, set out_idx = -1 to indicate that it shouldn't redirect its output
            out_idx = -1;
        }
        // fork a child process to call run_piped_command()
        pid_t child_pid = fork();
        if (child_pid == -1) {  // check for fork error, cleanup
            perror("fork");
            for (int j = 0; j < num_pipes; j++) {
                if (close(pipe_fds[2*j]) == -1) {
                    exit(1); // nested error
                }
                if (close(pipe_fds[2*j + 1]) == -1) {
                    exit(1); // nested error
                }
            }
            exit(1);  // exit process with error, should be noticed by the waiting parent
        } else if (child_pid == 0) {  // child process
            // close unused pipes and handle errors
            for (int j = 0; j < num_pipes; j++) {  // close all but 2 pipe ends we need
                if ((2 * j != in_idx) && close(pipe_fds[2 * j]) == -1) {
                    perror("close");
                    for (int k = 2 * j; k < 2 * num_pipes; k++) {  // make sure every pipe available is closed before child terminates
                        if (close(pipe_fds[k]) == -1) {
                            exit(1); // nested error
                        }
                    }
                    exit(1);  // exit process with error, should be noticed by the waiting parent
                }
                if ((2 * j + 1) != out_idx && close(pipe_fds[2 * j + 1]) == -1) {
                    perror("close");
                    for (int k = 2 * j + 1; k < 2 * num_pipes; k++) {  // make sure every pipe available is closed before child terminates
                        if (close(pipe_fds[k]) == -1) {
                            exit(1); // nested error
                        }
                    }
                    exit(1);  // exit process with error, should be noticed by the waiting parent
                }
            }
            // trim tokens, extract the command token, call run_piped_command
            int tokenator = num_pipes;
            int idx;
            int end_target_token = tokens->length;  // exclusive right-bound of our target command token
            while (tokenator > i) {  // each child starts with unique copy of the full tokens strvec, so we need to chop off the last (num_pipes - i) commands
                if ((idx = strvec_find_last(tokens, "|")) == -1) {  // pipe char not found, there should be one if this loop was entered
                    strvec_clear(tokens);
                    if (in_idx > -1) {  // if input was redirected to a pipe, close that pipe
                        if (close(pipe_fds[in_idx]) == -1) { 
                            perror("close");
                            // continue to close out_idx before exiting
                        }
                    }
                    if (out_idx > -1) {  // if output was redirected to a pipe, close that pipe
                        if (close(pipe_fds[out_idx]) == -1) { 
                            perror("close");
                        }
                    }
                    exit(1);  // exit process with error, should be noticed by the waiting parent 
                }
                end_target_token = idx;  // update end_target_token
                strvec_take(tokens, end_target_token);  // chop off pipe character, its preceding space, and the last token
                tokenator--;
            }
            int remaining = strvec_num_occurrences(tokens, "|");
            if (remaining > 0) {  // not first command, still need to find beginning of target command from the last |
                strvec_t command_token;
                idx = strvec_find_last(tokens, "|");  // index of | preceding target command
                idx++;
                if (strvec_slice(tokens, &command_token, idx, end_target_token) == -1) {  // command_token = " <target_command>"
                    strvec_clear(&command_token);
                    if (in_idx > -1) {  // if input was redirected to a pipe, close that pipe
                        if (close(pipe_fds[in_idx]) == -1) { 
                            perror("close");
                            // continue to close out_idx before exiting
                        }
                    }
                    if (out_idx > -1) {  // if output was redirected to a pipe, close that pipe
                        if (close(pipe_fds[out_idx]) == -1) { 
                            perror("close");
                        }
                    }
                    exit(1);  // exit process with error, should be noticed by the waiting parent 
                }
                if (run_piped_command(&command_token, pipe_fds, num_pipes, in_idx, out_idx) == -1) {  // send command_token off to be ran
                    strvec_clear(&command_token);
                    if (in_idx > -1) {  // if input was redirected to a pipe, close that pipe
                        if (close(pipe_fds[in_idx]) == -1) { 
                            perror("close");
                            // continue to close out_idx before exiting
                        }
                    }
                    if (out_idx > -1) {  // if output was redirected to a pipe, close that pipe
                        if (close(pipe_fds[out_idx]) == -1) { 
                            perror("close");
                        }
                    }
                    exit(1);  // exit process with error, should be noticed by the waiting parent 
                }
                // piped command ran successfully
                strvec_clear(&command_token);
            } else {  // first token
                if (run_piped_command(tokens, pipe_fds, num_pipes, in_idx, out_idx) == -1) {  // send first token off to be ran
                    strvec_clear(tokens);
                    if (in_idx > -1) {  // if input was redirected to a pipe, close that pipe
                        if (close(pipe_fds[in_idx]) == -1) { 
                            perror("close");
                            // continue to close out_idx before exiting
                        }
                    }
                    if (out_idx > -1) {  // if output was redirected to a pipe, close that pipe
                        if (close(pipe_fds[out_idx]) == -1) { 
                            perror("close");
                        }
                    }
                    exit(1);  // exit process with error, should be noticed by the waiting parent 
                }
            }
            // relevant pipe ends closed after use in run_pipelined_command
            exit(0);  // exit normally unless failed to close in_idx/out_idx
        }  // end of child process
    }  // end of command loop

    // close all pipes in parent ASAP
    for (int i = 0; i < 2 * num_pipes; i++) {
        if (close(pipe_fds[i]) == -1) {
            perror("close");
            close(pipe_fds[i]);
            free(pipe_fds);
            return -1;
        }
    }
    
    // wait for all children to finish, check their exit status for errors
    int status;
    int ret_val = 0;
    for (int i = 0; i < (num_pipes + 1); i++) {
        wait(&status);
        if (WIFEXITED(status)) {  // check if exited normally
            int child_ret_val = WEXITSTATUS(status);  // check the return value
            if (child_ret_val != 0) {  // if child exited abnormally, return error
                ret_val = -1;  // set ret_val to avoid redundant freeing and closing, which will happen next anyway
            }
        } else {  // child process terminated abnormally
            ret_val = -1;  // set ret_val to avoid redundant freeing and closing, which will happen next anyway
        }
    }

    free(pipe_fds);  // free pipe array
    return ret_val;
}
