// LEKCJA 09 - BREAKOUT: tablica struktur. Zadania w README.md.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

const int BRICK_COLS = 10;
const int BRICK_ROWS = 4;
const int BRICK_W    = 72;
const int BRICK_H    = 24;
const int BRICKS     = BRICK_ROWS * BRICK_COLS;   // 40
const int PADDLE_W   = 120;
const int PADDLE_H   = 16;
const int PADDLE_Y   = 450;
const int BALL_R     = 10;

// Struktura z lekcji 08...
struct Brick {
    int   x;
    int   y;
    bool  alive;
    Color color;
};

struct Ball {
    int x;
    int y;
    int vx;
    int vy;
};

// ...i TABLICA STRUKTUR: 40 cegielek, kazda ze swoim x, y, alive i kolorem. bricks[7].alive, bricks[i].x itd.
Brick bricks[BRICKS];

Ball ball;
int  paddle_x = 340;
int  score    = 0;
int  lives    = 3;

void reset_ball()
{
    ball.x  = paddle_x + PADDLE_W / 2;
    ball.y  = PADDLE_Y - BALL_R - 1;
    ball.vx = 4;
    ball.vy = -6;
}

void setup()
{
    // Wypelnij tablice: numer cegielki i = row * BRICK_COLS + col
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            int i = row * BRICK_COLS + col;
            bricks[i].x     = 20 + col * (BRICK_W + 4);
            bricks[i].y     = 60 + row * (BRICK_H + 6);
            bricks[i].alive = true;
            if (row == 0) bricks[i].color = RED;
            if (row == 1) bricks[i].color = ORANGE;
            if (row == 2) bricks[i].color = YELLOW;
            if (row == 3) bricks[i].color = GREEN;
        }
    }
    paddle_x = 340;
    score    = 0;
    lives    = 3;
    reset_ball();
}

void frame()
{
    clear(BLACK);

    // --- paletka ---
    if (held(LEFT))  paddle_x = paddle_x - 8;
    if (held(RIGHT)) paddle_x = paddle_x + 8;
    paddle_x = clamp(paddle_x, 0, screen_width() - PADDLE_W);

    // --- pilka ---
    ball.x = ball.x + ball.vx;
    ball.y = ball.y + ball.vy;
    if (ball.x < BALL_R || ball.x > screen_width() - BALL_R) ball.vx = -ball.vx;
    if (ball.y < BALL_R) ball.vy = -ball.vy;
    // odbicie od paletki (tylko gdy pilka leci w dol)
    if (ball.vy > 0 && overlaps(ball.x - BALL_R, ball.y - BALL_R, 2 * BALL_R, 2 * BALL_R,
                                paddle_x, PADDLE_Y, PADDLE_W, PADDLE_H)) {
        ball.vy = -ball.vy;
    }
    // pilka spadla
    if (ball.y > screen_height()) {
        lives = lives - 1;
        reset_ball();
    }

    // Zderzenie z cegielkami - twoje zadanie 2

    // --- rysowanie: petla po tablicy struktur ---
    for (int i = 0; i < BRICKS; i++) {
        if (bricks[i].alive) {
            rect(bricks[i].x, bricks[i].y, BRICK_W, BRICK_H, bricks[i].color);
        }
    }
    rect(paddle_x, PADDLE_Y, PADDLE_W, PADDLE_H, WHITE);
    circle(ball.x, ball.y, BALL_R, WHITE);
    text(20, 8, score, WHITE, 3);
    text(screen_width() - 40, 8, lives, RED, 3);

    watch("score", score);
    watch("lives", lives);
    watch("ball_y", ball.y);
}

}  // namespace

LAKE_GAME(breakout, "Breakout", "Lekcja 09: tablica struktur")
