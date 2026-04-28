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

void print_prompt() {
    char cwd[SIZE];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("myshell> ");
        fflush(stdout);
        return;
    }

    printf("myshell:%s> ", cwd);
    fflush(stdout);
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

void manual_pwd() {
    char cwd[SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return;
    }
    printf("%s\n", cwd);
}

void manual_ls(char *args[]) {
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

void manual_cd(char *args[]) {
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

void manual_echo(char *args[]) {
    int i;
    for (i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) printf(" ");
    }
    printf("\n");
}

void manual_mkdir(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: mkdir foldername\n");
        return;
    }
    if (mkdir(args[1], 0777) != 0) perror("mkdir");
}

void manual_touch(char *args[]) {
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

void manual_rm(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: rm filename\n");
        return;
    }
    if (remove(args[1]) != 0) perror("rm");
}

void manual_mv(char *args[]) {
    if (count_args(args) != 3) {
        printf("Usage: mv oldname newname\n");
        return;
    }
    if (rename(args[1], args[2]) != 0) perror("mv");
}

void manual_cat(char *args[]) {
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

void manual_cp(char *args[]) {
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
    ssize_t n;
    while ((n = read(src, buf, sizeof(buf))) > 0) {
        if (write(dst, buf, n) != n) {
            perror("cp");
            close(src);
            close(dst);
            return;
        }
    }

    if (n < 0) perror("cp");

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
    if (strcmp(args[0], "pwd") == 0) manual_pwd();
    else if (strcmp(args[0], "ls") == 0) manual_ls(args);
    else if (strcmp(args[0], "cd") == 0) manual_cd(args);
    else if (strcmp(args[0], "echo") == 0) manual_echo(args);
    else if (strcmp(args[0], "mkdir") == 0) manual_mkdir(args);
    else if (strcmp(args[0], "touch") == 0) manual_touch(args);
    else if (strcmp(args[0], "rm") == 0) manual_rm(args);
    else if (strcmp(args[0], "mv") == 0) manual_mv(args);
    else if (strcmp(args[0], "cat") == 0) manual_cat(args);
    else if (strcmp(args[0], "cp") == 0) manual_cp(args);
}

void execute_command(char *args[], int background) {
    if (args[0] == NULL) return;

    if (strcmp(args[0], "cd") == 0 && !background) {
        manual_cd(args);
        return;
    }

    if (strcmp(args[0], "exit") == 0 && !background) {
        if (count_args(args) > 1) {
            printf("exit: too many arguments\n");
            return;
        }
        exit(0);
    }

    if (is_builtin(args[0]) && !background &&
        strcmp(args[0], "cd") != 0 &&
        strcmp(args[0], "exit") != 0) {
        run_builtin(args);
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        if (strcmp(args[0], "exit") == 0) {
            printf("exit cannot run in background\n");
            exit(1);
        }

        if (strcmp(args[0], "cd") == 0) {
            printf("cd cannot run in background\n");
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

int main() {
    char line[SIZE];

    while (1) {
        print_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        remove_newline(line);

        if (strlen(line) == 0) continue;

        int background = 0;

        int len = strlen(line);
        while (len > 0 && line[len - 1] == ' ') {
            line[len - 1] = '\0';
            len--;
        }

        if (len > 0 && line[len - 1] == '&') {
            background = 1;
            line[len - 1] = '\0';

            len = strlen(line);
            while (len > 0 && line[len - 1] == ' ') {
                line[len - 1] = '\0';
                len--;
            }
        }

        char *args[MAX_ARGS];
        parse_args(line, args);

        if (args[0] == NULL) continue;

        execute_command(args, background);
    }

    return 0;
}
