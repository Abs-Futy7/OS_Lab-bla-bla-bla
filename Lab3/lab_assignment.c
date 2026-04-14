/*
 * Custom Shell - OS Lab Assignment [SH_01]
 *
 * Side-by-side quick view:
 * User command               -> Internal flow
 * ls -l                      -> parse -> fork -> execvp -> wait
 * cd folder                  -> builtin_cd -> chdir
 * echo hello                 -> builtin_echo
 * echo hello > a.txt         -> detect redirection -> fork -> dup2 -> echo
 * cat < a.txt                -> fork -> dup2(STDIN) -> execvp(cat)
 * exit                       -> break loop
 */

#include <stdio.h>      // printf, fgets
#include <stdlib.h>     // exit
#include <string.h>     // strtok, strcmp, strcspn
#include <unistd.h>     // fork, execvp, chdir, dup2
#include <sys/wait.h>   // wait
#include <fcntl.h>      // open, O_* flags

#define MAX_INPUT 1024  // max input line length
#define MAX_ARGS  64    // max token count in one command

/*
 * parse_input()
 * Parse user input into argv-style tokens.
 * Example: "ls -l /home" -> args[0]="ls", args[1]="-l", args[2]="/home", NULL
 */
void parse_input(char *input, char **args) {
    int i = 0;                               // token index
    args[i] = strtok(input, " \n");         // first token (command)
    while (args[i] != NULL) {                // continue until no token left
        i++;                                 // move to next slot
        args[i] = strtok(NULL, " \n");      // get next token
    }
}

/*
 * builtin_echo()
 * Built-in implementation of echo command.
 */
void builtin_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) {  // start from 1 to skip "echo"
        printf("%s ", args[i]);              // print each argument + space
    }
    printf("\n");                            // final newline
}

/*
 * has_redirection()
 * Returns 1 if token list contains '>' or '<', otherwise 0.
 */
int has_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {                        // scan all tokens
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0) { // found redirection symbol
            return 1;                                               // yes, redirection exists
        }
    }
    return 0;                                  // no redirection symbol found
}

/*
 * builtin_cd()
 * Built-in implementation of cd command.
 * Uses chdir() so directory change affects the shell process.
 */
void builtin_cd(char **args) {
    if (args[1] == NULL) {                     // user typed only "cd"
        printf("cd: missing argument\n");      // show usage error
    } else {
        if (chdir(args[1]) != 0) {             // try to change directory
            perror("cd failed");               // print OS-level error reason
        }
    }
}

/*
 * handle_redirection()
 * Supports:
 *   command > file   : redirect stdout to file
 *   command < file   : redirect stdin from file
 *
 * args[i] = NULL cuts command arguments before redirection symbols.
 */
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {    // scan tokens for > or <

        if (strcmp(args[i], ">") == 0) {      // output redirection case
            if (args[i + 1] == NULL) {         // no filename after >
                printf("Error: no file specified after >\n");
                exit(1);                        // child exits with error
            }

            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // open target file
            if (fd < 0) {                       // file open failed
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);            // redirect stdout to file descriptor
            close(fd);                          // close original descriptor after dup2
            args[i] = NULL;                     // cut argv at > so exec/echo ignores redirection tokens
        }

        else if (strcmp(args[i], "<") == 0) { // input redirection case
            if (args[i + 1] == NULL) {          // no filename after <
                printf("Error: no file specified after <\n");
                exit(1);
            }
            int fd = open(args[i + 1], O_RDONLY); // open source file for reading
            if (fd < 0) {                        // file open failed
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);              // redirect stdin from file descriptor
            close(fd);                           // close original descriptor after dup2
            args[i] = NULL;                      // cut argv at <
        }
    }
}

/*
 * main()
 * REPL loop:
 * 1) show prompt
 * 2) read input
 * 3) parse
 * 4) run built-ins
 * 5) fork + exec external command
 * 6) parent waits
 */
int main() {
    char  input[MAX_INPUT];                    // raw input buffer
    char *args[MAX_ARGS];                      // token array (argv-like)

    printf("Welcome to MyShell! Type 'exit' to quit.\n"); // startup message

    while (1) {                                // main REPL loop
        printf("myshell> ");                  // show shell prompt
        fflush(stdout);                        // force prompt display immediately

        if (fgets(input, MAX_INPUT, stdin) == NULL) // read one command line
            break;                             // EOF (Ctrl+D) ends shell

        input[strcspn(input, "\n")] = 0;      // remove trailing newline

        parse_input(input, args);              // split line into tokens

        if (args[0] == NULL) continue;         // skip empty input

        if (strcmp(args[0], "exit") == 0) {   // built-in: exit
            printf("Goodbye!\n");
            break;                             // terminate shell loop
        }

        if (strcmp(args[0], "cd") == 0) {     // built-in: cd
            builtin_cd(args);                  // change parent shell directory
            continue;                          // go next prompt
        }

        if (strcmp(args[0], "echo") == 0) {   // built-in: echo
            if (has_redirection(args)) {       // echo with > or < needs child
                pid_t ep = fork();             // create child process for redirected echo
                if (ep == 0) {                 // child path
                    handle_redirection(args);  // set up stdin/stdout redirection
                    builtin_echo(args);        // run echo with cleaned args
                    exit(0);                   // child completes successfully
                }
                else if (ep > 0) {             // parent path
                    wait(NULL);                // wait for redirected echo child
                }
                else {                         // fork failed
                    perror("fork failed");
                }
            } else {
                builtin_echo(args);            // normal echo without redirection
            }
            continue;                          // do not fall into external command path
        }

        pid_t pid = fork();                    // fork for external command execution

        if (pid == 0) {                        // child process block
            handle_redirection(args);          // apply > or < if present
            if (execvp(args[0], args) == -1) { // replace child with command program
                printf("myshell: command not found: %s\n", args[0]); // if exec fails
            }
            exit(1);                           // exit child if command failed
        }
        else if (pid > 0) {                    // parent process block
            wait(NULL);                        // wait until child command finishes
        }
        else {                                 // fork failed in parent
            perror("fork failed");
        }
    }

    return 0;                                  // clean program exit
}