// LEKCJA 04 - PONG: funkcje (wlasne klocki). Zadania w README.md.
// Gracz 1 (lewa paletka): UP / DOWN.  Gracz 2 (prawa): X (gora) / B (dol).  W emulatorze X = klawisz S, B = klawisz X.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int PADDLE_W     = 16;
const int PADDLE_H     = 96;
const int PADDLE_SPEED = 8;
const int MARGIN       = 20;    // odstep paletek od krawedzi
const int RADIUS       = 12;

int left_y  = 192;  // gorna krawedz lewej paletki
int right_y = 192;
int ball_x  = 400;
int ball_y  = 240;
int vx      = 6;
int vy      = 4;
int score_left  = 0;
int score_right = 0;

// FUNKCJA = wlasny klocek. Ma nazwe (draw_paddle), parametry w nawiasach (x, y) i cialo w klamrach.
// Piszesz ja raz, uzywasz ile chcesz - tu dwa razy, dla dwu paletek.
void draw_paddle(int x, int y)
{
    rect(x, y, PADDLE_W, PADDLE_H, WHITE);
}

// Funkcja moze ZWRACAC wynik slowem return. Ta odpowiada na pytanie: czy pilka dotyka paletki,
// ktorej lewy gorny rog jest w (px, py)? Odpowiedz jest typu bool (prawda/falsz).
bool hits_paddle(int px, int py)
{
    return overlaps(ball_x - RADIUS, ball_y - RADIUS, 2 * RADIUS, 2 * RADIUS,
                    px, py, PADDLE_W, PADDLE_H);
}

// Funkcja bez parametrow: nowa pilka na srodku, w losowa strone. W lekcji 03 ten kod byl dwa razy -
// teraz jest raz, a wolamy go, kiedy trzeba.
void new_ball()
{
    ball_x = screen_width() / 2;
    ball_y = screen_height() / 2;
    if (chance(50)) {
        vx = 6;
    } else {
        vx = -6;
    }
    vy = random(-4, 4);
}

void setup()
{
    left_y  = 192;
    right_y = 192;
    score_left  = 0;
    score_right = 0;
    new_ball();
}

void frame()
{
    clear(BLACK);

    // --- sterowanie ---
    if (held(UP))   left_y = left_y - PADDLE_SPEED;
    if (held(DOWN)) left_y = left_y + PADDLE_SPEED;
    if (held(X))    right_y = right_y - PADDLE_SPEED;
    if (held(B))    right_y = right_y + PADDLE_SPEED;
    left_y  = clamp(left_y, 0, screen_height() - PADDLE_H);
    right_y = clamp(right_y, 0, screen_height() - PADDLE_H);

    // --- pilka ---
    ball_x = ball_x + vx;
    ball_y = ball_y + vy;
    if (ball_y < RADIUS || ball_y > screen_height() - RADIUS) {
        vy = -vy;
    }

    const int right_x = screen_width() - MARGIN - PADDLE_W;   // x prawej paletki
    if (hits_paddle(MARGIN, left_y) && vx < 0) {
        vx = -vx;
    }
    if (hits_paddle(right_x, right_y) && vx > 0) {
        vx = -vx;
    }

    // --- punkty ---
    if (ball_x < 0) {
        score_right = score_right + 1;
        new_ball();
    }
    if (ball_x > screen_width()) {
        score_left = score_left + 1;
        new_ball();
    }

    // --- rysowanie ---
    draw_paddle(MARGIN, left_y);
    draw_paddle(right_x, right_y);
    circle(ball_x, ball_y, RADIUS, WHITE);
    text(330, 20, score_left, WHITE, 4);
    text(440, 20, score_right, WHITE, 4);

    watch("left_y", left_y);
    watch("right_y", right_y);
    watch("score_l", score_left);
    watch("score_r", score_right);
}

}  // namespace

LAKE_GAME(pong, "Pong", "Lekcja 04: funkcje")
