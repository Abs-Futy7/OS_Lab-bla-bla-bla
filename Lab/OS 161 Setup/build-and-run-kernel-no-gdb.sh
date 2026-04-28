#!/usr/bin/env bash

# Builds OS/161, then runs the kernel in SYS/161 without attaching GDB.

set -e
set -u
set -o pipefail

if ! grep docker /proc/1/cgroup -qa; then
  echo 'ERROR: PLEASE RUN THIS SCRIPT INSIDE THE INTERACTIVE CS350 SHELL (SEE `start-interactive-cs350-shell.sh`)'
  exit 1
fi

ASSIGNMENT=ASST0
ROOT_DIR=/root/cs350-os161/root
SOURCE_DIR=/root/cs350-os161/os161-1.99

mkdir --parents "$ROOT_DIR"
cp --update /root/sys161/share/examples/sys161/sys161.conf.sample "$ROOT_DIR/sys161.conf"

cd "$SOURCE_DIR"
./configure --ostree="$ROOT_DIR" --toolprefix=cs350-

cd "$SOURCE_DIR/kern/conf"
./config "$ASSIGNMENT"

cd "$SOURCE_DIR/kern/compile/$ASSIGNMENT"
bmake depend
bmake
bmake install

cd "$SOURCE_DIR"
bmake
bmake install

cd "$ROOT_DIR"
exec sys161 kernel
