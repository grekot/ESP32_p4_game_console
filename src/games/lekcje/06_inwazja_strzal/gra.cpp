// LEKCJA 06 - INWAZJA: STRZAL. Tablice. Zadania w README.md.
// Punkt startowy = Inwazja z lekcji 05 (3 rzedy) + pocisk. Strzal: A (klawisz Z w emulatorze).
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int COLS    = 8;
const int ROWS    = 3;
const int ALIEN_W = 48;
const int ALIEN_H = 32;
const int GAP_X   = 32;
const int GAP_Y   = 16;
const int SHIP_W  = 40;
const int SHIP_H  = 20;
const int SHIP_Y  = 440;

// TABLICA: wiele zmiennych pod jedna nazwa, kazda z numerem (indeksem) od 0.
// alive[row][col] mowi, czy kosmita w rzedzie row i kolumnie col jeszcze zyje.
bool alive[ROWS][COLS];

int block_x = 80;
int block_y = 60;
int dir     = 1;
int ship_x  = 380;

// Pocisk: jest tylko jeden naraz
bool bullet_active = false;
int  bullet_x      = 0;
int  bullet_y      = 0;

int score = 0;

void draw_alien(int x, int y)
{
    rect(x + 8, y, ALIEN_W - 16, ALIEN_H - 12, GREEN);
    rect(x, y + 8, ALIEN_W, 12, GREEN);
    rect(x + 12, y + ALIEN_H - 12, 8, 12, GREEN);
    rect(x + ALIEN_W - 20, y + ALIEN_H - 12, 8, 12, GREEN);
    rect(x + 16, y + 6, 4, 4, BLACK);
    rect(x + ALIEN_W - 20, y + 6, 4, 4, BLACK);
}

// Pozycja kosmity o numerach (row, col) - liczona z numerow, jak w lekcji 05
int alien_x(int col) { return block_x + col * (ALIEN_W + GAP_X); }
int alien_y(int row) { return block_y + row * (ALIEN_H + GAP_Y); }

void setup()
{
    // Wypelnij tablice: na starcie wszyscy zyja
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            alive[row][col] = true;
        }
    }
    block_x = 80;
    block_y = 60;
    dir     = 1;
    ship_x  = 380;
    bullet_active = false;
    score = 0;
}

void frame()
{
    clear(BLACK);

    // --- kosmici: rysuj tylko zywych ---
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if (alive[row][col]) {
                draw_alien(alien_x(col), alien_y(row));
            }
        }
    }
    if (frame_count() % 10 == 0) {
        block_x = block_x + dir * 12;
        int block_w = COLS * (ALIEN_W + GAP_X) - GAP_X;
        if (block_x + block_w > screen_width() || block_x < 0) {
            dir = -dir;
            block_y = block_y + 16;
        }
    }

    // --- statek ---
    if (held(LEFT))  ship_x = ship_x - 6;
    if (held(RIGHT)) ship_x = ship_x + 6;
    ship_x = clamp(ship_x, 0, screen_width() - SHIP_W);
    rect(ship_x, SHIP_Y, SHIP_W, SHIP_H, WHITE);

    // --- pocisk ---
    if (pressed(A) && !bullet_active) {      // strzal tylko, gdy pocisku nie ma w powietrzu
        bullet_active = true;
        bullet_x = ship_x + SHIP_W / 2;
        bullet_y = SHIP_Y;
    }
    if (bullet_active) {
        bullet_y = bullet_y - 12;
        rect(bullet_x - 3, bullet_y, 6, 16, YELLOW);
        if (bullet_y < 0) {
            bullet_active = false;
        }
        // Trafienie - to twoje zadanie 2: sprawdz w petli, czy pocisk dotyka zywego kosmity
    }

    text(20, 8, score, WHITE, 3);
    watch("score", score);
    watch("bullet_y", bullet_y);
}

}  // namespace

LAKE_GAME(inwazja_strzal, "Inwazja: strzal", "Lekcja 06: tablice")
