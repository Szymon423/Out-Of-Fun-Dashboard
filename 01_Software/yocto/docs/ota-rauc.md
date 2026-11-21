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
| Kernel | W rootfs (`/boot/Image`) | Kernel i jego moduły aktualizują się razem i wracają razem. |
| DTB i overlaye | Z partycji boot | Firmware stosuje je według `config.txt`, a U-Boot przekazuje kernelowi gotowe DT (`${fdt_addr}`). |
| Zmienne U-Boota | Zapisane wprost na karcie, poza partycjami, w dwóch kopiach | Plik `uboot.env` na FAT zniknąłby przy aktualizacji partycji boot, a razem z nim wybór slotu. Dwie kopie chronią przed przerwą w zasilaniu. |
| Aktualizacja partycji boot | Slot RAUC `boot-mbr-switch` | RAUC zapisuje nową partycję boot obok starej i przełącza wpis w MBR jedną operacją, więc nie ma stanu pośredniego. |
| Pakiet aktualizacji | `.raucb` w formacie `verity`, podpisany własnym kluczem | Pi sprawdza podpis i integralność przed zapisem. |
| Przesyłanie | `scp` i `rauc install` przez skrypt `ota.sh` | Najprostsze rozwiązanie. Później można dodać strumieniowanie przez HTTP, bo RAUC ma `streaming`. |
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
| `p4` partycja rozszerzona | | | kontener na `p5` |
| `p5` data (ext4) | po `p3` | 256 MiB, przy pierwszym starcie powiększana do końca karty | `/data` |

- **Rozmiar slotów.** Obecny rootfs zajmuje 495 MB, więc 2 GiB zostawia zapas na aplikację i większy WebKit. Plan zakłada kartę co najmniej 8 GB.
- **Obszar boot.** Zakres od 4 do 260 MiB to region `boot-mbr-switch`. RAUC zapisuje nową partycję boot w tej połowie, która jest akurat wolna, i przestawia na nią wpis `p1` w MBR.
- **Partycja `p5`.** W obrazie ma stały rozmiar, bo `wic` nie zna rozmiaru karty. Przy pierwszym starcie usługa powiększa partycję (`parted resizepart`), a system plików powiększa się sam dzięki `x-systemd.growfs`. Tak samo robi przykład z `meta-rauc-community`.
- **Możliwości `wic`.** Wersja z wrynose obsługuje `--offset`, `--align` i `--fixed-size`, więc taki układ da się opisać w pliku `.wks`.

## Start systemu

1. Firmware Pi czyta `config.txt` z `p1`, stosuje overlaye do DT i ładuje `u-boot.bin`.
2. U-Boot wczytuje swoje zmienne z karty i uruchamia `boot.scr`:
   - wybiera pierwszy slot z `BOOT_ORDER`, który ma `BOOT_x_LEFT > 0`;
   - zmniejsza jego licznik i zapisuje zmienne.
3. U-Boot ładuje `/boot/Image` z wybranego rootfs i startuje kernel z `root=/dev/mmcblk0p2` albo `p3`, `rauc.slot=A` albo `B` oraz `panic=5`. DT pochodzi od firmware.
4. systemd uruchamia kolejno Westona, kiosk i sprawdzenie stanu. Gdy wszystko działa, `rauc status mark-good` przywraca licznik slotu do 3.
5. Gdy system się zawiesi, restartuje go watchdog: przy kernel panic po 5 sekundach (`panic=5`), przy zawieszonym systemd watchdog sprzętowy. Każdy taki start zmniejsza licznik. Po trzech nieudanych startach U-Boot uruchamia drugi slot.

## Aktualizacja

1. Build na PC: `./build.sh dashboard-bundle` tworzy podpisany pakiet `.raucb`.
2. `./ota.sh` wysyła pakiet do `/data/ota/`, uruchamia `rauc install`, restartuje Pi, czeka, aż wróci SSH, i pokazuje `rauc status`.
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
| `wic/dashboard-ab.wks.in` | układ karty z tabeli powyżej |
| `recipes-bsp/u-boot/u-boot_%.bbappend` | fragment konfiguracji `env-in-mmc.cfg` i własny `fw_env.config` |
| `recipes-bsp/rpi-u-boot-scr/rpi-u-boot-scr.bbappend` | własny `boot.cmd.in` z logiką slotów |
| `recipes-bsp/bootfiles/rpi-cmdline.bbappend` | `root=` znika z `cmdline.txt`, bo ustawia go U-Boot |
| `recipes-core/rauc/rauc-conf.bbappend` | `system.conf` i `ca.cert.pem` (publiczny certyfikat CA) |
| `recipes-core/bundles/dashboard-bundle.bb` | przepis pakietu aktualizacji |
| `recipes-core/base-files/base-files_%.bbappend` | `fstab` z `/data` i `/boot` tylko do odczytu |
| `recipes-dashboard/dashboard-ota/` | usługi: powiększanie `/data`, sprawdzenie stanu z `mark-good`, watchdog, klucz hosta SSH na `/data` |
| `recipes-bsp/bootfs/dashboard-bootfs.bb` | obraz vfat partycji boot dla slotu `boot-mbr-switch` (etap OTA-5) |
| `recipes-core/images/dashboard-image.bb` | `WKS_FILE`, `rauc`, `kernel-image`, `BAD_RECOMMENDATIONS += "rauc-mark-good"` |
| `01_Software/yocto/keys/` | klucze podpisu, w całości w `.gitignore` |
| `01_Software/yocto/ota.sh` | skrypt narzędziowy do wysyłania i instalacji pakietu |

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
CONFIG_SYS_MMC_ENV_DEV=0
CONFIG_ENV_OFFSET=0x100000
CONFIG_ENV_OFFSET_REDUND=0x120000
CONFIG_SYS_REDUNDAND_ENVIRONMENT=y
CONFIG_ENV_SIZE=0x4000
```

`fw_env.config` dla `fw_printenv` i `fw_setenv` w Linuksie. Plik z meta-raspberrypi wskazuje `/boot/uboot.env`, więc trzeba go podmienić:

```
/dev/mmcblk0 0x100000 0x4000
/dev/mmcblk0 0x120000 0x4000
```

`boot.cmd.in` budujemy na wzór `meta-rauc-community/meta-rauc-raspberrypi`, z czterema zmianami:
- usunąć zapis `uboot.env` na FAT;
- slot A to `p2`, a slot B to `p3`;
- kernel ładować z `boot/Image` w rootfs;
- do `bootargs` dodać `panic=5`.

## Klucze

- **Generowanie.** Klucze tworzy skrypt `layers/meta-rauc/scripts/openssl-ca.sh` i zapisuje je w `01_Software/yocto/keys/`. Cały ten katalog jest w `.gitignore`.
- **Kopia zapasowa poza repozytorium.** Bez klucza CA żadne już zainstalowane urządzenie nie przyjmie nowej aktualizacji, więc trzeba by znowu flashować kartę.
- **Certyfikat CA w warstwie.** Publiczny `ca.cert.pem` trafia do `recipes-core/rauc/files/`.
- **Gdzie podpis szuka kluczy.** `RAUC_KEY_FILE` i `RAUC_CERT_FILE` w `dashboard-bundle.bb` wskazują `${TOPDIR}/../keys/...`.
- **Ważność certyfikatów.** Pi nie ma zegara z baterią, więc tuż po starcie ma datę z obrazu. Data `notBefore` certyfikatów musi być wcześniejsza niż ta data. Trzeba to sprawdzić w certyfikatach ze skryptu.

## Etapy

### OTA-0: Przygotowanie (bez flashowania)
- Dodać submoduł `meta-rauc` (`wrynose`) i wpisać go do `bblayers.conf`.
- Wygenerować klucze i zrobić ich kopię zapasową.
- Dopisać `keys/` do `.gitignore`.
- **Sprawdzenie:** parsowanie bez błędów (`./build.sh -p`).

### OTA-1: U-Boot ze zmiennymi na karcie (flashowanie)
- Ustawić `RPI_USE_U_BOOT = "1"`, dodać fragment `env-in-mmc.cfg` i własny `fw_env.config`.
- Rootfs zostaje na razie jeden, a kernel nadal leży na `p1` (domyślny `boot.cmd`).
- **Sprawdzenie:**
  - Pi startuje przez U-Boot;
  - `fw_printenv` działa;
  - wartość ustawiona przez `fw_setenv test 1` przetrwa restart;
  - zmierzyć, o ile wydłużył się start systemu.

### OTA-2: Układ A/B i RAUC (ostatnie flashowanie)
- Plik `.wks` z nowym układem, `boot.cmd.in` z logiką slotów i `system.conf`.
- Do obrazu dochodzą `rauc` i `kernel-image`, do tego `/data` z powiększaniem przy pierwszym starcie i `fstab`.
- Klucz hosta Dropbeara przenieść na `/data`: `/etc/dropbear` jako dowiązanie do `/data/dropbear` i `RequiresMountsFor=/data` w `dropbearkey.service`.
  - Samo `DROPBEAR_RSAKEY_DIR` nie wystarczy, bo warunki `ConditionPathExists` w tej usłudze sprawdzają tylko `/etc/dropbear` i `/var/lib/dropbear`.
- **Sprawdzenie:**
  - `rauc status` pokazuje uruchomiony slot A i nieaktywny B;
  - `fw_printenv BOOT_ORDER` zwraca `A B`;
  - `df -h /data` obejmuje resztę karty;
  - klucz hosta SSH nie zmienia się po restarcie.

### OTA-3: Pakiet aktualizacji i `ota.sh`
- Przepis `dashboard-bundle.bb` (`RAUC_BUNDLE_SLOTS = "rootfs"`) i skrypt `ota.sh`.
- **Sprawdzenie:**
  - instalacja z A na B, restart, `rauc status` pokazuje uruchomiony slot B;
  - potem z B na A;
  - zmierzyć czas całej aktualizacji.

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
