# Nauka C++ na konsoli Lake – przewodnik dla rodzica

Konsola i emulator służą jako platforma do nauki programowania: uczeń (11–13 lat, po Scratchu) pisze małe gry
w C++ przez uproszczone API `lake`, a silnik, LVGL, platforma i pętla konsoli zostają po stronie rodzica.
Instrukcja dla ucznia: [DLA_UCZNIA.md](DLA_UCZNIA.md).

## Założenia

- **Styl Arduino/Processing.** Gra to `setup()` (raz) i `frame()` (60 razy na sekundę). Bez klas, wskaźników, `dt`
  i przestrzeni nazw w kodzie ucznia; matematyka całkowita (`x = x + 2` = 120 px/s).
- **Ekran 800x480 w natywnej rozdzielczości**, napisy wygładzaną czcionką (`text(..., rozmiar 1-4)` = 16/24/32/48 px),
  obrazki z liter rysowane z powiększeniem (`sprite(..., 3)`), kafelki mapy 32 px. Lake Mario jako jedyna gra celowo
  rysuje pixel-art 400x240 powiększany x2 (`canvas_scale()`).
- **Jedna nowa koncepcja na lekcję**, reszta się kumuluje. Każda lekcja to działająca gra od pierwszej minuty.
- **Nazwy API po angielsku** (`rect`, `held`, `random`), komentarze i lekcje po polsku. Pliki źródłowe bez polskich znaków
  (czcionka konsoli 5x7 ich nie ma, toolchain bywa kapryśny), dokumenty `.md` – z polskimi znakami.
- **Natychmiastowa informacja zwrotna:** F6 buduje i uruchamia grę z otwartego pliku, `watch()` pokazuje zmienne na ekranie,
  testy zadań mówią, co jest zrobione.
- **Determinizm:** w trybie testowym (`--frames`) stały krok czasu i stałe ziarno losowania, więc test daje zawsze ten sam wynik.

## Jak to jest zbudowane

| warstwa | gdzie | kto rusza |
|---|---|---|
| API ucznia | `src/lake/lake_api.h` (deklaracje z opisami), `lake_runtime.cpp` (implementacja), `simple_game.*` (adapter na `engine::Game`) | rodzic |
| rejestracja | `LAKE_GAME(id, "Nazwa", "opis")` na końcu `gra.cpp` + `LEKCJA(id)` w `src/games/lekcje/lista.h` | uczeń (jedna linia) |
| lekcje | `src/games/lekcje/NN_nazwa/` – `gra.cpp`, `README.md`, `testy.txt`, `rozwiazania/*.cpp.txt` | uczeń: `gra.cpp` |
| testy | `tools/testy.ps1`, `tests/scenarios.txt` (regresja Lake Mario), `testy.txt` w lekcjach | rodzic |

Dlaczego jawna lista zamiast automatycznej rejestracji: firmware ESP-IDF linkuje kod jako bibliotekę statyczną,
więc plik, do którego nikt się nie odwołuje, wypada z programu razem ze swoją grą. `registry.cpp` odwołuje się
do każdego wpisu z `lista.h`, dzięki czemu lekcje trafiają też do menu prawdziwej konsoli.

Uczeń pisze wszystko wewnątrz `namespace { ... }` – wszystkie lekcje trafiają do jednego programu i bez tego
dwie funkcje `setup()` zderzyłyby się w linkerze. Szablon ma to wpisane; błąd `multiple definition of 'setup()'`
oznacza, że w jakiejś lekcji klamry zabrakło.

Zmienne globalne ucznia **nie zerują się** przy ponownym wejściu w grę z menu (instancja gry żyje cały czas),
dlatego wartości startowe nadaje `setup()`. Lekcja 02 uczy tego wprost.

## Program

| nr | katalog | jedna nowa koncepcja | gra | testy | stan |
|---|---|---|---|---|---|
| 00 | `00_szablon` | budowa pliku gry, co wolno ruszać | napis | – | gotowa |
| 01 | `01_wizytowka` | zmienne, stałe, `Color`, współrzędne | wizytówka | 1 | gotowa |
| 02 | `02_pilka` | `if`, ruch = pozycja + prędkość | odbijająca się piłka | 2 | gotowa |
| 03 | `03_lapacz` | `held()`, `&&` `\|\|`, `else`, `random`, `clamp`, `print` | łapanie piłek | 2 | gotowa |
| 04 | `04_pong` | funkcje: parametry, `return`, `bool` | Pong 2 graczy / vs komputer | 1 | gotowa |
| 05 | `05_inwazja` | pętle `for`, zagnieżdżone, `%` | ściana kosmitów + statek | 1 | gotowa |
| 06 | `06_inwazja_strzal` | tablice `bool alive[3][8]`, `!`, `break` | Space Invaders | 1 | gotowa |
| 07 | `07_waz` | tablica jako lista, pętla w dół, `while` | Snake | 1 | gotowa |
| 08 | `08_flappy` | struktury, struktura jako parametr i wynik | Flappy (1–2 rury) | 1 | gotowa |
| 09 | `09_breakout` | tablica struktur, `break`/`continue` | Breakout | 1 | gotowa |
| 10 | `10_bohater` | sprite z liter, `flip_x`, animacja, tablica struktur | zbieracz monet | 1 | gotowa |
| 11 | `11_flappy_pelny` | `enum`, `switch`, maszyna stanów, rekord w globalu | Flappy kompletny | 1 | gotowa |
| 12 | `12_mini_mario` | mapa kafelków (`load_map`, `move_box`, `int&`), kamera | mini-platformówka | 1 | gotowa, stretch |
| 13 | `13_twoja_gra` | projekt własny (tylko README z listą kontrolną) | dowolna | – | gotowa, stretch |

Każda lekcja została sprawdzona 23.09.2026: kod startowy **nie przechodzi** swoich testów, rozwiązanie z `rozwiazania/` **przechodzi**.
Lekcja 12 używa API poziomu 2 (`load_map`, `map_tile`, `map_set`, `map_col/row`, `draw_tiles`, `move_box`) zbudowanego
na `engine::TileMap` – tym samym kodzie kolizji, na którym chodzi Lake Mario.

Odstępstwo od „klasycznej" kolejności: **funkcje (04) przed pętlami (05)**. Pong z dwiema paletkami naturalnie
motywuje własny klocek (uczeń zna „Moje bloki" ze Scratcha), a pętle motywuje ściana kosmitów.

## Jak prowadzić lekcję (30–45 min)

1. Razem przeczytajcie „Co nowego" w README – to 5–10 linii z analogią do Scratcha.
2. Uczeń uruchamia kod startowy (F6) i opisuje, co widzi. „Jak to działa" – przejdźcie po kodzie linia po linii.
3. Zadania ★ robi sam. ★★ z podpowiedzią z README. ★★★ to praca domowa albo na drugie spotkanie.
4. „Sprawdź sam" – uczeń uruchamia zadanie **LEKCJA: Sprawdz zadania** i widzi OK/BLAD.
5. Commit w Source Control (uczeń), push (rodzic).

Gdy uczeń utknie na 15 minut, może zajrzeć do `rozwiazania/` – ale najpierw niech opisze słowami, co chce osiągnąć.

## Testy

**Regresja Lake Mario** (`tests/scenarios.txt`): ślady i zrzuty bajt w bajt. Uruchamiać po każdej zmianie w silniku:

```
powershell -File tools/testy.ps1 mario
powershell -File tools/testy.ps1 mario -Update     # tylko po SWIADOMEJ zmianie fizyki albo wyglądu
```

Zrzut `menu` zmienia się po dodaniu każdej lekcji – wtedy `-Update` jest oczekiwane.

**Testy zadań** (`testy.txt` w katalogu lekcji): linia = `argumenty emulatora | regex`, skrypt dokleja `--game <id>`
i szuka regexu w wyjściu (głównie w linii `TRACE`, gdzie są wartości z `watch()`). Kod startowy **nie przechodzi**,
rozwiązanie **przechodzi** – tak są zaprojektowane. Cała weryfikacja:

```
powershell -File tools/testy.ps1                                 # Mario + wszystkie lekcje
powershell -File tools/testy.ps1 src/games/lekcje/03_lapacz      # jedna lekcja
```

Pisząc test: użyj `--hold KLAWISZ od do` do zasymulowania gracza, `--frames N --trace N-1` do odczytu ostatniej klatki,
a w regexie nazw z `watch()`. Przykład z lekcji 03: `--hold LEFT 0 300 --frames 301 --trace 300 | paddle_x=0( |$)`.

## Jak dodać lekcję

1. Skopiuj `00_szablon` do `NN_nazwa`, zmień `LAKE_GAME(nazwa, "Nazwa", "Lekcja NN: ...")`, dopisz `LEKCJA(nazwa)` w `lista.h`.
2. README wg [lekcje/SZABLON_README.md](lekcje/SZABLON_README.md): jedna koncepcja, analogia do Scratcha, zadania ★/★★/★★★, „Sprawdź sam".
3. Rozwiązania w `rozwiazania/zadN.cpp.txt` (rozszerzenie `.txt`, żeby się nie kompilowały). Zbuduj je raz podmieniając `gra.cpp` –
   test musi na nich przechodzić.
4. `testy.txt` z co najmniej jednym testem sprawdzalnym przez `watch()`.
5. Zrzut do README: zadanie **LEKCJA: Zrzut ekranu**, potem `tools/bmp2png.ps1`.
6. Emulator wykrywa nowe pliki sam (`CONFIGURE_DEPENDS`). Firmware: `pio run -t clean` przed `pio run`.

## Komputer ucznia

Instalacja: `tools/setup_kid_pc.ps1` (PowerShell jako administrator). Instaluje przez winget Git, CMake, Ninja, VS Code,
MSYS2 (+ g++, gdb), dwa rozszerzenia VS Code, klonuje gałąź `lekcje`, pobiera LVGL do `third_party/`, buduje emulator,
kładzie skrót „Lekcje C++" na pulpicie i przypisuje F6. Parametry i tryb bez wingeta – w nagłówku skryptu.

| | komputer rodzica | komputer ucznia |
|---|---|---|
| firmware | PlatformIO (`pio run`) | brak |
| LVGL dla emulatora | `managed_components/` (z PlatformIO) | `third_party/lvgl` (`tools/fetch_lvgl.ps1`) albo pobranie przez CMake |
| IntelliSense | PlatformIO generuje `c_cpp_properties.json` | CMake Tools (ustawienia użytkownika); na pytanie o providera: **Yes** |
| domyślne zadanie Ctrl+Shift+B | LEKCJA: Uruchom (PIO: Build w liście) | LEKCJA: Uruchom, także F6 |

Bez internetu: `tools/fetch_lvgl.ps1 -Zip <plik z pendrive'a>` (zip `v9.5.0` z GitHuba, suma SHA-256 w skrypcie).
Ścieżka repozytorium bez spacji i polskich znaków (`C:\Gry\LakeMarioGame`) – kompilator MinGW i CMake tego nie lubią.

## Git

- Uczeń pracuje na gałęzi `lekcje`, commituje lokalnie przez Source Control w VS Code („punkt zapisu").
- Push robi rodzic z komputera ucznia (Git Credential Manager zapamięta konto po pierwszym logowaniu; GitHub wymaga 13 lat na własne konto).
- Rodzic przegląda na GitHubie i merguje do `main` po skończonej lekcji. Firmware budować z `main` – zepsuta lekcja
  na `lekcje` nie psuje wtedy konsoli.
- Zasada dla ucznia: zmienia tylko `src/games/lekcje/`.

## Znane ograniczenia

- Czcionka konsoli 5x7 ma litery (wielkie i małe), cyfry i podstawowe znaki, ale nie polskie litery – napisy w grach bez ą, ę, ł...
- Panel `watch()` mieści 8 wartości na klatkę; nazwy do 11 znaków.
- Sprite'y ucznia mieszczą się w arenie 64 kB (ok. 128 obrazków 16x16), zerowanej przy każdym `setup()`. `load_sprite()`
  wołać w `setup()`, nie w `frame()`.
- START i SELECT należą do konsoli (pauza) – `held(Key::Start)` zawsze zwraca `false`.
- Lekcje kompilują się także do firmware: błąd w `gra.cpp` ucznia zatrzymuje `pio run`. Stąd gałąź `lekcje`.
