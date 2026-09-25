# Dziennik decyzji projektowych

Każdy wpis: kontekst, decyzja, konsekwencje. Kolejność chronologiczna (22-24.09.2026).

## 1. ESP-IDF przez PlatformIO (pioarduino), nie Arduino ani gołe ESP-IDF

**Kontekst.** Płytka Guition JC4880P443C: ESP32-P4 z ekranem MIPI-DSI, PPA, PSRAM HEX. Użytkownik miał
zainstalowane PlatformIO. Oficjalna platforma `platformio/espressif32` nie wspiera P4.
**Decyzja.** Fork pioarduino `55.03.312` (ESP-IDF 5.5.5), `framework = espidf`. Arduino odrzucone: nakładka bez
wygodnego dostępu do DSI/PPA/DMA2D. Gołe ESP-IDF odrzucone: PlatformIO daje jedno polecenie, IntelliSense
i izolację środowiska.
**Konsekwencje.** Wymagało aktualizacji PlatformIO Core do 6.2.0. Sterownik ST7701 wzięty z pakietu producenta,
bo komponent z rejestru daje czarny ekran.

## 2. Własny renderer 2D dla gier, LVGL tylko dla UI

**Kontekst.** Platformówka rysuje całą klatkę 60 razy na sekundę; LVGL jest biblioteką widgetów.
**Decyzja.** Gry rysują na płótno RGB565 własnym blitem (kafelki, sprite'y, czcionka). LVGL obsługuje menu
konsoli i ekran pauzy. Użytkownik początkowo chciał LVGL „do UI gier" — spełnione mechanizmem zamrożonej klatki.
**Konsekwencje.** LVGL nie jest wołane podczas rozgrywki (zero kosztu). UI w grze = zamrozić klatkę, podać jako tło
widgetu canvas, zbudować widgety na wierzchu. Próba nakładki z przezroczystym tłem (tryb DIRECT) nie działa.

## 3. Płótno 400x240 skalowane sprzętowo x2, nie 800x480

**Kontekst.** Panel 480x800 pionowy; gra pozioma. ESP32-P4 ma PPA (skalowanie + obrót w jednym przebiegu).
**Decyzja.** Gra rysuje 400x240, PPA skaluje x2 i obraca do bufora DPI. Cztery razy mniej pikseli dla CPU,
klasyczny wygląd pixel-art, 16x16 kafelków daje 25x15 widocznych.
**Konsekwencje.** Emulator powiększa najbliższym sąsiadem, więc wygląda identycznie. Menu LVGL też jest w 400x240
(czcionki 12-20 px, skalowane x2 — czytelne, „konsolowe").

## 4. Warstwa `platform::` i emulator na Windows

**Kontekst.** Płytka w drodze; użytkownik chciał pracować nad grami od razu i mieć emulator działający
„dokładnie jak płytka".
**Decyzja.** Cały kod nad cienką warstwą `platform::` (obraz, wejście, czas, pamięć) jest wspólny. Dwie
implementacje: ESP i Win32 GDI. Bez SDL — zero zależności, exe linkowany statycznie.
**Konsekwencje.** Ta sama logika, kolizje, UI. Emulator nie sprawdza wydajności ani sprzętu. Zrzuty do
dokumentacji generuje `--shot`.

## 5. Wspólny `lv_conf.h` dla płytki i emulatora

**Kontekst.** Dwa cele, jeden interfejs — konfiguracja LVGL nie może się rozjechać.
**Decyzja.** Jeden plik `src/ui/lv_conf.h`, minimalny (LVGL ma 542 wartości domyślne). Na ESP: `CONFIG_LV_CONF_SKIP=n`
plus include dodany globalnie w `CMakeLists.txt`. W emulatorze: `LV_BUILD_CONF_DIR`. Wersja przypięta `~9.5.0`;
emulator używa kopii z `managed_components`, a bez niej pobiera ten sam tag.
**Konsekwencje.** Kosztowało trzy nieoczywiste poprawki (Kconfig SKIP, ścieżka include, strażnik `LV_CONF_H`),
opisane w CLAUDE.md.

## 6. Kontroler: przełączniki wprost na GPIO, bez matrycy i diod

**Kontekst.** Użytkownik chciał klawiaturę mechaniczną. Na JP1 wyprowadzonych jest 12 GPIO (11 bez BOOT),
odczytane ze schematu producenta; ESP32-P4 ma osobne wyprowadzenia pamięci, więc żaden nie jest zajęty.
**Decyzja.** Podłączenie bezpośrednie: brak maskowania klawiszy (prawo + bieg + skok naraz działa), brak diod,
odkłócanie programowe 8 ms w zadaniu 1 kHz. Matryca odrzucona (wymaga diody na klawisz dla tego samego efektu).
BOOT (GPIO35) odrzucony jako klawisz gry (przytrzymany przy starcie = tryb programowania).
**Konsekwencje.** Budżet pinów jest ciasny: 11 na 12 potrzebnych w układzie z gałką.

## 7. Układ jak w padzie Switch: krzyżak + gałka + A/B/X/Y + START, bez SELECT

**Kontekst.** Użytkownik ma moduł joysticka analogowego i zaproponował układ Switcha. Gałka musi być na
GPIO49-52 (jedyne z ADC na JP1). Pełny układ potrzebuje 12 pinów, jest 11.
**Decyzja.** Odpadł SELECT (w menu cofa B). Kod obsługuje SELECT (`GPIO_NUM_NC`, sterownik pomija). Gałka
dubluje krzyżak po progowaniu i wystawia surowe osie dla przyszłych gier. Moduł zasilany z 3,3 V, kalibracja
środka przy starcie (tanie moduły mają rozrzut), strefa martwa 25 %.
**Konsekwencje.** Otwarte pytanie: gałka jest dziś przerostem formy (platformówka jej nie potrzebuje) i zjada
dokładnie 2 brakujące piny. Rekomendacja: budować bez gałki, zostawić miejsce. Użytkownik nie zdecydował.
Alternatywa na więcej klawiszy: ekspander I2C PCF8574 na JP1 (piny 23/25), zero GPIO.

## 8. Emulator: mapowanie klawiszy w pliku, pad USB przez winmm

**Kontekst.** Użytkownik chciał sam definiować, który klawisz PC to który klawisz konsoli.
**Decyzja.** `sim/keymap.cfg` (kopiowany obok exe), wypisywany przy starcie. Gałkę emuluje lewa gałka pada USB
(winmm, wbudowane w Windows), a bez pada klawisze `STICK_*`. Zastrzyk wciśnięć z CLI (`--hold`) do testów.
**Konsekwencje.** Stan klawiszy czytany na początku klatki z zatrzaskiem naciśnięć (wcześniej: klatka
opóźnienia i gubione krótkie tapnięcia — użytkownik zgłosił „nieresponsywny skok").

## 9. Skok: ucięcie do -240, bufor 120 ms, czas kojota 100 ms

**Kontekst.** Zgłoszenie: skok nieresponsywny, czasem brak reakcji. Pomiar: tapnięcie dawało 9 px (ucięcie do -140),
wciśnięcie tuż przed lądowaniem przepadało.
**Decyzja.** Standardowe ułatwienia z platformówek; wartości zmierzone: tap 28 px, pełny 52 px; bufor działa
4 klatki przed lądowaniem, nie działa 15 klatek przed.
**Konsekwencje.** Każda zmiana fizyki powinna przejść te same skrypty (`--hold`/`--trace`).

## 10. Podłoże sprawdzane pod dolną krawędzią (`y + h`)

**Kontekst.** Zgłoszenie: Mario „migocze". Diagnoza: `y + h - 1` po przyciągnięciu do kafelka trafiało w pusty
wiersz nad ziemią; stan „na ziemi" przełączał się co 2-3 klatki, sprite stania/skoku na przemian.
**Decyzja.** `y + h`. Test: 53 kolejne klatki `ground=1`.
**Konsekwencje.** Ten sam błąd powtarzał się u przeciwników (podskakiwanie o 1 px) — też naprawiony.

## 11. Pauza należy do konsoli, nie do gry

**Kontekst.** Gra i konsola reagowały na START; po powrocie z menu pauzy gra zostawała we własnej pauzie.
Dodatkowo START na tytule uruchamiał grę i od razu ją pauzował.
**Decyzja.** Gra nie zna START. Tytuł reaguje na A, B albo dotyk.
**Konsekwencje.** Każda kolejna gra dostaje pauzę „za darmo" i nie może jej zepsuć.

## 12. Poziom 1 przechodzony skokiem z chodu

**Kontekst.** Przeliczenie fizyki wykazało rurę 5 kafelków (max skok 4,3), przepaść 6 kafelków i bloki
niedostępne z ziemi.
**Decyzja.** Zasady zapisane w `mario_level.cpp`: ściany/rury ≤ 3, przepaście ≤ 3, platformy ≤ 3 nad startem,
`?` w wierszu ≥ 9. Bieg tylko do bonusów.
**Konsekwencje.** Poziom przeprojektowany; monety i bloki zweryfikowane skryptem.

## 13. Stały krok czasu w trybie skryptowanym

**Kontekst.** Dwa identyczne skrypty dały różne wyniki (czas rzeczywisty, obciążenie PC).
**Decyzja.** `--frames` włącza `app::set_fixed_dt(1/60)`. Tryb okienkowy nadal liczy czas rzeczywisty, jak płytka.
**Konsekwencje.** Ślady są porównywalne bajt w bajt — można je trzymać jako testy regresji.

## 14. VS Code: PlatformIO dla firmware, CMake Tools dla emulatora

**Kontekst.** Użytkownik otworzył projekt i nie wiedział, na co odpowiadać. CMake Tools próbowało konfigurować
główny `CMakeLists.txt` ESP-IDF.
**Decyzja.** `cmake.sourceDirectory` -> `sim/`, preset `mingw`, debug emulatora przez `cmake.debugConfig`
(bez `launch.json`, który generuje PlatformIO). pioarduino IDE na liście niechcianych.
**Konsekwencje.** IntelliSense pochodzi z PlatformIO, więc pliki `sim/` podkreślają `<windows.h>` — kosmetyka.

## 15. Warstwa `console` dla ucznia zamiast uproszczania `engine::Game` (23.09.2026)

**Kontekst.** Konsola ma być platformą do nauki C++ dla 11–13-latka po Scratchu. `engine::Game` wymaga klasy,
`override`, referencji, `float dt` i RGB565 — za dużo na pierwszą lekcję.
**Decyzja.** Osobny katalog `src/console/`: funkcje globalne w stylu Arduino/Processing (`setup()`/`frame()`, `rect`, `held`,
`random`, `watch`), silny typ `Color`, matematyka całkowita przy stałych 60 FPS. Adapter `console::SimpleGame` opakowuje
funkcje ucznia w `engine::Game`, więc pętla konsoli, menu, pauza i `--trace` działają bez zmian. Nazwy API po angielsku
(jak w każdym kursie), komentarze po polsku. `frame()` zamiast `loop()` (dzieci piszą wtedy `while(true)`) i zamiast
`draw()` (w tej funkcji też się porusza obiektami).
**Konsekwencje.** `engine::Game` zostaje pełnym interfejsem dla „dorosłych" gier. Zmienne globalne ucznia żyją między
wejściami z menu — wartości startowe nadaje `setup()`. Sprite'y ucznia idą do areny 64 kB zerowanej przy `setup()`,
żeby `load_sprite` nie wyciekało.

## 16. Rejestracja lekcji: X-makro `lista.h`, nie samorejestracja

**Kontekst.** Uczeń ma dodać grę jedną linią. Kuszące jest `static` z konstruktorem rejestrującym.
**Decyzja.** `CONSOLE_ADD_GAME(id, ...)` definiuje `extern const engine::GameEntry console_entry_id`, a `registry.cpp` włącza
`lista.h` dwa razy (deklaracje i elementy `GAMES[]`). Jawna lista, bo ESP-IDF linkuje komponent jako bibliotekę
statyczną: obiekt bez odwołań wypada z programu razem ze swoim inicjalizatorem. Do tego `GameEntry::id` i `find_game()`:
`--game pilka`, `--game src/games/lekcje/02_pilka` (zadanie VS Code z `${relativeFileDirname}`).
**Konsekwencje.** Kod ucznia siedzi w `namespace {}` — wszystkie lekcje trafiają do jednego binarium (także firmware).
Brak klamry = czytelny błąd linkera `multiple definition of 'setup()'`, opisany w DLA_UCZNIA.md.

## 17. Własny RNG (xorshift32) seedowany przez konsolę

**Kontekst.** Lekcje potrzebują losowości, a testy `--frames` muszą być powtarzalne.
**Decyzja.** `engine::rng()`; `app::start_game` ustawia ziarno stałe przy stałym `dt` (tryb testowy) i z zegara w oknie.
Nie `rand()` — różne implementacje na PC i ESP.
**Konsekwencje.** Mario nie losuje, więc jego ślady bez zmian. `console::random(int,int)` nie koliduje z `random(void)`
z newlib (inna arność) — sprawdzone na `pio run`.

## 18. Siatka regresji przed refaktorem: ślady i zrzuty bajt w bajt

**Kontekst.** Plan wyciągnięcia `TileMap`, cząsteczek i palety z `mario_game.cpp` do silnika.
**Decyzja.** `tests/scenarios.txt` + `tests/expected/` nagrane z binarki sprzed zmian; `tools/testy.ps1 mario` porównuje
dokładnie. Tryb `--frames` przestał czekać na 60 FPS (`sim::set_unthrottled`), więc 9 scenariuszy liczy się w 3,5 s.
**Konsekwencje.** Każdy krok refaktoru musi dać 9/9. Zrzut `menu` zmienia się z każdą lekcją — wtedy `-Update` jest oczekiwane.

## 19. Komputer ucznia bez PlatformIO: trzy źródła LVGL

**Kontekst.** Emulator brał LVGL z `managed_components` (tylko po `pio run`) albo klonował 180 MB gitem.
**Decyzja.** Kolejność: `managed_components` → `third_party/lvgl` (`tools/fetch_lvgl.ps1`: zip taga, SHA-256, ~30 MB
potrzebnych plików, `-Zip` z pendrive'a) → `FetchContent` z `URL` zipa (bez gita). `tools/setup_kid_pc.ps1` stawia
całe środowisko przez winget i przypisuje F6.
**Konsekwencje.** Na komputerze ucznia IntelliSense pochodzi z CMake Tools (odpowiedź „Yes" na pytanie o providera —
odwrotnie niż u rodzica). Domyślne zadanie Ctrl+Shift+B to `LEKCJA: Uruchom`.

## 20. Płótno 800x480 zamiast 400x240 x2; pixel-art tylko tam, gdzie gra o to prosi (23.09.2026)

**Kontekst.** Użytkownik ocenił menu LVGL jako brzydkie: litery ze schodkami. Przyczyną było powiększanie x2 najbliższym
sąsiadem całej klatki 400x240 (PPA / `StretchBlt`), które niszczyło antyaliasing czcionek. Rekomendacja brzmiała:
UI w 800x480, gry zostawić w 400x240; użytkownik wybrał **wszystko w 800x480**, „nowocześnie, nie retro".
**Decyzja.** `engine::CANVAS_W/H = 800x480`, skala 1 (PPA robi sam obrót). Gra może zadeklarować
`Game::canvas_scale() == 2` — dostaje pod-płótno 400x240 w SRAM, a konsola powiększa klatkę x2 (`blit_upscale2x`);
tak działa Lake Mario, którego grafiki 16x16 są pixel-artem z założenia. Tekst w grach ucznia i etykiety wirtualnego pada
rysuje `gfx/text.h` z glifów Montserrat LVGL (A8 mieszane z RGB565). Menu i pauza przeprojektowane (`ui/theme.h`: paleta
slate, karty 64 px, pasek przewijania, panel z cieniem). W `console`: `sprite(..., scale)`, kafelek 32 px; wszystkie lekcje
przeliczone (rozmiary, prędkości, testy).
**Konsekwencje.** Płótno 768 kB w PSRAM (było 192 kB w SRAM) — na płytce do zmierzenia czas klatki gier 800x480
(rysowanie CPU do PSRAM) i koszt powiększenia x2 dla Mario (~1 ms szacunkowo; w razie potrzeby PPA). Ślady Mario bez zmian,
zrzuty regresji to dokładne powiększenie x2 starych. Emulator 1:1, zrzuty 1,15 MB. Czcionka 5x7 zostaje tylko w HUD Mario.
Po drodze: `LV_ASSERT_HANDLER abort()` w `lv_conf.h`, bo domyślne `while(1)` LVGL zawiesiło emulator bez komunikatu.

## 21. Nazwa konsoli: „Console", nie „Lake" (23.09.2026)

**Kontekst.** Nazwa „Lake" pochodziła z nazwy projektu `LakeMarioGame` i przeszła bez ustalenia na menu („Lake Console"),
emulator (`lake_sim.exe`), makra (`LAKE_*`) oraz — w tej sesji — na warstwę ucznia (`lake::`, `LAKE_GAME`). Użytkownik
uznał ją za mylącą markę i nie chce jej ani w interfejsie, ani w kodzie ucznia.
**Decyzja.** Podstawowa nazwa to **Console**: menu „Console", `console_sim.exe`, `src/console/` z `console.h`,
przestrzeń `console::`, makro `CONSOLE_ADD_GAME`, `CONSOLE_LOG*`, `CONSOLE_HOST_BUILD`, `CONSOLE_DISPLAY_ROTATION`, klasa okna
`ConsoleSim`, projekt CMake `Console`. Bez zmian: gra „Lake Mario", katalog `LakeMarioGame` i adres repozytorium
(decyzja użytkownika), ziarno RNG `0x4C414B45` (zmiana zmieniłaby wyniki testów lekcji).
**Konsekwencje.** Mechaniczna zamiana w ~60 plikach, regresja Mario bez zmian, wzorzec zrzutu menu nagrany na nowo (tytuł).
Nazwy w kodzie, dokumentach i interfejsie ustalać z użytkownikiem przed użyciem — zwłaszcza te, które zobaczy uczeń.
Na prośbę użytkownika makro rejestrujące dostało czasownik: `CONSOLE_ADD_GAME` („konsola, dodaj grę"), nie `CONSOLE_GAME`.

## 22. Obrazki PNG dekodowane w grze z partycji SPIFFS (23.09.2026)

**Kontekst.** Użytkownik chce grafiki z plików PNG (Piskel/Paint) zamiast ASCII-artu. Dwie drogi: A — konwersja PNG na
tablice C przy budowaniu (bez systemu plików, bez ryzyka na sprzęcie), B — dekodowanie w trakcie gry z partycji plików.
Rekomendacja brzmiała A; użytkownik wybrał **B**.
**Decyzja.** `platform::read_file(path)`: emulator czyta `assets/<path>` (od katalogu roboczego, potem od exe), płytka czyta
`/assets/<path>` z partycji `assets` (SPIFFS, 0xBF0000 w `partitions.csv`), montowanej przy starcie z formatowaniem przy
pierwszym uruchomieniu. Dekoder lodepng (`src/gfx/lodepng/`, licencja zlib, kompilowany bez enkodera i dysku).
`gfx::load_png` → RGB565 z kolorem-kluczem (alfa < 128 = przezroczyste). W API ucznia `load_image("plik.png")` szuka
w `assets/<id gry>/` — id z `CONSOLE_ADD_GAME` trafia do adaptera `SimpleGame`. Arena obrazków 64 kB → 256 kB (PSRAM).
Wgrywanie: `data_dir = assets` w `platformio.ini`, `pio run -t uploadfs`. `CONFIG_SPIFFS_OBJ_NAME_LEN=64`.
**Konsekwencje.** Na PC działa od razu (plakat API rysuje PNG, scenariusz `api_demo` w regresji). Na płytce niesprawdzone:
montowanie/formatowanie SPIFFS, `uploadfs` z pioarduino, czas dekodowania. Firmware większy o dekoder i komponent spiffs.
Półprzezroczystość jest tracona (RGB565 bez alfy) — krawędzie w PNG rysować twardo.

## 23. „Gra 3D": raycasting zamiast wielokątów (23.09.2026)

**Kontekst.** Użytkownik poprosił o przykład gry z ładną grafiką i o ocenę, czy da się zrobić grę 3D. ESP32-P4 nie ma GPU;
ma 2 rdzenie RISC-V 360 MHz, FPU pojedynczej precyzji, PPA (skalowanie/obrót) i 2D-DMA. Pełne 3D z wielokątami
(transformacje, Z-bufor, teksturowanie perspektywiczne) dla 800x480 przy 60 FPS to setki milionów operacji na sekundę —
poza zasięgiem; nawet 400x240 z kilkuset trójkątami wypadłoby poniżej 20 FPS i zjadłoby cały projekt.
**Decyzja.** Pseudo-3D metodą raycastingu (Wolfenstein 3D): jeden promień na kolumnę ekranu po mapie kafelków z liter,
ściany jako pionowe paski tekstury, przedmioty jako sprite'y skalowane odległością z buforem głębi na kolumnę.
Gra „Labirynt 3D" (`src/games/labirynt3d/`) na płótnie 400x240 (`canvas_scale()==2`): 400 promieni na klatkę.
Drugi przykład, „Kosmos" (`src/games/kosmos/`), pokazuje pełne 800x480 z PNG, paralaksą i wybuchami klatkowymi.
Grafika obu gier to PNG generowane skryptem `tools/gen_demo_assets.py` (czysty Python, deterministyczne) — łatwe
do podmienienia na własne rysunki.
**Konsekwencje.** Na PC raycasting kosztuje ok. 0,1 ms ponad tło (`--bench`), szacunek na P4: 2-4 ms z 16,7 ms.
Mapa labiryntu ma 32x24 kafelki — więcej niż `engine::TileMap::MAX_ROWS` (16), więc gra trzyma własną tablicę;
podniesienie limitu w `TileMap` zwiększyłoby RAM każdej instancji (Mario, runtime ucznia). Drzwi otwierają się
(znikają) przy podejściu — najprostsza wersja, bez animacji. Sprawdzone BFS-em: wyjście i wszystkie 16 monet są
osiągalne. Alternatywy na przyszłość, jeśli syn zechce „więcej 3D": pseudo-3D droga (Outrun), izometria, siatka
wireframe.

## 24. Ziarno losowości ustawiane przed startem gry w trybie `--frames` (23.09.2026)

**Kontekst.** Kosmos losuje asteroidy; dwa identyczne uruchomienia `--frames` dawały różne ślady. Przyczyna:
`sim/main.cpp` wołało `app::start_game_by_index()` przed `app::set_fixed_dt()`, a to `start_game()` wybiera ziarno
(stałe przy stałym kroku, z zegara w oknie) — ziarno szło z zegara. Lekcje z `random()` przechodziły testy tylko dlatego,
że sprawdzają regexem zakresy, nie dokładne wartości.
**Decyzja.** `set_fixed_dt` przed startem gry. Dodatkowo `--bench` (średni/maksymalny czas `app::frame()`) i limit
`--hold` podniesiony z 8 do 12 (trasa w labiryncie potrzebuje 9 odcinków).
**Konsekwencje.** Ślady Mario bez zmian (Mario nie losuje). Scenariusze z losowością (`kosmos_play`) są w regresji.
Przy okazji naprawiony błąd w Kosmosie: `spawn_asteroid()` przy rozpadzie mogło zająć slot właśnie niszczonej
asteroidy i odczytać jej **nowy** rozmiar (`size-1-1 = -1` → sprite spoza tablicy, na ekranie „statek-widmo") —
dane potrzebne po zwolnieniu slotu kopiować do zmiennych lokalnych wcześniej.

## 25. Kart: Mode 7 zamiast raycastingu dla wyścigów, autopilot jako narzędzie testowe (23.09.2026)

**Kontekst.** Użytkownik poprosił o „wypasioną grę w stylu Mario Kart”. Raycasting z Labiryntu 3D nie nadaje się do
wyścigów (świat z kafelków-ścian, brak otwartego terenu). Super Mario Kart na SNES używał trybu Mode 7: płaska
tekstura podłoża rzutowana perspektywicznie – każdy wiersz ekranu to prosta w świecie, więc w pętli wewnętrznej
są tylko dwa dodawania w stałym przecinku i jeden odczyt tekstury.
**Decyzja.** `src/games/kart/`: tekstura toru 1024×1024 RGB565 (2 MB, PSRAM przez `platform::alloc_pixels(fast=false)`)
generowana przy starcie z 16 punktów kontrolnych (Catmull-Rom → 256 próbek linii środkowej, stemplowanie kół:
krawężniki, asfalt, linia startu, pola przyspieszenia). Osobna mapa nawierzchni 128×128 (komórki 8×8) daje
tanie `surface_at()` dla fizyki. Płótno 400×240 x2 – 144 wiersze podłoża. Sprite'y (gokarty 16 kierunków × 4 kolory
w arkuszach 512×32, drzewa, skrzynki, przedmioty) sortowane po odległości i skalowane `FOCAL / fwd`.
AI jedzie do punktu linii kilka próbek przed sobą z własnym pasem; tempo bazowe 0,89/0,94/0,99 MAX + „guma”
(+10 % gdy daleko za graczem, −6 % gdy przed). Przedmioty losowane zależnie od miejsca (lider: banany, ostatni: grzyby).
**Autopilot (Y):** gokart gracza prowadzi to samo AI. Dla dziecka – demo; dla testów – jedyny sposób, żeby skrypt
`--hold` przejechał całe okrążenie bez ręcznego strojenia skrętów. Scenariusz `kart_auto` (1800 klatek) jest w regresji
i jest deterministyczny (losowanie przedmiotów z `engine::rng` ze stałym ziarnem).
**Konsekwencje.** Na PC Mode 7 kosztuje tyle co Kosmos (0,55 ms z tłem 0,52). Na P4: 57 600 próbek tekstury z PSRAM
na klatkę, dostęp przy dalekich wierszach skacze po pamięci (cache), szacunek 3-6 ms – **do zmierzenia**; awaryjnie
tekstura 512×512 (0,5 MB) albo co drugi wiersz. Generowanie tekstury przy pierwszym wejściu do gry (1 M pikseli
z szumem) na P4 potrwa zauważalnie (rząd 0,3-0,5 s) – jeśli przeszkadza, przenieść do PNG w `assets/`.
Nazwy „Kart”, „Niebieski/Zielony/Zolty” to robocze nazwy do zmiany przez użytkownika.

## 26. Kart bez pikselozy: natywne 800×480, PNG z alfą, filtrowanie, jakość adaptacyjna (23.09.2026)

**Kontekst.** Pierwsza wersja Karta rysowała 400×240 powiększane ×2, sprite'y 32 px i teksturę 1 teksel na jednostkę
– użytkownik: „ma wyglądać jak na nowoczesnej konsoli, żadnej pikselozy”. Zgłosił też błąd: skręt w lewo pokazywał
gokart obrócony w prawo (generator arkusza obracał model w lewo przy rosnącym numerze klatki, renderer zakładał prawo).
**Decyzja.** Kart renderuje natywnie 800×480 (`canvas_scale()` 1). Nowy typ `gfx::Image` (RGB565 + alfa 0..255,
`gfx::load_png_rgba`, bufor w PSRAM) dla obrazów z wygładzonymi krawędziami; `draw_image` miesza alfę i przy jakości 0
filtruje bilinearnie z wagami mnożonymi przez alfę (bez ciemnych obwódek). Tekstura toru 2048×2048 (2 teksele na
jednostkę świata – fizyka i ślady testów bez zmian), stemple drogi z miękką krawędzią, 5 poziomów mipmap wybieranych
per wiersz, filtrowanie bilinearne dla bliskich wierszy, mgła przy horyzoncie i z odległością. Niebo: gradient, słońce
z poświatą, pas chmur 1024×160 i pas gór 2048×160 z alfą mapowane po kącie kamery. Efekty: dym spod kół (drift),
kurz na trawie, płomień turbo, iskry. Grafika generowana Pillow z supersamplingiem ×4: model gokarta z cieniowaniem
wg normalnych ścian (kierunek klatek zgodny z rendererem – naprawa błędu skrętu), 128 px na klatkę.
**Jakość adaptacyjna.** Pełna jakość to na PC 3,8 ms; na P4 na pewno za dużo. `render()` mierzy swój czas i po 45
wolnych klatkach (> 13 ms) schodzi na poziom 1 (bez bilinearnego, bez chmur), potem 2 (podłoże co drugi piksel,
góry bez mieszania). Dzięki temu ta sama binarka wygląda najlepiej tam, gdzie może, i trzyma płynność tam, gdzie musi.
**Konsekwencje.** PSRAM: ~14 MB (tekstura 8 MB + mipmapy 2,7 MB + arkusze 3,1 MB). Czas generowania toru na P4 do
zmierzenia (szacunek 1-2 s). Narzędzie `gen_kart_assets.py` wymaga Pillow (`pip install pillow`) – wcześniejsze
generatory były w czystym Pythonie; wygenerowane PNG są w repo, więc uczeń ani płytka Pillow nie potrzebują.

## 27. Silnik 3D `gfx3d` zamiast Mode 7: Kart w prawdziwym low-poly 3D (23.09.2026)

**Kontekst.** Użytkownik ocenił Karta jako słabego i zapytał, czy ogranicza nas silnik grafiki. Tak: warstwa `gfx`
to płótno 2D i sprite'y, więc Mode 7 dawał płaski tor bez wzniesień, a gokarty były obrazkami przełączanymi między
16 kierunkami. Nowocześnie wyglądające wyścigi wymagają geometrii 3D: toru z górkami, modeli oświetlanych z każdej
strony, kamery w przestrzeni. P4 nie ma GPU, ale software'owy rasteryzator trójkątów low-poly (bez tekstur, bez
Z-bufora) mieści się w budżecie: kilka tysięcy trójkątów i ~600 tys. wypełnionych pikseli na klatkę.
**Decyzja.** Nowa warstwa `src/gfx3d/` nad `gfx::Canvas`: `math3d.h` (Vec3, Mat4 wierszowa, `heading()` z jawną bazą
prawo/góra/przód), `mesh.h/.cpp` (siatka z kolorem na trójkąt, normalne wierzchołków do Gouraud, bryły: box, klin,
koło, walec, stożek, kula, dysk; nawinięcie poprawiane wg wektora „na zewnątrz"), `renderer.h/.cpp` (kamera lookAt,
światło kierunkowe + ambient, mgła, odrzucanie tylnych ścian po normalnej w świecie, przycinanie do z = 1,
rasteryzacja płaska i Gouraud po skanliniach, sortowanie malarskie: 1024 kubełków głębokości × 2 warstwy
(teren, obiekty), listy FIFO – kolejność zgłaszania zachowana w kubełku, dzięki czemu cień rysuje się na drodze).
Bez Z-bufora: 800×480×16 bit w PSRAM kosztowałoby na P4 więcej niż całe rysowanie, a artefakty malarskie przy
osobnej warstwie terenu są niewidoczne. Kart: tor = 256 przekrojów × 11 wierzchołków (4 pasy asfaltu, krawężniki,
pobocza) + siatka terenu 32×32 z pominięciem komórek pod drogą; wysokość terenu to suma sinusów (fizyka 2D bez zmian,
ślady testów zmieniły się tylko przez nową, czystszą mapę nawierzchni); gokart = 17 brył + kask (kula Gouraud) +
4 koła obracające się z prędkością, przednie skręcające, przechył z tempa skrętu i nachylenia terenu; drzewa, skrzynki
(bez oświetlenia – „świecą"), przedmioty, flaga jako bryły. Niebo, chmury, góry, dym, płomień pozostały obrazami
z alfą. Mode 7 i arkusze gokartów usunięte.
**Konsekwencje.** Na PC 2,7 ms/klatkę (było 3,8 ms w Mode 7 800×480), PSRAM ~1,5 MB zamiast ~14 MB, tor generuje się
natychmiast (brak tekstury 8 MB). Na P4 szacunek 6-12 ms – do zmierzenia; jakość adaptacyjna skraca zasięg.
`gfx3d` jest gotowe dla kolejnych gier (i lekcji zaawansowanej): `Mesh` + `Renderer::draw_mesh(model, layer)`.
Ograniczenia: brak tekstur, brak Z-bufora (przecinające się bryły w jednej warstwie mogą się źle sortować),
max 4096 wierzchołków na siatkę w jednym `draw_mesh`.

## 28. Kart – wygląd: Gouraud z kolorami wierzchołków, oznaczenia w lukach siatki, obiekty przy torze (24.09.2026)

**Kontekst.** Użytkownik: Kart „wygląda słabo, trzeba go przerobić, aby wyglądał naprawdę atrakcyjnie wizualnie”.
Diagnoza ze zrzutów: (1) asfalt i trawa w szachownicę – jeden kolor na trójkąt daje wzór kafelków, a płaskie cieniowanie
pokazuje każdą fasetę terenu; (2) pusty świat – sama droga, drzewa i flaga; (3) klockowaty gokart z kierowcą-pudełkiem;
(4) surowy HUD z prostokątów; (5) światło niezgodne z tarczą słońca, cień gokarta zielony na asfalcie.
**Decyzja.** Silnik: `Vertex::c` + `Mesh::vertex_colors` (kolor bazowy z wierzchołka, interpolowany Gouraud), bryły
`add_hexa` (8 dowolnych naroży) i `add_loft` (prostokąt→prostokąt), `Renderer::set_flat_only` (tryb oszczędny) oraz
`depth_bias` w `draw_mesh` (cienie zawsze po podłożu). Gra: asfalt i trawa jako siatki Gouraud z kolorami z szumu
i wysokości terenu; oznaczenia (krawężniki, linie, oś, start, szewrony) jako **osobna płaska siatka wypełniająca luki**
w siatce Gouraud – nie nakładka, bo dwie współpłaszczyznowe siatki migotałyby w sortowaniu malarskim, a jeden wspólny
wierzchołek między pasami rozmywałby ostre granice. Statyczne obiekty (brama, trybuna, stosy opon w zakrętach z krzywizny
toru, banery) w jednej siatce `props_` w współrzędnych świata (jedno `draw_mesh`); krzaki i drzewa instancjonowane
z losową skalą i cieniami. Gokart z brył ściętych, kierowca z ramionami, kask z wizjerem – po ocenie „bolidy wyglądają
bardzo słabo” druga iteracja: kadłub jako łańcuch lofów o wspólnych przekrojach (zamiast pudełek na płycie), koła
z osobną felgą (`add_wheel(..., rim)`: ściana boczna opony, wklęsła grafitowa felga z fasetami, jasny kapsel – bok koła
był jednolicie jasnoszary, a płaska jasna tarcza na pół boku nadal wyglądała jak bęben), wahacze i oś jako pręty (`add_rod`),
pontony z wlotami, airbox, dyfuzor, dwa płaty skrzydła, kierowca z karkiem i rękawicami. Światło = kierunek słońca na
niebie. HUD z zaokrąglonych paneli (`fill_round_rect_alpha`), medal miejsca, kolejność, gradientowy pasek prędkości,
komunikaty okrążeń, odliczanie w kole, tabela mety z czasami. Kamera z FOV rosnącym przy turbo; na tytule kołysze się
za polami startowymi (pełna orbita wjeżdżała w gokarty i bramę – przycinanie z ≥ 1 rozrywało geometrię).
**Konsekwencje.** PC: 3,4 ms/klatkę (było 2,05) przy ~9,5 tys. trójkątów; na P4 koszt Gouraud na podłożu nieznany –
przy > 13 ms gra sama przechodzi na cieniowanie płaskie (`quality_ ≥ 1`) i wyłącza krzaki/cienie (`≥ 2`). Fizyka
nietknięta: ślady `kart_auto` i `kart_player` identyczne bajt w bajt, nagrano tylko nowy wzorzec zrzutu `kart_race`.
Odrzucone: tekstury (brak w gfx3d, na P4 za drogie), Z-bufor (jak w 27), winieta/post-efekty (pełnoekranowe mieszanie
384 tys. pikseli to na P4 kilka ms).

## 29. Kart: teksturowanie z colormapą, cienie rzutowane przez maskę, odblask, ślady opon, Z-bufor (25.09.2026)

**Kontekst.** Po dwóch iteracjach modeli użytkownik: „nadal grafika pozostawia wiele do życzenia. Czy na tym silniku nie
jesteśmy w stanie uzyskać ładniejszych efektów?”. Ocena: ograniczeniem jest silnik – jeden kolor na trójkąt (brak
tekstur), brak prawdziwych cieni, brak odblasku, brak AA. Zaproponowano kroki A-E z kosztami; użytkownik: „działaj po
kolei od A do E”. Pytanie o źródło tekstur (skrypt czy ręcznie) bez odpowiedzi – przyjęto skrypt (deterministyczny,
Pillow), z możliwością podmiany PNG o tym samym układzie.
**Decyzja.** (A) Tekstury w stylu Quake/N64, bo P4 nie ma SIMD ani GPU: atlas 8-bit z paletą 256 kolorów (indeks 0
przezroczysty) i colormapa 16 odcieni × 12 poziomów mgły × 256 wpisów RGB565 – piksel to dwa odczyty z tablic, bez
mnożenia kanałów. Mapowanie afiniczne w blokach 16 px z dzieleniem na granicach (u/z, v/z, 1/z liniowe w ekranie)
zamiast pełnej korekcji perspektywy. Gouraud na teksturze = interpolacja wiersza colormapy. Jeden atlas 512×512 dla całej
gry (jedna paleta, jedno wczytanie, UV w tekselach atlasu w `Tri`). Drzewa i krzaki jako billboardy z alpha-test: tańsze
i ładniejsze niż kule low-poly. (B) Cienie rzutowane macierzą na płaszczyznę podłoża, ale nie jako ciemne trójkąty
(nakładające się trójkąty bolidu podwójnie by się przyciemniały), tylko przez maskę 1 B/piksel i jedno przyciemnienie
po warstwie 0 – półprzezroczysty cień na każdej nawierzchni. (C) Odblask Blinna z półwektorem liczonym raz na siatkę,
kadłub z normalnymi uśrednionymi (zaokrąglony lakier). (D) Ślady opon w logice gry (deterministyczne, nie w śladzie
testów), rysowane jako czworokąty z `depth_bias`. (E) Z-bufor 16-bit opcjonalny (`quality_ == 0`), sortowanie
malarskie zostaje – Z-bufor rozstrzyga tylko przecięcia.
**Konsekwencje.** PC ~5 ms/klatkę (było 2,5) – teksturowane podłoże to większość pikseli. Na P4 koszt nieznany;
szacunek z liczby operacji na piksel: 8-12 ms na podłoże, czyli cel 30 FPS, z awaryjnym zejściem przez `quality_`.
PSRAM: +256 kB atlas, +96 kB colormapa, +384 kB maska, +768 kB Z-bufor, bufor trójkątów 18 000 × ~100 B = 1,8 MB.
Wykryta pułapka testów: adaptacyjna jakość mierzy zegar, więc pod obciążeniem CPU (regresja i benchmark równolegle)
zrzut różnił się między uruchomieniami – w trybie `--frames` jakość jest zamrożona (`engine::deterministic()`).
Pozostałe ograniczenia: brak mipmap (ostra trawa migocze w oddali – złagodzone rozmytym kafelkiem na dalekie plany),
brak AA, brak filtrowania bilinearnego (za drogie na P4).
**Uzupełnienie (25.09).** Użytkownik: bolid „częściowo ginie pod asfaltem”. Z-bufor ujawnił rozjazd między siatką
drogi (płaska w poprzek, na wysokości środka toru) a wysokością, na której stawiano obiekty (`ground_height` w ich
punkcie). Dodano `surface_height()` zgodne z siatką i użyto go dla wszystkiego, co stoi na drodze. To nie wystarczyło:
przechył nadwozia w zakrętach obracał także koła wokół punktu na ziemi (zewnętrzne koła 1,1 jednostki pod drogą), a skok
kąta przy ustawianiu na polach startowych dawał ten sam przechył na starcie. Teraz każde koło stoi na wysokości nawierzchni
w swoim punkcie, nadwozie liczy pochylenie i przechył z czterech kół, a przechył w zakrętach obraca tylko nadwozie wokół
osi kół. Wniosek ogólny: z Z-buforem obiekty muszą być stawiane na tej samej funkcji wysokości, z której zbudowano
podłoże, a efekty „kosmetyczne” nie mogą ruszać punktów styku z podłożem.
**Uzupełnienie 2 (25.09).** „Przednie koła skręcają się przeciwnie” – nie znak skrętu, lecz `Mat4::heading` o wyznaczniku
−1 (odbicie): odwrócone nawinięcie sprawiało, że renderer odrzucał bliższe ściany i pokazywał lustrzane odbicie bolidu
(koła odwrotnie, malowanie niewidoczne). Renderer sprawdza teraz wyznacznik macierzy modelu i odwraca normalną geometryczną.
Diagnoza wymagała pomiaru (rzut czubka koła na ekran) zamiast oceny na oko – obraz i rachunek się nie zgadzały, i to
obraz miał rację co do objawu, a rachunek co do geometrii; łącznikiem był cull.
**Uzupełnienie 3 (25.09).** Kolizje z obiektami przy torze (drzewa, stosy opon, słupy, trybuna) jako okręgi
rejestrowane przy budowie sceny i sprawdzane w fizyce – wcześniej dekoracje były przenikalne. Emulator w `--frames`
nie pokazuje okna: wyskakujące okno kradło fokus osobie piszącej w innym edytorze w trakcie testów.

## 30. Snake: grafika z Gemini jako PNG z alfą, ciało węża generowane w kodzie, tło „wypalane” raz na poziom (25.09.2026)

**Kontekst.** Użytkownik: nowa, pełnowartościowa gra w węża (lekcja 07 zostaje) „na ładnych grafikach”, grafiki
wygenerować w jego Gemini przez przeglądarkę; poziomy, przyspieszanie, dodatkowe przedmioty, ładny interfejs.
**Decyzja.** (1) Obiekty z Gemini jako arkusze 4×3 na jednolitej magencie, wycinane skryptem (`tools/gen_snake_assets.py`):
tło = obszar magenty połączony z brzegiem komórki (flood fill – pierwsza wersja kluczująca po samym odcieniu zjadła
fioletowy grzyb), miękka alfa i zdjęcie różowej poświaty tylko w pasie 3 px przy tle. Wynik: PNG RGBA 24/36/40/48 px,
wczytywane `gfx::load_png_rgba` i rysowane z mieszaniem alfa. Surowe obrazy trzymane w `assets_src/snake/`, żeby dało się
przerobić rozmiary bez ponownego generowania (Gemini nie jest deterministyczny). (2) Ciało węża nie z grafiki, tylko
cieniowane kulki generowane przy starcie (8 promieni × 2 tony × 3 kolory): płynnie się zwężają, pasują do dowolnego
kształtu i koloru głowy; głowa z Gemini (widok z boku) obracana o 90° (w lewo = lustro, żeby nie była do góry nogami).
Dwa przejścia (obrys, potem kolor) dają jeden kontur zamiast „koralików”; cień przez maskę 1 B/piksel (jak w Karcie).
(3) Tło świata + szachownica + winieta + przeszkody z cieniami wypalane do bufora raz na poziom, klatka zaczyna się od
jednego `memcpy` 717 kB. (4) Ruch po kratkach ze stałym krokiem, rysowanie z interpolacją `prev_ → body_`; zawijanie
krawędzi liczone jako ruch o kratkę „na zewnątrz”. (5) Autopilot (BFS + test wolnej przestrzeni zalewaniem) jako
narzędzie testów – deterministyczny, przechodzi całą kampanię (~19 000 klatek), dzięki czemu regresja sprawdza wszystkie
10 plansz i wszystkie moce.
**Konsekwencje.** PC 0,84 ms/klatkę średnio (Kosmos 0,26); max 28 ms to dekodowanie tła PNG 800×448 przy starcie poziomu
(na P4 szacunkowo 100-300 ms – w trybie Bez końca zmiana świata w trakcie gry da jedno zacięcie). Firmware +42 kB flash,
+8,5 kB RAM statycznie; PSRAM: tło 2×717 kB + maska 358 kB + obrazki ~0,5 MB. Partycja assets: +2,8 MB PNG.
Rekordy tylko w RAM (NVS w planach, pkt 7 „Następne kroki”). Na sprzęcie nic nie sprawdzone.
