#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>

#define SIZE 1024
#define MAX_ARGS 100

void remove_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

int count_args(char *args[]) {
    int i = 0;
    while (args[i] != NULL) i++;
    return i;
}

void parse_args(char *line, char *args[]) {
    int i = 0;
    char *token = strtok(line, " \t");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL;
}

void print_permissions(mode_t mode) {
    printf(S_ISDIR(mode) ? "d" : "-");
    printf(mode & S_IRUSR ? "r" : "-");
    printf(mode & S_IWUSR ? "w" : "-");
    printf(mode & S_IXUSR ? "x" : "-");
    printf(mode & S_IRGRP ? "r" : "-");
    printf(mode & S_IWGRP ? "w" : "-");
    printf(mode & S_IXGRP ? "x" : "-");
    printf(mode & S_IROTH ? "r" : "-");
    printf(mode & S_IWOTH ? "w" : "-");
    printf(mode & S_IXOTH ? "x" : "-");
}

void cmd_pwd() {
    char cwd[SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return;
    }
    printf("%s\n", cwd);
}

void cmd_ls(char *args[]) {
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
    } else if (argc == 3 && strcmp(args[1], "-l") == 0) {
        long_format = 1;
        path = args[2];
    } else {
        printf("Usage: ls OR ls -l OR ls folder OR ls -l folder\n");
        return;
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("ls");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (!long_format) {
            printf("%s  ", entry->d_name);
        } else {
            char fullpath[SIZE];
            struct stat st;
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            if (stat(fullpath, &st) == -1) {
                perror("ls");
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
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));
            printf(" %s %s\n", timebuf, entry->d_name);
        }
    }

    if (!long_format) printf("\n");
    closedir(dir);
}

void cmd_cd(char *args[]) {
    int argc = count_args(args);

    if (argc == 1) {
        char *home = getenv("HOME");
        if (home == NULL) {
            printf("cd: HOME not set\n");
            return;
        }
        if (chdir(home) != 0) perror("cd");
        return;
    }

    if (argc > 2) {
        printf("cd: too many arguments\n");
        return;
    }

    if (chdir(args[1]) != 0) perror("cd");
}

void cmd_echo(char *args[]) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) printf(" ");
    }
    printf("\n");
}

void cmd_mkdir(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: mkdir foldername\n");
        return;
    }
    if (mkdir(args[1], 0777) != 0) perror("mkdir");
}

void cmd_touch(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: touch filename\n");
        return;
    }

    int fd = open(args[1], O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("touch");
        return;
    }
    close(fd);

    struct utimbuf t;
    t.actime = time(NULL);
    t.modtime = time(NULL);
    if (utime(args[1], &t) != 0) perror("touch");
}

void cmd_rm(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: rm filename\n");
        return;
    }
    if (remove(args[1]) != 0) perror("rm");
}

void cmd_mv(char *args[]) {
    if (count_args(args) != 3) {
        printf("Usage: mv oldname newname\n");
        return;
    }
    if (rename(args[1], args[2]) != 0) perror("mv");
}

void cmd_cat(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: cat filename\n");
        return;
    }

    FILE *fp = fopen(args[1], "r");
    if (fp == NULL) {
        perror("cat");
        return;
    }

    char buf[SIZE];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        printf("%s", buf);
    }

    fclose(fp);
}

void cmd_cp(char *args[]) {
    if (count_args(args) != 3) {
        printf("Usage: cp source destination\n");
        return;
    }

    int src = open(args[1], O_RDONLY);
    if (src < 0) {
        perror("cp");
        return;
    }

    int dst = open(args[2], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dst < 0) {
        perror("cp");
        close(src);
        return;
    }

    char buf[SIZE];
    int n;
    while ((n = read(src, buf, sizeof(buf))) > 0) {
        write(dst, buf, n);
    }

    close(src);
    close(dst);
}

int is_builtin(char *cmd) {
    if (cmd == NULL) return 0;

    return strcmp(cmd, "pwd") == 0 ||
           strcmp(cmd, "ls") == 0 ||
           strcmp(cmd, "cd") == 0 ||
           strcmp(cmd, "echo") == 0 ||
           strcmp(cmd, "mkdir") == 0 ||
           strcmp(cmd, "touch") == 0 ||
           strcmp(cmd, "rm") == 0 ||
           strcmp(cmd, "mv") == 0 ||
           strcmp(cmd, "cat") == 0 ||
           strcmp(cmd, "cp") == 0 ||
           strcmp(cmd, "exit") == 0;
}

void run_builtin(char *args[]) {
    if (strcmp(args[0], "pwd") == 0) cmd_pwd();
    else if (strcmp(args[0], "ls") == 0) cmd_ls(args);
    else if (strcmp(args[0], "cd") == 0) cmd_cd(args);
    else if (strcmp(args[0], "echo") == 0) cmd_echo(args);
    else if (strcmp(args[0], "mkdir") == 0) cmd_mkdir(args);
    else if (strcmp(args[0], "touch") == 0) cmd_touch(args);
    else if (strcmp(args[0], "rm") == 0) cmd_rm(args);
    else if (strcmp(args[0], "mv") == 0) cmd_mv(args);
    else if (strcmp(args[0], "cat") == 0) cmd_cat(args);
    else if (strcmp(args[0], "cp") == 0) cmd_cp(args);
}

void execute_simple(char *args[], int background, char *input_file, char *output_file) {
    if (args[0] == NULL) return;

    if (strcmp(args[0], "cd") == 0 && !background && input_file == NULL && output_file == NULL) {
        cmd_cd(args);
        return;
    }

    if (strcmp(args[0], "exit") == 0 && !background && input_file == NULL && output_file == NULL) {
        exit(0);
    }

    if (is_builtin(args[0]) && !background && input_file == NULL && output_file == NULL &&
        strcmp(args[0], "cd") != 0 && strcmp(args[0], "exit") != 0) {
        run_builtin(args);
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        if (input_file != NULL) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                perror("input");
                exit(1);
            }
            dup2(fd, 0);
            close(fd);
        }

        if (output_file != NULL) {
            int fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (fd < 0) {
                perror("output");
                exit(1);
            }
            dup2(fd, 1);
            close(fd);
        }

        if (strcmp(args[0], "exit") == 0) {
            printf("exit cannot run here\n");
            exit(1);
        }

        if (is_builtin(args[0])) {
            run_builtin(args);
            exit(0);
        }

        execvp(args[0], args);
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

void execute_pipe(char *left_args[], char *right_args[], int background) {
    int fd[2];

    if (pipe(fd) < 0) {
        perror("pipe");
        return;
    }

    pid_t p1 = fork();
    if (p1 < 0) {
        perror("fork");
        return;
    }

    if (p1 == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);

        if (is_builtin(left_args[0])) {
            run_builtin(left_args);
            exit(0);
        }

        execvp(left_args[0], left_args);
        perror("execvp");
        exit(1);
    }

    pid_t p2 = fork();
    if (p2 < 0) {
        perror("fork");
        return;
    }

    if (p2 == 0) {
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);

        if (is_builtin(right_args[0])) {
            run_builtin(right_args);
            exit(0);
        }

        execvp(right_args[0], right_args);
        perror("execvp");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    if (!background) {
        waitpid(p1, NULL, 0);
        waitpid(p2, NULL, 0);
    } else {
        printf("[background pipe started]\n");
    }
}

int main() {
    char line[SIZE];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        remove_newline(line);

        if (strlen(line) == 0) continue;

        int background = 0;
        if (line[strlen(line) - 1] == '&') {
            background = 1;
            line[strlen(line) - 1] = '\0';
        }

        char *pipe_pos = strchr(line, '|');

        if (pipe_pos != NULL) {
            *pipe_pos = '\0';

            char left[SIZE], right[SIZE];
            strcpy(left, line);
            strcpy(right, pipe_pos + 1);

            char *left_args[MAX_ARGS];
            char *right_args[MAX_ARGS];

            parse_args(left, left_args);
            parse_args(right, right_args);

            if (left_args[0] == NULL || right_args[0] == NULL) {
                printf("Invalid pipe command\n");
                continue;
            }

            execute_pipe(left_args, right_args, background);
            continue;
        }

        char *input_file = NULL;
        char *output_file = NULL;

        char *in_pos = strchr(line, '<');
        if (in_pos != NULL) {
            *in_pos = '\0';
            input_file = in_pos + 1;
            while (*input_file == ' ') input_file++;
        }

        char *out_pos = strchr(line, '>');
        if (out_pos != NULL) {
            *out_pos = '\0';
            output_file = out_pos + 1;
            while (*output_file == ' ') output_file++;
        }

        char *args[MAX_ARGS];
        parse_args(line, args);

        if (args[0] == NULL) continue;

        execute_simple(args, background, input_file, output_file);
    }

    return 0;
}