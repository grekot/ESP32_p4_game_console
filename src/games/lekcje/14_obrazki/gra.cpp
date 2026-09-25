// LEKCJA 14 - OBRAZKI PNG: grafika z plikow, klatki animacji w tablicy, odbicie lustrzane. Zadania w README.md.
#include "console/console.h"

namespace {   // pudelko na twoja gre - nie usuwaj

// Obrazki leza w katalogu assets/obrazki/ (nazwa katalogu = pierwsze slowo z CONSOLE_ADD_GAME na dole).
// load_image("plik.png") wczytuje PNG; przezroczyste tlo z PNG dziala. Wolaj w setup(), nie w frame().
Sprite hero[2];    // dwie klatki chodu: hero_0.png, hero_1.png
Sprite bee[2];     // dwie klatki skrzydel
Sprite apple;
Sprite tree;

const int SCALE = 3;   // obrazek 16x16 rysujemy 3x wiekszy (48x48 px)
const int SPEED = 3;   // pikseli na klatke

const int HERO_START_X = 100;
const int HERO_START_Y = 228;

int  hero_x = HERO_START_X;
int  hero_y = HERO_START_Y;
bool facing_left = false;
bool walking     = false;

int apple_x = 350;
int apple_y = 240;
int apples  = 0;

int bee_x  = 620;
int bee_y  = 100;
int bee_dx = 2;        // kierunek lotu pszczoly (patrol lewo-prawo)
int lives  = 3;

void setup()
{
    hero[0] = load_image("hero_0.png");
    hero[1] = load_image("hero_1.png");
    bee[0]  = load_image("bee_0.png");
    bee[1]  = load_image("bee_1.png");
    apple   = load_image("apple.png");
    tree    = load_image("tree.png");

    hero_x = HERO_START_X;
    hero_y = HERO_START_Y;
    facing_left = false;
    apple_x = 350;
    apple_y = 240;
    apples  = 0;
    bee_x   = 620;
    bee_y   = 100;
    bee_dx  = 2;
    lives   = 3;
}

void frame()
{
    clear(rgb(70, 150, 70));   // trawa

    // Drzewa - ozdoba (w zadaniu 5 stana sie przeszkodami)
    sprite(tree, 300, 60, false, SCALE);
    sprite(tree, 650, 330, false, SCALE);
    sprite(tree, 120, 380, false, SCALE);

    // Ruch w 4 kierunkach. Zapamietujemy, czy bohater idzie (animacja) i w ktora strone patrzy (odbicie).
    walking = false;
    if (held(LEFT))  { hero_x = hero_x - SPEED; facing_left = true;  walking = true; }
    if (held(RIGHT)) { hero_x = hero_x + SPEED; facing_left = false; walking = true; }
    if (held(UP))    { hero_y = hero_y - SPEED; walking = true; }
    if (held(DOWN))  { hero_y = hero_y + SPEED; walking = true; }
    hero_x = clamp(hero_x, 0, screen_width() - hero[0].w * SCALE);
    hero_y = clamp(hero_y, 0, screen_height() - hero[0].h * SCALE);

    // Pszczola lata w lewo i w prawo (patrol). W zadaniu 4 zacznie gonic bohatera.
    bee_x = bee_x + bee_dx;
    if (bee_x < 560 || bee_x > 760) bee_dx = -bee_dx;

    // Zbieranie jablka - twoje zadanie 3

    // Rysowanie. Klatka animacji: co 8 klatek zmiana 0 -> 1 -> 0...; stojacy bohater ma zawsze klatke 0.
    int hero_frame = 0;
    if (walking) hero_frame = (frame_count() / 8) % 2;
    int bee_frame = (frame_count() / 4) % 2;

    sprite(apple, apple_x, apple_y, false, SCALE);
    sprite(hero[hero_frame], hero_x, hero_y, facing_left, SCALE);
    sprite(bee[bee_frame], bee_x, bee_y, bee_dx < 0, SCALE);

    // HUD: jablka i zycia
    text(20, 8, apples, WHITE, 3);
    for (int i = 0; i < lives; i++) {
        circle(screen_width() - 30 - i * 30, 24, 10, RED);
    }

    watch("hero_x", hero_x);
    watch("hero_y", hero_y);
    watch("apples", apples);
    watch("lives", lives);
}

}  // namespace

CONSOLE_ADD_GAME(obrazki, "Obrazki PNG", "Lekcja 14: grafika z plikow")
