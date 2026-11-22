#!/usr/bin/env bash
# Runs the diagnostic Canvas 2D and DOM scenes of stress.html on the Pi, once per WebKit
# rendering setting, and prints the results side by side.
#
# Usage: ./diag.sh [setting...]          default: all settings, see SETTINGS below
#   ONLY=cv-img,dom-text ./diag.sh base   run only the listed scenes
#   HOST=other-host ./diag.sh             SSH host, default: dashboard
#
# Copies the current stress.html to the Pi first. The kiosk settings are restored at the end,
# also after Ctrl+C. Raw results go to results/diag-<date>/.
set -euo pipefail

HOST=${HOST:-dashboard}
ONLY=${ONLY:-}
TIMEOUT_S=${TIMEOUT_S:-600}
SPIKE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ENV_FILE=/etc/default/dashboard-kiosk
PAGE_DIR=/usr/share/dashboard-spike

# Environment variables read by WPE WebKit 2.54 (WebCore/platform/graphics/skia).
declare -A SETTINGS=(
  [base]=""
  [nomsaa]="WEBKIT_SKIA_MSAA_SAMPLE_COUNT=0"
  [gpu1]="WEBKIT_SKIA_GPU_PAINTING_THREADS=1"
  [gpu4]="WEBKIT_SKIA_GPU_PAINTING_THREADS=4"
  [cpu]="WEBKIT_SKIA_ENABLE_CPU_RENDERING=1"
  [cpu4]="WEBKIT_SKIA_ENABLE_CPU_RENDERING=1 WEBKIT_SKIA_CPU_PAINTING_THREADS=4"
)
declare -A LABELS=(
  [base]="domyślnie: malowanie na GPU, 2 wątki, MSAA 4x"
  [nomsaa]="GPU bez MSAA"
  [gpu1]="GPU, 1 wątek malowania"
  [gpu4]="GPU, 4 wątki malowania"
  [cpu]="malowanie na CPU, 2 wątki"
  [cpu4]="malowanie na CPU, 4 wątki"
)
ORDER=(base nomsaa gpu1 gpu4 cpu cpu4)

die() { echo "PRZERWANO: $*" >&2; exit 1; }

names=("$@")
(( ${#names[@]} )) || names=("${ORDER[@]}")
for n in "${names[@]}"; do
  [[ -v "SETTINGS[$n]" ]] || die "nieznane ustawienie '$n', dostępne: ${ORDER[*]}"
done

SOCK="${XDG_RUNTIME_DIR:-/tmp}/dashboard-diag-ssh.$$"
SSH_OPTS=(-o ControlMaster=auto -o "ControlPath=$SOCK" -o ControlPersist=120 -o ConnectTimeout=10)
ssh_pi() { ssh "${SSH_OPTS[@]}" "$HOST" "$@"; }

out="$SPIKE_DIR/results/diag-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$out"

ssh_pi cat "$ENV_FILE" > "$out/kiosk.env.orig" || die "nie mogę odczytać $ENV_FILE z $HOST"
[[ -s "$out/kiosk.env.orig" ]] || die "$ENV_FILE na $HOST jest pusty"

restore() {
  echo
  echo "Przywracam ustawienia kiosku…"
  ssh_pi "cat > $ENV_FILE && systemctl restart dashboard-kiosk" < "$out/kiosk.env.orig" \
    || echo "UWAGA: nie udało się przywrócić $ENV_FILE, kopia: $out/kiosk.env.orig" >&2
  ssh "${SSH_OPTS[@]}" -O exit "$HOST" 2>/dev/null || true
}
trap restore EXIT
trap 'exit 130' INT TERM

scp -q "${SSH_OPTS[@]}" "$SPIKE_DIR/stress.html" "$HOST:$PAGE_DIR/stress.html"

# Prints one line per finished scene: name, largest smooth level, FPS at each level.
scene_line() {
  python3 -c '
import json, sys
s = json.loads(sys.argv[1])
levels = " ".join("{}:{}".format(l["n"], l["fps"]) for l in s["levels"])
print("    {:<38} płynnie do {:>5}   {}".format(s["name"], s["max"] or "-", levels))
' "$1"
}

url="file://$PAGE_DIR/stress.html?auto=diag${ONLY:+&only=$ONLY}"
for name in "${names[@]}"; do
  echo "== $name: ${LABELS[$name]}"
  {
    echo "DASHBOARD_URL=$url"
    for kv in ${SETTINGS[$name]}; do echo "$kv"; done
  } | ssh_pi "cat > $ENV_FILE"
  since=$(ssh_pi "date +%s && systemctl restart dashboard-kiosk")

  start=$SECONDS shown=0 result=""
  while (( SECONDS - start < TIMEOUT_S )); do
    sleep 5
    lines=$(ssh_pi "journalctl -u dashboard-kiosk --since @$since -o cat --no-pager | grep -E ' (SCENE|DIAG)_RESULT '" || true)
    scenes=$(grep -F ' SCENE_RESULT ' <<<"$lines" || true)
    count=$(grep -c . <<<"$scenes" || true)
    while (( shown < count )); do
      shown=$(( shown + 1 ))
      scene_line "$(sed -n "${shown}p" <<<"$scenes" | sed 's/.* SCENE_RESULT //')"
    done
    result=$(grep -F ' DIAG_RESULT ' <<<"$lines" | tail -n 1 || true)
    [[ -n "$result" ]] && break
  done
  [[ -n "$result" ]] || die "brak wyniku dla '$name' po ${TIMEOUT_S} s"
  sed 's/.* DIAG_RESULT //' <<<"$result" > "$out/$name.json"
done

echo
echo "Wyniki: FPS / czas JS na klatkę w ms, ✓ = płynnie. Surowe dane: $out"
python3 - "$out" "${names[@]}" <<'EOF'
import json, sys
out, names = sys.argv[1], sys.argv[2:]
runs = {n: json.load(open(f"{out}/{n}.json")) for n in names}
first = runs[names[0]]
print(f"Progi płynności: {first['minFps']} FPS i p95 <= {first['maxP95']} ms (ekran {first['refreshHz']} Hz)")
order = [s["id"] for s in first["scenes"]]
for sid in order:
    scene = next(s for s in first["scenes"] if s["id"] == sid)
    all_levels = sorted({l["n"] for r in runs.values() for s in r["scenes"] if s["id"] == sid for l in s["levels"]})
    print(f"\n{scene['name']} ({scene['unit']})")
    print("  " + " " * 8 + "".join(f"{n:>13}" for n in all_levels))
    for n in names:
        s = next((s for s in runs[n]["scenes"] if s["id"] == sid), None)
        cells = []
        for lvl in all_levels:
            l = next((l for l in s["levels"] if l["n"] == lvl), None) if s else None
            if not l:
                cells.append(f"{'-':>13}")
                continue
            ok = l["fps"] >= runs[n]["minFps"] and l["p95"] <= runs[n]["maxP95"]
            cells.append(f"{l['fps']:>6.1f}/{l.get('js', 0):<4.1f}{'✓' if ok else ' ':>2}")
        print(f"  {n:<8}" + "".join(cells))
EOF
