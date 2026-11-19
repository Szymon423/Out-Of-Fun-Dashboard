# Test silnika przeglądarki na Raspberry Pi

Strona `index.html` udaje docelowy dashboard w 1280×720 i mierzy płynność w trzech scenariuszach.
Skrypt `measure.sh` mierzy pamięć. Dotykowy ekran nie jest potrzebny, wystarczy monitor HDMI
ustawiony na 1280×720 i 60 Hz. Dotyk po obrocie trzeba sprawdzić osobno, już na Touch Display 2.

Wynik z HDMI jest nieco zawyżony. Touch Display 2 jest natywnie pionowy, a kontroler obrazu Pi 4
nie obraca o 90°, więc w układzie poziomym każdą klatkę obraca dodatkowo GPU.

## Przygotowanie

Podłącz monitor do portu HDMI0, najbliżej zasilania. Skopiuj ten katalog na Pi, ustaw rozdzielczość
i uruchom serwer w sesji graficznej labwc:

```bash
wlr-randr --output HDMI-A-1 --mode 1280x720@60
chmod +x measure.sh
python3 -m http.server 8080 --bind 127.0.0.1
```

## Chromium

```bash
chromium --kiosk --ozone-platform=wayland --noerrdialogs "http://127.0.0.1:8080/?auto=1"
```

## Cog

```bash
sudo apt install cog
COG_PLATFORM_WL_VIEW_FULLSCREEN=1 cog -P wl "http://127.0.0.1:8080/?auto=1"
```

## Pomiar

Test płynności startuje sam i trwa około 2 minut. Po wynikach zostaw stronę włączoną i w drugim
terminalu zmierz pamięć przez 30 minut:

```bash
./measure.sh chromium 1800
./measure.sh cog 1800
```

Uruchamiaj tylko jedną przeglądarkę naraz i zrestartuj Pi między silnikami.

## Jak wybrać

| Kryterium | Próg |
|---|---|
| Scenariusz „Zdjęcia, gotowe rozmycie” po HDMI | co najmniej 55 FPS |
| Dostępna pamięć po 30 minutach | co najmniej 500 MB |
| Swap | brak stałego wzrostu |
| Dotyk po obrocie ekranu | trafia w przyciski |

Docelowo planowany jest obraz Yocto z WPE 2.54. Cog na Raspberry Pi OS używa starszego WPE 2.48,
więc jego wynik jest ostrożnym oszacowaniem, a Chromium służy jako punkt odniesienia.
Scenariusz „rozmycie na żywo” pokazuje tylko koszt `backdrop-filter`.

Licznik FPS mierzy pętlę `requestAnimationFrame`, więc może nie wychwycić zacięć samego kompozytora.
Dlatego oceń też wzrokowo przenikanie zdjęć i ruch paska postępu.
