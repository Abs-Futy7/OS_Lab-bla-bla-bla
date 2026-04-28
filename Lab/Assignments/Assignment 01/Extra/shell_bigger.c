#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>

 

#define INPUT_SIZE 2048
#define MAX_TOKENS 256
#define MAX_ARGS 128
#define MAX_CMDS 32
#define BUFFER_SIZE 1024

typedef struct {
    char *argv[MAX_ARGS];
    char *input_file;
    char *output_file;
} Command;

void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

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

int count_args(char *args[]) {
    int count = 0;
    while (args[count] != NULL) count++;
    return count;
}

void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void preprocess_input(const char *input, char *output) {
    int i = 0, j = 0;
    while (input[i] != '\0') {
        if (input[i] == '|' || input[i] == '<' || input[i] == '>' || input[i] == '&') {
            output[j++] = ' ';
            output[j++] = input[i];
            output[j++] = ' ';
        } else {
            output[j++] = input[i];
        }
        i++;
    }
    output[j] = '\0';
}

int tokenize(char *input, char *tokens[]) {
    int count = 0;
    char *token = strtok(input, " \t");
    while (token != NULL && count < MAX_TOKENS - 1) {
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }
    tokens[count] = NULL;
    return count;
}

void init_command(Command *cmd) {
    int i;
    for (i = 0; i < MAX_ARGS; i++) cmd->argv[i] = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
}

/*
   Parse tokens into commands split by |
   Also support:
   - input redirection <
   - output redirection >
   - background execution &
*/
int parse_commands(char *tokens[], int token_count, Command commands[], int *background) {
    int cmd_index = 0;
    int arg_index = 0;
    int i;

    *background = 0;
    init_command(&commands[cmd_index]);

    for (i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            if (arg_index == 0) {
                fprintf(stderr, "Syntax error: empty command before pipe\n");
                return -1;
            }
            commands[cmd_index].argv[arg_index] = NULL;
            cmd_index++;
            if (cmd_index >= MAX_CMDS) {
                fprintf(stderr, "Too many piped commands\n");
                return -1;
            }
            init_command(&commands[cmd_index]);
            arg_index = 0;
        } else if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 >= token_count) {
                fprintf(stderr, "Syntax error: missing input file\n");
                return -1;
            }
            commands[cmd_index].input_file = tokens[++i];
        } else if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 >= token_count) {
                fprintf(stderr, "Syntax error: missing output file\n");
                return -1;
            }
            commands[cmd_index].output_file = tokens[++i];
        } else if (strcmp(tokens[i], "&") == 0) {
            if (i != token_count - 1) {
                fprintf(stderr, "Syntax error: '&' must be at the end\n");
                return -1;
            }
            *background = 1;
        } else {
            if (arg_index >= MAX_ARGS - 1) {
                fprintf(stderr, "Too many arguments\n");
                return -1;
            }
            commands[cmd_index].argv[arg_index++] = tokens[i];
        }
    }

    if (arg_index == 0 && cmd_index == 0) {
        return 0;
    }

    commands[cmd_index].argv[arg_index] = NULL;
    return cmd_index + 1;
}

/* =========================
   Built-in command handlers
   ========================= */

int builtin_echo(char *args[]) {
    int i = 1;
    while (args[i] != NULL) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) printf(" ");
        i++;
    }
    printf("\n");
    return 0;
}

int builtin_pwd(char *args[]) {
    if (count_args(args) > 1) {
        fprintf(stderr, "pwd: too many arguments\n");
        return 1;
    }

    char cwd[INPUT_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return 1;
    }

    printf("%s\n", cwd);
    return 0;
}

int builtin_ls(char *args[]) {
    int argc = count_args(args);
    int long_format = 0;
    char *path = ".";

    if (argc == 1) {
        long_format = 0;
    } else if (argc == 2) {
        if (strcmp(args[1], "-l") == 0) {
            long_format = 1;
        } else {
            path = args[1];
        }
    } else if (argc == 3) {
        if (strcmp(args[1], "-l") == 0) {
            long_format = 1;
            path = args[2];
        } else {
            fprintf(stderr, "ls: invalid arguments\n");
            fprintf(stderr, "Usage: ls OR ls -l OR ls <dir> OR ls -l <dir>\n");
            return 1;
        }
    } else {
        fprintf(stderr, "ls: too many arguments\n");
        return 1;
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("ls");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (!long_format) {
            printf("%s  ", entry->d_name);
        } else {
            char fullpath[INPUT_SIZE];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            struct stat st;
            if (stat(fullpath, &st) == -1) {
                perror("ls stat");
                continue;
            }

            print_permissions(st.st_mode);
            printf(" %ld", (long)st.st_nlink);

            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);

            printf(" %s", pw ? pw->pw_name : "unknown");
            printf(" %s", gr ? gr->gr_name : "unknown");
            printf(" %5ld", (long)st.st_size);

            char timebuf[64];
            struct tm *tm_info = localtime(&st.st_mtime);
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
            printf(" %s", timebuf);

            printf(" %s\n", entry->d_name);
        }
    }

    if (!long_format) printf("\n");
    closedir(dir);
    return 0;
}

int builtin_cd(char *args[]) {
    int argc = count_args(args);

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

    if (argc > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }

    if (chdir(args[1]) != 0) {
        perror("cd");
        return 1;
    }

    return 0;
}

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

    int fd = open(args[1], O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        perror("touch");
        return 1;
    }
    close(fd);

    struct utimbuf new_times;
    new_times.actime = time(NULL);
    new_times.modtime = time(NULL);

    if (utime(args[1], &new_times) != 0) {
        perror("touch");
        return 1;
    }

    return 0;
}

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

    FILE *fp = fopen(args[1], "r");
    if (fp == NULL) {
        perror("cat");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        fputs(buffer, stdout);
    }

    fclose(fp);
    return 0;
}

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

    int src = open(args[1], O_RDONLY);
    if (src < 0) {
        perror("cp source");
        return 1;
    }

    int dest = open(args[2], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dest < 0) {
        perror("cp destination");
        close(src);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(src, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;
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

void apply_redirection(Command *cmd) {
    if (cmd->input_file != NULL) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror("input redirection");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 input");
            close(fd);
            exit(1);
        }
        close(fd);
    }

    if (cmd->output_file != NULL) {
        int fd = open(cmd->output_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) {
            perror("output redirection");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            close(fd);
            exit(1);
        }
        close(fd);
    }
}

void execute_single_command(Command *cmd, int background) {
    if (cmd->argv[0] == NULL) return;

    /*
      Built-ins that change shell state should run in parent
      only when no pipe is involved.
    */
    if (strcmp(cmd->argv[0], "exit") == 0) {
        if (count_args(cmd->argv) > 1) {
            fprintf(stderr, "exit: too many arguments\n");
            return;
        }
        exit(0);
    }

    if (strcmp(cmd->argv[0], "cd") == 0 && cmd->input_file == NULL && cmd->output_file == NULL && !background) {
        builtin_cd(cmd->argv);
        return;
    }

    if (is_builtin(cmd->argv[0]) && cmd->input_file == NULL && cmd->output_file == NULL && !background &&
        strcmp(cmd->argv[0], "cd") != 0) {
        execute_builtin(cmd->argv);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        apply_redirection(cmd);

        if (strcmp(cmd->argv[0], "exit") == 0) {
            fprintf(stderr, "exit cannot be used in child context\n");
            exit(1);
        }

        if (is_builtin(cmd->argv[0])) {
            int rc = execute_builtin(cmd->argv);
            exit(rc == 0 ? 0 : 1);
        }

        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(1);
    } else {
        if (!background) {
            waitpid(pid, NULL, 0);
        } else {
            printf("[background pid %d]\n", pid);
        }
    }
}

void execute_pipeline(Command commands[], int ncmds, int background) {
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    int i, j;

    for (i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    for (i = 0; i < ncmds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        if (pids[i] == 0) {
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 pipe input");
                    exit(1);
                }
            }

            if (i < ncmds - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 pipe output");
                    exit(1);
                }
            }

            for (j = 0; j < ncmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            apply_redirection(&commands[i]);

            if (commands[i].argv[0] == NULL) {
                fprintf(stderr, "Empty command in pipeline\n");
                exit(1);
            }

            if (strcmp(commands[i].argv[0], "exit") == 0) {
                fprintf(stderr, "exit cannot be used in pipeline\n");
                exit(1);
            }

            if (is_builtin(commands[i].argv[0])) {
                int rc = execute_builtin(commands[i].argv);
                exit(rc == 0 ? 0 : 1);
            }

            execvp(commands[i].argv[0], commands[i].argv);
            perror("execvp");
            exit(1);
        }
    }

    for (i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (!background) {
        for (i = 0; i < ncmds; i++) {
            waitpid(pids[i], NULL, 0);
        }
    } else {
        printf("[background pipeline started]\n");
    }
}

int main() {
    char input[INPUT_SIZE];
    char processed[INPUT_SIZE * 3];
    char *tokens[MAX_TOKENS];
    Command commands[MAX_CMDS];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        trim_newline(input);

        if (strlen(input) == 0) {
            continue;
        }

        preprocess_input(input, processed);
        int token_count = tokenize(processed, tokens);

        if (token_count == 0) {
            continue;
        }

        int background = 0;
        int ncmds = parse_commands(tokens, token_count, commands, &background);
        if (ncmds < 0) {
            continue;
        }
        if (ncmds == 0) {
            continue;
        }

        /*
          exit must affect the shell itself when used as a normal command
        */
        if (ncmds == 1 && commands[0].argv[0] != NULL && strcmp(commands[0].argv[0], "exit") == 0 &&
            commands[0].input_file == NULL && commands[0].output_file == NULL && !background) {
            if (count_args(commands[0].argv) > 1) {
                fprintf(stderr, "exit: too many arguments\n");
                continue;
            }
            break;
        }

        if (ncmds == 1) {
            execute_single_command(&commands[0], background);
        } else {
            execute_pipeline(commands, ncmds, background);
        }
    }

    return 0;
}