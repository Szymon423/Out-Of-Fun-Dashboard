# Out Of Fun Dashboard: aplikacja

Frontend w Svelte 5 + TypeScript (`web/`) i serwer w C++20 (`server/`). Architektura, API i etapy są w [docs/plan.md](docs/plan.md).

## Wygląd na PC

```bash
cd web && npm install && npm run dev
```

Podgląd: `epiphany http://localhost:5173/?mock`. Inne widoki: `?page=weather`, `?overview`, `?saver`, `?theme=light` i `?fonts`, czyli wybór kroju zegara (pełna lista parametrów jest w [docs/plan.md](docs/plan.md)). Na PC mysz działa jak palec: przeciągnięcie przewija pulpity, a dwuklik w puste miejsce otwiera przegląd. Epiphany ma ten sam silnik WebKit co kiosk na Pi. Ekran 1280×720 skaluje się do okna. W trybie deweloperskim bez działającego serwera strona sama przechodzi na dane testowe i pokazuje znaczek „dane testowe”. Zbudowana strona tego nie robi: czeka na serwer i co 2 s próbuje znowu.

## Serwer na PC

Biblioteki: `sudo dnf install libcurl-devel json-devel`. `cpp-httplib` jest w drzewie (`server/third_party/`).

```bash
meson setup build/server server
ninja -C build/server && meson test -C build/server
build/server/dashboard-server --web web/dist --data devdata
```

- Słucha na `127.0.0.1:8080`. Serwer Vite przekazuje do niego `/api`, więc `npm run dev` pokazuje już prawdziwe dane, a z `?mock` dane testowe.
- `devdata/` (poza gitem) zastępuje `/data/dashboard` z Pi. Plik `devdata/config.json` serwer tworzy sam przy pierwszym starcie; trzeba w nim wpisać miejscowość, żeby była pogoda (opis w [docs/plan.md](docs/plan.md)).
- Podświetlenie: na PC domyślnie `--backlight none`, czyli jasność jest tylko zapamiętywana. Na Pi usługa uruchamia serwer z `--backlight auto`.
- Lokalny build i `devdata/` leżą poza `server/`, bo przepis Yocto kopiuje cały ten katalog.
- Spotify: `./tools/spotify-login.py --local devdata` loguje serwer na PC, a `./tools/spotify-login.py` loguje Pi. Szczegóły są w [docs/plan.md](docs/plan.md).

## Wygląd na ekranie Pi, z przeładowaniem na żywo

1. Na PC działa `npm run dev` w `web/`.
2. Port Vite musi być otwarty na PC, do restartu: `sudo firewall-cmd --add-port=5173/tcp`.
3. Przełączenie kiosku: `./tools/kiosk-url.sh dev`. Każda zapisana zmiana w `web/` od razu pojawia się na ekranie.
4. Powrót do strony z obrazu: `./tools/kiosk-url.sh reset`.

Wersja produkcyjna: `./tools/kiosk-url.sh prod` buduje `web/dist/` i pokazuje go na Pi przez ten sam serwer Vite, pod `/prod/`. Po zmianach trzeba ją uruchomić ponownie.

Gotowego buildu nie da się otworzyć z `file://`, bo WebKit blokuje wtedy moduły ES (`Origin null`). Na Pi podaje go `dashboard-server`.

## Pomiar płynności na Pi

`?bench` sam odtwarza przejścia: pulpity, przegląd, pasek i wygaszacz. Dla każdego zapisuje do journala liczbę klatek na sekundę, najdłuższą klatkę i liczbę klatek dłuższych niż 25 ms.

```bash
./tools/kiosk-url.sh prod "mock&bench&rounds=2"
ssh dashboard 'journalctl -u dashboard-kiosk -o cat | grep -o "BENCH.*TOTAL.*"'
```

Warianty do porównań: `&still` wyłącza ruch tła, a `&flat` usuwa wszystkie cienie.

## Sprawdzenie i build

```bash
cd web && npm run check && npm run build
```

Wynik trafia do `web/dist/`: około 80 KB JS (29 KB po gzip) i czcionka Inter.
