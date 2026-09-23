// LEKCJA 11 - FLAPPY PELNY: stan gry (enum), switch, ekran tytulowy, HUD, rekord. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int BIRD_R        = 16;
const int PIPE_W        = 80;
const int GAP           = 160;
const int PIPES         = 3;      // tyle rur naraz (tablica struktur z lekcji 09)
const int PIPE_SPACING  = 320;    // odstep miedzy rurami
const int PIPE_SPEED    = 4;
const int GRAVITY_EVERY = 2;
const int FLAP          = -10;

// ENUM: typ, ktory ma kilka nazwanych wartosci. Gra jest ZAWSZE w dokladnie jednym z tych stanow.
enum State {
    TITLE,       // ekran tytulowy, czeka na A
    PLAYING,     // gra trwa
    GAME_OVER    // ptak sie rozbil, pokazujemy wynik
};

struct Bird {
    int x;
    int y;
    int vy;
};

struct Pipe {
    int  x;
    int  gap_y;
    bool passed;
};

State state = TITLE;
Bird  bird;
Pipe  pipes[PIPES];
int   score = 0;

Pipe make_pipe(int start_x)
{
    Pipe p;
    p.x      = start_x;
    p.gap_y  = random(60, screen_height() - GAP - 60);
    p.passed = false;
    return p;
}

// Poczatek rozgrywki (nie cala gra - restart() zaczynalby od tytulu)
void start_round()
{
    bird.x  = 200;
    bird.y  = 240;
    bird.vy = 0;
    score   = 0;
    for (int i = 0; i < PIPES; i++) {
        pipes[i] = make_pipe(screen_width() + i * PIPE_SPACING);
    }
    state = PLAYING;
}

void setup()
{
    state = TITLE;
}

bool hits_pipe(Pipe p)
{
    int bx = bird.x - BIRD_R, by = bird.y - BIRD_R, bs = 2 * BIRD_R;
    if (overlaps(bx, by, bs, bs, p.x, 0, PIPE_W, p.gap_y)) return true;
    if (overlaps(bx, by, bs, bs, p.x, p.gap_y + GAP, PIPE_W, screen_height() - p.gap_y - GAP)) return true;
    return false;
}

void draw_pipe(Pipe p)
{
    rect(p.x, 0, PIPE_W, p.gap_y, GREEN);
    rect(p.x, p.gap_y + GAP, PIPE_W, screen_height() - p.gap_y - GAP, GREEN);
}

void draw_world()
{
    clear(SKY);
    for (int i = 0; i < PIPES; i++) {
        draw_pipe(pipes[i]);
    }
    circle(bird.x, bird.y, BIRD_R, YELLOW);
}

void update_playing()
{
    if (frame_count() % GRAVITY_EVERY == 0) bird.vy = bird.vy + 1;
    if (pressed(A)) bird.vy = FLAP;
    bird.y = bird.y + bird.vy;

    for (int i = 0; i < PIPES; i++) {
        pipes[i].x = pipes[i].x - PIPE_SPEED;
        if (pipes[i].x + PIPE_W < 0) {
            pipes[i] = make_pipe(pipes[i].x + PIPES * PIPE_SPACING);   // za ostatnia rura
        }
        if (pipes[i].x + PIPE_W < bird.x && !pipes[i].passed) {
            score = score + 1;
            pipes[i].passed = true;
        }
        if (hits_pipe(pipes[i])) state = GAME_OVER;
    }
    if (bird.y + BIRD_R > screen_height() || bird.y - BIRD_R < 0) state = GAME_OVER;
}

void frame()
{
    // SWITCH: wybiera galaz wedlug wartosci state. break konczy galaz.
    switch (state) {
        case TITLE:
            clear(SKY);
            text(260, 150, "FLAPPY", WHITE, 4);
            text(310, 260, "Wcisnij A", WHITE, 2);
            if (pressed(A)) start_round();
            break;

        case PLAYING:
            update_playing();
            draw_world();
            text(screen_width() / 2 - 14, 16, score, WHITE, 4);
            break;

        case GAME_OVER:
            draw_world();
            rect(160, 120, 480, 220, BLACK);
            text(230, 140, "GAME OVER", RED, 4);
            text(220, 220, "Wynik", WHITE, 3);
            text(380, 220, score, YELLOW, 3);
            text(220, 280, "A - jeszcze raz", WHITE, 2);
            if (pressed(A)) start_round();
            break;
    }

    // Stan jako napis w watch - do sledzenia w --trace
    if (state == TITLE)     watch("state", "TITLE");
    if (state == PLAYING)   watch("state", "PLAYING");
    if (state == GAME_OVER) watch("state", "GAME_OVER");
    watch("score", score);
    watch("bird_y", bird.y);
}

}  // namespace

LAKE_GAME(flappy_pelny, "Flappy pelny", "Lekcja 11: stan gry, enum")
