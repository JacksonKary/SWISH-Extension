#ifndef SWISH_FUNCS_H
#define SWISH_FUNCS_H

/*
 * Divide a string with substrings separated by a single space (" ")
 * into tokens . These tokens should be stored in the 'tokens' vector using
 * "strvec_add".
 * s: String to tokenize
 * vec: Pointer to vector in which to store tokens
 * Returns 0 on success or -1 on error
 */
int tokenize(char *s, strvec_t *tokens);

/*
 * Run a user-specified command (including arguments)
 * This should be called within a CHILD process of the shell
 * tokens: Vector containing tokens input by user into shell
 * Doesn't return on success (similar to exec) or returns -1 on error
 */
int run_command(strvec_t *tokens);

/*
 * Run a sequence of commands in a shell pipeline. For each program 'i' in the
 * sequence, standard input is consumed from the output of program 'i-1' while
 * standard output is sent as the standard input of program 'i+1'. The
 * exceptions are the first program, which does not have a predecessor program
 * from which to consume output, and the last program, which does not have a
 * successor program to which to send output.
 * tokens: Vector containing tokens input by user into shell.
 * Returns 0 on success or -1 on error.
 */
int run_pipelined_commands(strvec_t *tokens);

#endif // SWISH_FUNCS_H
