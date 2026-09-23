# CLAUDE.md - LakeMarioGame

Mini konsola do gier na **ESP32-P4** (płytka Guition JC4880P443C_I_W_Y: ekran 4,3" 480x800 MIPI-DSI,
dotyk GT911, ESP32-C6 jako radio) plus **emulator na Windows**, który uruchamia ten sam kod.
Pierwsza gra: platformówka „Lake Mario". UI konsoli w LVGL 9.5, gry na własnym rendererze 2D.

Kod i komentarze po polsku, **BEZ znaków diakrytycznych w plikach źródłowych** (kodowanie w toolchainie);
dokumentacja w `docs/`, README i ten plik z polskimi znakami.

## Stan na 2026-09-23

- **Płytka jeszcze nie dotarła** (zamówiona 22.09.2026, dostawa ok. 27.09-01.10). Cały kod powstał bez
  sprzętu. Po przyjściu płytki zacząć od listy „Do zweryfikowania na sprzęcie" w `docs/HARDWARE.md`.
- Firmware kompiluje się (23.09 wieczorem, płótno 800x480, warstwa `lake`, 13 lekcji): **1 115 kB flash (26,6 % z 4 MB)**,
  45,9 kB RAM statycznie. Wzrost z 832 kB to czcionki Montserrat 24/32/40/48 (~280 kB) — jeśli flash zacznie brakować,
  wyłączyć 40 i 48 w `lv_conf.h` (używa ich tylko `lake::text` w rozmiarze 4 i etykiety pada). Emulator kompiluje się i działa.
- Wszystkie mechaniki Lake Mario zweryfikowane skryptami w emulatorze (lista niżej). Bez testu skryptowego,
  tylko przegląd kodu: meta `F`, śmierć w przepaści, koniec czasu.
- **Platforma nauki C++ dla syna (11-13 lat, po Scratchu) — gotowa 23.09:** warstwa `src/lake/`, 13 lekcji
  w `src/games/lekcje/` (00 szablon … 12 mini mario, 13 własny projekt) z README, `testy.txt` i rozwiązaniami; każda
  sprawdzona: kod startowy pada, rozwiązanie przechodzi. Regresja Mario (`tools/testy.ps1 mario`) 9/9 po refaktorze
  silnika (math2d, palette, ParticlePool, TileMap wycięte z `mario_game.cpp`: 641 -> 538 linii). Dokumentacja:
  `docs/NAUKA.md` (rodzic), `docs/DLA_UCZNIA.md` (uczeń). **Niesprawdzone:** `tools/setup_kid_pc.ps1` na czystej
  maszynie (tylko parser), gałąź `lekcje` nieutworzona, prace **niezacommitowane**.
- **Przejście na 800x480 (23.09 po południu, decyzja użytkownika „nowocześnie, nie retro"):** płótno natywne, LVGL bez
  schodków, nowy design menu/pauzy (`ui/theme.h`), wygładzony tekst w grach (`gfx/text.h`), Mario jako pixel-art x2 przez
  `canvas_scale()`, lekcje przeliczone; regresja Mario 9/9 (ślady identyczne, zrzuty 800x480 = x2 starych), 12 lekcji
  zweryfikowanych (start pada, rozwiązanie przechodzi). Emulator: okno 1:1, zrzuty 800x480 (1,15 MB BMP).
- Repozytorium: **https://github.com/grekot/ESP32_p4_game_console.git**, gałąź `main`, pierwszy commit 23.09.2026.
  Commit i push tylko na wyraźne polecenie użytkownika. `.gitattributes` wymusza LF w repozytorium.
  Uwaga historyczna: repozytorium bez żadnego commita wywala build ESP-IDF (woła `git describe`).
- Około 7750 linii własnego kodu (bez sterownika ST7701 producenta), w tym ~1500 w `src/lake` + lekcjach + `tools/`.

## Komendy

```bash
pio run                              # firmware (pierwszy raz ~15 min: pobiera ESP-IDF i toolchain); po nowych plikach: pio run -t clean
pio run -t upload -t monitor         # wgranie + logi 115200
cmake -S sim --preset mingw          # konfiguracja emulatora (raz; nowe .cpp wykrywa sam build - CONFIGURE_DEPENDS)
cmake --build sim/build              # emulator (kilka sekund) - ZAMKNIJ dzialajacy lake_sim.exe!
./sim/build/lake_sim.exe             # emulator okienkowy
./sim/build/lake_sim.exe --list      # gry: numer, id, nazwa
./sim/build/lake_sim.exe --game pilka --hold RIGHT 10 70 --frames 90 --trace 15      # po id/nazwie/katalogu lekcji
./sim/build/lake_sim.exe --game 0 --hold B 3 4 --hold RIGHT 20 90 --frames 90 --trace 15   # test skryptowany
powershell -File tools/testy.ps1 mario                       # regresja Lake Mario: 6 sladow + 3 zrzuty bajt w bajt (3,5 s)
powershell -File tools/testy.ps1 src/games/lekcje/02_pilka   # testy zadan jednej lekcji (kod startowy PADA, rozwiazanie przechodzi)
powershell -File tools/testy.ps1 mario -Update               # nowe wzorce - tylko po swiadomej zmianie (np. menu po nowej lekcji)
```

Ścieżki: PlatformIO `C:\Users\grzegorz.kotarba\.platformio\penv\Scripts\pio.exe`, kompilator
`C:\msys64\mingw64\bin\g++.exe` (musi być w PATH razem ze swoimi DLL-ami), CMake 4.x w `C:\Program Files\CMake`,
Ninja w `C:\Prg\ninja-win`. Z Git Basha: `export PATH="/c/msys64/mingw64/bin:$PATH"` przed cmake.
**`pio run` uruchamiać z PowerShella, nie z Git Basha** — instalator narzędzi pioarduino odmawia pracy pod MSYS.

## Dwa cele, jeden kod

| warstwa | płytka | emulator |
|---|---|---|
| `src/platform/platform.h` | `src/platform/platform_esp.cpp` | `sim/platform_win32.cpp` |
| obraz | MIPI-DSI + PPA (skala x2, obrót) | okno Win32 GDI, `StretchBlt` |
| wejście | krzyżak + gałka + przyciski (JP1), GT911, BOOT | klawisze PC (`keymap.cfg`), pad USB (winmm), mysz |
| czas | `esp_timer` | `QueryPerformanceCounter`; w trybie `--frames` stały krok 1/60 s |

Zasada: **nic nad `platform::` nie może zawierać `#include "esp_*"` ani FreeRTOS.** Logowanie przez
`core/log.h` (`LAKE_LOGI/W/E`), nie `ESP_LOGx`. Sprawdzenie: `grep -rn "esp_\|freertos" src/app src/engine
src/gfx src/input src/ui --include=*.h --include=*.cpp | grep include` ma nic nie zwracać.

## Struktura

```
platformio.ini          firmware: platforma pioarduino 55.03.312 (ESP-IDF 5.5.5), board esp32-p4, LAKE_DISPLAY_ROTATION
CMakeLists.txt          projekt ESP-IDF; dodaje src/ui (lv_conf.h) do include WSZYSTKICH komponentów
sdkconfig.defaults      ESP-IDF: rewizja P4, PSRAM HEX, flash 16 MB DIO, LVGL (CONF_SKIP=n, bez dem/przykładów)
partitions.csv          nvs, phy, factory 4 MB @0x10000, assets (spiffs) ~12 MB
src/main.cpp            wejście płytki: NVS -> platform::init -> task "console" (rdzeń 1) -> app::run
src/platform/           interfejs platformy + implementacja ESP
src/board/              sterowniki płytki: pins.h, display (DSI+PPA), touch (GT911), keypad, joystick (ADC2), buttons, st7701/
src/core/log.h          logowanie zależne od celu
src/gfx/                Canvas RGB565 (blit, blit_scaled, blit_upscale2x, line, circle), Sprite z ASCII-artu (make_sprite, domyślna paleta),
                        text.h (wygładzony tekst Montserrat 12-48 px z glifów LVGL - menu i gry ucznia),
                        palette.h (19 kolorów gfx::pal::* + DEFAULT_PALETTE, wspólna dla Mario i lake), czcionka 5x7 (wielkie+małe)
src/input/              keys.h (14 klawiszy), pad.h (PadState + osie, held/pressed), virtual_pad (dotyk + klawisze -> PadState, zbocza)
src/engine/             Game (init/update/render/canvas_scale/debug_line), screen.h (800x480 + PIXEL_CANVAS 400x240), stats, game_registry (GameEntry{id,name,desc,create},
                        find_game), rng (xorshift32, seed_rng), math2d.h, particles.h (ParticlePool<N>), tilemap (TileMap: ASCII,
                        solid_at, move_x/move_y AABB, draw z kamerą - wycięte 1:1 z Mario)
src/ui/                 lv_conf.h (WSPÓLNY), lvgl_glue (PARTIAL, flush do płótna, indev dotyk+klawisze, grupa), menu (36 px/pozycja,
                        4 widoczne, pasek przewijania), pause
src/app/                maszyna stanów konsoli Menu -> Playing -> Paused; set_fixed_dt; debug_line; seed_rng przy starcie gry
src/lake/               API dla ucznia: lake.h (using namespace lake + makro LAKE_GAME), lake_api.h (deklaracje z opisami),
                        lake_runtime.cpp (implementacja, arena sprite'ów 64 kB, watch, mapa poziom 2), simple_game (adapter -> engine::Game)
src/games/registry.cpp  lista gier: Mario + lekcje z lekcje/lista.h (X-makro)
src/games/mario/        assets (sprite'y ASCII na wspólnej palecie), level (6 segmentów 25x15), game (fizyka, HUD; mapa = engine::TileMap)
src/games/lekcje/       lista.h (LEKCJA(id) na lekcję) + 00_szablon … 13_twoja_gra: gra.cpp, README.md, testy.txt, rozwiazania/*.cpp.txt
sim/                    emulator: CMakeLists (LVGL: managed_components → third_party/lvgl → FetchContent zip), CMakePresets,
                        platform_win32 (set_unthrottled), keymap_win32 + keymap.cfg, gamepad_win32 (winmm), main (--list, --game id)
tests/                  scenarios.txt + expected/ (ślady i BMP Lake Mario nagrane 23.09 przed refaktorem)
tools/                  testy.ps1 (regresja + testy lekcji), setup_kid_pc.ps1 (PC ucznia), fetch_lvgl.ps1, bmp2png.ps1
third_party/            (gitignore) LVGL z fetch_lvgl.ps1 na komputerze bez PlatformIO
docs/HARDWARE.md        płytka, pinout, JP1, kontroler, kalibracja gałki, lista do sprawdzenia na sprzęcie, źródła
docs/EMULATOR.md        budowanie, sterowanie, keymap, tryby CLI, testy skryptowane, źródła LVGL
docs/VSCODE.md          rozszerzenia, zadania (LEKCJA:/SIM:/PIO:), debug, na co odpowiadać przy pierwszym otwarciu
docs/NAUKA.md           nauka C++ (dla rodzica): założenia, program 13 lekcji, testy zadań, komputer ucznia, git
docs/DLA_UCZNIA.md      instrukcja dla ucznia: start, klawisze, ściągawka API, czytanie błędów, zasady
docs/lekcje/SZABLON_README.md  wzór README lekcji
docs/DECYZJE.md         dziennik decyzji projektowych z uzasadnieniami (wpisy 15-19: warstwa lake, rejestracja, RNG, regresja, LVGL)
.vscode/                settings (CMake Tools -> sim/), tasks (LEKCJA:/SIM:/PIO:; domyślne = LEKCJA: Uruchom), extensions;
                        c_cpp_properties i launch GENERUJE PlatformIO
```

## Toolchain - pułapki

- Oficjalna platforma `platformio/espressif32` nie wspiera P4; używamy **pioarduino** (wymaga PlatformIO Core ≥ 6.2.0;
  Core zaktualizowano z 6.1.18). `pio upgrade` uruchomione przez `pio.exe` psuje własne środowisko — aktualizować
  przez `penv\Scripts\python.exe -m pip install -U platformio`.
- Konfiguracja ESP-IDF: `sdkconfig.defaults` to źródło prawdy; wygenerowany `sdkconfig.jc4880p443c` jest w
  .gitignore. Po zmianie defaults skasować wygenerowany plik i `pio run -t clean`.
- Listy plików są wyliczane przy konfiguracji (GLOB bez CONFIGURE_DEPENDS — ESP-IDF wykonuje CMakeLists
  komponentu także w trybie skryptu). Po dodaniu pliku: `pio run -t clean` oraz `cmake -S sim --preset mingw`.
- Kompilator MinGW nie startuje bez `C:\msys64\mingw64\bin` w PATH (brak DLL); CMake raportuje to mylnie jako
  „kompilator nie jest w stanie skompilować prostego programu".
- **Przed `cmake --build sim/build` zamknąć działający emulator** — Windows nie pozwala nadpisać uruchomionego exe,
  linker kończy się błędem `collect2` bez czytelnego komunikatu.
- `espressif/esp_hosted` ≥ 3.0 nie kompiluje się pod PlatformIO na Windows (`SHELL:-include`); przy dodawaniu
  Wi-Fi przypiąć `>=2.11,<3.0`.
- VS Code: PlatformIO IDE = firmware, CMake Tools = emulator (`cmake.sourceDirectory` -> `sim/`, preset `mingw`,
  debug przez `cmake.debugConfig` z gdb MSYS2). **Nie instalować pioarduino IDE obok PlatformIO IDE.**
  PlatformIO dopisuje `pioarduino.pioarduino-ide` do `extensions.json` przy każdym buildzie — jest też na liście
  `unwantedRecommendations`, co neutralizuje podpowiedź; nie walczyć z tym ręcznie.
- **Końce linii.** Bloby w repozytorium są LF (`.gitattributes` `eol=lf`), ale `core.autocrlf=true` na tym komputerze
  wypisywał część plików do drzewa roboczego z CRLF. 23.09 drzewo robocze przepisano na LF (`sed -i 's/\r$//'` po
  `git ls-files`) — dla gita to zero różnicy. Uwaga dla narzędzi: `perl -0pi` z wzorcem `\n` nie trafia w pliki CRLF,
  a `git diff --quiet` ignoruje `--ignore-cr-at-eol` przy kodzie wyjścia (używać `--name-only`).
- Emulator w trybie `--frames` nie czeka na 60 FPS (`sim::set_unthrottled`), stąd 600 klatek w ułamku sekundy; wyniki bez zmian.
- **Nie ruszać `managed_components/` podczas `pio run`.** Menedżer komponentów ESP-IDF przy każdym buildzie sprawdza hash
  katalogu; gdy 23.09 przemianowałem `managed_components` na czas testu ścieżek LVGL emulatora, a w tle szedł `pio run`,
  komponent `lvgl__lvgl` został uznany za uszkodzony i wyczyszczony (zostały `tests/` i `zephyr/`). Naprawa: `rm -rf
  managed_components/lvgl__lvgl` i `pio run` (pobiera wg `dependencies.lock`). Testy ścieżek LVGL emulatora robić przy
  zatrzymanym PlatformIO albo przez `-DLAKE_LVGL_DIR=`.
- Skrypty PowerShell w `tools/` są pod Windows PowerShell 5.1 (bez `&&`, bez `?:`); składnię sprawdza
  `[System.Management.Automation.Language.Parser]::ParseFile`.

## LVGL - pułapki, które już kosztowały czas

- Na ESP-IDF LVGL domyślnie **ignoruje `lv_conf.h`** (`CONFIG_LV_CONF_SKIP=y`). `sdkconfig.defaults` ustawia `n`.
- `src/ui` musi być na include **wszystkich** komponentów: `idf_build_set_property(COMPILE_OPTIONS "-I.../src/ui" APPEND)`
  w głównym `CMakeLists.txt`. Flagi z `platformio.ini` nie docierają do kompilacji LVGL.
- `lv_conf.h` musi mieć strażnik **`LV_CONF_H`**, inaczej LVGL ostrzega o nieudanym include.
- `CONFIG_LV_BUILD_EXAMPLES` i `CONFIG_LV_BUILD_DEMOS` są domyślnie włączone (258 zbędnych plików) — wyłączone
  w `sdkconfig.defaults`; to opcje Kconfiga, nie `lv_conf.h`.
- Emulator bierze LVGL z `managed_components/lvgl__lvgl` (identyczna wersja co firmware), a bez niego pobiera
  tag `v9.5.0`; obie ścieżki sprawdzone. Katalog z `lv_conf.h` wskazuje `LV_BUILD_CONF_DIR`.
- LVGL w trybie **PARTIAL**: rysuje do bufora 400x80, `flush_cb` kopiuje na płótno. Tryb DIRECT z przezroczystym
  tłem **nie działa** jako nakładka na grę (LVGL zamalowuje tło na biało). Nakładka = zamrozić klatkę gry i podać
  przez `ui::set_background_frame()` jako widget canvas (robi tak pauza).
- Podczas rozgrywki `ui::tick()` nie jest wołane — gra włada całym płótnem.
- `lv_obj_clear_flag` istnieje tylko w mapie zgodności z v8 — używać `lv_obj_remove_flag`.
- Domyślna obsługa asercji LVGL to `while(1)` — emulator „wisi" bez słowa. `lv_conf.h` ustawia `LV_ASSERT_HANDLER abort();`
  (kod wyjścia + stos w gdb: `gdb -p PID -batch -ex "thread 1" -ex bt`). Tak znaleziono brak `handlers` w `lv_draw_buf_t`.
- Wbudowane czcionki Montserrat mają tylko ASCII — napisy w UI i grach bez polskich liter (rysują się jako `?`).

## Architektura

- Płótno konsoli **800x480 RGB565** (768 kB, PSRAM) w natywnej rozdzielczości logicznego ekranu — od 23.09 po południu
  (wcześniej 400x240 skalowane x2, co dawało schodki na czcionkach; decyzja użytkownika: „nowocześnie, nie retro").
  Panel jest pionowy 480x800; `display::present()` obraca przez **PPA** (skala 1) do tylnego z dwóch buforów DPI,
  `esp_lcd_panel_draw_bitmap` z adresem własnego bufora sterownika przełącza bufory bez kopiowania, czekamy na
  `on_frame_buf_complete`. `LAKE_DISPLAY_ROTATION` (90/270) steruje obrotem i mapowaniem dotyku jednocześnie.
  `display::present()` nadal obsługuje dowolną całkowitą skalę, więc powrót do 400x240 to zmiana w `engine/screen.h`.
- **Gry pixel-art** (Lake Mario, sprite'y 16x16) deklarują `engine::Game::canvas_scale() == 2`: `app.cpp` daje im pod-płótno
  `PIXEL_CANVAS_W x H` = 400x240 (192 kB, SRAM) i po `render()` powiększa x2 na płótno (`Canvas::blit_upscale2x`, CPU).
  Ślady Mario są identyczne z czasów 400x240, zrzuty to dokładnie powiększenie x2 (sprawdzone piksel w piksel).
  Menu, pauza, gry ucznia rysują natywnie w 800x480.
- **Tekst w grach:** `gfx/text.h` (`draw_text_px`, `text_width_px`, `text_height_px`) miesza glify A8 z czcionek Montserrat
  LVGL (`lv_font_get_glyph_bitmap` do własnego `lv_draw_buf_t` przez `lv_draw_buf_init` — bez tego LVGL zatrzymuje się
  na asercji). `lake::text(..., scale)` mapuje 1..4 -> 16/24/32/48 px. Czcionka 5x7 (`gfx/font.h`) zostaje dla HUD Mario
  i innych gier pixel-art. UI: `ui/theme.h` (paleta slate, czcionki, wymiary), menu z kartami 64 px, pauza z panelem.
- `lake`: `sprite(s, x, y, flip, scale)` (16x16 rysuje się x3), kafelek mapy `TILE = 32` (15 wierszy = 480 px, 25 kolumn =
  ekran), `draw_tiles` powiększa sprite do kafelka. Lekcje przeliczone na 800x480 (rozmiary i prędkości x2), testy
  zaktualizowane (np. `square_x=241`).
- Kontroler: krzyżak + gałka analogowa + A/B/X/Y + START (układ jak w padzie Switch). Płytka: przełączniki
  wprost na GPIO, odkłócanie 8 ms w zadaniu 1 kHz (`board/keypad.cpp`); gałka na ADC2 z kalibracją środka przy
  starcie, strefa martwa 25 %, próg kierunku 0,5 (`board/joystick.cpp`). Jedno wejście: `platform::controller()`
  zwraca klawisze ORAZ osie -1..1; wychylenie gałki jest już przełożone na kierunki. **SELECT nie ma pinu**
  (`pins::KEY_SELECT = GPIO_NUM_NC`, sterownik pomija NC); w menu cofa B.
- Na JP1 ADC mają **tylko GPIO49-52** — osie gałki muszą być na dwóch z nich (49 = X, 50 = Y).
- `app/app.cpp`: Menu (LVGL) -> Playing -> Paused (LVGL na zamrożonej klatce). **Pauza należy do konsoli**, gra
  nie może reagować na START (obie warstwy widziały ten sam klawisz). Ekran tytułowy reaguje na A, B albo dotyk.
- Podpowiedzi dotykowe (wirtualny pad) znikają po pierwszym użyciu klawiszy (`VirtualPad::keys_used`).
- Menu i pauza mają nawigację klawiszami przez grupę LVGL (`ui::nav_group()`, `mark_focusable`).
- `engine::Game`: init/update/render + `debug_line()` (linia stanu do `--trace`). Rejestr gier w `games/registry.cpp`:
  `GameEntry{id, name, description, create}`; `find_game()` rozumie numer, id, nazwę i ścieżkę katalogu lekcji (`NN_id`).
- **Warstwa `lake` (nauka C++ syna, 11-13 lat, po Scratchu):** uczeń pisze `setup()`/`frame()` w `namespace {}` i kończy plik
  `LAKE_GAME(id, "Nazwa", "opis")`; jedna linia `LEKCJA(id)` w `games/lekcje/lista.h` (X-makro w `registry.cpp`, jawnie —
  ESP-IDF linkuje komponent statycznie, samorejestracja by przepadła). API po angielsku (`rect`, `held(LEFT)`, `random`,
  `watch`, `load_sprite`, `load_map`/`move_box`), komentarze po polsku, matematyka całkowita przy 60 FPS. Adapter
  `lake::SimpleGame` -> `engine::Game`, więc menu, pauza, `--trace` działają bez zmian. START/SELECT dla ucznia zawsze false.
  Zmienne globalne ucznia żyją między wejściami z menu — wartości startowe nadaje `setup()`. `watch()` (max 8) rysuje panel
  w rogu i trafia do `debug_line` → testy zadań w `testy.txt` (`argumenty | regex`) czytają je z linii TRACE.
- Losowość: `engine::rng()` (xorshift32); `app::start_game` seeduje stałą przy `s_fixed_dt > 0`, zegarem w oknie. Nie `rand()`.
- Sprite'y i czcionka to ASCII-art zamieniany na bitmapy przy starcie (`gfx::make_sprite`), kolor-klucz magenta.
  Wspólna paleta 19 kolorów w `gfx/palette.h` (litery k w e E r R o y Y g G b B t s u U p P); sprite'y ucznia idą do areny 64 kB
  zerowanej przy `setup()` (`platform::alloc_pixels(fast=false)` = PSRAM).
- Struktury ESP-IDF inicjalizować przez `= {}` + przypisania pól (kolejność pól w makrach IDF bywa niezgodna z C++).
- Duże bufory jawnie w PSRAM (`platform::alloc_pixels(..., fast=false)`), wyrównane do 128 B (PPA/cache).
  Płótno 192 kB próbuje najpierw SRAM. Bez wyjątków C++: nieudany `new` = abort.

## Weryfikacja gier w emulatorze (bez człowieka)

`lake_sim.exe --game N --hold KLAWISZ OD DO ... --frames M --trace K` (do 8 wpisów `--hold`; także `--pause-at`,
`--shot plik.bmp`, `--keymap`). W trybie `--frames` krok czasu jest stały (`app::set_fixed_dt(1/60)`), więc ten sam
skrypt daje **identyczny** ślad. Na ekranie tytułowym najpierw wcisnąć B (`--hold B 3 4`), bo tytuł nie reaguje na START.

Zweryfikowane 23.09.2026 (Lake Mario): podłoże stabilne co klatkę; chód 100 px/s; bieg 165 px/s; ściana x=0;
skok: tap 28 px, 3 klatki 40 px, pełny 52 px, bufor 120 ms działa, poza oknem nie; uderzenie `?` (+200, moneta)
i rozbicie `B` (+50); przeskok nad przeciwnikiem; zadeptanie (+100, odbicie, przeciwnik znika); śmierć od
przeciwnika -> respawn z lives-1; 4 śmierci -> GAME OVER -> nowa gra; monety na platformie; pauza konsoli
i powrót (czas dalej liczy); START na tytule nie startuje gry. **Zmieniając fizykę, powtórz te scenariusze.**

**Siatka regresji (od 23.09 po południu):** `tests/scenarios.txt` = 6 śladów (chód, skok tap/pełny, bieg, 600 klatek,
pauza) + 3 zrzuty BMP (tytuł, gra, menu), wzorce w `tests/expected/` nagrane z binarki **sprzed** refaktoru silnika
(math2d, palette, ParticlePool, TileMap). `tools/testy.ps1 mario` musi dać 9/9 po każdej zmianie w `src/engine`, `src/gfx`,
`src/games/mario`. Zrzut `menu` zmienia się po dodaniu lekcji — wtedy `-Update`. Testy lekcji: kod startowy w `gra.cpp`
**ma padać**, `rozwiazania/zadN.cpp.txt` skopiowane do `gra.cpp` **ma przechodzić** (sprawdzone dla 01-12).

## Lake Mario - fizyka i poziom

- Podłoże: sprawdzać kafelek pod **dolną krawędzią** (`y + h`), nie ostatni piksel hitboxa. Wersja `y + h - 1`
  dawała migotanie sprite'a (stanie/skok na przemian co 2-3 klatki).
- `GRAVITY 1100`, `JUMP_V -350` / `-390` z biegu, ucięcie po puszczeniu do `-240` (min. 28 px), `JUMP_BUFFER 0.12 s`,
  `COYOTE 0.10 s`, chód 100, bieg 165 px/s. Wysokości: tap 1,8 kafelka, pełny 3,3, z biegu 4,3; dal 4 / 7 kafelków.
- Poziom 1 do przejścia **skokiem z chodu**: ściany i rury na trasie max 3 kafelki, przepaście max 3, platformy
  max 3 kafelki nad powierzchnią startu, bloki `?` w wierszu ≥ 9 (gracz stoi w wierszu 12). Bieg tylko do bonusów.
- Hitboxy: gracz 12x16 (sprite 16, offset x 2), przeciwnik 14x13 (offset x 1, y 3). Kafelek 16 px, 25x15 widocznych.

## Sprzęt - fakty krytyczne (szczegóły w docs/HARDWARE.md)

- `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` + `CONFIG_ESP32P4_REV_MIN_0=y` — P4 rev 1.x; zły wybór = crash w bootloaderze.
- Taktowanie **360 MHz** (400 MHz tylko rev ≥ 3.0 albo chipy kwalifikowane przez Espressif).
- LDO kanał 3 (2,5 V) zasila DSI-PHY — włączyć przed `esp_lcd_new_dsi_bus`. Kanał 4 (3,3 V) zasila kartę SD.
- Podświetlenie GPIO23 = zwykły GPIO, stan wysoki. Sterownik ST7701 producenta w `src/board/st7701/` (komponent
  z rejestru daje czarny ekran). Timingi: 2 linie DSI 500 Mbps, DPI 34 MHz, 480x800, h 12/42/42, v 2/8/166.
- GT911: I2C0 SDA 7 / SCL 8, adres 0x5D lub 0x14, RST/INT nieużywane. Magistrala wspólna z kodekiem ES8311 (0x18).
- JP1 (2x13): wolne GPIO 52, 51, 50, 49, 34, 33, 32, 31, 30, 29, 28 (+35 = BOOT, nie używać jako klawisza gry:
  przytrzymany przy starcie = tryb programowania; służy jako zapasowy START). Pełna tabela pinów w HARDWARE.md.
- Wi-Fi/BT tylko przez ESP32-C6 (ESP-Hosted, SDIO slot 1). Fabryczne firmware C6 (2.3.0) nie współpracuje
  z hostem 2.12 — trzeba je wgrać przez UART na JP1. Nieużywane w grze.

## Otwarte decyzje użytkownika (nie podejmować za niego)

1. **Gałka analogowa czy SELECT.** Gałka zjada dokładnie 2 piny, których brakuje na pełny pad. Bez gałki: SELECT + 1 pin
   wolny. Rekomendacja z 23.09: zbudować najpierw bez gałki, zostawić dla niej miejsce w obudowie; kod obsługuje oba
   warianty (`pins.h`). Użytkownik nie zdecydował.
2. **Obudowa.** Przełączniki MX (raster 19 mm) dają krzyżak 57x57 mm — dwa bloki nie mieszczą się pod ekranem
   (płytka ~110 mm). Opcje: osobny pad na kablu (rekomendacja), format Game Boya (~130x150 mm, przełączniki
   niskoprofilowe), format Switcha (~230 mm). Płytkę zmierzyć po przyjściu.

## Następne kroki

1. **Komputer syna:** `tools/setup_kid_pc.ps1` przetestować na czystej maszynie (VM / konto bez narzędzi) — składnia
   sprawdzona, przebieg nie. Utworzyć gałąź `lekcje` przed klonowaniem u syna. Prawdziwy test: lekcja 01 bez pomocy.
2. Płytka: uruchomienie wg listy w `docs/HARDWARE.md` (koniec ramki DPI co klatkę, kierunek obrotu vs USB,
   mapowanie dotyku, rewizja krzemu, kalibracja gałki, odkłócanie klawiszy). Lekcje pojawią się w menu konsoli.
3. Testy skryptowe brakujących mechanik Mario: meta `F` (LevelClear), przepaść, koniec czasu (dopisać do `tests/scenarios.txt`).
4. Zrzuty lekcji do README (`LEKCJA: Zrzut ekranu` + `tools/bmp2png.ps1`), ewentualnie `docs/images/lekcje/`.
5. Dźwięk: ES8311 przez I2S (`espressif/esp_codec_dev`, adres 8-bitowy 0x30, I2S stereo slot mimo mono, jedna instancja IN_OUT);
   dla ucznia `lake::beep()`.
6. Assety z partycji SPIFFS (narzędzie PNG -> RGB565) zamiast ASCII-artu.
7. Ekran ustawień, wyniki w NVS (rekordy lekcji), ekran „o konsoli".
8. IntelliSense dla plików `sim/` (dziś pokazuje błąd na `<windows.h>` — tylko kosmetyka).
9. Gamepad BLE przez C6 (drugi gracz), ekspander I2C PCF8574 na JP1 pin 23/25, jeśli zabraknie klawiszy.

## Zasady współpracy

- Użytkownik pisze po polsku; odpowiadać po polsku, konkretnie, z liczbami i tabelami, bez lania wody.
- Gdy pyta „co myślisz?" — najpierw ocena z uzasadnieniem i rekomendacją, implementacja dopiero po decyzji.
- Weryfikować pomiarem, nie na oko: `--trace`, zrzuty `--shot`, porównania śladów. Mówić wprost, co NIE jest przetestowane.
- Commit/push tylko na wyraźne polecenie. Nie robić `git init` bez commita.
- Przed każdym `cmake --build sim/build` zamknąć emulator; po zakończeniu pracy zostawić emulator uruchomiony,
  jeśli użytkownik o to prosił.
- Lekcje: kod startowy ma być grywalny od pierwszej sekundy i **celowo** pozbawiony jednej mechaniki, którą uczeń dopisuje;
  jedna nowa koncepcja na lekcję; README wg `docs/lekcje/SZABLON_README.md`; test w `testy.txt` czyta `watch()`;
  rozwiązanie w `rozwiazania/*.cpp.txt` zbudować raz (podmiana `gra.cpp`) i przywrócić kod startowy. Nazwy zmiennych ucznia
  nie mogą kolidować z libc (`time`, `random`, `abs` — `abs`/`min`/`max` są w `lake`, unikać `time`).
