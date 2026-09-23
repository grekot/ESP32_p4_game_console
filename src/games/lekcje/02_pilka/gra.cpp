// LEKCJA 02 - PILKA: instrukcja warunkowa if. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int RADIUS = 16;   // promien pilki

int ball_x = 100;   // srodek pilki
int ball_y = 200;
int vx = 5;         // predkosc: o tyle pikseli pilka przesuwa sie co klatke (v jak velocity)
int vy = 3;

void setup()
{
    // Wartosci startowe nadajemy tutaj, zeby po powrocie z menu gra zaczynala sie od nowa.
    ball_x = 100;
    ball_y = 200;
    vx     = 5;
    vy     = 3;
}

void frame()
{
    clear(BLACK);

    // 1. Rusz pilke
    ball_x = ball_x + vx;
    ball_y = ball_y + vy;

    // 2. Odbij od prawej i lewej krawedzi: JESLI pilka wyszla za krawedz, TO odwroc predkosc
    if (ball_x > screen_width() - RADIUS) {
        vx = -vx;
    }
    if (ball_x < RADIUS) {
        vx = -vx;
    }

    // Gora i dol - to twoje zadanie 2!

    // 3. Narysuj
    circle(ball_x, ball_y, RADIUS, WHITE);

    watch("x", ball_x);
    watch("y", ball_y);
    watch("vx", vx);
    watch("vy", vy);
}

}  // namespace

LAKE_GAME(pilka, "Pilka", "Lekcja 02: instrukcja if")
