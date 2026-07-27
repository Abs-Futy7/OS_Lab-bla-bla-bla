## OS/161 Development Environment 

### Prerequisites & Requirements

Before starting, ensure we have:

- **Docker Desktop** installed (with WSL2 enabled)
- **At least 10GB free disk space** for OS/161 and Docker images
- **4GB+ RAM** available for Docker

### 1. Set Up Docker

Make sure Docker is installed and running on your system. If you're on Windows, ensure Docker Desktop is running with WSL2 enabled.

### 2. Start the Docker Container

Run the following command to start the Docker container and mount your local directory:

```bash
docker run --volume "D:/cs350-workspace/os161:/root/cs350-os161" --interactive --tty uberi/cs350:latest bash
```

This mounts the `D:/cs350-workspace/os161` directory from our local machine to `/root/cs350-os161` inside the container, allowing us to edit OS/161 files locally and run them inside the container.

> **Note:** Replace `D:/cs350-workspace` with the path where we have our OS/161 project (e.g., `D:/projects`, `C:/Users/YourName/Desktop`, etc.)

### 3. Extract the OS/161 Source Code (If Needed)

If we already have the `os161.tar.gz` archive, extract it inside the container. If not, download it from:
- **Direct link:** http://www.student.cs.uwaterloo.ca/~cs350/os161_repository/os161.tar.gz

**Verify the archive:**
```bash
ls /root/cs350-os161/
```

**Create the extraction directory:**
```bash
mkdir -p /root/cs350-os161/os161-1.99
```

**Extract the archive:**
```bash
tar -xvzf /root/cs350-os161/os161.tar.gz -C /root/cs350-os161/
```

This will extract files into `/root/cs350-os161/os161-1.99/`. If an extra nested folder is created (resulting in `/root/cs350-os161/os161-1.99/os161-1.99`), move the inner contents up one level to remove the redundant nesting.

### 4. Build and Run the Kernel

Execute the build and run script:

```bash
bash /root/cs350-os161/build-and-run-kernel.sh
```

This script automates the entire build process:
- Configures the OS/161 build environment
- Compiles the kernel
- Builds user-level programs
- Sets up SYS/161 emulator and GDB debugger in a tmux session

### 5. Understanding the Output

Once the script completes successfully, we'll see a tmux window split into two panes:

**Left Pane - SYS/161 Emulator:**
- Our OS/161 kernel runs here
- Type `?` for the help menu

**Right Pane - GDB Debugger:**
- GDB is attached to our running kernel
- Use GDB commands to debug our code
- Set breakpoints, step through execution, inspect variables

The script will set up everything we need to start developing and debugging our OS/161 kernel.

### 6. Write and Edit Files

To write files inside our container, we can use various methods depending on the editors available.

**Using echo to quickly create a file:**

```bash
echo "This is my new file content!" > /root/cs350-os161/my_new_file.txt
```

**Using a text editor (vi or nano):**

```bash
vi /root/cs350-os161/my_new_file.txt
nano /root/cs350-os161/my_new_file.txt
```

**Using cat with heredoc:**

```bash
cat > /root/cs350-os161/my_new_file.txt << EOF
This is my new file content!
We can write multiple lines here.
EOF
```

Since we mounted our local directory to the container, any files we edit locally will also be visible inside the container, and vice versa.

 

### 8. Quick Command Reference

**Essential Commands:**

```bash
# Start Docker container
docker run --volume "D:/cs350-workspace/os161:/root/cs350-os161" --interactive --tty uberi/cs350:latest bash

# Build and run the kernel
bash /root/cs350-os161/build-and-run-kernel.sh

# Access tmux session
tmux attach-session -t os161

# Split panes in tmux
tmux split-window -h -t os161:0

# Create a new file
echo "content" > /path/to/file.txt

# Edit files
vi /path/to/file.txt
nano /path/to/file.txt

# List directory contents
ls -la /root/cs350-os161/

# Navigate directories
cd /root/cs350-os161/os161-1.99/
```

**Key Directories:**

| Directory | Purpose |
|-----------|---------|
| `D:/cs350-workspace/os161` | Local project folder (adjust path as needed) |
| `/root/cs350-os161/` | Container project mount point |
| `/root/cs350-os161/os161-1.99/` | OS/161 source code |
| `/root/cs350-os161/root/` | Build output and kernel executable |
| `/root/cs350-os161/root/kernel` | Compiled kernel binary |
