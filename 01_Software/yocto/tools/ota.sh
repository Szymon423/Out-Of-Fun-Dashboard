#!/usr/bin/env bash
# Installs a RAUC update bundle on the Pi over SSH and reboots into it (docs/ota-rauc.md).
#
# Usage: ./tools/ota.sh [-H host] [-n] [bundle.raucb]
#        ./tools/ota.sh [-H host] -R
#   bundle  default: the latest dashboard-bundle from the deploy directory
#   -H      SSH host, default: dashboard
#   -n      install only; the new slot starts on the next reboot
#   -R      no install: reboot and watch the boot (for rollback tests); REBOOT_CMD replaces
#           the reboot, e.g. REBOOT_CMD='echo c > /proc/sysrq-trigger' for a kernel panic
#
# Steps: copy the bundle to /data/ota, let RAUC on the Pi check it (signature, compatible),
# install it into the inactive slot, reboot and wait until a boot is accepted, that is until
# rauc-mark-good has run after the health check. A slot that fails three times is given up
# by U-Boot, which boots the other one; the script then says so.
set -euo pipefail

YOCTO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
HOST=dashboard
MODE=install
REBOOT=1
OTA_DIR=/data/ota
# Three failed tries of about two minutes each, plus the boot of the other slot.
BOOT_TIMEOUT_S=${BOOT_TIMEOUT_S:-900}

die() { echo "PRZERWANO: $*" >&2; exit 1; }
step() { printf '\n== %s\n' "$*"; }

machine() {
  local m
  m=$(sed -n '/^OE_FRAGMENTS/s#.*machine/\([^ "]*\).*#\1#p' "$YOCTO_DIR/build/conf/toolcfg.conf" 2>/dev/null | head -n 1)
  echo "${m:-raspberrypi4-64}"
}

while getopts ":H:nRh" opt; do
  case $opt in
    H) HOST=$OPTARG ;;
    n) REBOOT=0 ;;
    R) MODE=reboot ;;
    h) sed -n '2,15p' "$0"; exit 0 ;;
    *) die "nieznana opcja. Pomoc: $0 -h" ;;
  esac
done
shift $(( OPTIND - 1 ))

SOCK="${XDG_RUNTIME_DIR:-/tmp}/dashboard-ota-ssh.$$"
SSH_OPTS=(-o ControlMaster=auto -o "ControlPath=$SOCK" -o ControlPersist=60
          -o ConnectTimeout=5 -o ServerAliveInterval=5 -o ServerAliveCountMax=3)
ssh_pi() { ssh "${SSH_OPTS[@]}" "$HOST" "$@"; }
close_master() { ssh "${SSH_OPTS[@]}" -O exit "$HOST" 2>/dev/null || true; }
trap close_master EXIT

booted_slot() { ssh_pi 'sed -n "s/.*rauc\.slot=\([^ ]*\).*/\1/p" /proc/cmdline'; }

# One line about the running system: boot_id, slot, rauc-mark-good state, health check state.
STATE_CMD='echo "$(cat /proc/sys/kernel/random/boot_id)" \
  "$(sed -n "s/.*rauc\.slot=\([^ ]*\).*/\1/p" /proc/cmdline)" \
  "$(systemctl show -P ActiveState rauc-mark-good.service)" \
  "$(systemctl show -P ActiveState dashboard-health.service)"'

health_text() {
  case $1 in
    activating) echo "trwa" ;;
    active) echo "zdrowy" ;;
    failed) echo "nie przeszło, Pi się restartuje" ;;
    *) echo "$1" ;;
  esac
}

# Reboots the Pi and waits until a boot is accepted (rauc-mark-good active). Every new
# boot and every change of the health check is printed, so failed tries are visible.
# Sets BOOTED_SLOT and BOOTS.
reboot_and_wait() {
  local t=$SECONDS old_boot info boot slot good health last_boot="" last_health="" n=0
  old_boot=$(ssh_pi 'cat /proc/sys/kernel/random/boot_id')
  ssh_pi "${REBOOT_CMD:-systemctl reboot}" || true
  close_master
  # The old system still answers SSH for a few seconds after the reboot command, so only
  # a different boot_id counts. No shared connection while polling.
  while :; do
    sleep 3
    info=$(ssh -o ControlPath=none -o BatchMode=yes -o ConnectTimeout=5 "$HOST" "$STATE_CMD" 2>/dev/null) || info=""
    if [[ -n $info ]]; then
      read -r boot slot good health <<< "$info"
      if [[ $boot != "$old_boot" ]]; then
        if [[ $boot != "$last_boot" ]]; then
          n=$(( n + 1 ))
          last_boot=$boot
          last_health=""
          echo "$(( SECONDS - t )) s: start $n, slot $slot"
        fi
        if [[ $health != "$last_health" ]]; then
          echo "$(( SECONDS - t )) s:   sprawdzenie stanu: $(health_text "$health")"
          last_health=$health
        fi
        if [[ $good == active ]]; then
          BOOTED_SLOT=$slot
          BOOTS=$n
          echo "Start zaakceptowany po $(( SECONDS - t )) s."
          return
        fi
      fi
    fi
    (( SECONDS - t < BOOT_TIMEOUT_S )) || die "żaden start nie został zaakceptowany w ciągu ${BOOT_TIMEOUT_S} s."
  done
}

show_state() {
  ssh_pi 'fw_printenv BOOT_ORDER BOOT_A_LEFT BOOT_B_LEFT; systemctl is-system-running || true'
  ssh_pi "rauc status" | sed -n '/=== Slot States ===/,$p'
  echo "Ostatnie sprawdzenia stanu (/data/dashboard-health.log):"
  ssh_pi 'tail -n 6 /data/dashboard-health.log 2>/dev/null || echo "  brak pliku"'
}

start=$SECONDS

if [[ $MODE == reboot ]]; then
  step "Pi przed restartem"
  old_slot=$(booted_slot) || die "brak połączenia z $HOST"
  ssh_pi 'fw_printenv BOOT_ORDER BOOT_A_LEFT BOOT_B_LEFT'
  step "Restart"
  reboot_and_wait
  step "Pi po restarcie"
  show_state
  echo
  echo "GOTOWE w $(( SECONDS - start )) s. Przed restartem działał slot $old_slot, teraz działa $BOOTED_SLOT (startów: $BOOTS)."
  exit 0
fi

bundle=${1:-$YOCTO_DIR/build/tmp/deploy/images/$(machine)/dashboard-bundle-$(machine).raucb}
[[ -f $bundle ]] || die "nie ma pakietu $bundle. Zbuduj go: ./build.sh dashboard-bundle"
bundle=$(readlink -f "$bundle")
name=$(basename "$bundle")

echo "Pakiet: $name"
echo "        $(du -h "$bundle" | cut -f1), zbudowany $(date -r "$bundle" '+%Y-%m-%d %H:%M')"

step "Pi przed aktualizacją"
old_slot=$(booted_slot) || die "brak połączenia z $HOST"
[[ $old_slot == A || $old_slot == B ]] || die "Pi nie podaje rauc.slot w /proc/cmdline"
new_slot=$([[ $old_slot == A ]] && echo B || echo A)
echo "Działa slot $old_slot, aktualizacja trafi do slotu $new_slot."
need_kb=$(( $(du -k "$bundle" | cut -f1) + 64 * 1024 ))
free_kb=$(ssh_pi "df -k /data | awk 'NR == 2 { print \$4 }'")
(( free_kb > need_kb )) || die "za mało miejsca na /data: wolne ${free_kb} KiB, potrzeba ${need_kb} KiB"

step "Wysyłanie do $OTA_DIR"
ssh_pi "mkdir -p $OTA_DIR && rm -f $OTA_DIR/*.raucb"
t=$SECONDS
# Plain ssh with cat: about 15 MB/s against 5 MB/s for scp, which goes through SFTP and
# is slow with Dropbear. A broken transfer cannot slip through, because RAUC checks the
# signature and the verity hash of the bundle.
dd if="$bundle" bs=1M status=progress | ssh_pi "cat > $OTA_DIR/$name && sync"
echo "Wysłano w $(( SECONDS - t )) s."

step "Sprawdzenie pakietu na Pi"
# rauc info checks the signature against the keyring of the running system.
if ! ssh_pi "rauc info $OTA_DIR/$name"; then
  ssh_pi "rm -f $OTA_DIR/$name"
  die "RAUC odrzucił pakiet."
fi

step "Instalacja do slotu $new_slot"
t=$SECONDS
status=0
ssh_pi "rauc install $OTA_DIR/$name" || status=$?
ssh_pi "rm -f $OTA_DIR/$name"
(( status == 0 )) || die "instalacja nie powiodła się (kod $status). Pi dalej działa na slocie $old_slot."
echo "Zainstalowano w $(( SECONDS - t )) s."
ssh_pi 'fw_printenv BOOT_ORDER BOOT_A_LEFT BOOT_B_LEFT'

if (( ! REBOOT )); then
  echo
  echo "GOTOWE bez restartu. Slot $new_slot uruchomi się przy następnym starcie."
  exit 0
fi

step "Restart"
reboot_and_wait

step "Pi po aktualizacji"
show_state
echo
if [[ $BOOTED_SLOT == "$new_slot" ]]; then
  (( BOOTS == 1 )) || echo "UWAGA: nowy system zaakceptowany dopiero przy starcie $BOOTS." >&2
  echo "GOTOWE w $(( SECONDS - start )) s. Działa slot $new_slot z nowym systemem."
else
  echo "UWAGA: działa slot $BOOTED_SLOT, a nie $new_slot. Nowy system nie przeszedł sprawdzenia" >&2
  echo "albo nie wystartował, więc U-Boot wrócił do starego. Powód jest w dzienniku sprawdzeń wyżej." >&2
  exit 1
fi
