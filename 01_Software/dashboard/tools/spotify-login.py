#!/usr/bin/env python3
"""Signs the dashboard in to Spotify, once (docs/plan.md, stage D4).

Spotify sends the browser back to http://127.0.0.1:8888/callback, an address that only this
PC can answer, so the login runs here. The refresh token it gets goes to the Pi as
/data/dashboard/spotify.json; from then on dashboard-server renews its access by itself.

The login uses the authorization code flow with PKCE, so no client secret is needed. The
client ID is read from spotify.clientId in the dashboard's config.json.

Usage: tools/spotify-login.py [--host HOST]   sign in for the Pi (SSH host, default: dashboard)
       tools/spotify-login.py --local DIR     sign in for the server on this PC (data dir DIR)

Run it again whenever the dashboard says it is not signed in, e.g. after the access was
removed on spotify.com.
"""

import argparse
import base64
import hashlib
import http.server
import json
import secrets
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request
import webbrowser
from pathlib import Path

REDIRECT_URI = "http://127.0.0.1:8888/callback"
# Playback, plus the playlists and recently played albums for the music page's library.
SCOPES = ("user-read-playback-state user-modify-playback-state user-read-currently-playing "
          "playlist-read-private playlist-read-collaborative user-read-recently-played")
AUTHORIZE_URL = "https://accounts.spotify.com/authorize"
TOKEN_URL = "https://accounts.spotify.com/api/token"
PI_DATA = "/data/dashboard"


def fail(message: str) -> None:
    print(f"PRZERWANO: {message}", file=sys.stderr)
    sys.exit(1)


def read_config(args: argparse.Namespace) -> dict:
    try:
        if args.local:
            return json.loads((Path(args.local) / "config.json").read_text())
        out = subprocess.run(["ssh", args.host, f"cat {PI_DATA}/config.json"], capture_output=True, text=True,
                             check=True)
        return json.loads(out.stdout)
    except (OSError, subprocess.CalledProcessError, json.JSONDecodeError) as e:
        fail(f"nie mogę odczytać config.json: {e}")


def wait_for_code(state: str) -> str:
    """Serves one request on 127.0.0.1:8888 and returns the code Spotify sent back."""
    result = {}

    class Callback(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            url = urllib.parse.urlparse(self.path)
            if url.path != "/callback":  # e.g. the browser asking for a favicon
                self.send_error(404)
                return
            query = urllib.parse.parse_qs(url.query)
            if query.get("state", [""])[0] != state:
                result["error"] = "niezgodny parametr state (stara karta logowania?)"
            elif "error" in query:
                result["error"] = f"Spotify odmówił: {query['error'][0]}"
            else:
                result["code"] = query.get("code", [""])[0]
            ok = "code" in result
            body = ("Zalogowano. Możesz zamknąć tę kartę." if ok else f"Błąd: {result['error']}").encode()
            self.send_response(200 if ok else 400)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, *args):
            pass

    with http.server.HTTPServer(("127.0.0.1", 8888), Callback) as server:
        while not result:
            server.handle_request()
    if "error" in result:
        fail(result["error"])
    return result["code"]


def exchange(client_id: str, code: str, verifier: str) -> dict:
    body = urllib.parse.urlencode({
        "grant_type": "authorization_code",
        "code": code,
        "redirect_uri": REDIRECT_URI,
        "client_id": client_id,
        "code_verifier": verifier,
    }).encode()
    request = urllib.request.Request(TOKEN_URL, data=body,
                                     headers={"Content-Type": "application/x-www-form-urlencoded"})
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            return json.load(response)
    except urllib.error.HTTPError as e:
        fail(f"wymiana kodu na token nie przeszła: HTTP {e.code} {e.read().decode(errors='replace')}")


def save(args: argparse.Namespace, refresh_token: str) -> None:
    content = json.dumps({"refreshToken": refresh_token}, indent=2) + "\n"
    if args.local:
        path = Path(args.local) / "spotify.json"
        path.write_text(content)
        path.chmod(0o600)
        print(f"Zapisane w {path}. Serwer na PC podchwyci je w ciągu 30 s.")
        return
    # Owner-only on the Pi as well: the token gives control of the Spotify account.
    script = (f"umask 077 && cat > {PI_DATA}/spotify.json.tmp && "
              f"chown dashboard:dashboard {PI_DATA}/spotify.json.tmp && "
              f"mv {PI_DATA}/spotify.json.tmp {PI_DATA}/spotify.json && "
              "systemctl restart dashboard-server")
    try:
        subprocess.run(["ssh", args.host, script], input=content, text=True, check=True)
    except subprocess.CalledProcessError as e:
        fail(f"nie udało się zapisać tokenu na {args.host}: {e}")
    print(f"Zapisane na {args.host} w {PI_DATA}/spotify.json, dashboard-server uruchomiony ponownie.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host", default="dashboard", help="SSH host of the Pi (default: dashboard)")
    parser.add_argument("--local", metavar="DIR", help="data dir of the server on this PC instead of the Pi")
    args = parser.parse_args()

    client_id = read_config(args).get("spotify", {}).get("clientId")
    if not client_id:
        fail('w config.json brakuje "spotify": { "clientId": "..." }')

    verifier = secrets.token_urlsafe(64)
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b"=").decode()
    state = secrets.token_urlsafe(16)
    url = AUTHORIZE_URL + "?" + urllib.parse.urlencode({
        "response_type": "code",
        "client_id": client_id,
        "scope": SCOPES,
        "redirect_uri": REDIRECT_URI,
        "state": state,
        "code_challenge_method": "S256",
        "code_challenge": challenge,
    })

    print("Otwieram logowanie Spotify w przeglądarce. Jeśli się nie otworzy, wejdź na:")
    print(url)
    webbrowser.open(url)
    code = wait_for_code(state)
    tokens = exchange(client_id, code, verifier)
    if "refresh_token" not in tokens:
        fail(f"Spotify nie dał tokenu odświeżania: {tokens}")
    print(f"Zalogowano, uprawnienia: {tokens.get('scope', '?')}")
    save(args, tokens["refresh_token"])


if __name__ == "__main__":
    main()
