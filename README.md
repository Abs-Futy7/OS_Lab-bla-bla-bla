# OS/161 Quick Start and Beginner Guide

This guide explains how to start OS/161 again from scratch, what each pane does, and which beginner commands to use.

## 1. What you need

- Docker Desktop installed and running
- WSL2 enabled in Docker Desktop (Windows)
- Project folder contains:
  - `os161.tar.gz`
  - `start-interactive-cs350-shell.ps1`
  - `build-and-run-kernel.sh`

## 2. Start OS/161 from Windows PowerShell

Open PowerShell in this folder:

E:/CSEDU/3-2/OS/Lab/OS 161 Setup

Run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\start-interactive-cs350-shell.ps1
```

This starts a Docker container and mounts this folder to:

/root/cs350-os161

## 2A. Linux lab PC setup (for tomorrow's lab)

If the lab machine is Linux, use this exact flow.

### Step 1: Copy your folder to the lab PC

Make sure this folder exists on the lab PC and includes at least:

- os161.tar.gz
- build-and-run-kernel.sh
- lab-linux-quickstart.sh

### Step 2: Open terminal in project folder

```bash
cd "/path/to/OS 161 Setup"
```

### Step 3: Make scripts executable once

```bash
chmod +x lab-linux-quickstart.sh build-and-run-kernel.sh start-interactive-cs350-shell.sh
```

### Step 4: Run end-to-end setup

```bash
./lab-linux-quickstart.sh all
```

This does:

1. Pull/update uberi/cs350 image
2. Extract os161.tar.gz (and fix nested folder if needed)
3. Build kernel and open SYS/161 + GDB in tmux

### Step 5: If your lab user needs sudo for Docker

The script automatically tries sudo docker if plain docker is blocked.
You may be prompted for password.

### Step 6: Manual mode (if you prefer step-by-step)

```bash
./lab-linux-quickstart.sh pull
./lab-linux-quickstart.sh extract
./lab-linux-quickstart.sh build
```

### Step 7: Kernel and debugger panes

After build command:

- Left pane: OS/161 kernel prompt
- Right pane: GDB prompt

Switch pane: Ctrl+b then o

## 3. First-time extraction (or when source folder is missing)

Inside container shell, check mount:

```bash
ls -la /root/cs350-os161/
```

If `os161-1.99` is missing, extract:

```bash
mkdir -p /root/cs350-os161/os161-1.99
tar -xvzf /root/cs350-os161/os161.tar.gz -C /root/cs350-os161/
```

If extraction created nested directory, fix it:

```bash
if [ -d /root/cs350-os161/os161-1.99/os161-1.99 ]; then
  mv /root/cs350-os161/os161-1.99/os161-1.99/* /root/cs350-os161/os161-1.99/
fi
```

## 4. Build and run kernel

Inside container shell:

```bash
bash /root/cs350-os161/build-and-run-kernel.sh
```

After success, tmux opens with two panes:

- Left pane: SYS/161 emulator (running kernel)
- Right pane: GDB debugger (attached)

## 5. Pane control (tmux basics)

Use these keybindings:

- Ctrl+b then o: switch pane
- Ctrl+b then Left Arrow: move to left pane
- Ctrl+b then Right Arrow: move to right pane
- Ctrl+b then q: show pane numbers
- Ctrl+b then d: detach tmux session

Reattach later (inside container shell):

```bash
tmux attach-session -t os161
```

## 6. What to type in each pane

Left pane (SYS/161 / kernel prompt):

- Kernel menu commands such as `?`, `?t`, `?o`, `kh`, `q`

Right pane (GDB):

- Debugger commands such as `c`, `break`, `bt`, `next`, `step`, `print`

Important:

Do not run bash commands in GDB pane.
If you accidentally type a shell command in GDB, it may show errors like:

- Undefined command: "cbash"

That just means command went to wrong pane.

## 7. Beginner OS/161 commands

At kernel prompt (`OS/161 kernel [? for menu]:`):

- `?` : show main menu
- `?t` : tests menu
- `?o` : operations menu
- `kh` : kernel heap stats
- `q` : shutdown

Suggested beginner flow:

1. Type `?`
2. Explore `?t` and run one simple test
3. Use `kh` before and after test
4. Type `q` to stop

## 8. Beginner GDB commands

In right pane:

- `c` : continue execution
- Ctrl+C : interrupt/pause target
- `break menu` : break when kernel menu handler runs
- `bt` : show stack trace
- `list` : show source around current line
- `next` : step over
- `step` : step into
- `print <var>` : inspect variable
- `info locals` : print local variables

Example mini-debug session:

```gdb
break menu
c
```

Then in left pane type `?`.
When breakpoint hits in GDB:

```gdb
bt
list
next
info locals
c
```

## 9. Restart quickly next time

From Windows PowerShell in project folder:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\start-interactive-cs350-shell.ps1
```

Inside container:

```bash
bash /root/cs350-os161/build-and-run-kernel.sh
```

If tmux already exists and you just want back in:

```bash
tmux attach-session -t os161
```

## 10. Exit and shutdown commands

Use the level that matches what you want to close:

### A. Exit only OS/161 kernel (stay in tmux)

In left pane at kernel prompt:

```text
q
```

### B. Leave tmux but keep session running

Press:

```text
Ctrl+b then d
```

### C. Reattach tmux later

Inside container shell:

```bash
tmux attach-session -t os161
```

### D. Exit container shell completely

In container shell:

```bash
exit
```

### E. Force stop container from host PowerShell

If container does not close normally:

```powershell
docker ps
docker stop <container_id>
```

## 11. Common issues and fixes

### A. "Undefined command: cbash" in GDB

Cause: shell command typed in GDB pane.
Fix:

1. Switch to left pane or shell pane.
2. Run shell commands there.
3. In GDB, use `c` to continue.

### B. "No such file or directory ... startup/start.S"

This can appear when GDB first attaches. Usually harmless if kernel runs and prompt appears.

### C. Script permission/execution policy in PowerShell

Use:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
```

### D. Docker mount path not visible

Inside container, verify:

```bash
ls -la /root/cs350-os161/
```

If empty, exit and rerun launcher from the correct host folder.

## 12. File map

- `start-interactive-cs350-shell.ps1`: Windows launcher
- `start-interactive-cs350-shell.sh`: Bash launcher
- `build-and-run-kernel.sh`: build kernel, launch SYS/161 + GDB in tmux
- `os161.tar.gz`: OS/161 source archive

## 13. One-screen command cheat sheet

```text
# Windows PowerShell (host)
Set-ExecutionPolicy -Scope Process Bypass
.\start-interactive-cs350-shell.ps1

# Container shell
ls -la /root/cs350-os161/
mkdir -p /root/cs350-os161/os161-1.99
tar -xvzf /root/cs350-os161/os161.tar.gz -C /root/cs350-os161/
bash /root/cs350-os161/build-and-run-kernel.sh

# Linux lab (host)
chmod +x lab-linux-quickstart.sh build-and-run-kernel.sh
./lab-linux-quickstart.sh all

# tmux
Ctrl+b then o
Ctrl+b then d
tmux attach-session -t os161

# Exit levels
# kernel only (left pane)
q
# container shell
exit
# host force stop
docker ps
docker stop <container_id>

# OS/161 (left pane)
?
?t
?o
kh
q

# GDB (right pane)
c
break menu
bt
list
next
step
print var
info locals
```
