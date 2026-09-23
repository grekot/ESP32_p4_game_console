# Lekcja 13: Twoja gra

## Cel

Własna gra od pomysłu do wersji, w którą zagra ktoś inny. Bez kodu startowego – zaczynasz od `00_szablon`.

## Zanim napiszesz pierwszą linię

1. **Jedno zdanie:** „Gra o …, w której gracz … , a przegrywa, gdy …". Jeśli nie mieści się w zdaniu, jest za duża na pierwszy raz.
2. **Rysunek ekranu** na kartce: co gdzie jest, co się rusza, gdzie wynik.
3. **Lista rzeczy** w grze (gracz, przeciwnicy, pociski, monety…). Każda to struktura; jeśli jest ich wiele – tablica struktur.
4. **Stany gry:** tytuł, gra, koniec (lekcja 11). Narysuj kółka i strzałki.
5. **Klawisze:** masz `UP DOWN LEFT RIGHT A B X Y`. START należy do konsoli.

## Plan pracy (w tej kolejności)

- [ ] Skopiuj `00_szablon` do `13_twoja_gra` (albo `14_nazwa`), zmień `CONSOLE_ADD_GAME`, dopisz do `lista.h`. F6 – pusty ekran działa.
- [ ] Gracz rusza się po ekranie. Sam. Nic więcej. F6.
- [ ] Jedna „rzecz", która coś robi (spada, leci, goni). F6.
- [ ] Zderzenie gracza z tą rzeczą i **jedna** konsekwencja (punkt albo życie). F6.
- [ ] Wiele rzeczy: tablica struktur. F6.
- [ ] Przegrana i wygrana: stan GAME_OVER, `restart()` po A. F6.
- [ ] Ekran tytułowy z nazwą gry.
- [ ] Grafika: sprite'y zamiast prostokątów (lekcja 10). **Dopiero teraz** – wcześniej to strata czasu, gdy gra się zmienia.
- [ ] Ktoś inny gra 5 minut. Zapisz, na co narzekał. Napraw jedną rzecz.
- [ ] README: jak grać, co jest w środku, co byś dodał.

Po każdym punkcie: F6 działa, commit w Source Control.

## Pomysły na pierwszą grę (sprawdzone, da się w 100–200 linii)

| gra | co potrzebujesz | z których lekcji |
|---|---|---|
| Frogger – przejdź przez ulicę | samochody w tablicy struktur, pasy ruchu | 03, 09 |
| Asteroidy w dół – unikaj spadających skał | tablica struktur, `random`, przyspieszanie | 03, 09 |
| Labirynt na czas | mapa z liter, `map_solid`, licznik czasu | 12, 11 |
| Ping-pong dla dwóch z bonusami | Pong + bonusy zmieniające paletki | 04, 09 |
| Whack-a-mole – trafiaj w wyskakujące cele | tablica struktur z czasem życia, `pressed` | 06, 09 |
| Dino run – skacz nad przeszkodami | grawitacja, przeszkody w tablicy, wynik = dystans | 08, 11 |

## Gdy utkniesz

- Zmniejsz: wyrzuć ostatnią rzecz, którą dodałeś, aż zacznie działać. Potem dodawaj po jednej.
- `watch()` na każdą zmienną, która „robi coś dziwnego".
- Napisz na kartce, co gra ma zrobić w **tej jednej klatce**, po kolei. Porównaj z `frame()`.
- Zajrzyj do Lake Mario (`src/games/mario/`) – tam jest wszystko: stany, mapa, przeciwnicy, cząsteczki, HUD.

## Co dalej po tej lekcji

- `engine::Game` – „dorosły" interfejs gry (klasa, `update(dt)`, `render(canvas)`), na którym chodzi Lake Mario.
  Twoja gra z `console` da się przepisać na niego w godzinę.
- Dźwięk, zapis wyników, gra na prawdziwej konsoli z ekranem dotykowym – gdy płytka przyjdzie, każda lekcja
  pojawi się w jej menu.
