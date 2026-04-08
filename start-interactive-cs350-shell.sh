#!/usr/bin/env bash

set -euo pipefail

# Mount the folder that contains this script into the container.
HOST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

docker run \
	--volume "${HOST_DIR}:/root/cs350-os161" \
	--interactive \
	--tty \
	uberi/cs350:latest \
	bash
