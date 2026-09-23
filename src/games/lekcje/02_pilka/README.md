# Lekcja 02: Piłka

## Cel

Piłka lata po ekranie i odbija się od krawędzi. Nauczysz się instrukcji **`if`** („jeżeli") i tego, jak z dwóch liczb zrobić ruch.

## Co nowego

W poprzedniej lekcji kwadrat wyjechał za ekran i nic go nie zatrzymało. Potrzebujemy decyzji:
**jeżeli** piłka jest za krawędzią, **to** zawróć. W Scratchu był na to klocek „jeżeli ... to". W C++:

```cpp
if (ball_x > 784) {    // warunek w nawiasach: czy ball_x jest wieksze niz 784?
    vx = -vx;          // to wykonuje sie TYLKO, gdy warunek jest prawdziwy
}
```

Porównania: `>` większe, `<` mniejsze, `>=` większe lub równe, `<=` mniejsze lub równe, `==` równe (dwa znaki!), `!=` różne.

**Ruch** to dodawanie prędkości do pozycji co klatkę: `ball_x = ball_x + vx;`. Gdy `vx` jest dodatnie, piłka leci w prawo,
gdy ujemne – w lewo. `vx = -vx;` zmienia znak, czyli odwraca kierunek. To całe „odbicie".

## Uruchom

Otwórz `gra.cpp`, **Ctrl+Shift+B**. Biała piłka leci w prawo-dół, odbija się od prawej i lewej ściany,
ale **wypada dołem** i znika. W rogu widać `x`, `y`, `vx`, `vy` – patrz, jak `y` rośnie i rośnie.

## Jak to działa

```cpp
ball_x = ball_x + vx;                     // przesun w poziomie o vx
ball_y = ball_y + vy;                     // przesun w pionie o vy
if (ball_x > screen_width() - RADIUS) {   // prawa krawedz: srodek pilki dalej niz 800 - 16
    vx = -vx;                             // zawroc
}
if (ball_x < RADIUS) {                    // lewa krawedz
    vx = -vx;
}
circle(ball_x, ball_y, RADIUS, WHITE);    // dopiero teraz rysuj - w nowym miejscu
```

Dlaczego `screen_width() - RADIUS`, a nie `screen_width()`? Bo `ball_x` to **środek** piłki, a odbić ma się jej brzeg.

## Zadania

1. ★ Zmień kolor piłki i `RADIUS` na 30. Zmień prędkość startową na `vx = 8`.
2. ★ Dopisz dwa `if`, żeby piłka odbijała się też od **góry** (`ball_y < RADIUS`) i **dołu**
   (`ball_y > screen_height() - RADIUS`). Teraz nie powinna nigdy zniknąć. *(test 1)*
3. ★★ Niech piłka **zmienia kolor** przy każdym odbiciu. Podpowiedź: zmienna `Color ball_color = WHITE;`,
   w każdym `if` przypisz jej inny kolor, a `circle(...)` rysuj tym kolorem.
4. ★★ Dodaj **licznik odbić**: `int bounces = 0;`, w każdym `if` dopisz `bounces = bounces + 1;`,
   pokaż go na ekranie: `text(20, 16, bounces, WHITE, 3);` i w rogu: `watch("bounces", bounces);`. *(test 2)*
5. ★★★ **Grawitacja.** Co klatkę dopisz `vy = vy + 1;` (piłka przyspiesza w dół). Przy odbiciu od dołu zamiast
   `vy = -vy` napisz `vy = -vy * 9 / 10;` (traci trochę energii). Ustaw też `ball_y = screen_height() - RADIUS;`
   w tym `if`, żeby nie „wpadała" w podłogę. Piłka skacze coraz niżej, jak prawdziwa.

## Sprawdź sam

Po zadaniu 2 uruchom na 5 sekund i patrz na `y`:

```
console_sim.exe --game pilka --frames 301 --trace 60
```

W **każdej** linii `y` musi być między 0 a 479. Bez zadania 2 po sekundzie `y` przekracza 480 i dalej rośnie.
Zadanie **LEKCJA: Sprawdz zadania** robi to samo automatycznie (test 1) i sprawdza, czy `bounces` jest większe od zera (test 2).

## Dla ciekawych

Co się stanie, gdy `vx = 0`? A gdy piłka wystartuje **poza** ekranem, np. `ball_x = 1000`? Dlaczego się „trzęsie"?
(Podpowiedź: warunek `>` jest prawdziwy w dwu kolejnych klatkach.) Jak to naprawić? Spróbuj `ball_x = screen_width() - RADIUS;` wewnątrz `if`.

## Słowniczek

| po polsku | w C++ |
|---|---|
| jeżeli warunek, to … | `if (warunek) { ... }` |
| większe / mniejsze | `>` / `<` |
| równe / różne | `==` / `!=` |
| odwróć znak | `vx = -vx;` |
| prędkość (o ile na klatkę) | `vx`, `vy` |
