// LEKCJA 05 - INWAZJA: petla for. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int COLS    = 8;     // ilu kosmitow w rzedzie
const int ALIEN_W = 48;
const int ALIEN_H = 32;
const int GAP_X   = 32;    // odstep miedzy kosmitami
const int SHIP_W  = 40;
const int SHIP_H  = 20;
const int SHIP_Y  = 440;

int block_x = 80;    // lewy gorny rog calego bloku kosmitow
int block_y = 60;
int dir     = 2;     // kierunek i predkosc marszu: 2 w prawo, -2 w lewo
int ship_x  = 380;

// Jeden kosmita. Funkcja, zeby latwo bylo zmienic wyglad w jednym miejscu.
void draw_alien(int x, int y)
{
    rect(x, y, ALIEN_W, ALIEN_H, GREEN);
}

void setup()
{
    block_x = 80;
    block_y = 60;
    dir     = 2;
    ship_x  = 380;
}

void frame()
{
    clear(BLACK);

    // PETLA for: powtorz COLS razy. Zmienna i przyjmuje po kolei 0, 1, 2, ..., COLS-1.
    //   for (start; warunek; krok)
    for (int i = 0; i < COLS; i = i + 1) {
        int x = block_x + i * (ALIEN_W + GAP_X);   // kazdy kolejny kosmita dalej w prawo
        draw_alien(x, block_y);
    }

    // Marsz bloku: w prawo, przy krawedzi zawroc i zejdz nizej
    block_x = block_x + dir;
    int block_w = COLS * (ALIEN_W + GAP_X) - GAP_X;   // szerokosc calego bloku
    if (block_x + block_w > screen_width() || block_x < 0) {
        dir = -dir;
        block_y = block_y + 16;
    }

    // Statek gracza
    if (held(LEFT))  ship_x = ship_x - 6;
    if (held(RIGHT)) ship_x = ship_x + 6;
    ship_x = clamp(ship_x, 0, screen_width() - SHIP_W);
    rect(ship_x, SHIP_Y, SHIP_W, SHIP_H, WHITE);

    watch("block_x", block_x);
    watch("block_y", block_y);
}

}  // namespace

LAKE_GAME(inwazja, "Inwazja", "Lekcja 05: petla for")
