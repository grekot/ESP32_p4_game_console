# Lekcja 00: Szablon

## Cel

Poznać, jak wygląda plik gry i co w nim wolno zmieniać. To także katalog, który kopiujesz, gdy zaczynasz własną grę.

## Jak wygląda plik gry

```cpp
#include "console/console.h"     // 1. bierzemy funkcje do rysowania i sterowania

namespace {                // 2. "pudelko" na twoja gre - nie ruszaj tej linii

// tu beda twoje zmienne

void setup()               // 3. wykonuje sie RAZ na starcie (jak "kiedy klikniete flage" w Scratchu)
{
}

void frame()               // 4. wykonuje sie 60 razy na sekunde - rysuje jedna klatke
{
    clear(BLACK);
    text(250, 220, "TU BEDZIE TWOJA GRA", WHITE, 2);   // (x, y, napis, kolor, rozmiar 1-4)
}

}  // namespace            // 5. koniec pudelka - nie ruszaj

CONSOLE_ADD_GAME(szablon, "Szablon", "Pusta gra do skopiowania")   // 6. rejestracja gry w konsoli
```

Zmieniasz **tylko** to, co jest między `namespace {` a `}  // namespace`, oraz ostatnią linię `CONSOLE_ADD_GAME`.

## Uruchom

1. Otwórz `gra.cpp`.
2. Wciśnij **Ctrl+Shift+B** (albo **F6**).
3. Otworzy się okno z czarnym ekranem i białym napisem. Zamknij je klawiszem **Esc**.

## Jak zrobić własną grę

1. Skopiuj katalog `00_szablon` i nazwij kopię, np. `05_moja_gra` (numer, podkreślnik, nazwa bez polskich znaków i spacji).
2. W ostatniej linii `gra.cpp` zmień `CONSOLE_ADD_GAME(szablon, "Szablon", ...)` na `CONSOLE_ADD_GAME(moja_gra, "Moja gra", "Opis")`.
3. Otwórz `src/games/lekcje/lista.h` i dopisz linię `LEKCJA(moja_gra)`.
4. Otwórz swoje `gra.cpp` i wciśnij Ctrl+Shift+B.

Jeśli emulator napisze `nie ma gry "..."`, to znaczy, że nazwa w `CONSOLE_ADD_GAME` i w `lista.h` się nie zgadzają.

## Słowniczek

| po polsku | w C++ |
|---|---|
| dołącz plik z funkcjami | `#include` |
| pudełko na nazwy | `namespace { ... }` |
| funkcja, która nic nie zwraca | `void nazwa()` |
| komentarz (komputer go nie czyta) | `// tekst` |
