# Dla ucznia: jak pisać gry na konsolę Lake

Masz przed sobą małą konsolę do gier i jej **emulator** – program, który udaje konsolę na komputerze.
Gry piszesz w języku **C++**, w tym samym, w którym pisze się prawdziwe gry na Nintendo czy PlayStation.
Każda gra, którą napiszesz, będzie działać w emulatorze, a potem na prawdziwej konsoli z ekranem dotykowym.

## Jak zacząć

1. Kliknij skrót **Lekcje C++** na pulpicie. Otworzy się VS Code – program do pisania kodu.
2. Po lewej stronie znajdź folder `src` → `games` → `lekcje` → `01_wizytowka` i kliknij **`gra.cpp`**.
3. Wciśnij **F6** (albo Ctrl+Shift+B). Komputer przez chwilę kompiluje (zamienia kod na program), potem otwiera okno z grą.
4. Zamykasz okno klawiszem **Esc**. Zmieniasz coś w kodzie, znów F6. I tak w kółko – to całe programowanie.

Obok każdego `gra.cpp` jest **`README.md`** – tam jest lekcja: co nowego, jak to działa i zadania.
Czytaj README, zmieniaj `gra.cpp`, sprawdzaj F6.

## Klawisze w emulatorze

Konsola ma krzyżak i przyciski jak pad. W emulatorze udają je klawisze:

| klawisz konsoli | na klawiaturze | do czego zwykle |
|---|---|---|
| góra / dół / lewo / prawo | **strzałki** | ruch |
| A | **Z** | skok, zatwierdź |
| B | **X** | akcja, bieg |
| X | **S** | dodatkowy |
| Y | **A** | dodatkowy |
| START | **Enter** | pauza (należy do konsoli, gra jej nie widzi) |
| – | **Esc** | zamknij emulator |

Lewy przycisk myszy = palec na ekranie dotykowym. Prostokąty na dole ekranu to dotykowy pad – znikają, gdy użyjesz klawiszy.

## Jak wygląda gra

```cpp
#include "lake/lake.h"      // funkcje do rysowania i sterowania

namespace {                 // "pudelko" na twoja gre - nie ruszaj

int x = 100;                // twoje zmienne

void setup()                // RAZ, na starcie
{
    x = 100;
}

void frame()                // 60 razy na sekunde
{
    clear(BLACK);           // wyczysc ekran
    if (held(RIGHT)) {      // jesli trzymasz strzalke w prawo...
        x = x + 2;          // ...przesun
    }
    rect(x, 100, 20, 20, RED);   // narysuj kwadrat
}

}  // namespace             // koniec pudelka - nie ruszaj

LAKE_GAME(moja, "Moja gra", "Opis w menu")   // rejestracja
```

Ekran ma **800 × 480** punktów (pikseli). `(0, 0)` to lewy górny róg. `x` rośnie w prawo, `y` rośnie **w dół**.

## Ściągawka: co możesz wywołać

| co | jak |
|---|---|
| wyczyść ekran | `clear(BLACK)` |
| prostokąt | `rect(x, y, szerokosc, wysokosc, KOLOR)` |
| sam obrys | `rect_outline(x, y, w, h, KOLOR)` |
| koło | `circle(srodek_x, srodek_y, promien, KOLOR)` |
| linia | `line(x1, y1, x2, y2, KOLOR)` |
| napis | `text(x, y, "Tekst", KOLOR, rozmiar 1-4)` – bez polskich liter |
| liczba na ekranie | `text(x, y, wynik, KOLOR, 3)` |
| kolory | `BLACK WHITE GRAY RED GREEN BLUE YELLOW ORANGE PURPLE BROWN SKY`, ciemne: `DARK_RED DARK_BLUE ...`, własny: `rgb(255, 0, 128)` |
| klawisz trzymany | `held(LEFT)` – `UP DOWN LEFT RIGHT A B X Y` |
| klawisz właśnie wciśnięty | `pressed(A)` |
| losowa liczba od 1 do 6 | `random(1, 6)` |
| przycięcie do zakresu | `clamp(x, 0, 760)` |
| czy prostokąty się dotykają | `overlaps(x1, y1, w1, h1, x2, y2, w2, h2)` |
| szerokość / wysokość ekranu | `screen_width()` / `screen_height()` |
| numer klatki, sekundy | `frame_count()`, `seconds()` |
| obrazek z liter | `Sprite s = load_sprite(rysunek);` potem `sprite(s, x, y, false, 3)` – 3 = powiększenie (lekcja 10) |
| mapa z liter | `load_map(MAPA, "#=B")`, `map_tile(col, row)`, `map_set(...)`, `draw_tiles('#', KOLOR, cam_x)` (lekcja 12) |
| ruch z kolizjami z mapą | `MoveResult r = move_box(x, y, w, h, vx, vy);` → `r.on_ground` (lekcja 12) |
| zacznij grę od nowa | `restart()` |
| pokaż zmienną w rogu | `watch("x", x)` |
| napisz w terminalu | `print("tekst", liczba)` |

## Jak czytać błąd

Gdy coś jest nie tak, po F6 zamiast gry pojawi się w terminalu czerwony tekst, na przykład:

```
src/games/lekcje/02_pilka/gra.cpp:14:5: error: 'predkosc' was not declared in this scope
```

Czytaj to tak: **plik** : **linia 14** : **znak 5** : błąd: „`predkosc` nie zostało zadeklarowane" – czyli użyłeś nazwy,
której komputer nie zna (literówka? zapomniałeś `int predkosc = 3;`?). Kliknij w tę linię w terminalu – VS Code
przeniesie cię w to miejsce w kodzie. Najczęstsze komunikaty:

| komunikat | co znaczy |
|---|---|
| `expected ';' before ...` | brakuje średnika na końcu poprzedniej linii |
| `'xyz' was not declared in this scope` | nieznana nazwa: literówka albo brak deklaracji |
| `expected '}' at end of input` | brakuje zamykającej klamry `}` |
| `no matching function for call to 'rect(...)'` | zła liczba lub typ argumentów – sprawdź w ściągawce |
| `redefinition of 'x'` | tę nazwę już zadeklarowałeś wyżej |
| `cannot convert 'Color' to 'int'` | pomyliłeś kolejność: kolor jest ostatni |
| `multiple definition of 'setup()'` | w jakiejś lekcji brakuje `namespace {` na górze |
| `Permission denied` / `collect2: error` przy linkowaniu | emulator jest jeszcze otwarty – zamknij okno (Esc) i F6 |

Komputer pokazuje najwyżej 3 błędy naraz. Napraw pierwszy – kolejne często znikają same.

## Podglądanie, co się dzieje

- **`watch("nazwa", zmienna)`** w `frame()` pokazuje wartość w prawym górnym rogu ekranu, na żywo. Idealne do sprawdzania „dlaczego piłka znika".
- **`print("tekst", liczba)`** pisze w terminalu pod oknem. Uwaga: w `frame()` to 60 linii na sekundę – lepiej w `if`.
- Zadanie **LEKCJA: Slad** (Ctrl+Shift+P → „Run Task") uruchamia grę na 5 sekund bez klawiszy i wypisuje wartości z `watch` co pół sekundy.
- Zadanie **LEKCJA: Zrzut ekranu** zapisuje obrazek `zrzut.bmp` w katalogu lekcji.
- Zadanie **LEKCJA: Sprawdz zadania** uruchamia testy – powie ci, które zadania z README są zrobione dobrze.

## Zapisywanie postępu (git)

Twój kod jest w **repozytorium git** – to jak punkty zapisu w grze. Po skończonym zadaniu:

1. Kliknij ikonę **Source Control** po lewej (trzy kółka połączone liniami) albo Ctrl+Shift+G.
2. W polu na górze wpisz, co zrobiłeś, np. `Lekcja 02 zadanie 3 - pilka zmienia kolor`.
3. Kliknij **✓ Commit**. Gotowe – punkt zapisu.

Wysyłaniem do internetu (push) zajmuje się rodzic.

## Zasady

- Zmieniasz pliki **tylko** w `src/games/lekcje/`. Reszta to silnik konsoli – jeśli chcesz coś w nim zmienić, powiedz.
- Zmienne nazywaj **małymi literami** po angielsku: `ball_x`, `score`, `speed`. Stałe WIELKIMI: `SIZE`.
- W kodzie **nie używaj polskich znaków** (ą, ę, ł...) – ani w napisach, ani w komentarzach. Czcionka konsoli ich nie ma.
- Nie usuwaj linii `namespace {` i `}  // namespace` – bez nich dwie lekcje pokłócą się o nazwy.
- Rozwiązania zadań są w katalogu `rozwiazania/` każdej lekcji. Najpierw spróbuj sam przez 15 minut – dopiero potem zaglądaj.

## Gdy coś nie działa

| objaw | co zrobić |
|---|---|
| F6 nic nie robi | kliknij najpierw w plik `gra.cpp` – zadanie uruchamia grę **z otwartego pliku** |
| `nie ma gry "..."` | nazwa w `LAKE_GAME(...)` w `gra.cpp` i w `lista.h` muszą być takie same |
| gra się nie zmieniła po edycji | plik niezapisany? (biała kropka na karcie); VS Code zapisuje sam po chwili |
| błąd `collect2` / `Permission denied` | okno emulatora jest otwarte – zamknij Esc |
| czerwone podkreślenia w edytorze, a kompiluje się | to tylko podpowiedzi; jeśli VS Code zapyta o „IntelliSense provider", wybierz **Yes** |
| Windows „chronił komputer" przy starcie `lake_sim.exe` | kliknij „Więcej informacji" → „Uruchom mimo to"; to twój własny, świeżo skompilowany program |

## Lekcje

| nr | katalog | czego się uczysz | gra |
|---|---|---|---|
| 00 | `00_szablon` | budowa pliku gry | napis |
| 01 | `01_wizytowka` | zmienne, stałe, współrzędne | wizytówka |
| 02 | `02_pilka` | `if` | odbijająca się piłka |
| 03 | `03_lapacz` | klawisze, `&&` `\|\|` `else`, losowanie | łapanie piłek |
| 04 | `04_pong` | funkcje | Pong |
| 05 | `05_inwazja` | pętle `for` | ściana kosmitów |
| 06 | `06_inwazja_strzal` | tablice | Space Invaders |
| 07 | `07_waz` | tablice jako listy | Snake |
| 08 | `08_flappy` | struktury | Flappy |
| 09 | `09_breakout` | tablice struktur | Breakout |
| 10 | `10_bohater` | własna grafika | zbieracz monet |
| 11 | `11_flappy_pelny` | stan gry, HUD, rekord | Flappy kompletny |
| 12 | `12_mini_mario` | mapa kafelków, skok | mini-platformówka |

| 13 | `13_twoja_gra` | własny projekt od pomysłu do gry | twoja |

Rób je po kolei – każda korzysta z poprzednich.
