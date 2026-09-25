# Emulator na Windows

Emulator uruchamia **ten sam kod, co firmware** — menu LVGL, pętlę konsoli, renderer 2D i wszystkie
gry. Różni je wyłącznie implementacja warstwy `platform::`:

| | płytka | emulator |
|---|---|---|
| implementacja | [src/platform/platform_esp.cpp](../src/platform/platform_esp.cpp) | [sim/platform_win32.cpp](../sim/platform_win32.cpp) |
| obraz | MIPI-DSI + PPA (obrót; płótno 800x480 natywne) | okno Win32 800x480 1:1 |
| wejście | przełączniki + gałka na ADC2, dotyk GT911, BOOT | klawiatura + pad USB + mysz (dotyk) |
| tempo klatek | synchronizacja pionowa panelu | odmierzanie do 60 FPS |

Dzięki temu logika gry, kolizje, wygląd sprite'ów i układ UI są identyczne. Zmiana w grze nie
wymaga wgrywania firmware — kompilacja emulatora trwa sekundy.

## Wymagania

- kompilator C++20: **MSYS2 MinGW-w64** (`C:\msys64\mingw64\bin\g++.exe`) albo MSVC
- **CMake** 3.20+ i **Ninja**

Żadnych bibliotek zewnętrznych: backend to czyste Win32 GDI, LVGL buduje się ze źródeł.
Gotowy plik `console_sim.exe` jest linkowany statycznie, więc działa bez DLL-i z MSYS2.

## Budowanie

```bash
cmake -S sim -B sim/build -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe
```

```bash
cmake --build sim/build
```

```bash
./sim/build/console_sim.exe
```

W VS Code te same kroki są pod Ctrl+Shift+B jako zadania `SIM: Configure`, `SIM: Build`, `SIM: Run`.
Nowe pliki `.cpp` (np. nowa lekcja) są wykrywane przy zwykłym `cmake --build` (`CONFIGURE_DEPENDS`),
rekonfiguracja jest potrzebna tylko po zmianie `CMakeLists.txt`.

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
./sim/build/console_sim.exe --keymap moje.cfg
```

Poza tym: **lewy przycisk myszy** działa jak palec na ekranie dotykowym (wybór gry w menu,
strefy wirtualnego pada), a **Esc przytrzymany 1,5 s** zamyka okno (pasek postępu na dole; krótkie wciśnięcie
pokazuje tylko podpowiedź – żeby dziecko nie wyłączyło konsoli przypadkiem; krzyżyk okna zamyka od razu). Podpowiedzi wirtualnego pada znikają po pierwszym
użyciu klawiszy — tak samo jak na płytce po podłączeniu klawiatury.

Menu i ekran pauzy obsługują się klawiszami: `GORA`/`DOL` przenoszą zaznaczenie, `A` albo `START`
zatwierdzają, `B` albo `SELECT` cofają.

## Tryby pomocnicze

```bash
./sim/build/console_sim.exe --list
./sim/build/console_sim.exe --game 0
./sim/build/console_sim.exe --game pilka
./sim/build/console_sim.exe --game src/games/lekcje/02_pilka
```

`--list` wypisuje gry (numer, id, nazwa). `--game` wchodzi od razu do gry — po numerze, po `id`
(pole `GameEntry::id`, dla lekcji nazwa z `CONSOLE_ADD_GAME`), po nazwie z menu albo po ścieżce katalogu lekcji
(ostatni segment bez numeru `NN_`). Dzięki temu zadanie VS Code może uruchomić grę z otwartego pliku.
Nieznana gra: komunikat, lista dostępnych i kod wyjścia 2, jeszcze przed otwarciem okna.

```bash
./sim/build/console_sim.exe --game 0 --pause-at 20 --frames 40 --shot pauza.bmp
```

Przelicza podaną liczbę klatek bez interakcji, opcjonalnie wciska START w wskazanej klatce,
zapisuje obraz do BMP i kończy. Tak powstały zrzuty w [docs/images/](images/). Nadaje się do
sprawdzania, czy zmiana w UI albo w grafice nie popsuła wyglądu.

## Testy skryptowane

W trybie `--frames` emulator nie pokazuje okna (zrzuty powstają z bitmapy w pamięci) i ignoruje prawdziwą klawiaturę
i mysz – liczą się tylko `--hold` i `--pause-at`. Można więc pisać i klikać w innych programach podczas `tools/testy.ps1`
bez psucia wyników i bez kradzieży fokusu.

```bash
./sim/build/console_sim.exe --game 0 --hold B 3 4 --hold RIGHT 20 90 --frames 90 --trace 15
```

`--hold KLAWISZ OD DO` trzyma klawisz konsoli od klatki OD do DO (można podać do dwunastu wpisów),
a `--trace K` co K klatek wypisuje linię stanu gry: pozycję, prędkości, podłoże, wynik, monety, życia,
czas i najbliższego przeciwnika. Razem dają powtarzalne testy mechanik bez udziału człowieka.
Przykład powyżej sprawdza, że postać po 20 klatkach chodu ma prędkość 100 px/s. Gra dostarcza tę linię
przez `engine::Game::debug_line()`, więc każda kolejna gra może mieć własną.

W trybie `--frames` krok czasu jest stały (1/60 s), generator losowy dostaje stałe ziarno, a emulator nie czeka
na 60 FPS (600 klatek liczy się w ułamku sekundy) — ten sam skrypt daje zawsze identyczny wynik, niezależnie od
obciążenia komputera. W trybie okienkowym emulator liczy czas rzeczywisty i losuje z zegara, tak jak płytka.
Stały krok jest ustawiany **przed** startem gry — inaczej ziarno byłoby brane z zegara (tak było do 23.09; gry
z `random()` dawały wtedy różne ślady przy identycznych argumentach).

### Pomiar czasu klatki

```bash
./sim/build/console_sim.exe --game labirynt3d --hold A 3 4 --hold UP 10 600 --frames 600 --bench
```

`--bench` wypisuje `BENCH klatek=590 srednio=… ms max=…` — czas `app::frame()` (logika + render + kopia do okna GDI)
bez 10 pierwszych klatek. To liczba z PC, nie z płytki; służy do porównania gier między sobą. Pomiar z 23.09:

| gra | średnio | uwagi |
|---|---|---|
| pilka (lekcja, prawie nic nie rysuje) | 0,52 ms | koszt samej kopii okna |
| Lake Mario | 0,54 ms | |
| Kosmos | 0,54 ms | |
| Labirynt 3D | 0,62 ms | raycasting 400 kolumn + tekstury: ok. 0,1 ms ponad tło |
| Kart | ~5 ms | renderer 3D gfx3d w 800x480 z teksturami (atlas 8-bit + colormapa, korekcja perspektywy co 16 px), cieniami rzutowanymi (maska), Z-buforem 16-bit (+0,3 ms; `KART_NOZ=1` wyłącza), ~12 tys. trójkątów przed odrzucaniem; 24.09 bez tekstur 2,5 ms, 23.09: 2,05 ms |

Skala na P4 (360 MHz, bez SIMD) jest szacunkowo 20-40 razy wolniejsza — raycasting kosztowałby 2-4 ms z 16,7 ms
budżetu. **Do zmierzenia na sprzęcie** (`engine::stats`).

### Zestaw regresji i testy lekcji

```bash
powershell -File tools/testy.ps1 mario          # tests/scenarios.txt: ślady i zrzuty Lake Mario bajt w bajt
powershell -File tools/testy.ps1                # to samo + testy.txt każdej lekcji (src/games/lekcje/*)
powershell -File tools/testy.ps1 mario -Update  # nagraj wzorce od nowa (po świadomej zmianie fizyki/wyglądu)
```

Scenariusze są w [tests/scenarios.txt](../tests/scenarios.txt), wzorce w `tests/expected/`: 6 śladów i 3 zrzuty Lake Mario,
zrzut plakatu API oraz (od 23.09) trasa w Labiryncie 3D (skręty, drzwi, moneta), rozgrywka w Kosmosie (ślad + zrzut)
i Kart (okrążenie autopilotem z rywalami i przedmiotami – ślad 1800 klatek, zrzut, oraz ślad sterowania gracza:
gaz, skręt, drift).
Refaktoring silnika ma zostawić je bez zmian. Testy lekcji (`testy.txt`: `argumenty | regex`) czytają wartości z `watch()` w linii `TRACE` —
szczegóły w [NAUKA.md](NAUKA.md).

Uwaga: przed przebudową zamknij działający emulator. Windows nie pozwala nadpisać uruchomionego
pliku i linker kończy się błędem bez czytelnego komunikatu.

## Skąd bierze się LVGL

CMake szuka kolejno:

1. `managed_components/lvgl__lvgl` — kopia ściągnięta przez ESP-IDF; jeśli firmware był budowany, emulator używa
   **dokładnie tej samej wersji** bez pobierania czegokolwiek;
2. `third_party/lvgl` — kopia rozpakowana przez `tools/fetch_lvgl.ps1` (komputer bez PlatformIO, np. ucznia;
   katalog jest w `.gitignore`; skrypt zostawia tylko ~30 MB potrzebne do kompilacji i sprawdza sumę SHA-256 zipa;
   `-Zip plik` pozwala podać archiwum z pendrive'a);
3. pobranie zipa taga `v9.5.0` z GitHuba przez `FetchContent` (bez gita, ~100 MB, wymaga internetu przy pierwszej konfiguracji).

Tag jest ten sam, który jest przypięty w [src/idf_component.yml](../src/idf_component.yml). Wszystkie trzy ścieżki są sprawdzone.

Własną kopię LVGL wskazuje się tak:

```bash
cmake -S sim -B sim/build -G Ninja -DCONSOLE_LVGL_DIR=C:/sciezka/do/lvgl
```

Konfiguracja LVGL ([src/ui/lv_conf.h](../src/ui/lv_conf.h)) jest jednym plikiem wspólnym dla obu
celów, więc UI nie może się rozjechać między płytką a PC.

**Pad USB** (winmm, widzi pady XInput i DirectInput): lewa gałka = gałka konsoli, krzyżak pada = krzyżak,
przyciski wg wpisów `PAD_A = 1` … `PAD_SELECT = 7` w `keymap.cfg` (numery jak w „Kontrolery gier” Windows;
domyślnie pad Xbox: A=1 B=2 X=3 Y=4 Back=7 Start=8, 0 = brak przypisania).

## Instalator dla dzieci i aktualizacje

Wersja do zabawy (bez narzędzi programisty): `KotarbaConsole-X.Y.Z-setup.exe` z
[wydań na GitHubie](https://github.com/grekot/ESP32_p4_game_console/releases). Instaluje się bez praw administratora do
`%LOCALAPPDATA%\Programs\KotarbaConsole`, dodaje skrót w menu Start i `STEROWANIE.txt`. Skrót uruchamia
`KotarbaConsole.exe`, który przy każdym starcie sprawdza najnowsze wydanie i pyta o instalację (bez internetu – startuje
normalnie). Rekordy i ustawienia (`save_*.bin`) przetrwają aktualizację.

Nowe wydanie (wszystko zacommitowane, potem):

```
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions (`.github/workflows/release.yml`) zbuduje instalator i opublikuje wydanie w kilka minut. Lokalnie:
`powershell -File tools/build_installer.ps1 -Version 1.0.0` → `dist/` (wymaga Inno Setup 6). Build `dev` (bez `-Version`)
nie sprawdza aktualizacji.

## Czego emulator nie sprawdzi

- rzeczywistej wydajności: PC jest znacznie szybszy niż ESP32-P4 przy 360 MHz,
- zużycia pamięci: emulator ma gigabajty, płytka ~200 kB pamięci wewnętrznej i 32 MB PSRAM,
- sprzętu: MIPI-DSI, PPA, kalibracji i wielodotyku GT911 (mysz daje tylko jeden punkt), audio, karty SD.

Wniosek praktyczny: **logikę i wygląd rób w emulatorze, wydajność i sprzęt sprawdzaj na płytce.**
