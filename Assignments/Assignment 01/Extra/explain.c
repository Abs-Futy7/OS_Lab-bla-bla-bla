#include <stdio.h>      // printf, fgets, perror, fputs
#include <stdlib.h>     // exit
#include <string.h>     // strcmp, strtok, strlen, strcspn, snprintf
#include <unistd.h>     // chdir, getcwd, fork, execvp, dup2, read, write, close
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/stat.h>   // stat, mkdir
#include <fcntl.h>      // open and file flags
#include <errno.h>      // errno
#include <time.h>       // time, localtime, strftime
#include <utime.h>      // utime for touch timestamp update
#include <pwd.h>        // getpwuid for username in ls -l
#include <grp.h>        // getgrgid for group name in ls -l
#include <ctype.h>      // character utilities (not heavily used here)

// Maximum input line size
#define INPUT_SIZE 2048

// Maximum number of tokens after splitting input
#define MAX_TOKENS 256

// Maximum number of arguments in a single command
#define MAX_ARGS 128

// Maximum number of commands connected by pipes
#define MAX_CMDS 32

// General buffer size for file copy / file read operations
#define BUFFER_SIZE 1024

// Structure to store one parsed command
typedef struct {
    char *argv[MAX_ARGS];   // command + arguments, execvp style
    char *input_file;       // file used after <
    char *output_file;      // file used after >
} Command;

// Remove trailing newline from input string
void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

// Check whether a command is one of our supported built-ins
int is_builtin(const char *cmd) {
    if (cmd == NULL) return 0;

    return strcmp(cmd, "cd") == 0 ||
           strcmp(cmd, "echo") == 0 ||
           strcmp(cmd, "exit") == 0 ||
           strcmp(cmd, "pwd") == 0 ||
           strcmp(cmd, "ls") == 0 ||
           strcmp(cmd, "mkdir") == 0 ||
           strcmp(cmd, "touch") == 0 ||
           strcmp(cmd, "rm") == 0 ||
           strcmp(cmd, "cp") == 0 ||
           strcmp(cmd, "mv") == 0 ||
           strcmp(cmd, "cat") == 0;
}

// Count how many arguments exist in a NULL-terminated argv array
int count_args(char *args[]) {
    int count = 0;
    while (args[count] != NULL) {
        count++;
    }
    return count;
}

// Print file permissions like rwxr-xr-x for ls -l
void print_permissions(mode_t mode) {
    // First character: d for directory, - for regular file
    printf((S_ISDIR(mode)) ? "d" : "-");

    // User permissions
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    // Group permissions
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    // Others permissions
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

// Add spaces around special shell symbols so tokenization becomes easier
// Example: ls|cat becomes ls | cat
void preprocess_input(const char *input, char *output) {
    int i = 0; // index for input
    int j = 0; // index for output

    while (input[i] != '\0') {
        // If special symbol is found, add spaces around it
        if (input[i] == '|' || input[i] == '<' || input[i] == '>' || input[i] == '&') {
            output[j++] = ' ';
            output[j++] = input[i];
            output[j++] = ' ';
        } else {
            output[j++] = input[i];
        }
        i++;
    }

    // End the processed string
    output[j] = '\0';
}

// Split input string into tokens using space and tab as delimiters
int tokenize(char *input, char *tokens[]) {
    int count = 0;

    // Get first token
    char *token = strtok(input, " \t");

    // Continue until no more tokens
    while (token != NULL && count < MAX_TOKENS - 1) {
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }

    // Mark end of token list
    tokens[count] = NULL;
    return count;
}

// Initialize a Command structure
void init_command(Command *cmd) {
    int i;

    // Set all argument pointers to NULL
    for (i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }

    // No redirection initially
    cmd->input_file = NULL;
    cmd->output_file = NULL;
}

/*
   Parse token list into one or more Command structures.
   Supports:
   - normal arguments
   - pipe |
   - input redirection <
   - output redirection >
   - background execution &
*/
int parse_commands(char *tokens[], int token_count, Command commands[], int *background) {
    int cmd_index = 0;  // current command number
    int arg_index = 0;  // current argument position in that command
    int i;

    // Default: foreground execution
    *background = 0;

    // Initialize first command
    init_command(&commands[cmd_index]);

    for (i = 0; i < token_count; i++) {
        // Pipe means current command ends, next command begins
        if (strcmp(tokens[i], "|") == 0) {
            // Error if nothing before pipe
            if (arg_index == 0) {
                fprintf(stderr, "Syntax error: empty command before pipe\n");
                return -1;
            }

            // End argv of current command
            commands[cmd_index].argv[arg_index] = NULL;

            // Move to next command
            cmd_index++;

            // Prevent too many piped commands
            if (cmd_index >= MAX_CMDS) {
                fprintf(stderr, "Too many piped commands\n");
                return -1;
            }

            // Initialize new command and reset argument index
            init_command(&commands[cmd_index]);
            arg_index = 0;
        }

        // Input redirection <
        else if (strcmp(tokens[i], "<") == 0) {
            // Need one more token for file name
            if (i + 1 >= token_count) {
                fprintf(stderr, "Syntax error: missing input file\n");
                return -1;
            }

            // Store input file name
            commands[cmd_index].input_file = tokens[++i];
        }

        // Output redirection >
        else if (strcmp(tokens[i], ">") == 0) {
            // Need one more token for file name
            if (i + 1 >= token_count) {
                fprintf(stderr, "Syntax error: missing output file\n");
                return -1;
            }

            // Store output file name
            commands[cmd_index].output_file = tokens[++i];
        }

        // Background execution &
        else if (strcmp(tokens[i], "&") == 0) {
            // & must appear only at the end
            if (i != token_count - 1) {
                fprintf(stderr, "Syntax error: '&' must be at the end\n");
                return -1;
            }

            *background = 1;
        }

        // Normal argument
        else {
            // Prevent too many arguments
            if (arg_index >= MAX_ARGS - 1) {
                fprintf(stderr, "Too many arguments\n");
                return -1;
            }

            commands[cmd_index].argv[arg_index++] = tokens[i];
        }
    }

    // No actual command found
    if (arg_index == 0 && cmd_index == 0) {
        return 0;
    }

    // End final command argv list
    commands[cmd_index].argv[arg_index] = NULL;

    // Return total number of commands
    return cmd_index + 1;
}

/* =========================
   Built-in command handlers
   ========================= */

// echo: print all arguments after the command name
int builtin_echo(char *args[]) {
    int i = 1; // skip args[0] = "echo"

    while (args[i] != NULL) {
        printf("%s", args[i]);

        // Print a space between words
        if (args[i + 1] != NULL) {
            printf(" ");
        }

        i++;
    }

    printf("\n");
    return 0;
}

// pwd: print current working directory
int builtin_pwd(char *args[]) {
    // pwd should not take extra arguments
    if (count_args(args) > 1) {
        fprintf(stderr, "pwd: too many arguments\n");
        return 1;
    }

    char cwd[INPUT_SIZE];

    // Get current directory path
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return 1;
    }

    printf("%s\n", cwd);
    return 0;
}

// ls and ls -l and directory argument support
int builtin_ls(char *args[]) {
    int argc = count_args(args);
    int long_format = 0;   // 0 = normal ls, 1 = ls -l
    char *path = ".";      // default path is current directory

    // Case: ls
    if (argc == 1) {
        long_format = 0;
    }

    // Case: ls -l OR ls folder
    else if (argc == 2) {
        if (strcmp(args[1], "-l") == 0) {
            long_format = 1;
        } else {
            path = args[1];
        }
    }

    // Case: ls -l folder
    else if (argc == 3) {
        if (strcmp(args[1], "-l") == 0) {
            long_format = 1;
            path = args[2];
        } else {
            fprintf(stderr, "ls: invalid arguments\n");
            fprintf(stderr, "Usage: ls OR ls -l OR ls <dir> OR ls -l <dir>\n");
            return 1;
        }
    }

    // Too many arguments
    else {
        fprintf(stderr, "ls: too many arguments\n");
        return 1;
    }

    // Open the target directory
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("ls");
        return 1;
    }

    struct dirent *entry;

    // Read directory entries one by one
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Normal ls: print only names
        if (!long_format) {
            printf("%s  ", entry->d_name);
        }

        // ls -l: print detailed info
        else {
            char fullpath[INPUT_SIZE];

            // Build full path like folder/file.txt
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            struct stat st;

            // Get file metadata
            if (stat(fullpath, &st) == -1) {
                perror("ls stat");
                continue;
            }

            // Print permission string
            print_permissions(st.st_mode);

            // Print link count
            printf(" %ld", (long)st.st_nlink);

            // Convert uid/gid to human-readable names
            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);

            // Print owner and group
            printf(" %s", pw ? pw->pw_name : "unknown");
            printf(" %s", gr ? gr->gr_name : "unknown");

            // Print file size
            printf(" %5ld", (long)st.st_size);

            // Format modification time
            char timebuf[64];
            struct tm *tm_info = localtime(&st.st_mtime);
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);

            // Print time and name
            printf(" %s", timebuf);
            printf(" %s\n", entry->d_name);
        }
    }

    // For normal ls, end the line after printing all names
    if (!long_format) {
        printf("\n");
    }

    closedir(dir);
    return 0;
}

// cd: change directory
int builtin_cd(char *args[]) {
    int argc = count_args(args);

    // If only "cd", go to HOME directory
    if (argc == 1) {
        char *home = getenv("HOME");

        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }

        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }

        return 0;
    }

    // Too many arguments
    if (argc > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }

    // Change to requested directory
    if (chdir(args[1]) != 0) {
        perror("cd");
        return 1;
    }

    return 0;
}

// mkdir: create directory
int builtin_mkdir(char *args[]) {
    int argc = count_args(args);

    if (argc < 2) {
        fprintf(stderr, "mkdir: missing directory name\n");
        return 1;
    }

    if (argc > 2) {
        fprintf(stderr, "mkdir: too many arguments\n");
        return 1;
    }

    if (mkdir(args[1], 0777) != 0) {
        perror("mkdir");
        return 1;
    }

    return 0;
}

// touch: create file if missing, update timestamps if it exists
int builtin_touch(char *args[]) {
    int argc = count_args(args);

    if (argc < 2) {
        fprintf(stderr, "touch: missing file name\n");
        return 1;
    }

    if (argc > 2) {
        fprintf(stderr, "touch: too many arguments\n");
        return 1;
    }

    // Open file in write mode, create if not exists
    int fd = open(args[1], O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        perror("touch");
        return 1;
    }
    close(fd);

    // Set current access and modification time
    struct utimbuf new_times;
    new_times.actime = time(NULL);
    new_times.modtime = time(NULL);

    if (utime(args[1], &new_times) != 0) {
        perror("touch");
        return 1;
    }

    return 0;
}

// rm: remove file
int builtin_rm(char *args[]) {
    int argc = count_args(args);

    if (argc < 2) {
        fprintf(stderr, "rm: missing file name\n");
        return 1;
    }

    if (argc > 2) {
        fprintf(stderr, "rm: too many arguments\n");
        return 1;
    }

    if (remove(args[1]) != 0) {
        perror("rm");
        return 1;
    }

    return 0;
}

// mv: move or rename file
int builtin_mv(char *args[]) {
    int argc = count_args(args);

    if (argc < 3) {
        fprintf(stderr, "mv: missing source or destination\n");
        return 1;
    }

    if (argc > 3) {
        fprintf(stderr, "mv: too many arguments\n");
        return 1;
    }

    if (rename(args[1], args[2]) != 0) {
        perror("mv");
        return 1;
    }

    return 0;
}

// cat: display file contents
int builtin_cat(char *args[]) {
    int argc = count_args(args);

    if (argc < 2) {
        fprintf(stderr, "cat: missing file name\n");
        return 1;
    }

    if (argc > 2) {
        fprintf(stderr, "cat: too many arguments\n");
        return 1;
    }

    // Open file in read mode
    FILE *fp = fopen(args[1], "r");
    if (fp == NULL) {
        perror("cat");
        return 1;
    }

    // Read file line by line
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        fputs(buffer, stdout);
    }

    fclose(fp);
    return 0;
}

// cp: copy one file to another
int builtin_cp(char *args[]) {
    int argc = count_args(args);

    if (argc < 3) {
        fprintf(stderr, "cp: missing source or destination\n");
        return 1;
    }

    if (argc > 3) {
        fprintf(stderr, "cp: too many arguments\n");
        return 1;
    }

    // Open source file for reading
    int src = open(args[1], O_RDONLY);
    if (src < 0) {
        perror("cp source");
        return 1;
    }

    // Open destination file for writing, create if needed, overwrite if exists
    int dest = open(args[2], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dest < 0) {
        perror("cp destination");
        close(src);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read source file block by block
    while ((bytes_read = read(src, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;

        // Keep writing until all bytes from this block are written
        while (total_written < bytes_read) {
            ssize_t bytes_written = write(dest, buffer + total_written, bytes_read - total_written);

            if (bytes_written < 0) {
                perror("cp write");
                close(src);
                close(dest);
                return 1;
            }

            total_written += bytes_written;
        }
    }

    if (bytes_read < 0) {
        perror("cp read");
        close(src);
        close(dest);
        return 1;
    }

    close(src);
    close(dest);
    return 0;
}

// Execute the correct built-in command based on command name
int execute_builtin(char *args[]) {
    if (args[0] == NULL) return 0;

    if (strcmp(args[0], "echo") == 0) return builtin_echo(args);
    if (strcmp(args[0], "pwd") == 0) return builtin_pwd(args);
    if (strcmp(args[0], "ls") == 0) return builtin_ls(args);
    if (strcmp(args[0], "cd") == 0) return builtin_cd(args);
    if (strcmp(args[0], "mkdir") == 0) return builtin_mkdir(args);
    if (strcmp(args[0], "touch") == 0) return builtin_touch(args);
    if (strcmp(args[0], "rm") == 0) return builtin_rm(args);
    if (strcmp(args[0], "mv") == 0) return builtin_mv(args);
    if (strcmp(args[0], "cat") == 0) return builtin_cat(args);
    if (strcmp(args[0], "cp") == 0) return builtin_cp(args);

    return -1;
}

// Apply input/output redirection for a command
void apply_redirection(Command *cmd) {
    // Handle input redirection: command < file
    if (cmd->input_file != NULL) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror("input redirection");
            exit(1);
        }

        // Replace standard input with this file
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 input");
            close(fd);
            exit(1);
        }

        close(fd);
    }

    // Handle output redirection: command > file
    if (cmd->output_file != NULL) {
        int fd = open(cmd->output_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) {
            perror("output redirection");
            exit(1);
        }

        // Replace standard output with this file
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            close(fd);
            exit(1);
        }

        close(fd);
    }
}

// Execute one command when there is no pipe
void execute_single_command(Command *cmd, int background) {
    // Ignore empty command
    if (cmd->argv[0] == NULL) return;

    // exit must terminate the shell itself
    if (strcmp(cmd->argv[0], "exit") == 0) {
        if (count_args(cmd->argv) > 1) {
            fprintf(stderr, "exit: too many arguments\n");
            return;
        }
        exit(0);
    }

    // cd must run in parent process because it changes shell's current directory
    if (strcmp(cmd->argv[0], "cd") == 0 &&
        cmd->input_file == NULL &&
        cmd->output_file == NULL &&
        !background) {
        builtin_cd(cmd->argv);
        return;
    }

    // Other built-ins without redirection/background can run directly
    if (is_builtin(cmd->argv[0]) &&
        cmd->input_file == NULL &&
        cmd->output_file == NULL &&
        !background &&
        strcmp(cmd->argv[0], "cd") != 0) {
        execute_builtin(cmd->argv);
        return;
    }

    // For external commands or commands needing child context, create child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    // Child process
    if (pid == 0) {
        // Apply < or >
        apply_redirection(cmd);

        // exit inside child context is not allowed here
        if (strcmp(cmd->argv[0], "exit") == 0) {
            fprintf(stderr, "exit cannot be used in child context\n");
            exit(1);
        }

        // If built-in, run it inside child then exit
        if (is_builtin(cmd->argv[0])) {
            int rc = execute_builtin(cmd->argv);
            exit(rc == 0 ? 0 : 1);
        }

        // Otherwise run external command
        execvp(cmd->argv[0], cmd->argv);

        // Only runs if execvp fails
        perror("execvp");
        exit(1);
    }

    // Parent process
    else {
        if (!background) {
            // Wait for child in foreground mode
            waitpid(pid, NULL, 0);
        } else {
            // Do not wait in background mode
            printf("[background pid %d]\n", pid);
        }
    }
}

// Execute multiple commands connected by pipes
void execute_pipeline(Command commands[], int ncmds, int background) {
    int pipes[MAX_CMDS - 1][2]; // store pipe fds
    pid_t pids[MAX_CMDS];       // store child process ids
    int i, j;

    // Create ncmds - 1 pipes
    for (i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    // Create one child for each command in pipeline
    for (i = 0; i < ncmds; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        // Child process
        if (pids[i] == 0) {
            // If not the first command, take input from previous pipe
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 pipe input");
                    exit(1);
                }
            }

            // If not the last command, send output to next pipe
            if (i < ncmds - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 pipe output");
                    exit(1);
                }
            }

            // Close all pipe ends after dup2
            for (j = 0; j < ncmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Apply possible redirection on this command
            apply_redirection(&commands[i]);

            // Reject empty pipeline command
            if (commands[i].argv[0] == NULL) {
                fprintf(stderr, "Empty command in pipeline\n");
                exit(1);
            }

            // exit should not be used inside pipeline
            if (strcmp(commands[i].argv[0], "exit") == 0) {
                fprintf(stderr, "exit cannot be used in pipeline\n");
                exit(1);
            }

            // Built-in command inside pipeline
            if (is_builtin(commands[i].argv[0])) {
                int rc = execute_builtin(commands[i].argv);
                exit(rc == 0 ? 0 : 1);
            }

            // External command inside pipeline
            execvp(commands[i].argv[0], commands[i].argv);
            perror("execvp");
            exit(1);
        }
    }

    // Parent closes all pipe ends
    for (i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Foreground: wait for all child processes
    if (!background) {
        for (i = 0; i < ncmds; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }

    // Background pipeline: do not wait
    else {
        printf("[background pipeline started]\n");
    }
}

// Main shell loop
int main() {
    char input[INPUT_SIZE];              // raw input from user
    char processed[INPUT_SIZE * 3];      // processed input with spaces around symbols
    char *tokens[MAX_TOKENS];            // token list
    Command commands[MAX_CMDS];          // parsed commands

    // Run shell until user exits
    while (1) {
        // Display prompt
        printf("myshell> ");
        fflush(stdout);

        // Read input line from keyboard
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // If EOF, exit shell gracefully
            printf("\n");
            break;
        }

        // Remove trailing newline
        trim_newline(input);

        // Ignore empty input line
        if (strlen(input) == 0) {
            continue;
        }

        // Add spaces around special symbols like | < > &
        preprocess_input(input, processed);

        // Convert processed string into tokens
        int token_count = tokenize(processed, tokens);

        // If nothing tokenized, continue loop
        if (token_count == 0) {
            continue;
        }

        int background = 0;

        // Parse tokens into structured command(s)
        int ncmds = parse_commands(tokens, token_count, commands, &background);

        // Syntax error during parse
        if (ncmds < 0) {
            continue;
        }

        // No commands found
        if (ncmds == 0) {
            continue;
        }

        /*
           Plain exit should terminate shell directly
           only when:
           - single command
           - no redirection
           - not background
        */
        if (ncmds == 1 &&
            commands[0].argv[0] != NULL &&
            strcmp(commands[0].argv[0], "exit") == 0 &&
            commands[0].input_file == NULL &&
            commands[0].output_file == NULL &&
            !background) {

            if (count_args(commands[0].argv) > 1) {
                fprintf(stderr, "exit: too many arguments\n");
                continue;
            }

            break;
        }

        // If one command, execute normally
        if (ncmds == 1) {
            execute_single_command(&commands[0], background);
        }

        // If more than one command, execute as pipeline
        else {
            execute_pipeline(commands, ncmds, background);
        }
    }

    return 0;
}