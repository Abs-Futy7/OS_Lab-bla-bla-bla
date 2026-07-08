# OS/161 Assignment 2 README

This folder contains the Assignment 2 implementation notes, source files, report source, and screenshot guide.

## What to Submit

The safest option is to submit the whole `Asst2` folder because it contains the report, screenshots, and source tree together. If sir asks only for changed code files, submit these files:

| File | Why it is needed |
|---|---|
| `source/os161-1.99/kern/thread/synch.c` | Implements locks and condition variables used by ASST2 tests. |
| `source/os161-1.99/kern/include/proc.h` | Adds PID fields, wait helpers, and per-process file table data. |
| `source/os161-1.99/kern/proc/proc.c` | Implements PID table logic, process creation/exit handling, wait support, and file table copy/cleanup. |
| `source/os161-1.99/kern/include/syscall.h` | Declares the new syscall handler functions. |
| `source/os161-1.99/kern/arch/mips/syscall/syscall.c` | Dispatches syscall numbers to the implemented syscall functions. |
| `source/os161-1.99/kern/syscall/proc_syscalls.c` | Implements process syscalls: `_exit`, `getpid`, `waitpid`, `fork`, and `execv`. |
| `source/os161-1.99/kern/syscall/file_syscalls.c` | Implements file syscalls: `open`, `read`, `write`, `close`, `remove`, create, and append. |
| `source/os161-1.99/kern/syscall/runprogram.c` | Implements argument passing for the first user program launched from the menu. |
| `source/os161-1.99/kern/startup/menu.c` | Passes menu command arguments into `runprogram_args`. |
| `source/os161-1.99/kern/include/test.h` | Declares `runprogram_args` for the menu code. |
| `source/os161-1.99/kern/conf/conf.kern` | Needed if new syscall source files were added to the kernel build. |

Also submit:

| File/Folder | Purpose |
|---|---|
| `47_Assignment2_Report.pdf` | Final report with explanations and screenshots. |
| `screenshots/` | Output and code screenshots used in the report. |
| `README.md` | Build guide, test commands, implementation notes, and viva Q&A. |

## Assignment Summary

Assignment 2 asks for:

1. Kernel locks and condition variables.
2. Process system calls: `fork`, `getpid`, `waitpid`, `_exit`, and `execv`.
3. Basic file system calls: `open`, `read`, `write`, `close`, and `remove`, including `O_CREAT` and `O_APPEND`.
4. Argument passing for both `execv` and the first process launched from the kernel menu using `p program args`.

The locks and condition variables are already implemented in `kern/thread/synch.c` and `kern/include/synch.h` from Assignment 1 and are included in the ASST2 kernel using `options A1` and `options A2`.

## Important Changed Files

| File | Purpose |
|---|---|
| `kern/thread/synch.c` | Implements lock and CV behavior: create, acquire, release, wait, signal, and broadcast. |
| `kern/include/proc.h` | Adds process PID information and file descriptor table definitions. |
| `kern/proc/proc.c` | Handles PID allocation, process exit/wait state, and file table setup/copy/cleanup. |
| `kern/syscall/proc_syscalls.c` | Implements `_exit`, `getpid`, `waitpid`, `fork`, and `execv`. |
| `kern/syscall/file_syscalls.c` | Implements `open`, `read`, `write`, `close`, `remove`, `O_CREAT`, and `O_APPEND`. |
| `kern/arch/mips/syscall/syscall.c` | Dispatches syscall numbers and returns syscall results to user mode. |
| `kern/syscall/runprogram.c` | Adds argument passing for the first process started by the menu. |
| `kern/startup/menu.c` | Sends menu arguments to `runprogram_args` for ASST2. |
| `kern/include/syscall.h` | Adds kernel prototypes for syscall handlers. |
| `kern/include/test.h` | Adds the `runprogram_args` prototype. |
| `kern/conf/conf.kern` | Includes new source files in the ASST2 kernel build. |

## How the Code Works

### PID and process waiting

Every user process gets a unique positive PID when it is created by `proc_create_runprogram`. The PID table stores:

- PID number
- parent PID
- whether the process has exited
- encoded exit status
- a lock and condition variable for `waitpid`

When a process exits, `sys__exit` calls `proc_record_exit(_MKWAIT_EXIT(exitcode))`. This records the status and wakes any parent blocked in `waitpid`.

`waitpid(pid, status, 0)` calls `proc_wait`. It checks that:

- the PID exists
- the current process is the parent
- the child has exited, or waits until it exits

Then it copies the encoded status back to user space.

### `fork`

`sys_fork` does four main things:

1. Copies the current trapframe.
2. Copies the current address space using `as_copy`.
3. Creates a child process with a new PID.
4. Starts a new kernel thread using `thread_fork`.

The child starts in `enter_forked_process`. That function:

- installs the copied address space
- adjusts the copied trapframe so the child sees return value `0`
- increments `tf_epc` so the syscall instruction is not repeated
- calls `mips_usermode`

The parent returns normally from `sys_fork` with the child PID.

### Basic file system calls

Each process now has a file descriptor table in `struct proc`. The table stores pointers to `struct filehandle` objects. A file handle stores:

- vnode pointer
- current file offset
- open flags
- reference count
- lock

`open` copies the filename from user space, validates flags, calls `vfs_open`, creates a file handle, and returns the lowest free descriptor above `stderr`.

`O_CREAT` is handled by `vfs_open`, so a file is created when the user opens with `O_CREAT`.

`read` checks that the descriptor is valid and readable, builds a user-space `uio`, calls `VOP_READ`, and advances the file offset.

`write` checks that the descriptor is valid and writable, builds a user-space `uio`, calls `VOP_WRITE`, and advances the file offset.

`O_APPEND` is handled by calling `VOP_STAT` before each write and setting the handle offset to the file size. This makes every append write go to the current end of the file.

`close` removes the descriptor from the process table and decrements the file handle reference count. When the count reaches zero, the vnode is closed with `vfs_close`.

`remove` copies the pathname from user space and calls `vfs_remove`. It is included because `filetest` removes its temporary file at the end.

During `fork`, parent and child share the same file handle objects, so they share file offsets as expected.

### `execv`

`sys_execv` replaces the current program image without changing the PID.

The important order is:

1. Copy program path and `argv` strings from the old user address space.
2. Open the new executable.
3. Create and activate a new address space.
4. Load the ELF file.
5. Define the new user stack.
6. Copy argument strings and the `argv` pointer array onto the new stack.
7. Destroy the old address space.
8. Enter the new program with `enter_new_process(argc, argv, stackptr, entrypoint)`.

Arguments must be copied before destroying the old address space because the old `argv` pointers point into the old user memory.

### `runprogram` argument passing

The kernel menu command:

```text
p /testbin/argtest hello world
```

now passes `hello` and `world` to the first user program. `menu.c` calls `runprogram_args`, and `runprogram.c` copies the kernel argument strings onto the new user stack in the same format used by `execv`.

## Build Commands

From Windows PowerShell, first enter the project folder and start the CS350 Docker shell:

```powershell
cd "E:\CSEDU\3-2\OS\Lab\OS 161 Setup"
docker run --rm -it -v "${PWD}:/root/cs350-os161" uberi/cs350:latest bash
```

Inside Docker, build and run the ASST2 kernel:

```sh
cd /root/cs350-os161
bash ./build-and-run-kernel.sh
```

The script now uses `ASSIGNMENT=ASST2`.

Manual build commands, if needed:

```sh
cd /root/cs350-os161/os161-1.99
./configure --ostree=/root/cs350-os161/root --toolprefix=cs350-
cd kern/conf
./config ASST2
cd ../compile/ASST2
bmake depend
bmake
bmake install
cd /root/cs350-os161/os161-1.99
bmake
bmake install
```

If `filetest` needs an SFS disk and `mount sfs lhd0` says wrong magic number, format `DISK1.img` once from the Docker shell:

```sh
cd /root/cs350-os161/root
./hostbin/host-mksfs DISK1.img LHD0
```

## Test Commands

After `build-and-run-kernel.sh` opens the tmux split view, press Enter in the right GDB pane if the command `c` is waiting. Then type OS/161 commands in the left pane.

Lock test:

```text
sy2
```

Condition variable test:

```text
sy3
```

Run a simple user program:

```text
p /testbin/palin
```

Test argument passing:

```text
p /testbin/argtest hello os161
```

Test basic file syscalls:

```text
mount sfs lhd0
p /testbin/filetest lhd0:testfile
```

`filetest` must use an SFS path like `lhd0:testfile` because the default `testfile` path is on `emu0`, and emufs does not support `remove`.

Quit OS/161:

```text
q
```

## Screenshots Included

The report uses screenshots from `Asst2/screenshots/` with these names:

| Screenshot | Filename |
|---|---|
| `sy2` lock test passes | `asst2-02-sy2-lock.png` |
| `sy3` CV test passes | `asst2-03-sy3-cv.png` |
| user program runs, e.g. `palin` | `asst2-04-palin.png` |
| argument passing works | `asst2-05-argtest.png` |
| file syscall test works | `asst2-06-filetest.png` |

### What each screenshot proves

| Screenshot | Command used | What to look for | What it demonstrates |
|---|---|---|---|
| `asst2-02-sy2-lock.png` | `sy2` | `Lock test done.` | The kernel lock implementation can synchronize many threads without corrupting shared state. |
| `asst2-03-sy3-cv.png` | `sy3` | `CV test done` | Condition variables correctly sleep, wake, and reacquire the associated lock. |
| `asst2-04-palin.png` | `p /testbin/palin` | `IS a palindrome` | User programs can run through `runprogram`; console `write` and process exit work. |
| `asst2-05-argtest.png` | `p /testbin/argtest hello os161` | `arg[0]`, `arg[1]`, `arg[2]`, `arg[3]: [NULL]` | Kernel menu argument passing sets up user `argc/argv` correctly. |
| `asst2-06-filetest.png` | `mount sfs lhd0`, then `p /testbin/filetest lhd0:testfile` | `Passed filetest.` | File syscalls can create, write, close, reopen, read, compare, and remove a file on SFS. |

The screenshots were found by running each OS/161 menu command in the left pane of the tmux session created by `build-and-run-kernel.sh`. The proof line at the end of each output is what matters for the report.

## Viva Q&A

**Q: Why do we need PIDs?**  
A: A PID uniquely identifies each user process. `fork` returns the child PID to the parent, `getpid` returns the current process PID, and `waitpid` uses a PID to find a child's exit status.

**Q: Where is PID stored?**  
A: The process structure has `p_pid`. The kernel also has a PID table in `proc.c` that stores parent PID, exit state, and exit status.

**Q: Why does `waitpid` only allow children?**  
A: The assignment says a process is interested in its children only. So `proc_wait` compares the target process parent PID with the current process PID.

**Q: Why is a condition variable used in the PID table?**  
A: If the parent calls `waitpid` before the child exits, it must sleep. The child broadcasts on the CV when it exits.

**Q: Why does `_exit` not simply destroy everything immediately?**  
A: It destroys the process resources, but first records the exit code in the PID table so the parent can still collect it later.

**Q: Why does `fork` copy the trapframe?**  
A: The trapframe contains the user CPU register state at the syscall. The child must resume from almost the same state as the parent.

**Q: Why does the child return 0 from `fork`?**  
A: Unix-style `fork` returns twice: child gets `0`, parent gets the child PID. `enter_forked_process` sets `tf_v0 = 0` for the child.

**Q: Why increment `tf_epc` in the child?**  
A: Without incrementing the program counter, the child would re-execute the syscall instruction and fork again.

**Q: Why copy `execv` arguments before replacing the address space?**  
A: The original `argv` pointers point into the old user memory. After destroying/replacing that address space, those pointers are invalid.

**Q: How are arguments passed to a new program?**  
A: The kernel copies strings to the top of the new user stack, builds a user-space `argv[]` array of pointers to those strings, and passes `argc` and `argv` through `enter_new_process`.

**Q: Does `execv` create a new process?**  
A: No. It keeps the same process and PID, but replaces the program image/address space.

**Q: Where are open files stored?**  
A: Each process has `p_filetable[OPEN_MAX]` in `struct proc`. Each descriptor points to a `struct filehandle`.

**Q: What does a file handle contain?**  
A: It contains the vnode, current offset, open flags, reference count, and a lock.

**Q: How does `open` create a file?**  
A: `sys_open` passes `O_CREAT` to `vfs_open`; the VFS layer creates the file if needed.

**Q: How does `read` know where to read from?**  
A: It uses the file handle offset as `uio_offset`. After `VOP_READ`, the updated `uio_offset` is saved back into the handle.

**Q: How does `write` know where to write?**  
A: It writes at the file handle offset and saves the new offset after `VOP_WRITE`.

**Q: How is append implemented?**  
A: If `O_APPEND` is set, `write` calls `VOP_STAT` before writing and sets the offset to `st_size`.

**Q: What happens to file descriptors on `fork`?**  
A: The child shares the same file handle objects as the parent. The reference count is incremented, so both processes share offsets.

**Q: Why does each file handle need a lock?**  
A: The lock makes reads/writes and offset updates atomic for that open file object.

**Q: What happens if `execv` fails?**  
A: It restores the old address space when possible and returns an error code to the caller.

**Q: What are `sy2` and `sy3`?**  
A: `sy2` tests locks; `sy3` tests condition variables.

**Q: Why must final kernel output be quiet?**  
A: The assignment says "Silence is Golden"; extra debug output can interfere with automated tests and grading.
