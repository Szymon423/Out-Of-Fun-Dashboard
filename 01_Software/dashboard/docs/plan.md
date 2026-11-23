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
    {
      "location": { "name": "Kraków" },
      "immich": { "url": "http://192.168.1.20:2283", "apiKey": "…" },
      "spotify": { "clientId": "…" },
      "printer": { "host": "192.168.1.181", "serial": "…", "accessCode": "…", "model": "X2D" },
      "calendars": [ { "id": "m4t", "name": "M4T", "color": "#0a84ff", "url": "https://…/calendar.ics" } ]
    }
    ```

    Plik zawiera klucz API, więc leży tylko w `/data/dashboard` na Pi i w `devdata/` na PC, oba poza gitem. Sama nazwa wystarcza, bo serwer szuka współrzędnych w geokoderze Open-Meteo. Można też podać `latitude` i `longitude`, wtedy nazwa jest tylko wyświetlana. Bez miejscowości nie ma pogody.
  - Zepsuty `config.json` nie zatrzymuje serwera: działa dalej bez tej części. Gdyby się zatrzymywał, health check odrzuciłby aktualizację, a poprzedni slot czyta ten sam `/data`.
  - Pogoda: Open-Meteo co 15 min. Po błędzie ponawia po 5 s, potem co dwa razy dłużej, maksymalnie co 15 min. Zdjęcia i Spotify robią tak samo. Przy starcie sieć wstaje około sekundę po serwerze (pomiar z 23.11.2026: serwer w 6,3 s, adres DHCP w 7,2 s), a przy ponowieniu po 1 min dashboard przez pierwszą minutę nie miał pogody ani zdjęć.
  - Światła: makieta zapisywana w `lights.json`, więc stan przetrwa restart.
  - Zdjęcia z Immicha (D3):
    - Co 30 min serwer prosi o 100 losowych zdjęć z całej biblioteki. Zostawia do 30 poziomych, od 5:4 do 2,4:1, z uwzględnieniem orientacji EXIF. Odpadają pionowe, panoramy, zarchiwizowane, ukryte i usunięte. Pionowe źle wyglądałyby przycięte do ekranu 16:9.
    - Do tego raz dziennie wspomnienie „N lat temu”.
    - Podglądy (`size=preview`, JPEG około 1440 px) trafiają do `/data/dashboard/photos` i są podawane jako `/api/photos/<id>.jpg`. Klucz API nie trafia do przeglądarki, a wygaszacz nie czeka na Immicha. Pamięć podręczna trzyma bieżącą partię, poprzednią i wspomnienie, czyli około 60 plików.
    - Klucz API potrzebuje uprawnień `asset.read` (wyszukiwanie), `asset.view` (podglądy) i `memory.read` (wspomnienia).
    - Podglądy z Immicha mają 2560×1440 albo 1920×1440. To więcej niż trzeba dla ekranu, ale pamięć jest w normie.
      - Pomiar 23.11.2026 przez 4 min wygaszacza: proces WebKit trzyma 250–295 MB, oddaje pamięć i nie rośnie. Wolnej pamięci zostaje około 1,4 GB.
      - Gdyby przenikanie się zacinało, serwer może zmniejszać podglądy do około 1600×900 przy pobieraniu (libjpeg-turbo, skalowanie w dekoderze).
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
  - Kafelek kalendarza (od 23.11.2026) nie powtarza daty, którą pokazuje już zegar obok.
    - Pokazuje 5 najbliższych wydarzeń z widocznych kalendarzy, rozłożonych na całą wysokość.
    - Każde ma godzinę z dniem („dziś”, „jutro”, „śr.”, „2 gru”), pasek w kolorze kalendarza, tytuł, nazwę kalendarza i miejsce.
    - Do tego pigułka „trwa”, „za 45 min” albo „za 3 dni”. Pigułki nie ma, gdy powtarzałaby dzień z kolumny.
- **Przegląd pulpitów.**
  - Otwiera go przeciągnięcie palcem w górę (podpowiada to pasek na dole, jak w iOS) albo dwuklik w puste miejsce. Przeciągnięcie w dół go zamyka.
  - Uszczypnięcie (pierwsza wersja) zastąpiliśmy 22.11.2026 przeciągnięciem, na życzenie użytkownika.
  - Przegląd od razu znikał, bo `click` palca, który go otworzył, trafiał w kafelek, który właśnie się pojawił. Gest połyka teraz ten `click`, a kafelki ignorują stuknięcia przez 400 ms po otwarciu.
  - Pokazuje kafelki jak przełącznik aplikacji: ikona w kolorze pulpitu, nazwa i jedna bieżąca informacja. Kafelek zdjęć ma w tle wspomnienie z Immicha.
  - Od 23.11.2026 podsumowanie stoi w środku jako duży kafelek z zegarem, datą i pogodą. Strony leżące w pagerze na lewo od niego (Dom, Drukarka, Kalendarz) są kolumną z lewej, a te na prawo (Pogoda, Muzyka, Zdjęcia) kolumną z prawej, w kolejności przesuwania.
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
- **Wygaszacz w stylu Apple TV i Nest Hub** (od 23.11.2026).
  - Zdjęcie zajmuje cały ekran.
  - W lewym dolnym rogu jest cienki zegar (150 px) z datą i pogodą. W prawym drobno widać miejsce oraz miesiąc i rok zdjęcia.
  - Napisy są czytelne dzięki łagodnemu przyciemnieniu samego dołu ekranu.
  - Wcześniejszy duży, matowy panel pod zegarem użytkownik odrzucił, bo za bardzo zasłaniał zdjęcie. Razem z nim zniknęła rozmyta kopia zdjęcia (`blurUrl`, `thumbhash`).
  - Startuje po 5 min bez dotyku i wraca pod spodem na podsumowanie.
  - Zdjęcia z Immicha z przenikaniem i powolnym przybliżaniem, zmiana co 20 s.
  - Następne i poprzednie zdjęcie są dekodowane z wyprzedzeniem (`Image.decode()`), więc przenikanie zaczyna się od gotowego obrazu.
  - Zdjęcia są o każdej porze. Nocny wygląd z samym przyciemnionym zegarem na czarnym tle usunęliśmy 22.11.2026 na życzenie użytkownika. W nocy ekran może ściemnić podświetlenie (D6).
  - Strony pod zasłoniętym wygaszaczem nie są rysowane.
- **Pulpit zdjęć.** Stuknięcie w lewą lub prawą trzecią część ekranu cofa albo przechodzi dalej, jak w aplikacji Zdjęcia; strzałka mignie przy dotknięciu. Ręczna zmiana zaczyna od nowa odliczanie do następnego zdjęcia. Przesunięcie palcem dalej zmienia pulpit.
- **Pogoda (od 23.11.2026).**
  - Duża karta: na górze bieżąca pogoda, pod nią wykres na 24 h.
    - Krzywa temperatury zaczyna się od „Teraz”. Wartości i ikony są co 3 h.
    - Słupki szansy opadów; z ilością w mm, gdy pada.
    - Noc widać w kolorze krzywej: pomarańczowa w dzień, lawendowa w nocy, jak ikony księżyca. Szare pasy nocy z pierwszej wersji wyglądały jak puste prostokąty, więc wypadły.
  - Pod wykresem pięć kafelków jak w Apple Weather:
    - UV z porą szczytu;
    - wiatr z porywami i kierunkiem;
    - ciśnienie z kierunkiem zmiany w ciągu 3 h;
    - wschód i zachód z długością dnia;
    - suma opadów.
  - Z prawej, spokojniej: 7 dni.
- **Muzyka (od 23.11.2026).**
  - Z lewej: co gra, sterowanie, głośnik i głośność. Stuknięcie w głośnik otwiera wybór urządzenia Spotify.
  - Suwak głośności jest tylko dla urządzeń z `supports_volume`. Telefony go nie mają i Spotify odpowiada tam 403.
  - Z prawej: biblioteka 4×2 ze stronami.
    - Zakładka „Playlisty” zaczyna się od Polubionych utworów, dalej idą playlisty.
    - Zakładka „Ostatnio grane” to albumy z ostatnio granych utworów.
  - Stuknięcie w okładkę otwiera listę utworów, 7 na stronę.
    - Stuknięcie w utwór gra listę od tego utworu, a dalej leci reszta (`context_uri` z `offset`).
    - „Odtwórz” gra całość od początku. Grający utwór jest wyróżniony na zielono.
    - Gdy nic nie jest aktywne, najpierw trzeba wybrać, gdzie grać.
- **Zegar.** Wysoki i cienki krój do wyboru na ekranie przez `?fonts`. Kandydaci są w `clockFont.svelte.ts`, a po wyborze zbędne paczki znikną.
- **Światła.** Na razie to makieta.
  - Lampka lewa i prawa: tylko włącz/wyłącz.
  - Listwa LED to RGBW: jasność, kanał bieli, barwa i gotowe ustawienia.
- **Kalendarz.**
  - Tryby Dzień / Tydzień / Miesiąc, przewijanie strzałkami i „Dziś”.
  - Trzy źródła: Prywatny, M4T i Itens, każde w swoim kolorze. Chipy pod tytułem włączają i wyłączają źródła, a filtr działa też na widżecie podsumowania.
  - Tryb i filtr są zapamiętane.
  - Stuknięcie w wydarzenie w widoku dnia lub tygodnia otwiera kartę szczegółów:
    - kalendarz, tytuł i data;
    - godziny z czasem trwania i stanem („za 2 h”, „trwa, do 10:30”, „jutro”, „zakończone”);
    - miejsce i pełny opis.
    Kartę zamyka stuknięcie obok albo krzyżyk; zamyka się też przy zmianie pulpitu. Jest zamontowana obok stron, nie w nich, bo strony blokują przewijanie dotykiem (gesty), a opis musi się przewijać.
  - Wydarzenia całodniowe stoją w cienkim pasku pod nazwami dni, jako belki przez wszystkie dni, które obejmują.
    - Mieszczą się dwa rzędy, w widoku dnia trzy. Nadmiar to „+N więcej”, które otwiera widok tego dnia.
    - Bez wydarzeń całodniowych paska nie ma.
    - Wcześniej każdy dzień miał własną kolumnę chipów, przez co pasek rozpychał siatkę godzin.
- **Drukarka Bambu Lab X2D.**
  - Odczyt: stan, postęp, czas do końca, warstwy, dwie dysze, stół, komora, AMS z wilgotnością i komunikaty o błędach.
  - Szczegóły odczytu są w części o serwerze.

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
| `?event=next` | od razu karta najbliższego wydarzenia (z opisem, jeśli jest) |
| `?printer=idle` / `finished` / `offline` / `error` | stan drukarki w danych testowych |
| `?openList` | od razu lista utworów pierwszej playlisty na pulpicie muzyki |
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

- **Spotify (D4).**
  - Aplikacja na developer.spotify.com ma tylko Web API i adres przekierowania `http://127.0.0.1:8888/callback`. Spotify od 2025 r. nie przyjmuje `localhost`.
  - Logowanie odbywa się raz, na PC: `tools/spotify-login.py`. To kod autoryzacji z PKCE, więc nie trzeba sekretu klienta.
    - Przeglądarka wraca na adres pętli zwrotnej PC, więc to musi dziać się na PC, a nie na Pi.
    - Token odświeżania trafia do `/data/dashboard/spotify.json` (0600), a serwer uruchamia się ponownie.
    - `--local devdata` loguje serwer na PC.
  - Serwer odnawia dostęp sam i zapisuje nowy token odświeżania, gdy Spotify go wymieni.
    - Gdy logowanie wygaśnie lub zostanie cofnięte, serwer usuwa plik, a w logu prosi o ponowne logowanie.
    - Nowy plik podchwytuje w ciągu 30 s.
  - Odczyt `GET /v1/me/player`: co 5 s, gdy coś gra (i zaraz po spodziewanym końcu utworu), co 20 s w przeciwnym razie. Szanuje `Retry-After` przy 429.
  - `music` wychodzi tylko wtedy, gdy zmienia się utwór, stan, urządzenie albo pozycja odbiega o ponad 2,5 s. Pasek postępu strona przesuwa sama, więc częste aktualizacje tylko by go restartowały.
  - Sterowanie: `PUT /me/player/play|pause`, `POST /me/player/next|previous`. Bez aktywnego urządzenia serwer odpowiada 409, a bez logowania 503.
  - Biblioteka (`musicLibrary`, co 30 min): `/me/playlists`, albumy z `/me/player/recently-played` bez powtórzeń i Polubione jako `spotify:user:<id>:collection`. Do tego urządzenia (`/me/player/devices`), przeniesienie odtwarzania, głośność i odtwarzanie kontekstu na wskazanym urządzeniu.
  - Listy utworów: `/playlists/{id}/items`, `/albums/{id}/tracks` i `/me/tracks` (Polubione, uprawnienie `user-library-read`).
    - Zmiana API Spotify z 2026 r.: dla aplikacji w trybie deweloperskim stare `/playlists/{id}/tracks` daje 403 „Forbidden”.
    - W nowym formacie utwór jest w polu `item` zamiast `track`, a liczba utworów playlisty w `items.total` zamiast `tracks.total`.
    - Serwer czyta oba warianty.
  - Biblioteka potrzebuje uprawnień `playlist-read-private`, `playlist-read-collaborative`, `user-read-recently-played` i `user-library-read`.
    - Skrypt logowania prosi o nie od 23.11.2026.
    - Starsze logowanie dalej gra, tylko bez biblioteki; serwer prosi wtedy w logu o ponowne zalogowanie.
    - Serwer zauważa zmianę pliku `spotify.json` (czas modyfikacji). Nowe logowanie działa więc bez restartu i od razu wczytuje bibliotekę.
  - Sterowanie wymaga Spotify Premium.

- **Drukarka (D4b).** X2D, firmware 01.02.00.00. Sprawdzone 23.11.2026:
  - Ogłasza się w sieci przez SSDP (UDP 2021): model `N6`, tryb chmurowy, „secure link”.
  - Lokalne MQTT (`ssl://<ip>:8883`, użytkownik `bblp`, hasło to kod dostępu z ekranu drukarki) działa w zwykłym trybie chmurowym. Tryb tylko LAN i tryb deweloperski nie są potrzebne, więc aplikacja Bambu Handy działa dalej.
  - Serwer subskrybuje `device/<serial>/report` i prosi o pełny stan (`pushall`) przy połączeniu i co 5 min. Częściowe aktualizacje dokleja do całości: obiekty klucz po kluczu, tablice z `id` (AMS, sloty, ekstrudery) element po elemencie. Do drukarki nic poza `pushall` nie wysyła.
  - X2D ma dwie dysze w `device.extruder.info[]`. Ich temperatura jest zapisana jako `(cel << 16) | bieżąca`; to samo dotyczy `device.ctc` (komora). Na idle'u cel wynosi 0. Rozpakowanie w trakcie grzania trzeba jeszcze potwierdzić na wydruku.
  - Kafelek na podsumowaniu w trakcie druku pokazuje pierścień postępu. W spoczynku, na wzór widżetu Baterie w iOS:
    - cztery sloty AMS jako pierścienie w kolorach załadowanych filamentów, z materiałem w środku;
    - aktywny slot ma dodatkową obwódkę, a pusty to cienki przerywany okrąg;
    - pod spodem jest ostatni wydruk albo komunikat drukarki.
  - Czego próbowaliśmy wcześniej (23.11.2026):
    - Rysunek drukarki w SVG odstawał estetycznie od reszty, więc wypadł.
    - Render ostatniego modelu z pliku 3MF nie jest osiągalny lokalnie.
      - FTPS drukarki (port 990, `bblp` z kodem dostępu) działa tylko z TLS 1.2 i z wznowieniem sesji TLS na połączeniu danych; bez tego vsftpd odpowiada 522.
      - FTPS pokazuje jednak tylko pamięć zewnętrzną, a X2D jej nie ma: `tl_external_total_kb` wynosi 0.
      - Wydruki z chmury trafiają do pamięci wewnętrznej.
  - Na ekran trafia zmiana stanu, zadania, AMS lub komunikatu od razu. Same temperatury (w pełnych stopniach) idą najwyżej co 3 s.
  - Klient MQTT to `paho-mqtt-c` (biblioteka `paho-mqtt3cs`): na PC pakiet `paho-c-devel`, w Yocto przepis z meta-oe. Certyfikat drukarki pochodzi z własnego CA Bambu, więc serwer go nie weryfikuje.
  - Identyfikator klienta MQTT zawiera nazwę maszyny (`out-of-fun-dashboard-<host>`).
    - Broker drukarki rozłącza starsze z dwóch połączeń o tym samym identyfikatorze.
    - Przy wspólnym identyfikatorze serwer na Pi i serwer na PC wyrzucały się nawzajem co około 5 s.
  - Kamera (port 6000) i miniaturka wydruku (FTPS, port 990) są możliwe, ale użytkownik ich nie potrzebuje.
- **Kalendarze (D5).** Kalendarze czytamy z opublikowanych linków ICS, tylko do odczytu i bez logowania.
  - Linki publikuje się w Outlooku: Ustawienia → Kalendarz → Kalendarze udostępnione → Opublikuj. Google ma do tego tajny adres iCal.
  - Link jest tajny: kto go ma, widzi kalendarz. Leży więc tylko w `config.json`.
  - Serwer pobiera każdy link co 10 min. Kalendarz, który nie odpowiada, zostawia swoje ostatnie wydarzenia.
  - Rozwijanie wydarzeń robi `libical`, od początku poprzedniego miesiąca do trzech miesięcy naprzód:
    - serie (`RRULE`) i wykluczone daty (`EXDATE`);
    - przeniesione i odwołane pojedyncze spotkania (`RECURRENCE-ID`): pierwotna data nie pochodzi wtedy z serii;
    - nazwy stref z Windows („Central European Standard Time”, „W. Europe Standard Time”) z definicji `VTIMEZONE` w samym pliku.
  - Opis wydarzenia (`description`) serwer przycina: odcina stopkę spotkania Outlooka od pierwszej linii podkreślników („Spotkanie w aplikacji Microsoft Teams”, linki, numery), ściska puste linie i skraca do 2000 znaków.
  - Wydarzenia z godziną idą w UTC (`…Z`). Całodniowe idą jako lokalna północ bez strefy, z końcem wyłącznym.
  - Źródła: Prywatny (jeszcze bez linku), M4T i Itens. Pomiar z 23.11.2026: M4T 31 wydarzeń, Itens 15. Godziny lokalne są poprawne po obu stronach zmiany czasu 25.10.

## API (wspólne dla frontendu i serwera)

| Ścieżka | Opis |
|---|---|
| `GET /api/state` | Pełny stan: `weather`, `calendars`, `events`, `music`, `musicLibrary`, `lights`, `printer`, `photos`, `display`. Każde pole może być `null`, gdy dostawca jeszcze nie ma danych albo nie jest skonfigurowany. |
| `GET /api/health` | `{ "ok": true, "uptimeS": … }`, dla health checku |
| `GET /api/events` | SSE, zdarzenie `update` z `{ "topic": "weather" \| "calendars" \| "events" \| "music" \| "lights" \| "printer" \| "photos" \| "display", "data": … }` |
| `POST /api/music/{play,pause,next,previous}` | Sterowanie Spotify: 204, 409 bez aktywnego urządzenia, 503 bez logowania |
| `GET /api/music/devices` | Urządzenia Spotify: `[{ id, name, type, isActive, volume }]` |
| `GET /api/music/tracks?uri=spotify:…` | Utwory playlisty, albumu albo Polubionych: `[{ name, artist, uri, durationMs }]`, do 100 |
| `POST /api/music/context` | `{ "uri": "spotify:…", "deviceId"?: "…", "offset"?: "spotify:track:…" }`: gra playlistę, album albo Polubione, od utworu `offset` |
| `POST /api/music/device` | `{ "id": "…" }`: przenosi odtwarzanie na urządzenie |
| `POST /api/music/volume` | `{ "percent": 0–100 }`: głośność aktywnego urządzenia |
| `GET /api/photos/{id}.jpg` | podgląd zdjęcia z Immicha, z pamięci podręcznej serwera |
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
| D3 | Zdjęcia z Immicha: losowe poziome z całej biblioteki, wspomnienia, pamięć podręczna na `/data`, stuknięcia na pulpicie zdjęć, nowy wygaszacz | sprawdzone 23.11.2026 na Pi przez `kiosk-url.sh dev`: zdjęcia, przenikanie i pamięć w porządku; do zrobienia aktualizacja OTA |
| D4 | Spotify: logowanie raz z PC (`tools/spotify-login.py`), co gra, sterowanie | kod gotowy 23.11.2026; sterowanie z ekranu po zalogowaniu |
| D4b | Drukarka Bambu Lab X2D przez lokalne MQTT (stan, postęp, dwie dysze, AMS) | kod gotowy 23.11.2026, odczyt sprawdzony na PC; do potwierdzenia temperatury przy grzaniu |
| D5 | Kalendarze z linków ICS (Outlook: M4T, Itens; prywatny później) | kod gotowy 23.11.2026, sprawdzony na prawdziwych kalendarzach na PC |
| D6 | Dopracowanie: noc (ściemnianie podświetlenia), ustawienia, stany błędów i braku sieci | – |
