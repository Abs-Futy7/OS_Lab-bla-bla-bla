# Assignment 1 Package Contents

This folder contains the final OS/161 Assignment 1 materials.

## Main Documents

- `README.md`: detailed implementation notes, build/run commands, and viva Q&A.
- `Assignment1_Report.pdf`: final LaTeX report.
- `Assignment1_Report.tex`: report source.
- `design.txt`: required plain-text design document.

## Screenshots

- `screenshots/asst1-01-debugger.png`
- `screenshots/asst1-02-1a.png`
- `screenshots/asst1-03-1b.png`
- `screenshots/asst1-04-bar.png`

## Source Files

`source/` preserves the important changed/added files using their original
relative paths.

Key files:

- `source/os161-1.99/kern/asst1/`: assignment solution files.
- `source/os161-1.99/kern/thread/synch.c`: completed synchronization primitives.
- `source/os161-1.99/kern/include/synch.h`: lock/CV structure fields.
- `source/os161-1.99/kern/startup/menu.c`: menu entries for `1a`, `1b`, `1c`.
- `source/os161-1.99/kern/conf/conf.kern`: ASST1 file compilation wiring.
- `source/root/sys161.conf`: 2 MiB System/161 RAM setting.
- `source/build-and-run-kernel.sh`: builds `ASST1`.
