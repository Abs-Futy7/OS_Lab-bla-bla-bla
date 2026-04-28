#!/usr/bin/env bash

# Runs the already-built OS/161 kernel in SYS/161 without attaching GDB.
# This script does not configure, compile, or install OS/161.

set -e
set -u
set -o pipefail

if ! grep docker /proc/1/cgroup -qa; then
  echo 'ERROR: PLEASE RUN THIS SCRIPT INSIDE THE INTERACTIVE CS350 SHELL (SEE `start-interactive-cs350-shell.sh`)'
  exit 1
fi

ROOT_DIR=/root/cs350-os161/root

if [ ! -e "$ROOT_DIR/kernel" ]; then
  echo 'ERROR: NO BUILT KERNEL FOUND AT /root/cs350-os161/root/kernel'
  echo 'Build it first: bash /root/cs350-os161/build-and-run-kernel-no-gdb.sh'
  exit 1
fi

mkdir --parents "$ROOT_DIR"
cp --update /root/sys161/share/examples/sys161/sys161.conf.sample "$ROOT_DIR/sys161.conf"

cd "$ROOT_DIR"
exec sys161 kernel
