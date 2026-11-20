#!/usr/bin/env bash
# Builds Yocto targets in the container. BitBake runs without its interactive
# terminal UI: the terminal shows compact progress and the full log goes to a file.
#
# Usage: ./build.sh [-b] [bitbake arguments...]
#   -b       run in the background; follow it with: tail -f build/logs/progress.log
#   no args  builds DEFAULT_TARGET
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/container.sh"

DEFAULT_TARGET=dashboard-image
CONTAINER_NAME=dashboard-yocto-build
LOG_DIR="$YOCTO_DIR/build/logs"
PROGRESS="$LOG_DIR/progress.log"
LOCK="$LOG_DIR/.build.lock"

# Turns BitBake's plain output into a short progress view.
summarize() {
  awk '
    function show(line) { print line; fflush() }
    BEGIN { last_pct = -100 }
    /^Build Configuration:/ { block = 1 }
    block { show($0); if ($0 == "") block = 0; next }
    /^NOTE: Running (setscene )?task [0-9]+ of [0-9]+/ {
      setscene = ($3 == "setscene")
      n = setscene ? $5 : $4
      total = setscene ? $7 : $6
      phase = setscene ? "pamięć podręczna" : "zadania"
      pct = int(n * 100 / total)
      if (phase != last_phase || pct >= last_pct + 5 || n == total) {
        task = $0
        sub(/^[^(]*\(/, "", task); sub(/\)$/, "", task); sub(/.*\//, "", task)
        printf "[%3d%%] %s %d/%d  %s\n", pct, phase, n, total, task
        fflush()
        last_phase = phase
        last_pct = pct
      }
      next
    }
    /^(WARNING|ERROR):|^\| |^Summary:|^NOTE: Tasks Summary|^Parsing of |^Sstate summary/ { show($0) }
  '
}

mkdir -p "$LOG_DIR"

# One build at a time: the lock is taken before any log is touched.
# A background run inherits the lock already held on descriptor 9.
if [[ -z ${BUILD_LOCK_HELD:-} ]]; then
  exec 9> "$LOCK"
  if ! flock -n 9; then
    echo "Build już trwa. Postęp: tail -f $PROGRESS" >&2
    echo "Przerwij: podman stop $CONTAINER_NAME" >&2
    exit 1
  fi
fi

if [[ ${1:-} == -b ]]; then
  shift
  BUILD_LOCK_HELD=1 setsid nohup "$0" "$@" > /dev/null 2>&1 < /dev/null &
  echo "Build ruszył w tle."
  echo "Postęp:   tail -f $PROGRESS"
  echo "Przerwij: podman stop $CONTAINER_NAME"
  exit 0
fi

(( $# )) || set -- "$DEFAULT_TARGET"
log="$LOG_DIR/build-$(date +%Y%m%d-%H%M%S).log"
ln -sfn "$(basename "$log")" "$LOG_DIR/latest.log"
echo "bitbake $* (pełny log: $log)" | tee "$PROGRESS"

# The log and progress filters ignore Ctrl+C, so BitBake can stop cleanly and
# its final messages still reach the log.
set +e
run_in_container --name "$CONTAINER_NAME" -- bash -c "$BUILD_ENV && bitbake \"\$@\"" bitbake "$@" 2>&1 |
  ( trap '' INT; tee "$log" ) |
  ( trap '' INT; summarize ) |
  ( trap '' INT; tee -a "$PROGRESS" )
status=${PIPESTATUS[0]}
set -e

if (( status == 0 )); then
  echo "GOTOWE." | tee -a "$PROGRESS"
  if [[ -d "$YOCTO_DIR/build/tmp/deploy/images" ]]; then
    echo "Obrazy: $YOCTO_DIR/build/tmp/deploy/images/" | tee -a "$PROGRESS"
  fi
else
  echo "BŁĄD (kod $status). Pełny log: $log" | tee -a "$PROGRESS"
fi
exit "$status"
