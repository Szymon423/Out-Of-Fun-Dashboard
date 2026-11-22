#!/usr/bin/env bash
# Manages the RAUC signing keys in keys/, which never go to git (see .gitignore).
#
# Usage: ./tools/rauc-keys.sh <command>
#   create        generate a new CA and signing key; refuses when keys already exist
#   info          show the certificates and check that the key, certificate and CA match
#   backup [dir]  write an encrypted (gpg) archive of keys/ to dir, default: $HOME
#   rotate        move the current keys to keys/archive/ and generate new ones
#
# The keys come from meta-rauc's scripts/openssl-ca.sh. The script in the layer is not
# modified: sed changes the organization name in a copy that is piped to bash.
# The certificates are valid from 1970 to 9999, so a Pi without a battery-backed clock
# accepts them right after boot.
set -euo pipefail

YOCTO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
KEYS_DIR="$YOCTO_DIR/keys"
CA_DIR="$KEYS_DIR/openssl-ca"
DEV_DIR="$CA_DIR/dev"
UPSTREAM="$YOCTO_DIR/layers/meta-rauc/scripts/openssl-ca.sh"
ORG="Out Of Fun"

die() { echo "PRZERWANO: $*" >&2; exit 1; }

usage() {
  cat >&2 <<EOF
Użycie: $0 <polecenie>
  create        nowe CA i klucz podpisu; odmawia, gdy klucze już istnieją
  info          pokazuje certyfikaty i sprawdza, czy klucz, certyfikat i CA pasują
  backup [dir]  zaszyfrowana kopia keys/ w katalogu dir, domyślnie \$HOME
  rotate        przenosi obecne klucze do keys/archive/ i tworzy nowe
EOF
  exit 2
}

generate() {
  [[ ! -e "$CA_DIR" ]] || die "$CA_DIR już istnieje"
  [[ -f "$UPSTREAM" ]] || die "brak $UPSTREAM, uruchom: git submodule update --init"
  grep -q '^ORG=' "$UPSTREAM" || die "skrypt z meta-rauc nie ma już linii ORG=, sprawdź go przed użyciem"
  git -C "$YOCTO_DIR" check-ignore -q "$KEYS_DIR/probe" || die "katalog keys/ nie jest w .gitignore"

  mkdir -p "$KEYS_DIR"
  chmod 700 "$KEYS_DIR"
  local log="$KEYS_DIR/openssl-ca.log"
  # umask 077: the private keys are readable only by their owner.
  if ! (umask 077 && cd "$KEYS_DIR" && sed "s/^ORG=.*/ORG=\"$ORG\"/" "$UPSTREAM" | bash) > "$log" 2>&1; then
    tail -n 20 "$log" >&2
    # The directory did not exist before this run, so it holds nothing worth keeping.
    rm -rf "$CA_DIR"
    die "generowanie kluczy nie powiodło się, pełny log: $log"
  fi
  mv "$log" "$CA_DIR/generate.log"
  echo "Wygenerowano klucze w $CA_DIR"
  echo
  info
}

info() {
  [[ -f "$DEV_DIR/ca.cert.pem" ]] || die "brak kluczy w $CA_DIR, uruchom: $0 create"
  local name
  for name in ca development-1; do
    echo "== $name.cert.pem"
    openssl x509 -in "$DEV_DIR/$name.cert.pem" -noout -subject -issuer -dates -fingerprint -sha256
  done
  openssl verify -CAfile "$DEV_DIR/ca.cert.pem" "$DEV_DIR/development-1.cert.pem" > /dev/null \
    || die "certyfikat podpisu nie został wystawiony przez to CA"
  # A key and a certificate belong together when their public keys are the same.
  [[ "$(openssl pkey -in "$DEV_DIR/private/development-1.key.pem" -pubout)" == \
     "$(openssl x509 -in "$DEV_DIR/development-1.cert.pem" -noout -pubkey)" ]] \
    || die "klucz podpisu nie pasuje do certyfikatu"

  echo
  echo "Sprawdzenie: OK, certyfikat podpisu pochodzi z tego CA i pasuje do klucza."
  echo "Keyring dla Pi (do warstwy):  $DEV_DIR/ca.cert.pem"
  echo "Podpis pakietów:              $DEV_DIR/private/development-1.key.pem"
  echo "                              $DEV_DIR/development-1.cert.pem"
  if [[ -d "$KEYS_DIR/archive" ]]; then
    echo "Stare klucze:"
    ls -1d "$KEYS_DIR"/archive/*/ | sed 's/^/  /'
  fi
}

backup() {
  local dir=${1:-$HOME}
  [[ -d "$CA_DIR" ]] || die "brak kluczy do zapisania, uruchom: $0 create"
  [[ -d "$dir" ]] || die "katalog $dir nie istnieje"
  local file
  file="$dir/dashboard-rauc-keys-$(date +%Y%m%d-%H%M%S).tar.gz.gpg"
  tar -C "$YOCTO_DIR" -czf - keys | gpg --symmetric --cipher-algo AES256 --output "$file"
  # Read the archive back to be sure it decrypts; gpg-agent usually remembers the passphrase.
  local count
  count=$(gpg --quiet --decrypt "$file" | tar -tzf - | wc -l)
  echo "Kopia: $file ($count pozycji w archiwum)"
  echo "Przenieś ją poza ten komputer, a hasło zapisz w menedżerze haseł."
  echo "Przywracanie: gpg -d '$file' | tar -xzf - -C '$YOCTO_DIR'"
}

rotate() {
  [[ -d "$CA_DIR" ]] || die "brak kluczy do wymiany, uruchom: $0 create"
  echo "Nowe klucze zastąpią obecne w $CA_DIR, a obecne trafią do keys/archive/."
  echo "Urządzenia odrzucą pakiety z nowym podpisem, dopóki nie dostaną nowego keyringa"
  echo "(procedura: docs/ota-rauc.md, część „Wymiana kluczy”)."
  local answer
  read -r -p "Wpisz TAK, żeby kontynuować: " answer
  [[ "$answer" == "TAK" ]] || die "anulowano"

  local old
  old="$KEYS_DIR/archive/openssl-ca-$(date +%Y%m%d-%H%M%S)"
  mkdir -p "$KEYS_DIR/archive"
  mv "$CA_DIR" "$old"
  echo "Stare klucze: $old"
  generate
  echo
  echo "Zrób teraz nową kopię: $0 backup"
}

case "${1:-}" in
  create)
    [[ ! -e "$CA_DIR" ]] || die "klucze już istnieją w $CA_DIR; nowe tworzy: $0 rotate"
    generate
    ;;
  info) info ;;
  backup) backup "${2:-}" ;;
  rotate) rotate ;;
  *) usage ;;
esac
