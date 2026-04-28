#include <stdio.h>      // printf, fgets, perror, fopen
#include <stdlib.h>     // exit, getenv
#include <string.h>     // strcmp, strlen, strtok, strcspn
#include <unistd.h>     // getcwd, chdir, fork, execvp
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/wait.h>   // waitpid
#include <sys/stat.h>   // stat, mkdir, permission bits
#include <fcntl.h>      // open, O_CREAT, O_WRONLY, O_RDONLY
#include <time.h>       // time, localtime, strftime
#include <utime.h>      // utime for updating file time
#include <pwd.h>        // getpwuid for owner name
#include <grp.h>        // getgrgid for group name

#define SIZE 1024       // general buffer size
#define MAX_ARGS 100    // maximum number of command arguments

// Remove newline from input line
void remove_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

// Count how many arguments are in args[]
int count_args(char *args[]) {
    int i = 0;
    while (args[i] != NULL) i++;
    return i;
}

// Split command line into tokens by space/tab
// Example: "ls -l" -> args[0]="ls", args[1]="-l", args[2]=NULL
void parse_args(char *line, char *args[]) {
    int i = 0;
    char *token = strtok(line, " \t");

    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }

    args[i] = NULL;
}

// Print permission bits like rwxr-xr-x for ls -l
void print_permissions(mode_t mode) {
    printf(S_ISDIR(mode) ? "d" : "-");   // d if directory, otherwise -
    printf(mode & S_IRUSR ? "r" : "-");  // user read
    printf(mode & S_IWUSR ? "w" : "-");  // user write
    printf(mode & S_IXUSR ? "x" : "-");  // user execute
    printf(mode & S_IRGRP ? "r" : "-");  // group read
    printf(mode & S_IWGRP ? "w" : "-");  // group write
    printf(mode & S_IXGRP ? "x" : "-");  // group execute
    printf(mode & S_IROTH ? "r" : "-");  // others read
    printf(mode & S_IWOTH ? "w" : "-");  // others write
    printf(mode & S_IXOTH ? "x" : "-");  // others execute
}

// Built-in: pwd
// Print current working directory
void cmd_pwd() {
    char cwd[SIZE];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return;
    }

    printf("%s\n", cwd);
}

// Built-in: ls
// Supports:
//   ls
//   ls -l
//   ls folder
//   ls -l folder
void cmd_ls(char *args[]) {
    int argc = count_args(args);
    int long_format = 0;   // 0 = normal ls, 1 = ls -l
    char *path = ".";      // default path is current directory

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
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Normal ls: only print file/folder names
        if (!long_format) {
            printf("%s  ", entry->d_name);
        } else {
            // ls -l: print detailed information
            char fullpath[SIZE];
            struct stat st;

            // Create full path like folder/file.txt
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            // Read file information
            if (stat(fullpath, &st) == -1) {
                perror("ls");
                continue;
            }

            // Print permissions and link count
            print_permissions(st.st_mode);
            printf(" %ld", (long)st.st_nlink);

            // Get owner and group names
            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);

            printf(" %s", pw ? pw->pw_name : "unknown");
            printf(" %s", gr ? gr->gr_name : "unknown");

            // Print file size
            printf(" %5ld", (long)st.st_size);

            // Format modification time
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

            // Print time and file name
            printf(" %s %s\n", timebuf, entry->d_name);
        }
    }

    if (!long_format) printf("\n");
    closedir(dir);
}

// Built-in: cd
// Change current directory
void cmd_cd(char *args[]) {
    int argc = count_args(args);

    // If user types only "cd", go to HOME directory
    if (argc == 1) {
        char *home = getenv("HOME");
        if (home == NULL) {
            printf("cd: HOME not set\n");
            return;
        }

        if (chdir(home) != 0) perror("cd");
        return;
    }

    // Too many arguments
    if (argc > 2) {
        printf("cd: too many arguments\n");
        return;
    }

    // Change to specified directory
    if (chdir(args[1]) != 0) perror("cd");
}

// Built-in: echo
// Print text after echo
void cmd_echo(char *args[]) {
    int i;

    for (i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) printf(" ");
    }

    printf("\n");
}

// Built-in: mkdir
// Create a new directory
void cmd_mkdir(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: mkdir foldername\n");
        return;
    }

    if (mkdir(args[1], 0777) != 0) perror("mkdir");
}

// Built-in: touch
// Create file if missing and update timestamps
void cmd_touch(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: touch filename\n");
        return;
    }

    // Open file in write mode, create if needed
    int fd = open(args[1], O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("touch");
        return;
    }
    close(fd);

    // Update access time and modification time to current time
    struct utimbuf t;
    t.actime = time(NULL);
    t.modtime = time(NULL);

    if (utime(args[1], &t) != 0) perror("touch");
}

// Built-in: rm
// Remove a file
void cmd_rm(char *args[]) {
    if (count_args(args) != 2) {
        printf("Usage: rm filename\n");
        return;
    }

    if (remove(args[1]) != 0) perror("rm");
}

// Built-in: mv
// Move or rename a file
void cmd_mv(char *args[]) {
    if (count_args(args) != 3) {
        printf("Usage: mv oldname newname\n");
        return;
    }

    if (rename(args[1], args[2]) != 0) perror("mv");
}

// Built-in: cat
// Display file contents
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

    // Read file line by line and print
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        printf("%s", buf);
    }

    fclose(fp);
}

// Built-in: cp
// Copy one file to another
void cmd_cp(char *args[]) {
    if (count_args(args) != 3) {
        printf("Usage: cp source destination\n");
        return;
    }

    // Open source file
    int src = open(args[1], O_RDONLY);
    if (src < 0) {
        perror("cp");
        return;
    }

    // Open destination file
    int dst = open(args[2], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dst < 0) {
        perror("cp");
        close(src);
        return;
    }

    char buf[SIZE];
    ssize_t n;

    // Read from source and write to destination
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

// Check whether command is built-in
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

// Call the correct built-in function
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

// Execute a command
// If built-in, handle internally
// Otherwise use fork + execvp
void execute_command(char *args[], int background) {
    if (args[0] == NULL) return;

    // cd must run in parent shell process
    if (strcmp(args[0], "cd") == 0 && !background) {
        cmd_cd(args);
        return;
    }

    // exit must terminate shell itself
    if (strcmp(args[0], "exit") == 0 && !background) {
        if (count_args(args) > 1) {
            printf("exit: too many arguments\n");
            return;
        }
        exit(0);
    }

    // Other built-ins can run directly if not background
    if (is_builtin(args[0]) && !background &&
        strcmp(args[0], "cd") != 0 &&
        strcmp(args[0], "exit") != 0) {
        run_builtin(args);
        return;
    }

    // Create child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    // Child process
    if (pid == 0) {
        // Do not allow exit in background child
        if (strcmp(args[0], "exit") == 0) {
            printf("exit cannot run in background\n");
            exit(1);
        }

        // cd in child does not change parent directory
        if (strcmp(args[0], "cd") == 0) {
            printf("cd cannot run in background\n");
            exit(1);
        }

        // Run built-in inside child if needed
        if (is_builtin(args[0])) {
            run_builtin(args);
            exit(0);
        }

        // Run external command
        execvp(args[0], args);

        // Only runs if execvp fails
        perror("execvp");
        exit(1);
    }

    // Parent process
    else {
        if (!background) {
            // Foreground: wait for child
            waitpid(pid, NULL, 0);
        } else {
            // Background: do not wait
            printf("[background pid %d]\n", pid);
        }
    }
}

// Main shell loop
int main() {
    char line[SIZE];

    while (1) {
        // Show shell prompt
        printf("myshell> ");
        fflush(stdout);

        // Read one line from keyboard
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove ending newline
        remove_newline(line);

        // Ignore empty line
        if (strlen(line) == 0) continue;

        int background = 0;

        // Remove spaces from end of line
        int len = strlen(line);
        while (len > 0 && line[len - 1] == ' ') {
            line[len - 1] = '\0';
            len--;
        }

        // If command ends with &, run in background
        if (len > 0 && line[len - 1] == '&') {
            background = 1;
            line[len - 1] = '\0';

            // Remove spaces again after removing &
            len = strlen(line);
            while (len > 0 && line[len - 1] == ' ') {
                line[len - 1] = '\0';
                len--;
            }
        }

        char *args[MAX_ARGS];

        // Split line into command + arguments
        parse_args(line, args);

        // Ignore if no command found
        if (args[0] == NULL) continue;

        // Execute the command
        execute_command(args, background);
    }

    return 0;
}