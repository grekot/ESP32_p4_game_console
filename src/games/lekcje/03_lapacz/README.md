# Lekcja 03: Łapacz

## Cel

Pierwsza prawdziwa gra: paletką na dole ekranu łapiesz spadające piłki. Nauczysz się czytać **klawisze**,
łączyć warunki słowami **i** / **lub**, używać **`else`** i **losować** liczby.

## Co nowego

**Klawisze.** `held(LEFT)` zwraca prawdę, gdy klawisz w lewo jest wciśnięty (w emulatorze: strzałka w lewo).
Do wyboru: `UP DOWN LEFT RIGHT A B X Y`. W emulatorze A = klawisz **Z**, B = **X**.

```cpp
if (held(LEFT)) {
    paddle_x = paddle_x - 8;    // co klatke o 8 px w lewo, dopoki trzymasz
}
```

Jest też `pressed(A)` – prawda **tylko w klatce**, w której klawisz został wciśnięty (jak „kiedy klawisz naciśnięty" w Scratchu).
Do skoku czy strzału używaj `pressed`, do chodzenia `held`.

**Warunki złożone.** `&&` czyta się „i", `||` czyta się „lub":

```cpp
if (ball_x >= paddle_x && ball_x <= paddle_x + PADDLE_W) {   // pilka miedzy lewym I prawym koncem paletki
```

**`else`** – „w przeciwnym razie":

```cpp
if (lives > 0) {
    // gra trwa
} else {
    text(240, 190, "GAME OVER", RED, 4);
}
```

**Losowanie.** `random(10, 50)` daje liczbę od 10 do 50 (obie włącznie), za każdym razem inną.
`clamp(v, 0, 680)` przycina liczbę do zakresu: za małą podnosi do 0, za dużą obcina do 680.

## Uruchom

Otwórz `gra.cpp`, **Ctrl+Shift+B**. Strzałkami ruszasz paletką, żółta piłka spada z losowego miejsca.
Złapana piłka dodaje punkt (licznik w lewym górnym rogu) i w terminalu pojawia się linia `Zlapana! Wynik: 1` – to `print()`.

## Jak to działa

```cpp
if (held(LEFT))  { paddle_x = paddle_x - 8; }          // sterowanie
if (held(RIGHT)) { paddle_x = paddle_x + 8; }
ball_y = ball_y + speed;                                // pilka spada
if (ball_y + RADIUS >= PADDLE_Y                         // dolny brzeg pilki dotknal paletki
    && ball_x >= paddle_x                               // I srodek pilki na prawo od lewego konca
    && ball_x <= paddle_x + PADDLE_W) {                 // I na lewo od prawego konca
    score = score + 1;
    ball_y = 0;                                         // nowa pilka od gory
    ball_x = random(RADIUS, screen_width() - RADIUS);   // w losowym miejscu
}
```

Zauważ: kod „nowa piłka" (dwie linie) jest w programie **dwa razy**. Na następnej lekcji nauczysz się, jak napisać go raz.

## Zadania

1. ★ Zmień kolory i prędkość `speed`. Zmień szerokość paletki na 200 – łatwiej? Na 60 – trudniej?
2. ★ Paletka wyjeżdża za ekran. Po sterowaniu dopisz: `paddle_x = clamp(paddle_x, 0, screen_width() - PADDLE_W);`. *(test 1)*
3. ★★ Dodaj licznik **nieudanych** piłek `int misses = 0;` – zwiększaj go tam, gdzie piłka spada poza ekran.
   Pokaż go po prawej: `text(720, 16, misses, RED, 3);` i `watch("misses", misses);`. *(test 2)*
4. ★★ Po każdej złapanej piłce gra przyspiesza: `speed = speed + 1;`. Ale nie w nieskończoność – przytnij `clamp(speed, 5, 20)`.
5. ★★★ Życia. `int lives = 3;`, każda nieudana piłka odejmuje życie. Gdy `lives` spadnie do zera, gra ma się **zatrzymać**:
   otocz cały ruch (sterowanie, spadanie, łapanie) w `if (lives > 0) { ... } else { text(...GAME OVER...); }`.
   Dodatkowo: gdy jest GAME OVER i gracz wciśnie A (`pressed(A)`), wywołaj `restart();` – gra zacznie od `setup()`.

## Sprawdź sam

Po zadaniu 2 przytrzymaj strzałkę w lewo przez 5 sekund – paletka ma się zatrzymać na `paddle_x = 0`:

```
lake_sim.exe --game lapacz --hold LEFT 0 300 --frames 301 --trace 60
```

Bez `clamp` liczba spada poniżej zera i leci dalej w minus.

## Dla ciekawych

Dlaczego przy każdym uruchomieniu w oknie piłka startuje z innego miejsca, a w testach (`--frames`) zawsze z tego samego?
Odpowiedź: w trybie testowym konsola ustawia „ziarno" losowania na stałe (`random_seed`), żeby testy dały się powtórzyć.

## Słowniczek

| po polsku | w C++ |
|---|---|
| czy klawisz trzymany | `held(LEFT)` |
| czy klawisz właśnie wciśnięty | `pressed(A)` |
| i / lub / nie | `&&` / `\|\|` / `!` |
| w przeciwnym razie | `else` |
| losowa liczba od a do b | `random(a, b)` |
| przycięcie do zakresu | `clamp(v, min, max)` |
| wypisz do terminala | `print("tekst", liczba)` |
