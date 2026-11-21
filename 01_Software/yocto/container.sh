#!/usr/bin/env bash
# Opens a shell in the Yocto build container with the build environment ready.
# Other scripts source this file to use run_in_container.
set -euo pipefail

YOCTO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd "$YOCTO_DIR/../.." && pwd)
IMAGE=localhost/dashboard-yocto:wrynose
CONTAINER_HOME="${XDG_CACHE_HOME:-$HOME/.cache}/dashboard-yocto/home"
BUILD_ENV="source layers/openembedded-core/oe-init-build-env build > /dev/null"

# Resources kept free for the desktop while the container runs.
FREE_CPUS=1
FREE_RAM_GB=10
# Peak memory of one compile job; WebKit needs about this much per job.
RAM_PER_JOB_GB=3

# Usage: run_in_container [podman run options] -- command [args...]
run_in_container() {
  local opts=() cpus ram_gb jobs
  while (( $# )) && [[ $1 != -- ]]; do
    opts+=("$1")
    shift
  done
  shift

  # The container gets the remaining CPUs and RAM and no swap. If a build
  # needs more memory, the kernel kills a compiler inside the container
  # instead of freezing the desktop.
  cpus=$(( $(nproc) - FREE_CPUS ))
  ram_gb=$(( $(awk '/^MemTotal:/ { print int($2 / 1024 / 1024) }' /proc/meminfo) - FREE_RAM_GB ))
  jobs=$(( ram_gb / RAM_PER_JOB_GB ))
  (( jobs > cpus )) && jobs=$cpus

  podman build -q -t "$IMAGE" "$YOCTO_DIR/container" > /dev/null
  mkdir -p "$CONTAINER_HOME"
  podman run --rm --pull=never \
    --userns=keep-id \
    --security-opt label=disable \
    --cpus="$cpus" \
    --memory="${ram_gb}g" \
    --memory-swap="${ram_gb}g" \
    -e BB_NUMBER_THREADS="$cpus" \
    -e PARALLEL_MAKE="-j $jobs" \
    -v "$CONTAINER_HOME:$HOME" \
    -v "$PROJECT_DIR:$PROJECT_DIR" \
    -w "$YOCTO_DIR" \
    "${opts[@]}" "$IMAGE" "$@"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  run_in_container -it -- bash -c "$BUILD_ENV && echo 'Środowisko buildu gotowe w $YOCTO_DIR/build' && exec bash"
fi
