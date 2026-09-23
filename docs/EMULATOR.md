# Emulator na Windows

Emulator uruchamia **ten sam kod, co firmware** — menu LVGL, pętlę konsoli, renderer 2D i wszystkie
gry. Różni je wyłącznie implementacja warstwy `platform::`:

| | płytka | emulator |
|---|---|---|
| implementacja | [src/platform/platform_esp.cpp](../src/platform/platform_esp.cpp) | [sim/platform_win32.cpp](../sim/platform_win32.cpp) |
| obraz | MIPI-DSI + PPA (skalowanie x2, obrót) | okno Win32, `StretchBlt` najbliższym sąsiadem |
| wejście | przełączniki + gałka na ADC2, dotyk GT911, BOOT | klawiatura + pad USB + mysz (dotyk) |
| tempo klatek | synchronizacja pionowa panelu | odmierzanie do 60 FPS |

Dzięki temu logika gry, kolizje, wygląd sprite'ów i układ UI są identyczne. Zmiana w grze nie
wymaga wgrywania firmware — kompilacja emulatora trwa sekundy.

## Wymagania

- kompilator C++20: **MSYS2 MinGW-w64** (`C:\msys64\mingw64\bin\g++.exe`) albo MSVC
- **CMake** 3.20+ i **Ninja**

Żadnych bibliotek zewnętrznych: backend to czyste Win32 GDI, LVGL buduje się ze źródeł.
Gotowy plik `lake_sim.exe` jest linkowany statycznie, więc działa bez DLL-i z MSYS2.

## Budowanie

```bash
cmake -S sim -B sim/build -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe
```

```bash
cmake --build sim/build
```

```bash
./sim/build/lake_sim.exe
```

W VS Code te same kroki są pod Ctrl+Shift+B jako zadania `SIM: Configure`, `SIM: Build`, `SIM: Run`.

## Sterowanie i mapowanie klawiszy

Emulator odwzorowuje kontroler konsoli: te same klawisze (`UP DOWN LEFT RIGHT A B X Y START
SELECT`) oraz gałkę analogową, ta sama reakcja gry i menu. Który klawisz PC odpowiada któremu klawiszowi
konsoli, ustawia się w pliku [sim/keymap.cfg](../sim/keymap.cfg):

```
UP     = Up
DOWN   = Down
LEFT   = Left
RIGHT  = Right
A      = Z
B      = X
X      = S
Y      = A
START  = Enter
SELECT = Backspace

STICK_UP    = I
STICK_DOWN  = K
STICK_LEFT  = J
STICK_RIGHT = L
```

**Gałka analogowa.** Jeśli do PC podłączony jest pad USB, jego lewa gałka steruje gałką konsoli
z pełną rozdzielczością i ma pierwszeństwo przed klawiszami. Dzięki temu w emulatorze da się
sprawdzić gry korzystające z płynnego ruchu, a nie tylko z progowanych kierunków. Bez pada
działają klawisze `STICK_*`, dające wychylenie skrajne. Emulator pisze w logu przy starcie,
czy pad został wykryty.

Dozwolone nazwy po prawej: litery `A-Z`, cyfry `0-9`, `Up Down Left Right`, `Enter Space Tab Esc
Backspace Shift Ctrl Alt`, `F1`-`F12`, `Comma Period Slash Semicolon Minus Plus`.
Brakujący wpis oznacza wartość domyślną, a nieznana nazwa trafia do logu jako ostrzeżenie.
Przy starcie emulator wypisuje całe mapowanie, więc widać dokładnie, co zostało wczytane.

Plik jest szukany kolejno: obok pliku wykonywalnego (kopiowany tam przy budowaniu), potem
`sim/keymap.cfg`, potem `keymap.cfg`. Własny plik wskazuje się tak:

```bash
./sim/build/lake_sim.exe --keymap moje.cfg
```

Poza tym: **lewy przycisk myszy** działa jak palec na ekranie dotykowym (wybór gry w menu,
strefy wirtualnego pada), a **Esc** zamyka okno. Podpowiedzi wirtualnego pada znikają po pierwszym
użyciu klawiszy — tak samo jak na płytce po podłączeniu klawiatury.

Menu i ekran pauzy obsługują się klawiszami: `GORA`/`DOL` przenoszą zaznaczenie, `A` albo `START`
zatwierdzają, `B` albo `SELECT` cofają.

## Tryby pomocnicze

```bash
./sim/build/lake_sim.exe --game 0
```

Wchodzi od razu do gry o podanym numerze (kolejność z [registry.cpp](../src/games/registry.cpp)),
bez klikania w menu — przydatne przy pracy nad konkretną grą.

```bash
./sim/build/lake_sim.exe --game 0 --pause-at 20 --frames 40 --shot pauza.bmp
```

Przelicza podaną liczbę klatek bez interakcji, opcjonalnie wciska START w wskazanej klatce,
zapisuje obraz do BMP i kończy. Tak powstały zrzuty w [docs/images/](images/). Nadaje się do
sprawdzania, czy zmiana w UI albo w grafice nie popsuła wyglądu.

## Testy skryptowane

```bash
./sim/build/lake_sim.exe --game 0 --hold B 3 4 --hold RIGHT 20 90 --frames 90 --trace 15
```

`--hold KLAWISZ OD DO` trzyma klawisz konsoli od klatki OD do DO (można podać do ośmiu wpisów),
a `--trace K` co K klatek wypisuje linię stanu gry: pozycję, prędkości, podłoże, wynik, monety, życia,
czas i najbliższego przeciwnika. Razem dają powtarzalne testy mechanik bez udziału człowieka.
Przykład powyżej sprawdza, że postać po 20 klatkach chodu ma prędkość 100 px/s. Gra dostarcza tę linię
przez `engine::Game::debug_line()`, więc każda kolejna gra może mieć własną.

W trybie `--frames` krok czasu jest stały (1/60 s), więc ten sam skrypt daje zawsze identyczny wynik,
niezależnie od obciążenia komputera. W trybie okienkowym emulator liczy czas rzeczywisty, tak jak płytka.

Uwaga: przed przebudową zamknij działający emulator. Windows nie pozwala nadpisać uruchomionego
pliku i linker kończy się błędem bez czytelnego komunikatu.

## Skąd bierze się LVGL

CMake najpierw szuka kopii ściągniętej przez ESP-IDF (`managed_components/lvgl__lvgl`). Jeśli
firmware był już budowany, emulator używa **dokładnie tej samej wersji** bez pobierania czegokolwiek.
W przeciwnym razie pobiera tag `v9.5.0` z GitHuba — ten sam, który jest przypięty w
[src/idf_component.yml](../src/idf_component.yml). Obie ścieżki są sprawdzone.

Własną kopię LVGL wskazuje się tak:

```bash
cmake -S sim -B sim/build -G Ninja -DLAKE_LVGL_DIR=C:/sciezka/do/lvgl
```

Konfiguracja LVGL ([src/ui/lv_conf.h](../src/ui/lv_conf.h)) jest jednym plikiem wspólnym dla obu
celów, więc UI nie może się rozjechać między płytką a PC.

## Czego emulator nie sprawdzi

- rzeczywistej wydajności: PC jest znacznie szybszy niż ESP32-P4 przy 360 MHz,
- zużycia pamięci: emulator ma gigabajty, płytka ~200 kB pamięci wewnętrznej i 32 MB PSRAM,
- sprzętu: MIPI-DSI, PPA, kalibracji i wielodotyku GT911 (mysz daje tylko jeden punkt), audio, karty SD.

Wniosek praktyczny: **logikę i wygląd rób w emulatorze, wydajność i sprzęt sprawdzaj na płytce.**
