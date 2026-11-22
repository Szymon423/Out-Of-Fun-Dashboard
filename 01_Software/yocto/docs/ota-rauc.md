# Aktualizacje OTA z RAUC

Plan z 21.11.2026. Wdrażamy go po etapie z akceleracją GPU w WebKicie.

## Cel

- Aktualizacja systemu przez sieć, bez wyjmowania karty spod ekranu.
- Przerwa w zasilaniu w trakcie aktualizacji ani nieudany build nie psują urządzenia. Pi samo wraca do poprzedniej wersji.
- Aktualizacja obejmuje:
  - rootfs razem z kernelem i modułami;
  - partycję boot, czyli firmware Pi, `config.txt`, overlaye i U-Boot.
- Plan jest od razu przygotowany pod przyszły rootfs tylko do odczytu.
- Kartę trzeba będzie włożyć jeszcze dwa razy, przy etapach OTA-1 i OTA-2. Od OTA-3 aktualizacje idą wyłącznie przez sieć.

## Decyzje

| Temat | Wybór | Dlaczego |
|---|---|---|
| Narzędzie | RAUC 1.15.2 z `meta-rauc`, gałąź `wrynose` | Ma prosty model slotów, mały klient w C, podpisane pakiety i nie potrzebuje serwera. |
| Bootloader | U-Boot (`RPI_USE_U_BOOT = "1"`, U-Boot 2026.01 z meta-raspberrypi) | Wybiera slot A albo B i wraca do starego po nieudanych startach. Mechanizm `tryboot` wymagałby osobnej partycji boot dla każdego slotu i własnej integracji z RAUC. |
| Kernel | W rootfs (`/boot/Image.gz`) | Kernel i jego moduły aktualizują się razem i wracają razem. |
| DTB i overlaye | Z partycji boot | Firmware stosuje je według `config.txt`, a U-Boot przekazuje kernelowi gotowe DT (`${fdt_addr}`). |
| Zmienne U-Boota | Zapisane wprost na karcie, poza partycjami, w dwóch kopiach | Plik `uboot.env` na FAT zniknąłby przy aktualizacji partycji boot, a razem z nim wybór slotu. Dwie kopie chronią przed przerwą w zasilaniu. |
| Aktualizacja partycji boot | Slot RAUC `boot-mbr-switch` | RAUC zapisuje nową partycję boot obok starej i przełącza wpis w MBR jedną operacją, więc nie ma stanu pośredniego. |
| Pakiet aktualizacji | `.raucb` w formacie `verity`, podpisany własnym kluczem | Pi sprawdza podpis i integralność przed zapisem. |
| Przesyłanie | `scp` i `rauc install` przez skrypt `tools/ota.sh` | Najprostsze rozwiązanie. Później można dodać strumieniowanie przez HTTP, bo RAUC ma `streaming`. |
| Dane trwałe | Partycja `/data` | Zostają tam tokeny Spotify i Microsoft, ustawienia, klucz hosta SSH i stan RAUC. |

„Firmware” rozumiem tu jako całe oprogramowanie urządzenia, także pliki z partycji boot. Dlatego plan obejmuje `boot-mbr-switch`. Jeśli chodzi tylko o rootfs, etap OTA-5 można pominąć.

## Układ karty (MBR)

| Obszar | Początek | Rozmiar | Zawartość |
|---|---|---|---|
| MBR | 0 | 512 B | tablica partycji |
| Zmienne U-Boota, kopia 1 | 1 MiB | 16 KiB | `BOOT_ORDER`, `BOOT_A_LEFT`, `BOOT_B_LEFT` |
| Zmienne U-Boota, kopia 2 | 1 MiB + 128 KiB | 16 KiB | druga kopia |
| `p1` boot (vfat) | 4 MiB | 128 MiB | firmware, `config.txt`, overlaye, `u-boot.bin`, `boot.scr` |
| obszar zapasowy dla `p1` | 132 MiB | 128 MiB | miejsce na nową partycję boot przy aktualizacji |
| `p2` rootfs A (ext4) | 260 MiB | 2 GiB | system, slot A |
| `p3` rootfs B (ext4) | 2308 MiB | 2 GiB | system, slot B |
| `p4` data (ext4) | 4356 MiB | 256 MiB, przy pierwszym starcie powiększana do końca karty | `/data` |

- **Rozmiar slotów.** Obecny rootfs zajmuje 495 MB, więc 2 GiB zostawia zapas na aplikację i większy WebKit. Plan zakłada kartę co najmniej 8 GB.
- **Obszar boot.** Zakres od 4 do 260 MiB to region `boot-mbr-switch`. RAUC zapisuje nową partycję boot w tej połowie, która jest akurat wolna, i przestawia na nią wpis `p1` w MBR.
- **Cztery partycje podstawowe.** Partycja rozszerzona nie jest potrzebna, bo cztery wpisy MBR wystarczają. `wic` tworzy ją dopiero przy więcej niż czterech partycjach.
- **Partycja `p4`.** W obrazie ma stały rozmiar, bo `wic` nie zna rozmiaru karty.
  - Przy pierwszym starcie `dashboard-grow-data.service` powiększa ją do końca karty (`parted resizepart`), zanim zostanie zamontowana.
  - Skrypt zmienia MBR tylko wtedy, gdy jest co najmniej 16 MiB do zyskania, więc przy kolejnych startach nic nie zapisuje.
  - System plików powiększa się sam dzięki `x-systemd.growfs` w `fstab`.
- **Slot B w obrazie.** Jest kopią rootfs ze slotu A. Dzięki temu oba sloty działają od pierwszego startu, a przełączanie A/B można sprawdzić bez pakietu aktualizacji (`rauc status mark-active other`).
- **Partycja boot w Linuksie.** Jest montowana w `/boot/firmware` tylko do odczytu, jak w Raspberry Pi OS. `/boot` należy do rootfs i trzyma kernel slotu.
- **Możliwości `wic`.** Wersja z wrynose obsługuje `--offset`, `--align` i `--fixed-size`, więc taki układ da się opisać w pliku `.wks`.

## Start systemu

1. Firmware Pi czyta `config.txt` z `p1`, stosuje overlaye do DT i ładuje `u-boot.bin`.
2. U-Boot wczytuje swoje zmienne z karty i uruchamia `boot.scr`:
   - wybiera pierwszy slot z `BOOT_ORDER`, który ma `BOOT_x_LEFT > 0`;
   - zmniejsza jego licznik i zapisuje zmienne.
3. U-Boot ładuje `/boot/Image.gz` z wybranego rootfs i startuje kernel z `root=/dev/mmcblk0p2` albo `p3`, `rauc.slot=A` albo `B` oraz `panic=5`. DT pochodzi od firmware.
4. systemd uruchamia kolejno Westona, kiosk i sprawdzenie stanu. Gdy wszystko działa, `rauc status mark-good` przywraca licznik slotu do 3.
5. Gdy system się zawiesi, restartuje go watchdog: przy kernel panic po 5 sekundach (`panic=5`), przy zawieszonym systemd watchdog sprzętowy. Każdy taki start zmniejsza licznik. Po trzech nieudanych startach U-Boot uruchamia drugi slot.

## Aktualizacja

1. Build na PC: `./build.sh dashboard-bundle` tworzy podpisany pakiet `.raucb`.
2. `./tools/ota.sh` wysyła pakiet do `/data/ota/`, uruchamia `rauc install`, restartuje Pi, czeka, aż wróci SSH, i pokazuje `rauc status`.
3. RAUC na Pi:
   - sprawdza podpis i `compatible`;
   - zapisuje rootfs do nieaktywnego slotu;
   - jeśli pakiet zawiera też partycję boot, zapisuje ją do wolnej połowy regionu i przełącza MBR;
   - ustawia nowy slot jako pierwszy w `BOOT_ORDER`.
4. Po restarcie Pi startuje z nowego slotu. Slot zostaje oznaczony jako dobry dopiero po udanym sprawdzeniu stanu, a w przeciwnym razie Pi wraca do starego.

## Pliki do dodania

Wszystkie poniższe pliki trafią do `meta-dashboard`, z wyjątkiem kluczy i skryptu narzędziowego.

| Plik | Zawartość |
|---|---|
| `.gitmodules`, `build/conf/bblayers.conf` | submoduł `layers/meta-rauc` na gałęzi `wrynose` |
| `conf/distro/dashboard.conf` | `DISTRO_FEATURES:append = " rauc"`, `RPI_USE_U_BOOT = "1"` |
| `files/wic/dashboard-ab.wks` | układ karty z tabeli powyżej (`WKS_SEARCH_PATH` szuka w `files/wic/` każdej warstwy) |
| `conf/layer.conf` | priorytet warstwy 10, wyższy niż meta-raspberrypi (9), żeby nasze `.bbappend` i pliki wygrywały |
| `recipes-bsp/u-boot/u-boot_%.bbappend` | fragmenty konfiguracji `env-in-mmc.cfg` i `bootdelay.cfg` oraz własny `fw_env.config` |
| `recipes-bsp/rpi-u-boot-scr/rpi-u-boot-scr.bbappend` | własny `boot.cmd.in` z logiką slotów |
| `recipes-bsp/bootfiles/rpi-cmdline.bbappend` | `root=` znika z `cmdline.txt`, bo ustawia go U-Boot |
| `recipes-core/rauc/rauc-conf.bbappend` | `system.conf` i `ca.cert.pem` (publiczny certyfikat CA) |
| `recipes-core/bundles/dashboard-bundle.bb` | przepis pakietu aktualizacji |
| `recipes-core/base-files/base-files_%.bbappend` | `fstab` z `/data` i partycją boot w `/boot/firmware` tylko do odczytu |
| `recipes-dashboard/dashboard-ota/` | usługi: powiększanie `/data`, sprawdzenie stanu z `mark-good`, watchdog, klucz hosta SSH na `/data` |
| `recipes-bsp/bootfs/dashboard-bootfs.bb` | obraz vfat partycji boot dla slotu `boot-mbr-switch` (etap OTA-5) |
| `recipes-core/images/dashboard-image.bb` | `WKS_FILE`, `IMAGE_FSTYPES`, `kernel-image`, `dashboard-ota`; w OTA-4 `BAD_RECOMMENDATIONS += "rauc-mark-good"` |
| `01_Software/yocto/keys/` | klucze podpisu, w całości w `.gitignore` |
| `01_Software/yocto/tools/rauc-keys.sh` | skrypt narzędziowy: tworzenie, sprawdzanie, kopia i wymiana kluczy |
| `01_Software/yocto/tools/ota.sh` | skrypt narzędziowy do wysyłania i instalacji pakietu |

### Szkice konfiguracji

`system.conf`:

```ini
[system]
compatible=out-of-fun-dashboard-rpi4
bootloader=uboot
data-directory=/data/rauc
bundle-formats=-plain

[keyring]
path=ca.cert.pem

[slot.bootloader.0]
device=/dev/mmcblk0
type=boot-mbr-switch
region-start=4M
region-size=256M

[slot.rootfs.0]
device=/dev/mmcblk0p2
type=ext4
bootname=A

[slot.rootfs.1]
device=/dev/mmcblk0p3
type=ext4
bootname=B
```

Fragment konfiguracji U-Boota (`env-in-mmc.cfg`):

```
# CONFIG_ENV_IS_IN_FAT is not set
CONFIG_ENV_IS_IN_MMC=y
CONFIG_ENV_MMC_DEVICE_INDEX=0
CONFIG_ENV_SIZE=0x4000
CONFIG_ENV_OFFSET=0x100000
CONFIG_ENV_REDUNDANT=y
CONFIG_ENV_OFFSET_REDUND=0x120000
```

`fw_env.config` dla `fw_printenv` i `fw_setenv` w Linuksie. Plik z meta-raspberrypi wskazuje `/boot/uboot.env`, więc trzeba go podmienić:

```
/dev/mmcblk0 0x100000 0x4000
/dev/mmcblk0 0x120000 0x4000
```

`boot.cmd.in` budujemy na wzór `meta-rauc-community/meta-rauc-raspberrypi`, z czterema zmianami:
- usunąć zapis `uboot.env` na FAT;
- slot A to `p2`, a slot B to `p3`;
- kernel ładować z `/boot/Image.gz` w rootfs;
- do `bootargs` dodać `panic=5`.

## Klucze

- **Generowanie.** Kluczami zarządza `tools/rauc-keys.sh` (`create`, `info`, `backup`, `rotate`).
  - Pod spodem uruchamia skrypt `layers/meta-rauc/scripts/openssl-ca.sh` w katalogu `keys/`.
  - Plik w warstwie zostaje nietknięty: `sed` zmienia nazwę organizacji z `Test Org` na `Out Of Fun` w kopii przekazywanej do `bash`.
  - Cały katalog `keys/` jest w `.gitignore`, a skrypt sprawdza to przed generowaniem.
- **Pliki ze skryptu:**
  - `keys/openssl-ca/dev/ca.cert.pem`: publiczny certyfikat CA;
  - `keys/openssl-ca/dev/private/ca.key.pem`: klucz CA, potrzebny tylko do wystawienia nowego klucza podpisu;
  - `keys/openssl-ca/dev/development-1.cert.pem` i `keys/openssl-ca/dev/private/development-1.key.pem`: certyfikat i klucz podpisu pakietów.
- **Kopia zapasowa poza repozytorium.** Bez klucza CA żadne już zainstalowane urządzenie nie przyjmie nowej aktualizacji, więc trzeba by znowu flashować kartę. Kopia to zaszyfrowane archiwum `gpg` z `tools/rauc-keys.sh backup`, przechowywane poza tym komputerem. Nową kopię trzeba zrobić po każdej wymianie kluczy.
- **Certyfikat CA w warstwie.** Publiczny `ca.cert.pem` trafia do `recipes-core/rauc/files/`.
- **Gdzie podpis szuka kluczy.** `RAUC_KEY_FILE` i `RAUC_CERT_FILE` w `dashboard-bundle.bb` wskazują `${TOPDIR}/../keys/openssl-ca/dev/...`.
- **Ważność certyfikatów.** Pi nie ma zegara z baterią, więc tuż po starcie może mieć datę z obrazu. Nie przeszkadza to, bo certyfikaty ze skryptu są ważne od 1.01.1970 do 31.12.9999 (sprawdzone 22.11.2026 z OpenSSL 4.0).

### Wymiana kluczy

Pi sprawdza podpis pakietu keyringiem z systemu, który właśnie działa. Pakiet podpisany nowym kluczem zostanie więc odrzucony, dopóki Pi nie ma nowego CA. Wymiana idzie przez pakiet przejściowy:

1. `./tools/rauc-keys.sh rotate`: stare klucze trafiają do `keys/archive/openssl-ca-<data>/`, a w `keys/openssl-ca/` powstają nowe.
2. Keyring w warstwie zawiera oba CA, nowe i stare:
   ```bash
   cat keys/openssl-ca/dev/ca.cert.pem keys/archive/openssl-ca-<data>/dev/ca.cert.pem > meta-dashboard/recipes-core/rauc/files/ca.cert.pem
   ```
3. Pakiet przejściowy z tym keyringiem jest podpisany jeszcze starym kluczem: `RAUC_KEY_FILE` i `RAUC_CERT_FILE` w `dashboard-bundle.bb` wskazują tymczasowo `keys/archive/...`. Instalujemy go przez OTA na każdym urządzeniu.
4. Ścieżki wracają do `keys/openssl-ca/dev/...`, więc kolejne pakiety mają już nowy podpis.
5. Gdy żadne urządzenie nie ma już starego keyringa, stare CA usuwamy z `ca.cert.pem`. Potem robimy `./tools/rauc-keys.sh backup`.

Jeśli stary klucz zginął albo wyciekł, pakietu przejściowego nie da się bezpiecznie zrobić i zostaje flashowanie karty.

## Etapy

### OTA-0: Przygotowanie (bez flashowania)
- Dodać submoduł `meta-rauc` (`wrynose`) i wpisać go do `bblayers.conf`.
- Wygenerować klucze (`tools/rauc-keys.sh create`) i zrobić ich kopię zapasową (`tools/rauc-keys.sh backup`).
- Dopisać `keys/` do `.gitignore`.
- **Sprawdzenie:** parsowanie bez błędów (`./build.sh -p`).
- Do etapu OTA-2 każdy build pokazuje ostrzeżenie `meta-rauc`, że w `DISTRO_FEATURES` brakuje `rauc`. Tak ma być. Gdy ta cecha jest włączona, `packagegroup-base` sam dociąga do obrazu `rauc` z przykładowym `system.conf`, więc włączamy ją dopiero w OTA-2 razem z własnym `system.conf`.

### OTA-1: U-Boot ze zmiennymi na karcie (flashowanie)
- Ustawić `RPI_USE_U_BOOT = "1"`, dodać fragment `env-in-mmc.cfg` i własny `fw_env.config`.
  - Nazwy opcji są z U-Boota 2026.01. Od wersji 2025.x `SYS_MMC_ENV_DEV` nazywa się `ENV_MMC_DEVICE_INDEX`, a `SYS_REDUNDAND_ENVIRONMENT` nazywa się `ENV_REDUNDANT`. Stare nazwy `merge_config` pomija i zostawia tylko ostrzeżenie w logu `do_configure`, a build przechodzi.
- `bootdelay.cfg` z `CONFIG_BOOTDELAY=-2`: start bez czekania na klawisz. Szum na niepodłączonym UART-cie nie zatrzyma wtedy U-Boota. Tak samo robi meta-raspberrypi dla Pi 5.
- Priorytet `meta-dashboard` podniesiony do 10.
  - BitBake nakłada `.bbappend` od najniższego priorytetu warstwy, a `FILESEXTRAPATHS:prepend` z ostatniego `.bbappend` trafia na początek ścieżki wyszukiwania.
  - Przy priorytecie 6 nasz `fw_env.config` przegrałby z plikiem z meta-raspberrypi (priorytet 9).
- `RPI_USE_U_BOOT` dopisuje do `config.txt` `enable_uart=1`, którego U-Boot wymaga na Pi 4. `ENABLE_UART` zostaje nieustawione, więc w `cmdline.txt` nie ma konsoli szeregowej.
- Do obrazu dochodzą `libubootenv-bin` (`fw_printenv`, `fw_setenv`) i `u-boot-env` (`/etc/fw_env.config`).
- Rootfs zostaje na razie jeden, a kernel nadal leży na `p1`. Domyślny `boot.cmd` wywołuje `saveenv` przy każdym starcie, bo na FAT nie ma `uboot.env`. Dzięki temu zmienne zapisują się na karcie już przy pierwszym starcie.
- **Przed flashowaniem:** sprawdzić w `deploy` `fw_env.config`, opcje `ENV_*` w `.config` U-Boota i `config.txt`.
- **Sprawdzenie na Pi:**
  - `/proc/device-tree/chosen/u-boot,version` istnieje, czyli Pi wystartowało przez U-Boot;
  - `fw_printenv` działa;
  - wartość ustawiona przez `fw_setenv test 1` przetrwa restart;
  - `/proc/cmdline` jest taki jak wcześniej;
  - wydajność bez zmian (`ONLY=cv-rect,dom-both,dom-layer spike/diag.sh base`), bo `enable_uart` może zablokować taktowanie rdzenia VPU;
  - czas od włączenia zasilania do strony kiosku, mierzony stoperem przed zmianą i po niej.

#### Wyniki OTA-1 (22.11.2026)

U-Boot 2026.01 działa, a zmienne są zapisywane na karcie i przetrwają odłączenie zasilania. Pierwsza wersja wydłużyła start o 11 s. Mierzone stoperem od włączenia zasilania:

| Wariant | Logi kernela | Dashboard |
|---|---|---|
| bez U-Boota | 12 s | 17 s |
| U-Boot z domyślną konfiguracją | – | 28 s |
| + `console=tty1` | 18 s | 23 s |
| + bez `preboot` (`pci enum; usb start`) | 16 s | 20 s |
| + `Image.gz` zamiast `Image` | 13 s | 18 s |

Przyczyny i poprawki:
- **Konsola na UART.**
  - Przyczyna: `enable_uart=1` włącza port szeregowy, a device tree ma `stdout-path = "serial0:115200n8"`. Bez `console=` w `cmdline.txt` kernel uznaje UART za główną konsolę. Log kernela i komunikaty systemd szły z prędkością 115200 bodów, a systemd uruchamiał getty na `ttyS0`.
  - Poprawka: `console=tty1` w `rpi-cmdline.bbappend`.
- **`preboot`.**
  - Przyczyna: domyślne `pci enum; usb start` kosztowało około 2.5 s.
  - Poprawka: `# CONFIG_USE_PREBOOT is not set` w `autoboot.cfg`.
- **Wolny odczyt karty w U-Boocie.**
  - Pomiar `bootstage`: U-Boot czyta kartę z prędkością 4.8 MB/s, więc 27 MB kernela szło 5.6 s. Linux czyta tę samą kartę w trybie DDR50.
  - Poprawka: `KERNEL_IMAGETYPE_UBOOT = "Image.gz"`. Kernel ma 10.9 MB, wczytuje się w 2.3 s i rozpakowuje w 0.3 s.

Czasy w samym U-Boocie (`bootstage`) po poprawkach:

| Etap | Czas |
|---|---|
| inicjalizacja | 0.64 s |
| `bootflow scan` | 0.24 s |
| wczytanie `Image.gz` | 2.27 s |
| rozpakowanie | 0.28 s |
| **razem** | **3.57 s** |

Linux startuje w 5.3 s (wcześniej 6.8 s, bo konsola nie idzie już na UART). Sumarycznie start jest dłuższy o około 1 s niż bez U-Boota.

Wnioski na następne etapy:
- **Pomiar czasu.** `bootstage.cfg` (`CONFIG_BOOTSTAGE`, `CONFIG_BOOTSTAGE_FDT`) zostaje. Czasy U-Boota odczytuje się w Linuksie z `/proc/device-tree/bootstage/`.
- **Zmienne zapisane na karcie wygrywają z wartościami wbudowanymi w U-Boota.**
  - Dotyczy to m.in. `bootcmd`, `preboot` i `bootdelay`.
  - `bmaptool` nie nadpisuje obszaru 1–4 MiB, więc zmienne przetrwają też flashowanie nowego obrazu.
  - Dlatego w OTA-2 `flash.sh` zeruje ten obszar przy każdym pełnym flashowaniu.
  - Ustawienia, które mają działać mimo zapisanych zmiennych, wyłączamy w konfiguracji U-Boota, tak jak `USE_PREBOOT`.
- **`libubootenv`.** `fw_setenv nazwa` bez wartości zostawia pustą zmienną (`test=`), zamiast ją usunąć.

### OTA-2: Układ A/B i RAUC (ostatnie flashowanie)
- Plik `.wks` z nowym układem, `boot.cmd.in` z logiką slotów i `system.conf`.
- `flash.sh` zeruje obszar zmiennych U-Boota (1–4 MiB) przed zapisem obrazu, więc po pełnym flashowaniu obowiązują wartości domyślne z nowego U-Boota.
- `boot.cmd.in` wczytuje `boot/Image.gz` z rootfs wybranego slotu (`load mmc 0:<partycja>`) i zapisuje zmienne tylko wtedy, gdy zmienia licznik prób.
- `DISTRO_FEATURES:append = " rauc"` w `dashboard.conf`.
  - Od wrynose `rauc-conf` instaluje `system.conf` i keyring do `/usr/lib/rauc/`.
  - `rauc` trafia do obrazu przez `packagegroup-base`.
- Do obrazu dochodzą `kernel-image` i `dashboard-ota`: powiększanie `/data`, klucz hosta SSH na `/data` i `RequiresMountsFor=/data` dla RAUC.
- Własny `fstab` w `base-files`, a w `.wks` `--no-fstab-update`. Pakiet RAUC zawiera rootfs z buildu, a nie z `wic`, więc wpisy dodane przez `wic` zniknęłyby po pierwszej aktualizacji.
- `cmdline.txt` bez `root=`: ustawia go `boot.scr`, razem z `rauc.slot=` i `panic=5`.
- Na partycji boot nie ma już kernela (`RPI_EXTRA_IMAGE_BOOT_FILES:remove`). `IMAGE_FSTYPES` to `ext4` (pod pakiet RAUC w OTA-3) oraz `wic.bz2` i `wic.bmap`.
- Klucz hosta Dropbeara przenosimy na `/data` drop-inami usług `dropbearkey.service` i `dropbear@.service`: `DROPBEAR_RSAKEY_DIR=/data/dropbear` oraz `RequiresMountsFor=/data`.
  - Same zmienne nie wystarczą, bo warunki `ConditionPathExists` w `dropbearkey.service` sprawdzają tylko `/etc/dropbear` i `/var/lib/dropbear`.
  - Puste `ConditionPathExists=` w drop-inie kasuje te warunki, a nowy warunek sprawdza `/data/dropbear`.
- Do OTA-4 slot oznacza jako dobry standardowa usługa `rauc-mark-good` z meta-rauc. Bez niej liczniki prób spadałyby do zera.
- `resize=true` w `system.conf` sprawia, że RAUC po zapisaniu slotu uruchamia `resize2fs`. Domyślnie nie ma go w obrazie, więc `rauc-conf.bbappend` dodaje `e2fsprogs-resize2fs` do `RDEPENDS`.
- Do poprawnego działania przez sieć w OTA-2 musi być od razu:
  - układ partycji;
  - RAUC z kluczem i `system.conf`;
  - SSH.

  Pozostałe rzeczy w rootfs można później poprawić aktualizacją, a `boot.scr` w razie potrzeby podmienić przez SSH.
- **Sprawdzenie:**
  - `rauc status` pokazuje uruchomiony slot A, a B jako nieaktywny;
  - `fw_printenv BOOT_ORDER BOOT_A_LEFT BOOT_B_LEFT` zwraca `A B`, `3`, `3`, czyli `rauc-mark-good` oddało próbę;
  - `/proc/cmdline` zawiera `root=/dev/mmcblk0p2 rauc.slot=A panic=5`;
  - `df -h /data` obejmuje resztę karty;
  - klucz hosta SSH nie zmienia się po restarcie;
  - `rauc status mark-active other` i restart uruchamiają slot B, a potem tak samo wracamy na A.

#### Wyniki OTA-2 (22.11.2026)

- Pi startuje ze slotu A z `root=/dev/mmcblk0p2 rauc.slot=A panic=5`. `rauc status` pokazuje, że oba sloty są dobre.
- `rauc-mark-good` oddaje próbę, więc po starcie zmienne mają wartości `BOOT_ORDER=A B`, `BOOT_A_LEFT=3` i `BOOT_B_LEFT=3`.
- `dashboard-grow-data` przy pierwszym starcie powiększył `/data` o 24 932 MiB. Przy kolejnych startach nic nie zmienia.
- Klucz hosta SSH leży w `/data/dropbear` i przetrwał restart oraz przełączenie na drugi slot.
- `rauc status mark-active other` przełącza na slot B (`BOOT_ORDER=B A`). Slot B startuje i dostaje z powrotem swoje próby.
- Rootfs zajmuje 458 MB z 1.9 GiB.
- **Poprawka `/data`.**
  - Problem: pusty ext4 o rozmiarze 256 MiB z `wic` dostawał od `mke2fs` profil `small`, czyli bloki 1 KiB i 3.2 mln inode'ów po powiększeniu.
  - W `.wks`: `--mkfs-extraopts "-T default"`.
  - Na tej karcie: jednorazowo `mke2fs -T default` z przeniesieniem zawartości. Teraz `/data` ma bloki 4 KiB i 1.6 mln inode'ów.

### OTA-3: Pakiet aktualizacji i `tools/ota.sh`
- Przepis `dashboard-bundle.bb` (`RAUC_BUNDLE_SLOTS = "rootfs"`) i skrypt `tools/ota.sh`.
  - `RAUC_SLOT_rootfs[fstype] = "ext4"` trzeba podać wprost. Klasa `bundle` domyślnie bierze pierwszy typ z `IMAGE_FSTYPES`, ale tego z przepisu pakietu, czyli z maszyny (`tar.bz2`), a nie z obrazu.
  - Squashfs pakietu jest kompresowany zstd (`--mksquashfs-args`). Kernel ma `SQUASHFS_ZSTD`, a Pi rozpakowuje zstd szybciej niż domyślny gzip.
  - `RAUC_BUNDLE_VERSION` to `DISTRO_VERSION`, a pole `build` to data builda. `rauc status` pokazuje oba pola dla każdego slotu.
  - `tools/ota.sh`:
    - wysyła pakiet do `/data/ota` i sprawdza go na Pi (`rauc info`, czyli podpis i `compatible`);
    - instaluje go, usuwa plik i restartuje Pi;
    - czeka na nowy `boot_id`, bo stary system jeszcze przez chwilę odpowiada na SSH;
    - na końcu podaje, który slot działa, i zgłasza błąd, jeśli U-Boot wrócił do starego.
  - Pierwszy test podnosi `DISTRO_VERSION` do 0.2, żeby zmianę było widać w `/etc/os-release`.
- **Sprawdzenie:**
  - instalacja z A na B, restart, `rauc status` pokazuje uruchomiony slot B;
  - potem z B na A;
  - zmierzyć czas całej aktualizacji.

#### Wyniki OTA-3 (22.11.2026)

Pierwsza aktualizacja przez sieć: z A (0.1) na B (0.2) za pomocą `./tools/ota.sh`, w sumie 90 s.

| Etap | Czas |
|---|---|
| wysłanie 160 MB przez `scp` | 30 s |
| `rauc info`: podpis i łańcuch do CA | poniżej 1 s |
| `rauc install`: zapis 663 MB do slotu B, `resize2fs`, `BOOT_ORDER=B A` | 40 s |
| restart do SSH | 19 s |

- Po restarcie działa slot B z `PRETTY_NAME="Out Of Fun Dashboard 0.2 (wrynose)"`. `rauc-mark-good` oznaczyło go jako dobry, a klucz hosta SSH się nie zmienił.
- `rauc status --detailed` pokazuje przy slocie wersję pakietu, `build`, sumę obrazu i czas instalacji.
- Pakiet zstd ma 160 MB, czyli 24% z 663 MB obrazu ext4.
- **Wysyłanie.**
  - Pi 4 nie ma sprzętowego AES, ale SSH z chacha20 przenosi 32 MB/s.
  - `scp` przez SFTP z Dropbearem osiągał tylko 5.3 MB/s. `ssh 'cat > plik'` z zapisem na kartę daje 15.5 MB/s, czyli 10 s na pakiet, i tego używa teraz `ota.sh`.
  - Następne kroki do rozważenia: strumieniowanie RAUC przez HTTP (bez kopii na `/data`) albo aktualizacje adaptacyjne (`block-hash-index`).
- **Druga aktualizacja, z B na A, tym samym pakietem: 67 s.** Wysyłanie trwało 13 s zamiast 30, instalacja 35 s, a restart 19 s. Oba sloty mają teraz 0.2.

### OTA-4: Zabezpieczenia i testy powrotu
- **Watchdog.** `RuntimeWatchdogSec=15s` w `/etc/systemd/system.conf.d/` (watchdog `bcm2835_wdt` jest wbudowany w kernel) oraz `panic=5`.
- **Sprawdzenie stanu.** Usługa `dashboard-mark-good.service` zastępuje `rauc-mark-good`:
  - startuje po `dashboard-kiosk.service`, czeka około 30 sekund i sprawdza, czy Weston i kiosk działają;
  - jeśli tak, uruchamia `rauc status mark-good`, a jeśli nie, restartuje Pi, co zmniejsza licznik slotu.
- **Testy:**
  - pakiet z celowo zepsutym kioskiem: po trzech restartach Pi wraca do starego slotu;
  - pakiet z kernelem, który nie wstaje: `panic=5`, kolejne restarty i powrót do starego slotu;
  - wyłączenie zasilania w trakcie `rauc install`: Pi startuje ze starego slotu, a ponowna instalacja się udaje.

### OTA-5: Aktualizacja partycji boot
- Przepis `dashboard-bootfs.bb` buduje obraz vfat z plików partycji boot (`IMAGE_BOOT_FILES`, `config.txt`, `u-boot.bin`, `boot.scr`). Użyje do tego `dosfstools-native` i `mtools-native`.
- Dodać slot `bootloader` do pakietu aktualizacji.
- **Sprawdzenie:**
  - zmiana w `config.txt` dociera na Pi przez OTA;
  - `fdisk -l /dev/mmcblk0` pokazuje, że początek `p1` przeskakuje między 4 MiB a 132 MiB.
- **Ryzyko.** Partycja boot nie ma automatycznego powrotu. Zamiana jest atomowa, ale zepsuty `config.txt` oznacza naprawę przez kartę. Zmiany na `p1` trzeba więc testować ostrożnie.

### OTA-6: Rootfs tylko do odczytu (później)
- Włączyć `IMAGE_FEATURES += "read-only-rootfs"` i montować sloty ext4 tylko do odczytu. Opcjonalnie przejść na squashfs albo erofs, co da mniejsze pakiety.
  - squashfs albo erofs to skompresowany system plików tylko do odczytu. Pakiet RAUC i sloty byłyby około dwa razy mniejsze, a z `dm-verity` mamy też kontrolę integralności.
  - U-Boot potrafi wczytać kernel wprost z takiego slotu (`sqfsload`, `CONFIG_FS_SQUASHFS`; erofs: `CONFIG_FS_EROFS`). Wtedy kernel leży w nim nieskompresowany, a kompresję robi system plików.
  - Czas startu prawie się nie zmieni. W U-Boocie liczy się liczba wczytanych bajtów (karta czyta tam z prędkością 4.8 MB/s), a to wychodzi podobnie jak `Image.gz`. Linux czyta kartę szybko, w trybie DDR50.
  - Rozmiary kernela 6.18 (22.11.2026): `Image` 27.1 MB, `gzip -9` 10.9 MB, `zstd -19` 9.1 MB, `xz -9` 7.8 MB.
  - `zstd` dałby około 0.5 s krócej niż `gzip`, ale wymaga `CONFIG_ZSTD` w U-Boocie. `xz` jest najmniejszy, ale U-Boot rozpakowuje go wolno i nie obsługuje formatu `.xz`.
- Spisać stan, który musi być zapisywalny, i przenieść go na `/data` albo do tmpfs:
  - `/var/lib/systemd`: zegar `timesyncd`, podświetlenie, ziarno losowości;
  - `machine-id`;
  - logi, jeśli mają przetrwać restart;
  - dane aplikacji.
- Do rozważenia `overlayfs.bbclass` albo cecha obrazu `overlayfs-etc`.
- Później aktualizacje różnicowe (`adaptive=block-hash-index`) i strumieniowanie przez HTTP, żeby ograniczyć ilość przesyłanych danych.

## Ryzyka i otwarte kwestie

- **Dłuższy start.** U-Boot doda około 1–2 sekund do startu systemu. Zmierzymy to w OTA-1.
- **Stary kernel z nowym DT.** DT pochodzi z partycji boot, a kernel z rootfs. Po powrocie samego rootfs stary kernel dostaje nowy DT. W obrębie tej samej serii kernela to nie jest problem. Alternatywa: U-Boot ładuje DTB z rootfs i sam stosuje overlaye (`fdt apply`). Decyzja zapadnie w OTA-5.
- **Moduły kernela potrzebne do instalacji.** Sprawdzone na Pi 21.11.2026:
  - `dm-verity`, `dm-mod`, `squashfs` i `nbd` są modułami;
  - `loop` i `bcm2835_wdt` są wbudowane w kernel.

  Przy odchudzaniu obrazu z modułów kernela trzeba te moduły zostawić.
- **`cmdline.txt`.** U-Boot bierze `bootargs` z DT od firmware, które z kolei pochodzi z `cmdline.txt`, i dopisuje do nich `root=`. `cmdline.txt` nie może więc zawierać własnego `root=`.
- **Odzyskiwanie bez sieci.** Zostaje `flash.sh` z pełnym obrazem `.wic`, czyli powrót przez kartę.
