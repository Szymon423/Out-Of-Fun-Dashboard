#!/usr/bin/env bash
# Samples the browser's memory use (PSS, summed over all its processes) and the
# memory left on the system. Usage: ./measure.sh <chromium|cog> [seconds] [interval]
set -euo pipefail

engine="${1:-}"
duration="${2:-1800}"
interval="${3:-5}"

case "$engine" in
  chromium) default_re='^(chromium|chrome)' ;;
  cog) default_re='^(cog|WPEWebProcess|WPENetworkProce|WPEGPUProcess)$' ;;
  *)
    echo "Usage: $0 <chromium|cog> [seconds] [interval]" >&2
    exit 2
    ;;
esac
proc_re="${PROC_RE:-$default_re}"

output="results-${engine}-$(date +%Y%m%d-%H%M%S).csv"
echo "elapsed_s,processes,browser_pss_mb,mem_available_mb,swap_used_mb" > "$output"

meminfo_mb() {
  awk -v key="$1:" '$1 == key { printf "%d", $2 / 1024 }' /proc/meminfo
}

start=$SECONDS
peak_pss=0
min_available=-1
samples=0

while (( SECONDS - start < duration )); do
  pss_kb=0
  count=0
  while read -r pid comm; do
    [[ "$comm" =~ $proc_re ]] || continue
    value=$(awk '/^Pss:/ { print $2 }' "/proc/$pid/smaps_rollup" 2>/dev/null || true)
    [[ -n "$value" ]] || continue
    pss_kb=$(( pss_kb + value ))
    count=$(( count + 1 ))
  done < <(ps -eo pid=,comm=)

  pss_mb=$(( pss_kb / 1024 ))
  available=$(meminfo_mb MemAvailable)
  swap_total=$(meminfo_mb SwapTotal)
  swap_free=$(meminfo_mb SwapFree)
  swap_used=$(( swap_total - swap_free ))

  echo "$(( SECONDS - start )),$count,$pss_mb,$available,$swap_used" >> "$output"
  (( pss_mb > peak_pss )) && peak_pss=$pss_mb
  if (( min_available < 0 || available < min_available )); then min_available=$available; fi
  samples=$(( samples + 1 ))
  printf '\r%4ds  procesy: %-3d  przeglądarka: %5d MB  dostępne: %5d MB  swap: %4d MB ' \
    "$(( SECONDS - start ))" "$count" "$pss_mb" "$available" "$swap_used"
  sleep "$interval"
done

echo
echo "Próbki: $samples"
echo "Szczyt pamięci przeglądarki: $peak_pss MB"
echo "Najmniej dostępnej pamięci: $min_available MB"
echo "Dane zapisane w: $output"
