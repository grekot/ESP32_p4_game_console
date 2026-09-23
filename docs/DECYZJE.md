# Dziennik decyzji projektowych

Każdy wpis: kontekst, decyzja, konsekwencje. Kolejność chronologiczna (22-23.09.2026).

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
