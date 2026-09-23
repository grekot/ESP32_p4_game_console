// LEKCJA 10 - BOHATER: wlasna grafika (sprite z liter), powiekszenie, odbicie lustrzane, animacja. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

// SPRITE = obrazek narysowany literami. Kazda linia to rzad pikseli, litera = kolor z palety:
//   k czern, w biel, e szary, E ciemnoszary, r czerwien, R ciemna czerwien, o pomarancz, y zolty,
//   Y ciemny zolty, g zielen, G ciemna zielen, b braz, B ciemny braz, t bez, s skora,
//   u niebieski, U ciemny niebieski, p fiolet, P ciemny fiolet, '.' = przezroczysty (nic)
const char* const HERO_ROWS[16] = {
    "......rrrrrr....",
    ".....rrrrrrrrr..",
    ".....BBBsssks...",
    "....BsBsssskss..",
    "....BsBBssssss..",
    "....BBsssskkk...",
    "......ssssss....",
    ".....ggguggg....",
    "....ggggguggggg.",
    "...gggguuuuggg..",
    "...ssuuuuuuuuss.",
    "...ssuuuyyuuuss.",
    ".....uuuu.uuuu..",
    "....BBBB..BBBB..",
    "...BBBBB..BBBBB.",
    "................",
};

const char* const COIN_ROWS[8] = {
    "..yyyy..",
    ".yYYYYy.",
    "yYyyyyYy",
    "yYyyyyYy",
    "yYyyyyYy",
    "yYyyyyYy",
    ".yYYYYy.",
    "..yyyy..",
};

Sprite hero;     // obrazki budujemy raz, w setup()
Sprite coin;

const int SCALE    = 3;     // obrazek 16x16 rysujemy 3x wiekszy: 48x48 px (ekran ma 800x480)
const int GROUND_Y = 400;   // na tej wysokosci stoi bohater (dolna krawedz sprite'a)
const int SPEED    = 4;

int  hero_x      = 100;
bool facing_left = false;   // w ktora strone patrzy (do odbicia lustrzanego)
int  coin_x      = 500;
int  coins       = 0;

void setup()
{
    hero = load_sprite(HERO_ROWS);   // litery -> piksele
    coin = load_sprite(COIN_ROWS);
    hero_x      = 100;
    facing_left = false;
    coin_x      = 500;
    coins       = 0;
}

void frame()
{
    clear(SKY);
    rect(0, GROUND_Y, screen_width(), screen_height() - GROUND_Y, DARK_GREEN);   // trawa

    // Ruch: w lewo/prawo; zapamietaj kierunek, zeby bohater patrzyl tam, gdzie idzie
    if (held(LEFT)) {
        hero_x = hero_x - SPEED;
        facing_left = true;
    }
    if (held(RIGHT)) {
        hero_x = hero_x + SPEED;
        facing_left = false;
    }
    hero_x = clamp(hero_x, 0, screen_width() - hero.w * SCALE);

    // Rysowanie: sprite(obrazek, x, y, odbicie_lustrzane, powiekszenie). Rozmiar na ekranie = hero.w * SCALE.
    sprite(coin, coin_x, GROUND_Y - coin.h * SCALE, false, SCALE);
    sprite(hero, hero_x, GROUND_Y - hero.h * SCALE, facing_left, SCALE);

    // Zbieranie monety - twoje zadanie 3

    text(20, 8, coins, WHITE, 3);
    watch("hero_x", hero_x);
    watch("coins", coins);
}

}  // namespace

LAKE_GAME(bohater, "Bohater", "Lekcja 10: wlasna grafika")
