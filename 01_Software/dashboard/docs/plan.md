# Aplikacja dashboardu

Plan z 22.11.2026. Ekran: Touch Display 2 w poziomie, 1280×720, dotyk. Silnik: WPE WebKit 2.54 w kiosku Westona na Raspberry Pi 4 (2 GB).

## Architektura

```
┌──────────────── Raspberry Pi ────────────────────────────────────────┐
│ dashboard-launcher (WPE WebKit)  ──HTTP──▶  dashboard-server (C++20) │
│   frontend: Svelte 5 + TS              127.0.0.1:8080                │
│   /         statyczne pliki                 │  ├─ Open-Meteo (pogoda) │
│   /api/...  stan i akcje                    │  ├─ Spotify Web API     │
│   /api/events  SSE z aktualizacjami         │  ├─ Immich API          │
│                                             │  ├─ Microsoft Graph     │
│                                             │  └─ światła (makieta)   │
│                              /data/dashboard: config.json, tokeny,    │
│                                               pamięć podręczna zdjęć  │
└──────────────────────────────────────────────────────────────────────┘
```

- **Frontend:**
  - Svelte 5 + TypeScript + Vite, zbudowany do statycznych plików.
  - Nie rozmawia z usługami w internecie. Bierze stan z serwera (`/api/state`) i dostaje zmiany przez Server-Sent Events (`/api/events`).
  - Dzięki temu tokeny nie trafiają do przeglądarki, nie ma problemów z CORS, a wygląd można rozwijać na danych testowych.
- **Serwer:**
  - C++20: `cpp-httplib` (serwer HTTP, dołączony jako jeden nagłówek), `libcurl` (klient HTTPS) i `nlohmann/json`.
  - Każde źródło danych to osobny dostawca za małym interfejsem. Dostawca w tle odświeża dane w swoim rytmie i publikuje zmianę.
  - Światła zostają makietą w pamięci, dopóki nie ma sprzętu.
- **Dane trwałe** na `/data/dashboard`: konfiguracja, tokeny OAuth i pamięć podręczna zdjęć. Przetrwają aktualizacje RAUC.
- **Serwer w szczegółach (D2).**
  - Opcje: `--listen 127.0.0.1:8080`, `--web /usr/share/dashboard/web`, `--data /data/dashboard`, `--backlight auto|none|katalog` (domyślnie `none`).
  - `config.json` w katalogu danych tworzy się sam jako szablon:

    ```json
    { "location": { "name": "Kraków" } }
    ```

    Sama nazwa wystarcza, bo serwer szuka współrzędnych w geokoderze Open-Meteo. Można też podać `latitude` i `longitude`, wtedy nazwa jest tylko wyświetlana. Bez miejscowości nie ma pogody.
  - Zepsuty `config.json` nie zatrzymuje serwera: działa dalej bez tej części. Gdyby się zatrzymywał, health check odrzuciłby aktualizację, a poprzedni slot czyta ten sam `/data`.
  - Pogoda: Open-Meteo co 15 min. Po błędzie ponawia po 1 min, potem co dwa razy dłużej, maksymalnie co 15 min.
  - Światła: makieta zapisywana w `lights.json`, więc stan przetrwa restart.
  - Jasność: `/sys/class/backlight/*/brightness`. Touch Display 2 ma skalę 0–31. Serwer nigdy nie zapisuje 0, bo to całkiem gasi podświetlenie.
  - `Type=notify`: serwer zgłasza gotowość dopiero z otwartym portem, więc kiosk startuje po nim i nie trafia na pusty adres.
  - SSE zaczyna od wszystkich tematów naraz, więc nic nie ginie między `/api/state` a otwarciem strumienia. Bez zmian strumień co 15 s wysyła komentarz.
  - Zbudowana strona na Pi nie przechodzi na dane testowe. Czeka na serwer i co 2 s próbuje znowu.
  - Dopóki serwer nie ma zdjęć (D3), wygaszacz i pulpit zdjęć pokazują wygenerowane pejzaże.
- **Kiosk** ładuje `http://127.0.0.1:8080/`. `dashboard-health` sprawdza także `dashboard-server.service`, więc wersja z niedziałającym serwerem wraca do poprzedniego slotu.

## Interfejs (wymagania z 22.11.2026)

- **Styl Apple.** Zasady:
  - kolory systemowe Apple, Inter (bliski SF Pro) i zaokrąglone karty jak widżety iOS;
  - typografia zamiast ramek;
  - karty półprzezroczyste, bo prawdziwe rozmycie (`backdrop-filter`) jest za drogie przy przesuwaniu pulpitów.
- **Pulpity** przewijane palcem, z podsumowaniem pośrodku:

  ```
  Dom · Drukarka · Kalendarz · [ Podsumowanie ] · Pogoda · Muzyka · Zdjęcia
  ```

  - Widżet na podsumowaniu otwiera swój pulpit.
  - Na podsumowaniu jest: zegar, data, pogoda w jednej linii, trzy światła jak w Centrum sterowania, kalendarz w stylu „Up Next”, wspomnienie z Immicha, muzyka i drukarka.
- **Przegląd pulpitów.**
  - Otwiera go przeciągnięcie palcem w górę (podpowiada to pasek na dole, jak w iOS) albo dwuklik w puste miejsce. Przeciągnięcie w dół go zamyka.
  - Uszczypnięcie (pierwsza wersja) zastąpiliśmy 22.11.2026 przeciągnięciem, na życzenie użytkownika.
  - Przegląd od razu znikał, bo `click` palca, który go otworzył, trafiał w kafelek, który właśnie się pojawił. Gest połyka teraz ten `click`, a kafelki ignorują stuknięcia przez 400 ms po otwarciu.
  - Pokazuje kafelki jak przełącznik aplikacji: ikona w kolorze pulpitu, nazwa i jedna bieżąca informacja. Kafelek zdjęć ma w tle wspomnienie z Immicha.
  - Bieżący pulpit odpływa w tle jedną, już narysowaną warstwą.
  - Pierwsza wersja z 7 żywymi, pomniejszonymi stronami zacinała Pi przy otwieraniu i zamykaniu, bo WebKit musiał naraz przemalować wszystkie strony, do tego z maskami zaokrągleń i dużymi cieniami.
  - Po wyborze kafelka tor stron przeskakuje od razu, bez przewijania przez strony pośrednie, więc rysuje się tylko strona docelowa.
- **Pasek sterowania** wysuwany z góry: przeciągnięcie w dół od górnej krawędzi (pierwsze 110 px).
  - Zawiera motyw (Auto / Jasny / Ciemny), jasność ekranu i przycisk wygaszacza.
  - Jasność wysyła `POST /api/display`, a podświetlenie ustawi serwer.
  - Zamyka go stuknięcie obok, przeciągnięcie w górę albo 8 s bez dotyku.
- **Motywy.**
  - Jasny i ciemny jako zestawy zmiennych CSS w `app.css`.
  - Auto przełącza o wschodzie i zachodzie słońca z prognozy, a bez prognozy o 7:00 i 19:00.
  - Ręczny wybór działa do najbliższego wschodu albo zachodu, jak w iOS.
- **Wygaszacz w stylu StandBy.**
  - Zegar stoi na matowym, szklanym panelu, więc jest czytelny także na jasnym zdjęciu.
    - Panel pokazuje gotową, rozmytą kopię bieżącego zdjęcia (`Photo.blurUrl`), wyrównaną do tła i przybliżaną razem z nim. Na wierzchu jest lekkie przyciemnienie i jasna krawędź.
    - Bez żywego `backdrop-filter`, który przy ciągle przybliżanym zdjęciu kosztowałby Pi rozmycie w każdej klatce.
    - Rozmytą kopię da serwer, np. z `thumbhash` Immicha. Bez niej panel rozmywa zdjęcie na żywo (`filter: blur`).
  - Startuje po 5 min bez dotyku i wraca pod spodem na podsumowanie.
  - W dzień: zdjęcia z Immicha z przenikaniem i powolnym przybliżaniem, duży zegar i pogoda.
  - Zdjęcia są o każdej porze. Nocny wygląd z samym przyciemnionym zegarem na czarnym tle usunęliśmy 22.11.2026 na życzenie użytkownika. W nocy ekran może ściemnić podświetlenie (D6).
  - Strony pod zasłoniętym wygaszaczem nie są rysowane.
- **Zegar.** Wysoki i cienki krój do wyboru na ekranie przez `?fonts`. Kandydaci są w `clockFont.svelte.ts`, a po wyborze zbędne paczki znikną.
- **Światła.** Na razie to makieta.
  - Lampka lewa i prawa: tylko włącz/wyłącz.
  - Listwa LED to RGBW: jasność, kanał bieli, barwa i gotowe ustawienia.
- **Kalendarz.**
  - Tryby Dzień / Tydzień / Miesiąc, przewijanie strzałkami i „Dziś”.
  - Trzy źródła: Prywatny, M4T i Itens, każde w swoim kolorze. Chipy pod tytułem włączają i wyłączają źródła, a filtr działa też na widżecie podsumowania.
  - Tryb i filtr są zapamiętane.
- **Drukarka Bambu Lab.**
  - Odczyt: stan, postęp, czas do końca, warstwy, temperatury, AMS.
  - Serwer będzie czytał przez lokalne MQTT drukarki. Najpierw trzeba sprawdzić, na co pozwala obecny firmware.

Parametry adresu do testów:

| Parametr | Działanie |
|---|---|
| `?mock` | dane testowe |
| `?page=weather` | pulpit startowy |
| `?overview` | od razu przegląd |
| `?saver` | od razu wygaszacz |
| `?saverAfter=20` | wygaszacz po 20 s bez dotyku |
| `?theme=light` / `?theme=dark` | wymuszony motyw |
| `?controls` | otwarty pasek sterowania |
| `?calendar=day` / `week` / `month` | tryb kalendarza |
| `?fonts` | wybór kroju zegara |
| `?bench` (`&rounds=2`, `&still`, `&flat`) | pomiar płynności przejść, wyniki w journalu (README) |

## Zasady wydajności (z pomiarów na Pi)

Szczegóły i liczby są w pamięci projektu, w notatce o renderowaniu na Pi.
- Animacje tylko przez `transform` i `opacity`, jako animacje CSS albo Web Animations, bo te działają w kompozytorze.
- Brak zmian DOM w każdej klatce. Zegar zmienia się raz na minutę. Pasek postępu utworu to jedna animacja `scaleX` na cały czas do końca utworu, a nie aktualizacja co klatkę.
- Tekst zmieniany co najwyżej raz na sekundę, czyli czas utworu.
- Bez `backdrop-filter` na kartach. Karty są półprzezroczyste, a tło jest gotowym obrazem.
- Canvas 2D tylko z obrazami albo ImageBitmap. Cięższe efekty w WebGL.
- Bez rozmytych cieni (`box-shadow` z rozmyciem) na kartach i innych dużych elementach.
  - WPE maluje rozmycie na Pi bardzo wolno. Strona pełna kart z cieniem 40 px zatrzymywała każde przewinięcie pulpitu na 100–200 ms.
  - Karty są płaskie jak widżety iOS, z obwódką 1 px. Poświata listwy LED to gradient.
- Element animowany ma mieć stałą warstwę (`will-change`) i być namalowany zawczasu.
  - Element, który staje się warstwą dopiero na starcie animacji, jest malowany w tej właśnie klatce.
  - Dotyczy to kafelków przeglądu i wygaszacza. Przegląd jest stale w stronie, a pokazuje go tylko CSS.
- Najpierw malowanie, potem animacja (`afterPaint` w `nav.svelte.ts`).
  - Po wyborze pulpitu w przeglądzie strona przeskakuje pod jeszcze otwartym przeglądem, a zamyka go dopiero następna klatka.
  - Tak samo przy wyjściu z wygaszacza: najpierw odsłania się pulpit, potem wygaszacz zaczyna znikać.
  - Koszt malowania staje się krótkim opóźnieniem przed animacją, a nie szarpnięciem w jej trakcie.

Pomiar `?bench` na Pi 4 z 22.11.2026, wersja produkcyjna. Przy każdym przejściu podane są: klatki na sekundę / najdłuższa klatka.

| Przejście | Przed | Po zmianach |
|---|---|---|
| Przewijanie pulpitów | 43 / 217 ms | 54 / 117 ms |
| Przegląd | 36 / 280 ms | 47 / 168 ms |
| Pasek sterowania | 60 / 60 ms | 61 / 39 ms |
| Spoczynek | 60 | 60 |

- Tryb deweloperski Vite dawał te same liczby co wersja produkcyjna, więc JavaScript nie był wąskim gardłem.
- Ruch tła kosztuje 1–2 kl./s.

## API (wspólne dla frontendu i serwera)

| Ścieżka | Opis |
|---|---|
| `GET /api/state` | Pełny stan: `weather`, `calendars`, `events`, `music`, `lights`, `printer`, `photos`, `display`. Każde pole może być `null`, gdy dostawca jeszcze nie ma danych albo nie jest skonfigurowany. |
| `GET /api/health` | `{ "ok": true, "uptimeS": … }`, dla health checku |
| `GET /api/events` | SSE, zdarzenie `update` z `{ "topic": "weather" \| "calendars" \| "events" \| "music" \| "lights" \| "printer" \| "photos" \| "display", "data": … }` |
| `POST /api/music/{play,pause,next,previous}` | Sterowanie Spotify; do D4 odpowiada 501 |
| `POST /api/lights/{id}` | `{ "on": bool }`, dla RGBW też `"brightness": 0–100`, `"color": "#rrggbb"`, `"white": 0–100`; na razie makieta |
| `POST /api/display` | `{ "brightness": 0–100 }`, czyli podświetlenie ekranu |

Typy danych są w `web/src/lib/types.ts`. Serwer zwraca dokładnie te pola.

## Sposób pracy

- **Wygląd na PC:** `npm run dev` w `web/`, z danymi testowymi (`?mock`). Podgląd w Epiphany, który ma ten sam silnik WebKit co WPE. Strona skaluje się do okna, a na Pi ma dokładnie 1280×720.
- **Wygląd na prawdziwym ekranie, bez builda obrazu:**
  - `tools/kiosk-url.sh dev` przełącza kiosk na Pi na serwer Vite z PC, więc zmiany widać od razu (HMR).
  - `tools/kiosk-url.sh reset` przywraca adres docelowy.
- **Serwer na PC:** budowany natywnie (`meson`), z katalogiem danych `devdata/` zamiast `/data`. Komendy są w README. Serwer Vite przekazuje `/api` do serwera, więc `kiosk-url.sh dev` pokazuje na Pi prawdziwe dane z serwera na PC.
- **Wydanie:**
  - Przepisy Yocto `dashboard-server` i `dashboard-web`, potem `./build.sh dashboard-bundle && ./tools/ota.sh`.
  - `dashboard-web` pakuje gotowy `web/dist`. `build.sh` zbuduje go na PC przed BitBake'em, bo Node w Yocto to długi build i kłopotliwe zależności npm.

## Etapy

| Etap | Zakres | Sprawdzenie |
|---|---|---|
| D1 | Szkielet frontendu na danych testowych: pulpity z przeglądem, motywy, wygaszacz, podsumowanie i pierwsze wersje pulpitów | wygląd, gesty i płynność na ekranie Pi przez `kiosk-url.sh dev`; wybór kroju zegara |
| D2a | Serwer C++: pliki statyczne, `/api/state`, SSE, pogoda z Open-Meteo, światła (makieta), jasność ekranu; na PC | gotowe 23.11.2026: prawdziwa pogoda na Pi przez `kiosk-url.sh dev` |
| D2b | Przepisy Yocto `dashboard-server` i `dashboard-web`, kiosk na `127.0.0.1:8080`, serwer w `dashboard-health` | gotowe 23.11.2026: wersja 0.4 przez OTA, health check przeszedł, pogoda dla Wrocławia, jasność zapisuje się do podświetlenia, serwer zajmuje 14 MB RAM |
| D3 | Zdjęcia z Immicha jako tło: klucz API, wybór albumu, przenikanie, pamięć podręczna na `/data` | płynne przenikanie, pamięć RAM |
| D4 | Spotify: autoryzacja (jednorazowo z PC albo kod QR na ekranie), co gra, sterowanie | sterowanie z ekranu |
| D4b | Drukarka Bambu Lab przez lokalne MQTT (stan, postęp, temperatury, AMS) | prawdziwy wydruk widoczny na pulpicie |
| D5 | Kalendarze Microsoft: device code flow z kodem na ekranie | najpierw sprawdzić, czy polityka służbowego tenanta na to pozwala |
| D6 | Dopracowanie: noc (ściemnianie podświetlenia), ustawienia, stany błędów i braku sieci | – |
