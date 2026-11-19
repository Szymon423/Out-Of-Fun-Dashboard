#!/usr/bin/env bash
# Opens a shell in the Yocto build container with the build environment ready.
# Other scripts source this file to use run_in_container.
set -euo pipefail

YOCTO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd "$YOCTO_DIR/../.." && pwd)
IMAGE=localhost/dashboard-yocto:wrynose
CONTAINER_HOME="${XDG_CACHE_HOME:-$HOME/.cache}/dashboard-yocto/home"
BUILD_ENV="source layers/openembedded-core/oe-init-build-env build > /dev/null"

# Usage: run_in_container [podman run options] -- command [args...]
run_in_container() {
  local opts=()
  while (( $# )) && [[ $1 != -- ]]; do
    opts+=("$1")
    shift
  done
  shift

  podman build -q -t "$IMAGE" "$YOCTO_DIR/container" > /dev/null
  mkdir -p "$CONTAINER_HOME"
  podman run --rm --pull=never \
    --userns=keep-id \
    --security-opt label=disable \
    -v "$CONTAINER_HOME:$HOME" \
    -v "$PROJECT_DIR:$PROJECT_DIR" \
    -w "$YOCTO_DIR" \
    "${opts[@]}" "$IMAGE" "$@"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  run_in_container -it -- bash -c "$BUILD_ENV && echo 'Środowisko buildu gotowe w $YOCTO_DIR/build' && exec bash"
fi
