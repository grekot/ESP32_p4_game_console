# Lekcja 05: Inwazja

## Cel

Ściana kosmitów maszeruje nad twoim statkiem (strzelanie w następnej lekcji). Nauczysz się **pętli `for`** –
sposobu na powtórzenie czegoś wiele razy bez kopiowania kodu.

## Co nowego

Żeby narysować 8 kosmitów, mógłbyś napisać 8 razy `draw_alien(...)` z innym `x`. A 24? A 100?
W Scratchu był klocek „powtórz 10 razy". W C++ to **pętla `for`**:

```cpp
for (int i = 0; i < 8; i = i + 1) {     // start: i = 0;  powtarzaj dopoki i < 8;  po kazdym obrocie i = i + 1
    draw_alien(80 + i * 80, 60);        // i = 0, 1, 2, ..., 7  ->  x = 80, 160, 240, ..., 640
}
```

Zmienna `i` (licznik) żyje tylko w pętli i za każdym obrotem jest o 1 większa. Dzięki temu każdy kosmita ma inne `x`.
Skrót: `i = i + 1` pisze się też `i++`, a `i = i + 5` jako `i += 5`.

**Pętla w pętli** rysuje siatkę: zewnętrzna po rzędach, wewnętrzna po kolumnach:

```cpp
for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 8; col++) {
        draw_alien(block_x + col * 80, block_y + row * 48);
    }
}
```

**Reszta z dzielenia `%`** przydaje się do „co któryś": `frame_count() % 30 == 0` jest prawdą co 30 klatek,
a `(frame_count() / 30) % 2` daje na przemian 0 i 1 – gotowa animacja dwuklatkowa.

## Uruchom

Otwórz `gra.cpp`, **Ctrl+Shift+B**. Rząd 8 zielonych prostokątów maszeruje w prawo, przy krawędzi zawraca i schodzi
o 16 pikseli. Strzałkami ruszasz białym statkiem na dole.

## Jak to działa

```cpp
for (int i = 0; i < COLS; i = i + 1) {
    int x = block_x + i * (ALIEN_W + GAP_X);   // pozycja z numeru kosmity
    draw_alien(x, block_y);
}
block_x = block_x + dir;                       // dir = 2 (w prawo) albo -2 (w lewo)
int block_w = COLS * (ALIEN_W + GAP_X) - GAP_X;
if (block_x + block_w > screen_width() || block_x < 0) {   // ktorys koniec bloku za krawedzia
    dir = -dir;                                // zawroc
    block_y = block_y + 16;                    // i zejdz nizej
}
```

## Zadania

1. ★ Zmień `COLS` na 10, potem `GAP_X` na 16. Co się dzieje, gdy blok jest szerszy niż ekran (`COLS = 12`)? Dlaczego „drga"?
2. ★ Ładniejszy kosmita: w `draw_alien` narysuj korpus, dwie „nogi" i dwa czarne kwadraciki oczu (kilka `rect`).
3. ★★ Trzy rzędy kosmitów: dodaj stałą `ROWS = 3` i drugą pętlę (`row`) wokół pierwszej. Każdy rząd o `ALIEN_H + 16` niżej.
   Pokaż liczbę kosmitów: `watch("aliens", ROWS * COLS);`. *(test 1)*
4. ★★ Animacja marszu: co 30 klatek kosmici zmieniają pozę. Policz `int pose = (frame_count() / 30) % 2;` i przekaż
   do `draw_alien(x, y, pose)` – gdy `pose == 1`, nogi szerzej. Blok niech rusza się skokami: tylko gdy `frame_count() % 10 == 0`,
   za to o 12 pikseli.
5. ★★★ Gwiazdy w tle: przed kosmitami pętla rysująca 40 gwiazd. Bez tablic i losowania – z reszty z dzielenia:
   `rect((i * 37) % screen_width(), (i * 53) % screen_height(), 2, 2, GRAY)`. Potem: gdy blok zejdzie do `block_y > 360`,
   napis „PRZEGRALES" i `restart()` po A.

## Sprawdź sam

Po zadaniu 3 w rogu ekranu jest `aliens 24`, a na ekranie 3 rzędy po 8:

```
lake_sim.exe --game inwazja --frames 10 --trace 9
```

W linii `TRACE` ma być `aliens=24`.

## Dla ciekawych

Co się stanie, gdy w pętli napiszesz `i < COLS` zamiast `i <= COLS`? A gdy zapomnisz `i = i + 1`? (Uwaga: emulator
się zawiesi – zamknij go z paska zadań. To słynna „pętla nieskończona".)

## Słowniczek

| po polsku | w C++ |
|---|---|
| powtórz N razy | `for (int i = 0; i < N; i++) { ... }` |
| zwiększ o 1 | `i++` albo `i = i + 1` |
| zwiększ o 5 | `i += 5` |
| reszta z dzielenia | `a % b` |
| pętla w pętli | `for (...) { for (...) { ... } }` |
