// LEKCJA 03 - LAPACZ: sterowanie klawiszami, warunki zlozone (&&, ||), else, losowanie. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int PADDLE_W = 120;   // paletka: szerokosc, wysokosc, polozenie w pionie
const int PADDLE_H = 16;
const int PADDLE_Y = 440;
const int RADIUS   = 12;    // pilka

int paddle_x = 340;
int ball_x   = 400;
int ball_y   = 0;
int speed    = 5;           // ile pikseli pilka spada co klatke
int score    = 0;

void setup()
{
    paddle_x = 340;
    ball_x   = random(RADIUS, screen_width() - RADIUS);   // losowe miejsce startu
    ball_y   = 0;
    speed    = 5;
    score    = 0;
}

void frame()
{
    clear(DARK_BLUE);

    // Sterowanie: held(klawisz) mowi, czy klawisz jest wcisniety w tej klatce
    if (held(LEFT)) {
        paddle_x = paddle_x - 8;
    }
    if (held(RIGHT)) {
        paddle_x = paddle_x + 8;
    }

    // Pilka spada
    ball_y = ball_y + speed;

    // Zlapana? Pilka jest na wysokosci paletki I (&&) miedzy jej lewym I prawym koncem
    if (ball_y + RADIUS >= PADDLE_Y && ball_x >= paddle_x && ball_x <= paddle_x + PADDLE_W) {
        score = score + 1;
        print("Zlapana! Wynik:", score);
        ball_y = 0;
        ball_x = random(RADIUS, screen_width() - RADIUS);
    }

    // Spadla poza ekran - nowa pilka
    if (ball_y > screen_height()) {
        ball_y = 0;
        ball_x = random(RADIUS, screen_width() - RADIUS);
    }

    rect(paddle_x, PADDLE_Y, PADDLE_W, PADDLE_H, WHITE);
    circle(ball_x, ball_y, RADIUS, YELLOW);
    text(20, 16, score, WHITE, 3);

    watch("paddle_x", paddle_x);
    watch("score", score);
}

}  // namespace

LAKE_GAME(lapacz, "Lapacz", "Lekcja 03: klawisze i warunki")
