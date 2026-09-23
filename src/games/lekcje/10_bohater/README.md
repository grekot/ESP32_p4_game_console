# Lekcja 10: Bohater

## Cel

Własna grafika: bohater narysowany literami, odbicie lustrzane, animacja chodu, obracające się monety.

## Co nowego

Do tej pory rysowałeś prostokąty i koła. **Sprite** to gotowy obrazek – w tej konsoli rysuje się go **literami**
w kodzie, tak jak w Lake Mario:

```cpp
const char* const COIN_ROWS[8] = {   // 8 wierszy = 8 pikseli wysokosci
    "..yyyy..",                      // kazdy znak = 1 piksel: y = zolty, Y = ciemny zolty, . = nic
    ".yYYYYy.",
    "yYyyyyYy",
    ...
};
Sprite coin;
void setup() { coin = load_sprite(COIN_ROWS); }   // litery -> piksele (RAZ, w setup)
void frame() { sprite(coin, x, y, false); }        // rysuj; true = odbicie lustrzane
```

Litery palety: `k` czerń, `w` biel, `e`/`E` szary, `r`/`R` czerwień, `o` pomarańcz, `y`/`Y` żółty, `g`/`G` zieleń,
`b`/`B` brąz, `t` beż, `s` skóra, `u`/`U` niebieski, `p`/`P` fiolet (wielka litera = ciemniejszy odcień).
Obrazek ma tyle wierszy, ile linii, i tyle kolumn, ile znaków w najdłuższej. `hero.w` i `hero.h` to jego rozmiar.

**Odbicie lustrzane** (`flip_x = true`) załatwia patrzenie w lewo bez drugiego rysunku.
**Animacja** to dwa obrazki pokazywane na przemian: `(frame_count() / 8) % 2` wybiera, który.

## Uruchom

Otwórz `gra.cpp`, **Ctrl+Shift+B**. Na trawie stoi bohater w czerwonej czapce; strzałki go przesuwają i obracają.
Obok leży złota moneta – jeszcze nie da się jej zebrać.

## Jak to działa

Dwie tablice napisów (`HERO_ROWS`, `COIN_ROWS`) to rysunki. `setup()` zamienia je na sprite'y. W `frame()`:
ruch i zapamiętanie kierunku (`facing_left`), potem `sprite(hero, x, y, facing_left)`. `GROUND_Y - hero.h` stawia
dolną krawędź obrazka na trawie.

## Zadania

1. ★ Przerysuj bohatera: inny kolor czapki i ubrania, może kapelusz albo miecz? Zmień tło na noc (`DARK_BLUE`) i dorysuj księżyc.
2. ★★ **Animacja chodu.** Skopiuj `HERO_ROWS` do `HERO_WALK_ROWS` i w dwóch dolnych wierszach rozsuń nogi.
   `Sprite hero_walk;` w `setup`. Gdy bohater idzie (trzymasz LEFT/RIGHT) i `(frame_count() / 8) % 2 == 1` – rysuj `hero_walk`.
3. ★★ **Zbieranie.** Gdy `overlaps(hero_x, GROUND_Y - hero.h, hero.w, hero.h, coin_x, GROUND_Y - coin.h, coin.w, coin.h)` –
   `coins++` i moneta przenosi się w losowe `coin_x = random(0, screen_width() - coin.w)`. *(test 1)*
4. ★★ **Pięć monet** – tablica struktur `struct Coin { int x; bool taken; };  Coin all_coins[5];` rozstawionych co 70 px.
   Moneta obraca się: drugi rysunek `COIN_THIN_ROWS` (wąska, 2 kolumny) pokazywany na przemian co 10 klatek.
5. ★★★ **Skok.** Zmienne `hero_y` i `vy`; `pressed(A)` gdy stoi na ziemi – `vy = -6`; co 3 klatki `vy++`; `hero_y += vy`;
   nie niżej niż `GROUND_Y - hero.h`. Monety niech wiszą w powietrzu na różnych wysokościach – trzeba do nich doskoczyć.

## Sprawdź sam

Po zadaniu 3 bohater idący w prawo przez 5 sekund przechodzi przez monetę i ją zbiera:

```
lake_sim.exe --game bohater --hold RIGHT 0 300 --frames 301 --trace 60
```

W ostatniej linii `coins` większe od 0.

## Dla ciekawych

Obrazki Lake Mario są w `src/games/mario/mario_assets.cpp` – otwórz, obejrzyj, skopiuj przeciwnika do swojej gry.
Ta sama paleta, ten sam sposób.

## Słowniczek

| po polsku | w C++ |
|---|---|
| rysunek z liter | `const char* const NAZWA[wysokosc] = { "...", ... };` |
| zamień rysunek na obrazek | `Sprite s = load_sprite(NAZWA);` (w `setup`) |
| narysuj obrazek | `sprite(s, x, y, odbicie, powiekszenie)` |
| rozmiar obrazka | `s.w`, `s.h` |
| co któraś klatka | `(frame_count() / N) % 2` |
