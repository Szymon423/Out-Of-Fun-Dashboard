#!/usr/bin/env bash
# Installs a RAUC update bundle on the Pi over SSH and reboots into it (docs/ota-rauc.md).
#
# Usage: ./tools/ota.sh [-H host] [-n] [bundle.raucb]
#   bundle  default: the latest dashboard-bundle from the deploy directory
#   -H      SSH host, default: dashboard
#   -n      install only; the new slot starts on the next reboot
#
# Steps: copy the bundle to /data/ota, let RAUC on the Pi check it (signature, compatible),
# install it into the inactive slot, reboot, wait for SSH and show which slot is running.
# If the new slot fails to boot three times, U-Boot goes back to the old one; the script
# then says so.
set -euo pipefail

YOCTO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
HOST=dashboard
REBOOT=1
OTA_DIR=/data/ota
BOOT_TIMEOUT_S=${BOOT_TIMEOUT_S:-240}

die() { echo "PRZERWANO: $*" >&2; exit 1; }
step() { printf '\n== %s\n' "$*"; }

machine() {
  local m
  m=$(sed -n '/^OE_FRAGMENTS/s#.*machine/\([^ "]*\).*#\1#p' "$YOCTO_DIR/build/conf/toolcfg.conf" 2>/dev/null | head -n 1)
  echo "${m:-raspberrypi4-64}"
}

while getopts ":H:nh" opt; do
  case $opt in
    H) HOST=$OPTARG ;;
    n) REBOOT=0 ;;
    h) sed -n '2,12p' "$0"; exit 0 ;;
    *) die "nieznana opcja. Pomoc: $0 -h" ;;
  esac
done
shift $(( OPTIND - 1 ))

bundle=${1:-$YOCTO_DIR/build/tmp/deploy/images/$(machine)/dashboard-bundle-$(machine).raucb}
[[ -f $bundle ]] || die "nie ma pakietu $bundle. Zbuduj go: ./build.sh dashboard-bundle"
bundle=$(readlink -f "$bundle")
name=$(basename "$bundle")

SOCK="${XDG_RUNTIME_DIR:-/tmp}/dashboard-ota-ssh.$$"
SSH_OPTS=(-o ControlMaster=auto -o "ControlPath=$SOCK" -o ControlPersist=60
          -o ConnectTimeout=5 -o ServerAliveInterval=5 -o ServerAliveCountMax=3)
ssh_pi() { ssh "${SSH_OPTS[@]}" "$HOST" "$@"; }
close_master() { ssh "${SSH_OPTS[@]}" -O exit "$HOST" 2>/dev/null || true; }
trap close_master EXIT

booted_slot() { ssh_pi 'sed -n "s/.*rauc\.slot=\([^ ]*\).*/\1/p" /proc/cmdline'; }

start=$SECONDS
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
t=$SECONDS
old_boot=$(ssh_pi 'cat /proc/sys/kernel/random/boot_id')
ssh_pi 'systemctl reboot' || true
close_master
# The old system still answers SSH for a few seconds after the reboot command, so wait
# for a different boot_id rather than for any answer. No shared connection while polling.
while :; do
  sleep 3
  boot=$(ssh -o ControlPath=none -o BatchMode=yes -o ConnectTimeout=5 "$HOST" \
           'cat /proc/sys/kernel/random/boot_id' 2>/dev/null) || boot=""
  [[ -n $boot && $boot != "$old_boot" ]] && break
  (( SECONDS - t < BOOT_TIMEOUT_S )) || die "Pi nie wróciło po ${BOOT_TIMEOUT_S} s od restartu."
done
echo "Nowy start, SSH odpowiada po $(( SECONDS - t )) s."

step "Pi po aktualizacji"
slot=$(booted_slot)
ssh_pi 'fw_printenv BOOT_ORDER BOOT_A_LEFT BOOT_B_LEFT; systemctl is-system-running || true'
ssh_pi "rauc status" | sed -n '/=== Slot States ===/,$p'
echo
if [[ $slot == "$new_slot" ]]; then
  echo "GOTOWE w $(( SECONDS - start )) s. Działa slot $new_slot z nowym systemem."
else
  echo "UWAGA: działa slot $slot, a nie $new_slot. Nowy system nie wystartował i U-Boot wrócił do starego." >&2
  exit 1
fi
