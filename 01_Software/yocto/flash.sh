#!/usr/bin/env bash
# Writes a built Yocto image to an SD card with bmaptool.
#
# Usage: ./flash.sh [-n] [-d /dev/sdX] [image]
#   image  image recipe name, default: dashboard-image
#   -d     target device; without it the script finds the SD card itself
#   -n     dry run: show what would happen without writing anything
set -euo pipefail

YOCTO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
DEFAULT_IMAGE=dashboard-image
MAX_SIZE_BYTES=$(( 256 * 1000 ** 3 ))
WAIT_SECONDS=${WAIT_SECONDS:-120}
DRY_RUN=0

die() { echo "PRZERWANO: $*" >&2; exit 1; }

run() {
  if (( DRY_RUN )); then
    echo "[próba] $*"
  else
    "$@"
  fi
}

machine() {
  local m
  m=$(sed -n '/^OE_FRAGMENTS/s#.*machine/\([^ "]*\).*#\1#p' "$YOCTO_DIR/build/conf/toolcfg.conf" 2>/dev/null | head -n 1)
  echo "${m:-raspberrypi4-64}"
}

# Disks that hold the running system; never valid targets.
system_disks() {
  local target src
  for target in / /boot /boot/efi /home; do
    src=$(findmnt -no SOURCE "$target" 2>/dev/null | sed 's/\[.*//') || continue
    [[ -n $src ]] && lsblk -nplso NAME "$src" 2>/dev/null | tail -n 1
  done | sort -u
}

# Removable disks (or SD slots) up to MAX_SIZE_BYTES that are not system disks.
candidates() {
  local sys name rm size type
  sys=$(system_disks)
  while read -r name rm size type; do
    [[ $type == disk ]] || continue
    [[ $name =~ (boot[0-9]+|rpmb)$ ]] && continue
    [[ $rm == 1 || $name == /dev/mmcblk* ]] || continue
    (( size > 0 && size <= MAX_SIZE_BYTES )) || continue
    grep -qxF "$name" <<< "$sys" && continue
    echo "$name"
  done < <(lsblk -dnpbo NAME,RM,SIZE,TYPE)
}

choose_device() {
  local list=() waited=0 i
  while :; do
    mapfile -t list < <(candidates)
    (( ${#list[@]} )) && break
    (( waited == 0 )) && echo "Nie widzę karty SD. Włóż ją do czytnika (czekam do $WAIT_SECONDS s)..." >&2
    (( waited >= WAIT_SECONDS )) && die "nie znaleziono karty SD."
    sleep 2
    waited=$(( waited + 2 ))
  done
  if (( ${#list[@]} == 1 )); then
    echo "${list[0]}"
    return
  fi
  echo "Znaleziono kilka kart:" >&2
  for i in "${!list[@]}"; do
    printf '  %d) %s  %s\n' $(( i + 1 )) "${list[$i]}" "$(lsblk -dno SIZE,MODEL "${list[$i]}")" >&2
  done
  read -r -p "Wybierz numer: " i < /dev/tty
  [[ $i =~ ^[0-9]+$ ]] && (( i >= 1 && i <= ${#list[@]} )) || die "nieprawidłowy wybór."
  echo "${list[$(( i - 1 ))]}"
}

validate_device() {
  local dev=$1
  [[ -b $dev ]] || die "$dev nie jest urządzeniem blokowym."
  [[ $(lsblk -dno TYPE "$dev") == disk ]] || die "$dev nie jest całym dyskiem. Podaj na przykład /dev/sdb, a nie /dev/sdb1."
  ! grep -qxF "$dev" <<< "$(system_disks)" || die "$dev zawiera działający system."
  (( $(lsblk -dnbo SIZE "$dev") <= MAX_SIZE_BYTES )) || die "$dev ma ponad $(( MAX_SIZE_BYTES / 1000 ** 3 )) GB, to raczej nie jest karta SD."
}

unmount_all() {
  local part mountpoint
  while read -r part mountpoint; do
    [[ -n $mountpoint ]] || continue
    run sudo umount --all-targets "$part"
  done < <(lsblk -nlpo NAME,MOUNTPOINT "$1")
}

main() {
  local device="" opt image deploy img bmap dev answer
  while getopts ":nd:h" opt; do
    case $opt in
      n) DRY_RUN=1 ;;
      d) device=$OPTARG ;;
      h) sed -n '2,7p' "$0"; exit 0 ;;
      *) die "nieznana opcja. Pomoc: ./flash.sh -h" ;;
    esac
  done
  shift $(( OPTIND - 1 ))

  image=${1:-$DEFAULT_IMAGE}
  deploy="$YOCTO_DIR/build/tmp/deploy/images/$(machine)"
  img="$deploy/$image-$(machine).rootfs.wic.bz2"
  bmap="$deploy/$image-$(machine).rootfs.wic.bmap"
  if [[ ! -f $img ]]; then
    echo "Dostępne obrazy:" >&2
    find "$deploy" -maxdepth 1 -name "*-$(machine).rootfs.wic.bz2" -printf '  %f\n' 2>/dev/null |
      sed "s/-$(machine)\.rootfs\.wic\.bz2$//" >&2 || true
    die "nie znaleziono obrazu $img. Najpierw uruchom ./build.sh $image"
  fi
  [[ -f $bmap ]] || die "brak pliku $bmap."
  command -v bmaptool > /dev/null || die "brak bmaptool. Zainstaluj: sudo dnf install bmap-tools"

  echo "Obraz: $(basename "$img")"
  echo "       $(du -hL "$img" | cut -f1), zbudowany $(date -r "$img" '+%Y-%m-%d %H:%M')"

  dev=${device:-$(choose_device)}
  validate_device "$dev"

  echo
  echo "Karta: $dev  $(lsblk -dno SIZE,TRAN,MODEL,SERIAL "$dev")"
  lsblk -o NAME,SIZE,FSTYPE,LABEL,MOUNTPOINTS "$dev"
  echo

  if (( DRY_RUN )); then
    echo "[próba] potwierdzenie nazwą ${dev##*/}"
  else
    read -r -p "Cała zawartość $dev zostanie skasowana. Wpisz ${dev##*/}, aby kontynuować: " answer < /dev/tty
    [[ $answer == "${dev##*/}" ]] || die "brak potwierdzenia, nic nie zmieniono."
    sudo -v
  fi

  unmount_all "$dev"
  run sudo bmaptool copy --bmap "$bmap" "$img" "$dev"
  run sync
  run sudo blockdev --rereadpt "$dev" || true
  if (( ! DRY_RUN )); then
    udevadm settle
    echo
    lsblk -o NAME,SIZE,FSTYPE,LABEL "$dev"
    # The desktop may mount the new partitions right away.
    unmount_all "$dev"
  fi

  if run udisksctl power-off -b "$dev"; then
    echo "GOTOWE. Karta jest wyłączona i można ją wyjąć."
  else
    echo "GOTOWE. Nie udało się wyłączyć czytnika, ale partycje są odmontowane, więc kartę można wyjąć."
  fi
}

main "$@"
