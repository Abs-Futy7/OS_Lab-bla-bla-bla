#!/usr/bin/env bash

set -euo pipefail

IMAGE="uberi/cs350:latest"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MOUNT_POINT="/root/cs350-os161"

DOCKER_BIN="docker"
if ! docker info >/dev/null 2>&1; then
  if command -v sudo >/dev/null 2>&1; then
    DOCKER_BIN="sudo docker"
  else
    echo "ERROR: Docker is not accessible for current user, and sudo is unavailable."
    exit 1
  fi
fi

run_docker() {
  # shellcheck disable=SC2086
  $DOCKER_BIN "$@"
}

usage() {
  cat <<'EOF'
Usage:
  ./lab-linux-quickstart.sh pull
  ./lab-linux-quickstart.sh shell
  ./lab-linux-quickstart.sh extract
  ./lab-linux-quickstart.sh build
  ./lab-linux-quickstart.sh all

Commands:
  pull    Pull/update the CS350 Docker image.
  shell   Open interactive container shell with project mounted.
  extract Extract os161.tar.gz into os161-1.99 (host-mounted folder).
  build   Build kernel and run SYS/161 + GDB in tmux.
  all     Run pull + extract + build in one shot.
EOF
}

cmd_pull() {
  run_docker pull "$IMAGE"
}

cmd_shell() {
  run_docker run \
    --volume "${PROJECT_DIR}:${MOUNT_POINT}" \
    --interactive \
    --tty \
    "$IMAGE" \
    bash
}

cmd_extract() {
  if [[ ! -f "${PROJECT_DIR}/os161.tar.gz" ]]; then
    echo "ERROR: ${PROJECT_DIR}/os161.tar.gz not found."
    echo "Copy os161.tar.gz into this folder first."
    exit 1
  fi

  run_docker run --rm \
    --volume "${PROJECT_DIR}:${MOUNT_POINT}" \
    "$IMAGE" \
    bash -lc '
      set -euo pipefail
      mkdir -p /root/cs350-os161/os161-1.99
      tar -xvzf /root/cs350-os161/os161.tar.gz -C /root/cs350-os161/
      if [ -d /root/cs350-os161/os161-1.99/os161-1.99 ]; then
        mv /root/cs350-os161/os161-1.99/os161-1.99/* /root/cs350-os161/os161-1.99/
      fi
      echo "Extraction completed."
    '
}

cmd_build() {
  run_docker run \
    --volume "${PROJECT_DIR}:${MOUNT_POINT}" \
    --interactive \
    --tty \
    "$IMAGE" \
    bash -lc 'bash /root/cs350-os161/build-and-run-kernel.sh'
}

if [[ $# -ne 1 ]]; then
  usage
  exit 1
fi

case "$1" in
  pull)
    cmd_pull
    ;;
  shell)
    cmd_shell
    ;;
  extract)
    cmd_extract
    ;;
  build)
    cmd_build
    ;;
  all)
    cmd_pull
    cmd_extract
    cmd_build
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    echo "ERROR: Unknown command: $1"
    usage
    exit 1
    ;;
esac
