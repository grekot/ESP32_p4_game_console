# Lekcja 04: Pong

## Cel

Klasyczny Pong na dwóch graczy. Nauczysz się pisać **funkcje** – własne klocki, które piszesz raz, a używasz wiele razy.

## Co nowego

W Łapaczu kod „nowa piłka" był w programie **dwa razy**. Gdybyś chciał go zmienić, musiałbyś pamiętać o obu miejscach.
W Scratchu rozwiązaniem był klocek „Moje bloki → Utwórz blok". W C++ to **funkcja**:

```cpp
void new_ball()          // void = nic nie zwraca; new_ball = nazwa; () = bez parametrow
{
    ball_x = 200;        // cialo funkcji - co ma zrobic
    ball_y = 120;
}
```

Wołasz ją po nazwie: `new_ball();`. Funkcja może mieć **parametry** – dane, które jej podajesz:

```cpp
void draw_paddle(int x, int y)     // dwa parametry: x i y
{
    rect(x, y, PADDLE_W, PADDLE_H, WHITE);
}
draw_paddle(20, left_y);           // lewa paletka
draw_paddle(764, right_y);         // prawa - ten sam kod, inne liczby
```

I może **zwracać wynik** słowem `return`. Wtedy zamiast `void` piszesz typ wyniku:

```cpp
bool hits_paddle(int px, int py)   // zwraca prawde albo falsz
{
    return overlaps(...);          // wynik idzie do tego, kto zawolal
}
if (hits_paddle(20, left_y)) { ... }
```

`overlaps(ax, ay, aw, ah, bx, by, bw, bh)` sprawdza, czy dwa prostokąty na siebie nachodzą. Przyda się w każdej grze.

## Uruchom

Otwórz `gra.cpp`, **Ctrl+Shift+B**. Lewa paletka: **strzałki góra/dół**. Prawa: **S** (góra) i **X** (dół) –
to klawisze konsoli X i B. Piłka startuje ze środka w losową stronę; punkt dostaje gracz, który ją przepuścił… odwrotnie:
punkt dostaje ten, którego przeciwnik przepuścił piłkę.

## Jak to działa

Przeczytaj `gra.cpp` od góry. Zwróć uwagę, że funkcje `draw_paddle`, `hits_paddle` i `new_ball` są napisane **przed** `setup()`
i `frame()` – komputer musi je poznać, zanim ktoś ich użyje. `frame()` czyta się teraz jak przepis: sterowanie, piłka, punkty, rysowanie.

## Zadania

1. ★ Zmień wysokość paletek, prędkość piłki, kolory. Co zrobi `vy = random(-4, 4)`, gdy wylosuje 0? Napraw to w `new_ball()`
   (podpowiedź: `if (vy == 0) { vy = 2; }`).
2. ★ Napisz funkcję `void draw_score(int x, int value)`, która rysuje wynik z cieniem: najpierw `text(x + 3, 23, value, DARK_GRAY, 4)`,
   potem `text(x, 20, value, WHITE, 4)`. Użyj jej dla obu wyników.
3. ★★ Napisz funkcję `int paddle_center(int y)`, która zwraca środek paletki: `return y + PADDLE_H / 2;`. Przy odbiciu ustaw
   `vy` zależnie od tego, gdzie piłka trafiła: jeśli `ball_y < paddle_center(left_y)`, to `vy = -6`, inaczej `vy = 6`.
   Teraz można celować!
4. ★★ Napisz funkcję `bool game_over()`, która zwraca prawdę, gdy ktoś ma 5 punktów. Gdy zwraca prawdę: zatrzymaj piłkę
   i napisz na środku, kto wygrał („LEWY WYGRAL" / „PRAWY WYGRAL"). A po `pressed(A)` – `restart();`.
5. ★★★ Komputer jako drugi gracz. Napisz funkcję `int follow_ball(int paddle_y)`, która zwraca nowe `y` paletki przesunięte
   o `PADDLE_SPEED` w stronę piłki (jeśli `paddle_center(paddle_y) < ball_y`, to w dół; jeśli większe – w górę; inaczej bez zmian).
   Zamiast sterowania X/B napisz `right_y = follow_ball(right_y);`. *(test 1)*

## Sprawdź sam

Po zadaniu 5 prawa paletka rusza się sama. Uruchom bez klawiszy:

```
lake_sim.exe --game pong --frames 301 --trace 60
```

`right_y` ma być różne od 192 (startowe) i podążać za `ball_y`. Bez zadania 5 zostaje 192.

## Dla ciekawych

Czy komputer da się pokonać? Spróbuj `follow_ball` ruszać paletką o `PADDLE_SPEED - 1` – wolniejszy przeciwnik jest do ogrania.
A gdyby ruszał się tylko wtedy, gdy piłka leci w jego stronę (`vx > 0`)?

## Słowniczek

| po polsku | w C++ |
|---|---|
| funkcja bez wyniku | `void nazwa(...) { ... }` |
| funkcja z wynikiem | `int nazwa(...) { return wynik; }` |
| parametr | `void f(int x)` – x jest parametrem |
| wołanie funkcji | `nazwa(10, 20);` |
| prawda / fałsz | `bool`, `true` / `false` |
| czy prostokąty nachodzą | `overlaps(...)` |
