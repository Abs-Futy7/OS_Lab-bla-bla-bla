#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS  64


void parse_input(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " \n");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \n");
    }
}

// echo
void builtin_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");
}

int has_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0) {
            return 1;
        }
    }
    return 0;
}

// cd
void builtin_cd(char **args) {
    if (args[1] == NULL) {
        printf("cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd failed");
        }
    }
}

//  > file:sends stdout to a file
//  < file:reads stdin from a file

void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {

        if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] == NULL) {
                printf("Error: no file specified after >\n");
                exit(1);
            }
       
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);  
            close(fd);
            args[i] = NULL;     
        }

        
        else if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] == NULL) {
                printf("Error: no file specified after <\n");
                exit(1);
            }
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);  
            close(fd);
            args[i] = NULL;
        }
    }
}

int main() {
    char  input[MAX_INPUT];
    char *args[MAX_ARGS];
    printf("Welcome to MyShell! Type 'exit' to quit.\n");

    while (1) {
        // prompt dekhabe
        printf("myshell> ");
        fflush(stdout);

        // new line input nibe
        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;  

        input[strcspn(input, "\n")] = 0;

        parse_input(input, args);

        if (args[0] == NULL) continue;

        // exit
        if (strcmp(args[0], "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        // cd
        if (strcmp(args[0], "cd") == 0) {
            builtin_cd(args);
            continue;
        }

        // echo
        if (strcmp(args[0], "echo") == 0) {
            if (has_redirection(args)) {
                pid_t ep = fork();
                if (ep == 0) {
                    handle_redirection(args);
                    builtin_echo(args);
                    exit(0);
                }
                else if (ep > 0) {
                    wait(NULL);
                }
                else {
                    perror("fork failed");
                }
            } else {
                builtin_echo(args);
            }
            continue;
        }

        // In the child→ pid= 0 , parent→ pid == child er PID
        
        pid_t pid = fork();

        if (pid == 0) {
            handle_redirection(args);
            if (execvp(args[0], args) == -1) {
                printf("myshell: command not found: %s\n", args[0]);
            }
            exit(1);  
        }
        else if (pid > 0) {
            wait(NULL);
        }
        else {
            perror("fork failed");
        }
    }
    return 0;
}