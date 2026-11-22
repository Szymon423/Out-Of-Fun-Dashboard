# Out Of Fun Dashboard: aplikacja

Frontend w Svelte 5 + TypeScript (`web/`). Serwer w C++20 (`server/`) dojdzie w etapie D2. Architektura, API i etapy są w [docs/plan.md](docs/plan.md).

## Wygląd na PC

```bash
cd web && npm install && npm run dev
```

Podgląd: `epiphany http://localhost:5173/?mock`. Inne widoki: `?page=weather`, `?overview`, `?saver`, `?theme=light` i `?fonts`, czyli wybór kroju zegara (pełna lista parametrów jest w [docs/plan.md](docs/plan.md)). Na PC mysz działa jak palec: przeciągnięcie przewija pulpity, a dwuklik w puste miejsce otwiera przegląd. Epiphany ma ten sam silnik WebKit co kiosk na Pi. Ekran 1280×720 skaluje się do okna. Bez działającego serwera strona sama przechodzi na dane testowe i pokazuje znaczek „dane testowe”.

## Wygląd na ekranie Pi, z przeładowaniem na żywo

1. Na PC działa `npm run dev` w `web/`.
2. Port Vite musi być otwarty na PC, do restartu: `sudo firewall-cmd --add-port=5173/tcp`.
3. Przełączenie kiosku: `./tools/kiosk-url.sh dev`. Każda zapisana zmiana w `web/` od razu pojawia się na ekranie.
4. Powrót do strony z obrazu: `./tools/kiosk-url.sh reset`.

Wersja produkcyjna: `./tools/kiosk-url.sh prod` buduje `web/dist/` i pokazuje go na Pi przez ten sam serwer Vite, pod `/prod/`. Po zmianach trzeba ją uruchomić ponownie.

Gotowego buildu nie da się otworzyć z `file://`, bo WebKit blokuje wtedy moduły ES (`Origin null`). Na Pi buildem zajmie się `dashboard-server` (etap D2).

## Pomiar płynności na Pi

`?bench` sam odtwarza przejścia: pulpity, przegląd, pasek i wygaszacz. Dla każdego zapisuje do journala liczbę klatek na sekundę, najdłuższą klatkę i liczbę klatek dłuższych niż 25 ms.

```bash
./tools/kiosk-url.sh prod "bench&rounds=2"
ssh dashboard 'journalctl -u dashboard-kiosk -o cat | grep -o "BENCH.*TOTAL.*"'
```

Warianty do porównań: `&still` wyłącza ruch tła, a `&flat` usuwa wszystkie cienie.

## Sprawdzenie i build

```bash
cd web && npm run check && npm run build
```

Wynik trafia do `web/dist/`: około 80 KB JS (29 KB po gzip) i czcionka Inter.
