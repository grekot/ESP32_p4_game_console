// LEKCJA 07 - WAZ (Snake): tablica jako lista, przesuwanie elementow, ruch co N klatek. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int CELL    = 20;                       // rozmiar kratki w pikselach (800 / 40 = 20)
const int GRID_W  = 40;                       // 800 / 20 kratek w poziomie
const int GRID_H  = 24;                       // 480 / 20 kratek w pionie
const int MAX_LEN = 200;                      // maksymalna dlugosc weza
const int STEP    = 8;                        // waz rusza sie co 8 klatek

// Waz to LISTA kratek: snake_x[0], snake_y[0] to glowa, dalej kolejne segmenty az do ogona.
int snake_x[MAX_LEN];
int snake_y[MAX_LEN];
int length = 3;                               // ile segmentow jest uzywanych

int dir_x = 1;                                // kierunek ruchu: (1,0) prawo, (-1,0) lewo, (0,-1) gora, (0,1) dol
int dir_y = 0;
int food_x = 20;
int food_y = 12;
int score  = 0;

void place_food()
{
    food_x = random(0, GRID_W - 1);
    food_y = random(0, GRID_H - 1);
}

void setup()
{
    length = 3;
    for (int i = 0; i < length; i++) {        // glowa w (10,12), segmenty w lewo od niej
        snake_x[i] = 10 - i;
        snake_y[i] = 12;
    }
    dir_x = 1;
    dir_y = 0;
    score = 0;
    place_food();
}

void frame()
{
    clear(BLACK);

    // Skret: nie mozna zawrocic o 180 stopni (waz wjechalby w siebie)
    if (pressed(UP)    && dir_y != 1)  { dir_x = 0;  dir_y = -1; }
    if (pressed(DOWN)  && dir_y != -1) { dir_x = 0;  dir_y = 1;  }
    if (pressed(LEFT)  && dir_x != 1)  { dir_x = -1; dir_y = 0;  }
    if (pressed(RIGHT) && dir_x != -1) { dir_x = 1;  dir_y = 0;  }

    // Ruch co STEP klatek
    if (frame_count() % STEP == 0) {
        // Kazdy segment wchodzi na miejsce poprzedniego - OD OGONA, zeby nie nadpisac wartosci, ktorej jeszcze potrzebujemy
        for (int i = length - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        // Glowa idzie w kierunku ruchu
        snake_x[0] = snake_x[0] + dir_x;
        snake_y[0] = snake_y[0] + dir_y;

        // Jedzenie: waz rosnie o segment (nowy ogon w miejscu starego)
        if (snake_x[0] == food_x && snake_y[0] == food_y) {
            if (length < MAX_LEN) {
                snake_x[length] = snake_x[length - 1];
                snake_y[length] = snake_y[length - 1];
                length = length + 1;
            }
            score = score + 1;
            place_food();
        }

        // Sciany i wlasny ogon - twoje zadania 2 i 3
    }

    // Rysowanie: glowa zolta, reszta zielona
    for (int i = 0; i < length; i++) {
        Color c = GREEN;
        if (i == 0) {
            c = YELLOW;
        }
        rect(snake_x[i] * CELL, snake_y[i] * CELL, CELL - 1, CELL - 1, c);
    }
    rect(food_x * CELL, food_y * CELL, CELL - 1, CELL - 1, RED);
    text(12, 8, score, WHITE, 3);

    watch("length", length);
    watch("head_x", snake_x[0]);
    watch("head_y", snake_y[0]);
}

}  // namespace

LAKE_GAME(waz, "Waz", "Lekcja 07: tablice jako listy")
