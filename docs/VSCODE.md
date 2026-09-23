# Projekt w VS Code

Jeden folder, dwa cele: firmware na płytkę buduje **PlatformIO**, emulator na Windows buduje
**CMake Tools**. Każde z tych rozszerzeń widzi tylko swoją część, więc nic nie trzeba przełączać.

## Rozszerzenia

Po otwarciu folderu VS Code zaproponuje rekomendowane rozszerzenia. Potrzebne są cztery:

| rozszerzenie | do czego |
|---|---|
| PlatformIO IDE | firmware: budowanie, wgrywanie, monitor, debug przez JTAG |
| C/C++ (ms-vscode.cpptools) | podświetlanie, IntelliSense, debugger emulatora |
| CMake Tools | emulator: konfiguracja, budowanie, debug |
| ESP Exception Decoder | tłumaczenie adresów z paniki ESP32 na linie kodu |

**Nie instaluj pioarduino IDE** obok PlatformIO IDE. To fork PlatformIO IDE i oba naraz gryzą się
o te same komendy. Wystarcza oficjalne PlatformIO IDE, które masz.

## Na co odpowiedzieć przy pierwszym otwarciu

Wszystko poniżej jest już ustawione w `.vscode/settings.json`, więc pytań powinno być mało.
Gdyby się jednak pojawiły:

- **„Would you like to configure project?"** (CMake Tools) — nie trzeba, konfiguracja jest przez
  zadanie `SIM: Configure`. Możesz kliknąć *Not now*.
- **„Select a configure preset"** — jest jeden: *Emulator (MinGW z MSYS2)*. Wybierz go.
- **„Use CMake Tools as IntelliSense provider?"** — **No**. IntelliSense dostarcza PlatformIO
  z listą nagłówków ESP-IDF. Jeśli to zmienisz, podświetlanie w kodzie płytki przestanie działać.
  **Na komputerze ucznia (bez PlatformIO) odpowiedź jest odwrotna: Yes** — tam IntelliSense bierze się z CMake Tools
  (ustawia to `tools/setup_kid_pc.ps1`).
- **PlatformIO: „Rebuild IntelliSense index"** — tak, jeśli zaproponuje. Trwa chwilę, tworzy
  `c_cpp_properties.json`.

## Firmware na płytkę

Wszystko przez zadania VS Code (`Ctrl+Shift+B` pokazuje listę) albo z paska PlatformIO na dole:

| zadanie | co robi |
|---|---|
| `PIO: Build` | kompilacja, domyślne zadanie budowania |
| `PIO: Upload` | wgranie na płytkę |
| `PIO: Monitor` | logi z płytki (115200) |
| `PIO: Upload + Monitor` | jedno po drugim |
| `PIO: Clean` | po dodaniu plików źródłowych albo zmianie `sdkconfig.defaults` |
| `PIO: menuconfig` | konfiguracja ESP-IDF w terminalu |

Debugowanie przez JTAG (`F5`, konfiguracje *PIO Debug*) wymaga sondy podłączonej do płytki.
Do zwykłej pracy nad grami użyj emulatora.

## Emulator na Windows

| zadanie | co robi |
|---|---|
| `LEKCJA: Uruchom gre z otwartego pliku` | **domyślne (Ctrl+Shift+B)**: zamyka emulator, buduje, uruchamia grę z katalogu otwartego pliku (`--game ${relativeFileDirname}`) |
| `LEKCJA: Zrzut ekranu` | 120 klatek bez klawiszy, `zrzut.bmp` w katalogu lekcji |
| `LEKCJA: Slad` | 300 klatek, co 30 wypisuje wartości z `watch()` |
| `LEKCJA: Sprawdz zadania` | `tools/testy.ps1` na `testy.txt` otwartej lekcji |
| `SIM: Zamknij emulator` | `Stop-Process lake_sim` — Windows nie pozwala nadpisać działającego exe |
| `SIM: Configure` | jednorazowo; używa presetu `sim/CMakePresets.json` (nowe pliki wykrywa sam build) |
| `SIM: Build` | kompilacja emulatora, kilka sekund |
| `SIM: Run` | zbuduj i uruchom z menu konsoli |
| `SIM: Lista gier` | `lake_sim.exe --list` |
| `SIM: Testy regresji (Mario + lekcje)` | `tools/testy.ps1`: ślady i zrzuty Lake Mario + testy lekcji |

Domyślne zadanie budowania to `LEKCJA: Uruchom`, bo z niego korzysta uczeń (patrz [NAUKA.md](NAUKA.md)); `PIO: Build`
zostaje w liście i w pasku PlatformIO. Na komputerze ucznia skrypt instalacyjny przypisuje do tego zadania też **F6**.

**Zamknij działający emulator przed `SIM: Build`.** Windows nie pozwala nadpisać uruchomionego pliku
i linker kończy się błędem bez czytelnego komunikatu.

To samo z paska CMake Tools na dole okna: przycisk *Build*, obok wybór celu `lake_sim`, a przycisk
z robakiem uruchamia emulator pod debuggerem gdb z MSYS2. Punkty przerwania, podgląd zmiennych,
krokowanie po kodzie gry — bez żadnego `launch.json`.

## Dlaczego CMake Tools patrzy na `sim/`, a nie na główny folder

Główny `CMakeLists.txt` należy do ESP-IDF i da się go zbudować tylko przez PlatformIO, które ustawia
środowisko toolchainu. Gdyby CMake Tools próbowało go skonfigurować, dostałoby błąd o brakującym
`IDF_PATH`. Ustawienie `cmake.sourceDirectory` kieruje je na katalog emulatora.

## IntelliSense w plikach emulatora

Lista nagłówków pochodzi z PlatformIO, więc w plikach `sim/` edytor podkreśli `<windows.h>` jako
nieznany. To tylko podświetlanie, kompilacja jest poprawna. Cena za to, że firmware i emulator
żyją w jednym folderze.

## Pliki w `.vscode`

| plik | kto zarządza |
|---|---|
| `settings.json` | projekt (wersjonowany) |
| `tasks.json` | projekt (wersjonowany) |
| `extensions.json` | projekt (wersjonowany); PlatformIO potrafi dopisać swoje |
| `c_cpp_properties.json` | **PlatformIO, generowany**, w `.gitignore` |
| `launch.json` | **PlatformIO, generowany**, w `.gitignore` |
