// LEKCJA 08 - FLAPPY: struktury (struct). Zadania w README.md. Skrzydla: A (klawisz Z w emulatorze).
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int BIRD_R    = 16;    // promien ptaka
const int PIPE_W    = 80;    // szerokosc rury
const int GAP       = 160;   // wysokosc przerwy w rurze
const int PIPE_SPEED = 4;
const int GRAVITY_EVERY = 2; // co ile klatek ptak przyspiesza w dol
const int FLAP      = -10;   // predkosc po machnieciu skrzydlami (w gore = ujemna)

// STRUKTURA: kilka zmiennych zebranych pod jedna nazwa. Ptak ma x, y i predkosc pionowa.
struct Bird {
    int x;
    int y;
    int vy;
};

// Rura: jej x i wysokosc, na ktorej zaczyna sie przerwa
struct Pipe {
    int x;
    int gap_y;
};

Bird bird;      // jedna zmienna typu Bird - w srodku trzy liczby: bird.x, bird.y, bird.vy
Pipe pipe;      // jedna rura
int  score = 0;

void new_pipe()
{
    pipe.x     = screen_width();                    // startuje za prawa krawedzia
    pipe.gap_y = random(60, screen_height() - GAP - 60);
}

void setup()
{
    bird.x  = 200;      // do pola struktury dostajesz sie przez kropke
    bird.y  = 240;
    bird.vy = 0;
    score   = 0;
    new_pipe();
}

void draw_pipe(Pipe p)   // struktura jako parametr funkcji - jak zwykla zmienna
{
    rect(p.x, 0, PIPE_W, p.gap_y, GREEN);                                          // gorna czesc
    rect(p.x, p.gap_y + GAP, PIPE_W, screen_height() - p.gap_y - GAP, GREEN);      // dolna czesc
}

void frame()
{
    clear(SKY);

    // --- ptak: grawitacja co kilka klatek, skrzydla na A ---
    if (frame_count() % GRAVITY_EVERY == 0) {
        bird.vy = bird.vy + 1;
    }
    if (pressed(A)) {
        bird.vy = FLAP;
    }
    bird.y = bird.y + bird.vy;

    // --- rura jedzie w lewo, za ekranem pojawia sie nowa ---
    pipe.x = pipe.x - PIPE_SPEED;
    if (pipe.x + PIPE_W < 0) {
        new_pipe();
    }

    // Zderzenie z rura i ziemia - twoje zadanie 2

    // --- rysowanie ---
    draw_pipe(pipe);
    circle(bird.x, bird.y, BIRD_R, YELLOW);
    text(20, 8, score, WHITE, 3);

    watch("bird_y", bird.y);
    watch("bird_vy", bird.vy);
    watch("pipe_x", pipe.x);
}

}  // namespace

LAKE_GAME(flappy, "Flappy", "Lekcja 08: struktury")
