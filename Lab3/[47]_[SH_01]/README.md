# MyShell - OS Lab Assignment [SH_01]

## Files
- `shell.c`: main shell implementation in C.
- `run.sh`: helper script that compiles and launches MyShell.

## Build And Run

### Option 1: Use the script (recommended)
```bash
bash run.sh
```

`run.sh` does the following:
1. Runs `gcc shell.c -o myshell`
2. If compilation succeeds, starts `./myshell`
3. If compilation fails, prints `Compilation failed!`

### Option 2: Manual
```bash
gcc shell.c -o myshell
./myshell
```

## Features Implemented

### Core Shell
- Displays a prompt: `myshell> `
- Reads user input from keyboard using `fgets()`
- Parses input into command + arguments using `strtok()`
- Executes non-built-in commands using `fork()` + `execvp()` + `wait()`

### External Commands
Implemented by running system programs via `execvp()`:

| Command | Example | Description |
|---------|---------|-------------|
| `pwd`   | `pwd` | Print current directory |
| `ls`    | `ls -l` | List directory contents |
| `mkdir` | `mkdir myfolder` | Create a directory |
| `touch` | `touch file.txt` | Create an empty file |
| `rm`    | `rm file.txt` | Remove a file |
| `cp`    | `cp a.txt b.txt` | Copy a file |
| `mv`    | `mv a.txt b.txt` | Move/rename a file |
| `cat`   | `cat file.txt` | Print file contents |

### Built-in Commands
Handled directly inside the shell:

| Command | Example | Description |
|---------|---------|-------------|
| `cd`    | `cd /home` | Change directory using `chdir()` |
| `echo`  | `echo Hello World` | Print text to terminal |
| `exit`  | `exit` | Quit the shell |

### Bonus Features
- **Output Redirection (`>`)** — e.g., `ls > output.txt`
- **Input Redirection (`<`)** — e.g., `cat < file.txt`


Redirection is handled in the child process using `open()` + `dup2()`.



## System Calls / APIs Used
| System Call / API | What it does in this shell |
|---|---|
| `fork()` | Creates a child process to run external commands. |
| `execvp()` | Replaces the child process image with the requested command (searches in PATH). |
| `wait()` | Parent process waits until the child command finishes before showing the prompt again. |
| `chdir()` | Implements built-in `cd` by changing the shell's current working directory. |
| `open()` | Opens files used for input (`<`) and output (`>`) redirection. |
| `dup2()` | Redirects standard input/output to file descriptors opened by `open()`. |
| `close()` | Closes file descriptors after redirection setup in the child process. |


## Verified Run (Actual Output)
The following is an actual run from this project:

```text
$ bash run.sh
Welcome to MyShell! Type 'exit' to quit.
myshell> mkdir new
myshell> cd new
myshell> pwd
/mnt/e/CSEDU/3-2/OS/Lab/Lab3/[47]_[SH_01]/new

myshell> echo hello > a.txt
myshell> cat < b.txt
open failed: No such file or directory

myshell> cat < a.txt
hello

myshell> cp a.txt b.txt
myshell> ls
a.txt  b.txt

myshell> cat b.txt
hello

myshell> mv a.txt rename.txt
myshell> ls
b.txt  rename.txt

myshell> cat newname.txt
cat: newname.txt: No such file or directory
myshell> cat rename.txt
hello

myshell> cd ..
myshell> ls
new  README.md  run.sh  shell.c
myshell> pwd
/mnt/e/CSEDU/3-2/OS/Lab/Lab3/[47]_[SH_01]
myshell> exit
Goodbye!
```
