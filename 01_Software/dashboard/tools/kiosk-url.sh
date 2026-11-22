#!/usr/bin/env bash
# Points the kiosk on the Pi at another page, to work on the dashboard on the real screen
# without building an image (docs/plan.md).
#
# Usage: ./tools/kiosk-url.sh dev [params]   the Vite dev server on this PC; start "npm run dev"
#                                     in web/ first; changes show up on the Pi at once (hot reload);
#                                     params go into the address, e.g. "fonts" or "page=weather"
#        ./tools/kiosk-url.sh prod [params]  the production build (npm run build, made here)
#                                     served by the same dev server under /prod/; this is how
#                                     fast the dashboard really is on the Pi
#        ./tools/kiosk-url.sh reset   back to the page from the image
#        ./tools/kiosk-url.sh show    print the current page
#        ./tools/kiosk-url.sh URL     any other page
#   HOST=other-host ./tools/kiosk-url.sh ...   SSH host, default: dashboard
#
# Only /etc/default/dashboard-kiosk of the running slot changes; "reset" or the next OTA
# update brings back the page from the image.
set -euo pipefail

HOST=${HOST:-dashboard}
DEV_PORT=5173
ENV_FILE=/etc/default/dashboard-kiosk
BACKUP=/etc/default/dashboard-kiosk.image

die() { echo "PRZERWANO: $*" >&2; exit 1; }
ssh_pi() { ssh -o ConnectTimeout=5 "$HOST" "$@"; }

set_url() {
  local url=${1//&/\\&} # & means "the match" in a sed replacement
  ssh_pi "[ -e $BACKUP ] || cp $ENV_FILE $BACKUP
    sed -i 's|^DASHBOARD_URL=.*|DASHBOARD_URL=$url|' $ENV_FILE
    systemctl restart dashboard-kiosk
    grep '^DASHBOARD_URL=' $ENV_FILE"
}

# The address of this PC as seen from the Pi: the source address of the route to it.
pc_address() {
  local name pi
  name=$(ssh -G "$HOST" | awk '/^hostname / { print $2 }')
  pi=$(getent ahostsv4 "$name" | awk 'NR == 1 { print $1 }')
  [[ -n $pi ]] || die "nie znam adresu $name"
  ip -4 route get "$pi" | sed -n 's/.* src \([0-9.]*\).*/\1/p'
}

# Points the kiosk at the dev server on this PC; $1 is the path ("" or "prod/"), $2 the params.
use_dev_server() {
  curl -fsS -o /dev/null "http://127.0.0.1:$DEV_PORT/" 2>/dev/null ||
    die "serwer Vite nie działa. Uruchom w drugim terminalu: cd web && npm run dev"
  local pc url
  pc=$(pc_address)
  url="http://$pc:$DEV_PORT/$1${2:+?$2}"
  if ! ssh_pi "wget -q -O /dev/null -T 3 http://$pc:$DEV_PORT/$1"; then
    die "Pi nie widzi $url. Otwórz port na PC do restartu:
  sudo firewall-cmd --add-port=$DEV_PORT/tcp"
  fi
  set_url "$url"
}

case "${1:-}" in
  dev)
    use_dev_server "" "${2:-}"
    echo "Kiosk pokazuje serwer Vite z tego PC. Powrót: $0 reset"
    ;;
  prod)
    (cd "$(dirname "$0")/../web" && npm run build --silent >/dev/null) || die "npm run build nie przeszedł"
    use_dev_server "prod/" "${2:-}"
    echo "Kiosk pokazuje wersję produkcyjną z tego PC (po zmianach: $0 prod). Powrót: $0 reset"
    ;;
  reset)
    ssh_pi "if [ -e $BACKUP ]; then mv $BACKUP $ENV_FILE && systemctl restart dashboard-kiosk; fi
      grep '^DASHBOARD_URL=' $ENV_FILE"
    ;;
  show)
    ssh_pi "grep '^DASHBOARD_URL=' $ENV_FILE"
    ;;
  http://* | https://* | file://*)
    set_url "$1"
    ;;
  *)
    sed -n '2,17p' "$0"
    exit 2
    ;;
esac
