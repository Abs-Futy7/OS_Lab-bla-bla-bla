## OS/161 Development Environment

This guide explains how to start the CS350 Docker environment, extract OS/161, build the kernel, and run it with or without GDB.

### Prerequisites

Before starting, make sure you have:

- **Docker Desktop** installed and running
- **WSL2 enabled** if you are on Windows
- **At least 10 GB free disk space** for OS/161 and Docker images
- **4 GB or more RAM** available for Docker

### Project Layout

The examples below assume this container path:

```bash
/root/cs350-os161
```

That folder is a Docker mount of your local OS/161 workspace. Any file you edit locally is visible inside the container, and files created inside the container appear in the mounted local folder.

Expected files:

| Path | Purpose |
|------|---------|
| `/root/cs350-os161/os161.tar.gz` | OS/161 source archive |
| `/root/cs350-os161/os161-1.99/` | Extracted OS/161 source tree |
| `/root/cs350-os161/root/` | Installed OS/161 root directory |
| `/root/cs350-os161/root/kernel` | Compiled kernel binary |
| `/root/cs350-os161/*.sh` | Helper scripts for building and running |

### 1. Start the Docker Container

Start an interactive CS350 shell with Docker:

```bash
docker run --volume "D:/cs350-workspace/os161:/root/cs350-os161" --interactive --tty uberi/cs350:latest bash
```

Replace `D:/cs350-workspace/os161` with your local OS/161 folder.

If you use the provided helper script, run it from your host machine:

```bash
bash start-interactive-cs350-shell.sh
```

All build and run scripts in this guide must be run **inside** the interactive Docker shell.

### 2. Extract the OS/161 Source Code

If `os161.tar.gz` has not been extracted yet, create the source directory and unpack it:

```bash
mkdir -p /root/cs350-os161/os161-1.99
tar -xvzf /root/cs350-os161/os161.tar.gz -C /root/cs350-os161/
```

Verify the source tree:

```bash
ls -la /root/cs350-os161/os161-1.99/
```

If extraction creates `/root/cs350-os161/os161-1.99/os161-1.99`, move the inner contents up one level so the source files live directly in `/root/cs350-os161/os161-1.99/`.

### 3. Choose How to Build and Run

There are three common workflows.

#### Option A: Build and Run With GDB

Use this when you want a debugger attached:

```bash
bash /root/cs350-os161/build-and-run-kernel.sh
```

This script:

- Configures OS/161 with `--ostree=/root/cs350-os161/root`
- Builds the selected kernel assignment
- Builds user-level programs
- Installs the kernel and programs
- Opens a `tmux` session with SYS/161 and GDB side by side

The left pane runs SYS/161. The right pane runs `cs350-gdb`.

#### Option B: Build and Run Without GDB

Use this for normal testing when you do not need breakpoints or stepping:

```bash
bash /root/cs350-os161/build-and-run-kernel-no-gdb.sh
```

This script performs the same build and install steps as the GDB version, then starts SYS/161 directly:

```bash
sys161 kernel
```

This is simpler than the GDB workflow because it does not open `tmux`, does not install or start `tmux`, and does not wait for a debugger connection.

#### Option C: Run the Already-Built Kernel Without GDB

Use this when you already built the kernel and only want to boot it again:

```bash
bash /root/cs350-os161/run-kernel-no-gdb.sh
```

This script does **not** configure, compile, or install OS/161. It only:

- Checks that `/root/cs350-os161/root/kernel` exists
- Refreshes `/root/cs350-os161/root/sys161.conf`
- Runs `sys161 kernel`

If the kernel has not been built yet, run:

```bash
bash /root/cs350-os161/build-and-run-kernel-no-gdb.sh
```

### 4. What to Expect After Booting

When SYS/161 starts successfully, the OS/161 kernel boots in the terminal.

Useful kernel commands:

```bash
?
p /testbin/palin
p /bin/true
q
```

Use `?` to show the OS/161 menu. Use `q` to quit SYS/161.

With the GDB workflow, SYS/161 starts with:

```bash
sys161 -w kernel
```

That means SYS/161 waits until GDB connects. In the GDB pane, press Enter on the prepared `c` command to continue booting.

With either no-GDB workflow, the kernel starts immediately in the current terminal.

### 5. Edit Source Files

Edit OS/161 files from your host editor or inside the container. The most common source locations are:

| Directory | Purpose |
|-----------|---------|
| `/root/cs350-os161/os161-1.99/kern/` | Kernel source |
| `/root/cs350-os161/os161-1.99/kern/conf/` | Kernel configuration files |
| `/root/cs350-os161/os161-1.99/userland/` | User-level programs |
| `/root/cs350-os161/os161-1.99/testbin/` | Test programs |

Inside the container, you can edit with:

```bash
vi /root/cs350-os161/os161-1.99/kern/main/main.c
nano /root/cs350-os161/os161-1.99/kern/main/main.c
```

After changing kernel code, rebuild before running:

```bash
bash /root/cs350-os161/build-and-run-kernel-no-gdb.sh
```

After changing only non-kernel files that are already installed, you may still need to rebuild and reinstall from the source tree before the changes appear under `/root/cs350-os161/root/`.

### 6. Assignment Configuration

The build scripts currently use:

```bash
ASSIGNMENT=ASST0
```

If your course work requires another kernel configuration, edit the `ASSIGNMENT` value in the relevant build script, for example:

```bash
ASSIGNMENT=ASST1
```

The run-only script does not have an assignment setting because it only boots the already-installed kernel.

### 7. Troubleshooting

**Error: `PLEASE RUN THIS SCRIPT INSIDE THE INTERACTIVE CS350 SHELL`**

You ran the script from the host instead of inside Docker. Start the interactive Docker shell first, then run the script again.

**Error: `NO BUILT KERNEL FOUND AT /root/cs350-os161/root/kernel`**

You used `run-kernel-no-gdb.sh` before building the kernel. Build it first:

```bash
bash /root/cs350-os161/build-and-run-kernel-no-gdb.sh
```

**SYS/161 cannot find `sys161.conf`**

Run one of the helper scripts. Each script copies the sample SYS/161 configuration into:

```bash
/root/cs350-os161/root/sys161.conf
```

**GDB workflow opens but the kernel does not continue**

In the GDB pane, press Enter to run the prepared `c` command. That continues execution after GDB connects to SYS/161.

### 8. Quick Command Reference

```bash
# Start Docker container manually
docker run --volume "D:/cs350-workspace/os161:/root/cs350-os161" --interactive --tty uberi/cs350:latest bash

# Check mounted files
ls -la /root/cs350-os161/

# Extract OS/161 source
tar -xvzf /root/cs350-os161/os161.tar.gz -C /root/cs350-os161/

# Build and run with GDB/tmux
bash /root/cs350-os161/build-and-run-kernel.sh

# Build and run without GDB
bash /root/cs350-os161/build-and-run-kernel-no-gdb.sh

# Run already-built kernel without GDB
bash /root/cs350-os161/run-kernel-no-gdb.sh

# Attach to existing tmux session from the GDB workflow
tmux attach-session -t os161

# Go to OS/161 source tree
cd /root/cs350-os161/os161-1.99/

# Go to installed OS/161 root
cd /root/cs350-os161/root/
```
