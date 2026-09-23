# Lekcja 01: Wizytówka

## Cel

Narysować własną wizytówkę: imię, kwadrat, koło i linię. Przy okazji nauczysz się, czym jest **zmienna**, **stała** i jak działają **współrzędne** na ekranie.

## Co nowego

**Zmienna** to pudełko z nazwą, w którym leży liczba. Możesz ją wyjąć, zmienić i włożyć z powrotem.
W Scratchu robił to klocek „ustaw [x] na ...". W C++:

```cpp
int square_x = 120;   // int = liczba calkowita; pudelko "square_x", w srodku 120
square_x = 200;       // teraz w srodku jest 200
```

**Stała** to pudełko zaklejone taśmą: raz ustawiasz, potem tylko czytasz. Piszesz `const` przed typem:

```cpp
const int SIZE = 80;  // stale piszemy WIELKIMI LITERAMI, zeby je odroznic
```

**Współrzędne.** Ekran ma 800 pikseli szerokości i 480 wysokości. Punkt `(0, 0)` jest w **lewym górnym** rogu.
`x` rośnie w prawo, `y` rośnie **w dół** (odwrotnie niż w Scratchu!).

```
(0,0) ------------------> x (799)
  |
  |        (400,240) = srodek ekranu
  |
  v y (479)
```

**Kolory** mają nazwy: `BLACK WHITE GRAY RED GREEN BLUE YELLOW ORANGE PURPLE BROWN SKY` i ciemniejsze `DARK_RED`, `DARK_BLUE`...
Własny kolor: `rgb(255, 128, 0)` (czerwony, zielony, niebieski, każdy 0–255).

## Uruchom

1. Otwórz `gra.cpp`.
2. Wciśnij **Ctrl+Shift+B** (albo **F6**).
3. Zobaczysz granatowe tło, napis Wizytowka i Twoje imie, czerwony kwadrat, zielone koło i białą linię u dołu.
   W prawym górnym rogu mały panel `square_x 120` – to podgląd zmiennej z `watch(...)`.

## Jak to działa

```cpp
clear(BACKGROUND);                          // zamaluj caly ekran kolorem tla
text(40, 80, name, YELLOW, 4);              // napis w punkcie (40,80), zolty, rozmiar 4 (najwiekszy)
rect(square_x, square_y, SIZE, SIZE, RED);  // prostokat: lewy gorny rog, szerokosc, wysokosc, kolor
circle(circle_x, circle_y, radius, GREEN);  // kolo: srodek, promien, kolor
line(0, 460, 799, 460, WHITE);              // linia od (0,460) do (799,460)
watch("square_x", square_x);                // pokaz wartosc zmiennej w rogu ekranu
```

Cała funkcja `frame()` wykonuje się 60 razy na sekundę. Za każdym razem rysuje wszystko od nowa, dlatego zaczyna od `clear`.

## Zadania

1. ★ Wpisz swoje imię w `name` (bez polskich znaków) i zmień kolor tła na inny.
2. ★ Zmień stałą `SIZE` na 120, a `radius` na 30. Co się stało z kwadratem i kołem?
3. ★★ Dodaj **drugi kwadrat** obok pierwszego. Potrzebujesz nowych zmiennych (np. `square2_x`) i drugiej linii `rect(...)`.
4. ★★ Ożyw kwadrat: w `frame()` dopisz linię `square_x = square_x + 1;`. Patrz na panel `watch` – liczba rośnie, kwadrat jedzie w prawo. *(test 1)*
5. ★★★ Dodaj zmienną `int speed = 4;` i użyj jej zamiast `1`. Potem spraw, żeby kwadrat jechał **w lewo**.
   Co się dzieje, gdy wyjedzie za ekran? (To problem na następną lekcję.)

## Sprawdź sam

Po zadaniu 4 kwadrat startuje w `x = 120` i co klatkę przesuwa się o 1. Uruchom grę na 121 klatek i wypisuj wartości co 30:

```
console_sim.exe --game wizytowka --frames 121 --trace 30
```

Ostatnia linia to klatka numer **120**, ale komputer liczy klatki **od zera** (0, 1, 2, ... 120), więc `frame()` wykonało się 121 razy
i kwadrat przesunął się o 121 pikseli: `120 + 121 = 241`. W ostatniej linii ma być `square_x=241`. To samo sprawdza zadanie VS Code **LEKCJA: Sprawdz zadania**.

## Dla ciekawych

Zamiast liczby w `text(...)` możesz podać zmienną: `text(600, 40, square_x, WHITE, 3)` wypisze jej wartość dużymi cyframi.

## Słowniczek

| po polsku | w C++ |
|---|---|
| zmienna całkowita | `int nazwa = 5;` |
| stała | `const int NAZWA = 5;` |
| kolor | `Color`, np. `RED`, `rgb(r, g, b)` |
| napis | `"tekst w cudzyslowie"` |
| przypisanie (włóż do pudełka) | `x = 10;` |
