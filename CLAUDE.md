# CLAUDE.md - LakeMarioGame

Mini konsola do gier na **ESP32-P4** (płytka Guition JC4880P443C_I_W_Y: ekran 4,3" 480x800 MIPI-DSI,
dotyk GT911, ESP32-C6 jako radio) plus **emulator na Windows**, który uruchamia ten sam kod.
Pierwsza gra: platformówka „Lake Mario". UI konsoli w LVGL 9.5, gry na własnym rendererze 2D.

Kod i komentarze po polsku, **BEZ znaków diakrytycznych w plikach źródłowych** (kodowanie w toolchainie);
dokumentacja w `docs/`, README i ten plik z polskimi znakami.

## Stan na 2026-09-23

- **Płytka jeszcze nie dotarła** (zamówiona 22.09.2026, dostawa ok. 27.09-01.10). Cały kod powstał bez
  sprzętu. Po przyjściu płytki zacząć od listy „Do zweryfikowania na sprzęcie" w `docs/HARDWARE.md`.
- Firmware kompiluje się: 809 kB flash (19 % z 4 MB), 27,2 kB RAM statycznie. Emulator kompiluje się i działa.
- Wszystkie mechaniki Lake Mario zweryfikowane skryptami w emulatorze (lista niżej). Bez testu skryptowego,
  tylko przegląd kodu: meta `F`, śmierć w przepaści, koniec czasu.
- Repozytorium: **https://github.com/grekot/ESP32_p4_game_console.git**, gałąź `main`, pierwszy commit 23.09.2026.
  Commit i push tylko na wyraźne polecenie użytkownika. `.gitattributes` wymusza LF w repozytorium.
  Uwaga historyczna: repozytorium bez żadnego commita wywala build ESP-IDF (woła `git describe`).
- Około 6800 linii własnego kodu (bez sterownika ST7701 producenta).

## Komendy

```bash
pio run                              # firmware (pierwszy raz ~15 min: pobiera ESP-IDF i toolchain)
pio run -t upload -t monitor         # wgranie + logi 115200
cmake -S sim --preset mingw          # konfiguracja emulatora (raz i po dodaniu plików)
cmake --build sim/build              # emulator (kilka sekund) - ZAMKNIJ dzialajacy lake_sim.exe!
./sim/build/lake_sim.exe             # emulator okienkowy
./sim/build/lake_sim.exe --game 0 --hold B 3 4 --hold RIGHT 20 90 --frames 90 --trace 15   # test skryptowany
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
src/gfx/                Canvas RGB565 (blit z kolorem-kluczem), Sprite z ASCII-artu, czcionka 5x7
src/input/              keys.h (14 klawiszy), pad.h (PadState + osie), virtual_pad (dotyk + klawisze -> PadState, zbocza)
src/engine/             Game (init/update/render/debug_line), screen.h (400x240), stats, game_registry
src/ui/                 lv_conf.h (WSPÓLNY), lvgl_glue (PARTIAL, flush do płótna, indev dotyk+klawisze, grupa), menu, pause
src/app/                maszyna stanów konsoli Menu -> Playing -> Paused; set_fixed_dt; debug_line
src/games/registry.cpp  lista gier zasilająca menu
src/games/mario/        assets (sprite'y ASCII), level (6 segmentów 25x15), game (fizyka, kolizje, HUD)
sim/                    emulator: CMakeLists (LVGL z managed_components albo FetchContent v9.5.0), CMakePresets,
                        platform_win32, keymap_win32 + keymap.cfg, gamepad_win32 (winmm), main (opcje CLI)
docs/HARDWARE.md        płytka, pinout, JP1, kontroler, kalibracja gałki, lista do sprawdzenia na sprzęcie, źródła
docs/EMULATOR.md        budowanie, sterowanie, keymap, tryby CLI, testy skryptowane
docs/VSCODE.md          rozszerzenia, zadania, debug, na co odpowiadać przy pierwszym otwarciu
docs/DECYZJE.md         dziennik decyzji projektowych z uzasadnieniami
.vscode/                settings (CMake Tools -> sim/), tasks (PIO:/SIM:), extensions; c_cpp_properties i launch GENERUJE PlatformIO
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

## Architektura

- Płótno gry **400x240 RGB565**, skalowane x2 do 800x480. Panel jest pionowy 480x800; `display::present()`
  skaluje i obraca przez **PPA** do tylnego z dwóch buforów DPI, `esp_lcd_panel_draw_bitmap` z adresem własnego
  bufora sterownika przełącza bufory bez kopiowania, czekamy na `on_frame_buf_complete`. `LAKE_DISPLAY_ROTATION`
  (90/270) steruje obrotem i mapowaniem dotyku jednocześnie.
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
- `engine::Game`: init/update/render + `debug_line()` (linia stanu do `--trace`). Rejestr gier w `games/registry.cpp`.
- Sprite'y i czcionka to ASCII-art zamieniany na bitmapy przy starcie (`gfx::make_sprite`), kolor-klucz magenta.
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

1. Płytka: uruchomienie wg listy w `docs/HARDWARE.md` (koniec ramki DPI co klatkę, kierunek obrotu vs USB,
   mapowanie dotyku, rewizja krzemu, kalibracja gałki, odkłócanie klawiszy).
2. Testy skryptowe brakujących mechanik: meta `F` (LevelClear), przepaść, koniec czasu.
3. Dźwięk: ES8311 przez I2S (`espressif/esp_codec_dev`, adres 8-bitowy 0x30, I2S stereo slot mimo mono, jedna instancja IN_OUT).
4. Assety z partycji SPIFFS (narzędzie PNG -> RGB565) zamiast ASCII-artu.
5. Ekran ustawień, wyniki w NVS, ekran „o konsoli".
6. IntelliSense dla plików `sim/` (dziś pokazuje błąd na `<windows.h>` — tylko kosmetyka).
7. Gamepad BLE przez C6 (drugi gracz), ekspander I2C PCF8574 na JP1 pin 23/25, jeśli zabraknie klawiszy.

## Zasady współpracy

- Użytkownik pisze po polsku; odpowiadać po polsku, konkretnie, z liczbami i tabelami, bez lania wody.
- Gdy pyta „co myślisz?" — najpierw ocena z uzasadnieniem i rekomendacją, implementacja dopiero po decyzji.
- Weryfikować pomiarem, nie na oko: `--trace`, zrzuty `--shot`, porównania śladów. Mówić wprost, co NIE jest przetestowane.
- Commit/push tylko na wyraźne polecenie. Nie robić `git init` bez commita.
- Przed każdym `cmake --build sim/build` zamknąć emulator; po zakończeniu pracy zostawić emulator uruchomiony,
  jeśli użytkownik o to prosił.
