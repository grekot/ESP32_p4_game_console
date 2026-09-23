// LEKCJA 12 - MINI MARIO: mapa kafelkow, grawitacja, kolizje z mapa, kamera. Zadania w README.md.
// Sterowanie: LEFT/RIGHT chod, A skok (zadanie 2).
#include "console/console.h"

namespace {   // pudelko na twoja gre - nie usuwaj

// MAPA: kazdy wiersz to rzad kafelkow 32x32 px (15 wierszy = 480 px = caly ekran w pionie). Znaki:
//   '#' trawa (stala)  '=' ziemia (stala)  'B' blok (staly)  'o' moneta  ' ' powietrze
// Mapa ma 50 kolumn = 1600 px, czyli dwa ekrany - stad kamera.
const char* const MAP[15] = {
    "                                                  ",
    "                                                  ",
    "                                                  ",
    "                                                  ",
    "                                                  ",
    "                                                  ",
    "                       o                          ",
    "                      BBB                         ",
    "             o                       o   o        ",
    "            BBB                     BBBBBB        ",
    "                                                  ",
    "                                                  ",
    "        o   o                             BB      ",
    "##############################   #################",
    "==============================   =================",
};

const int PLAYER_W  = 24;
const int PLAYER_H  = 32;
const int WALK      = 4;      // px na klatke
const int JUMP_V    = -13;    // predkosc skoku (w gore = ujemna)
const int MAX_FALL  = 16;

struct Player {
    int  x;
    int  y;
    int  vx;
    int  vy;
    bool on_ground;
};

Player p;
int    cam_x = 0;
int    coins = 0;

void setup()
{
    load_map(MAP, "#=B");      // drugi argument: znaki, ktore sa stale (sciany i podloga)
    p.x = 2 * TILE;            // start na 2. kolumnie, tuz nad trawa (wiersz 13)
    p.y = 13 * TILE - PLAYER_H;
    p.vx = 0;
    p.vy = 0;
    p.on_ground = false;
    cam_x = 0;
    coins = 0;
}

void frame()
{
    clear(SKY);

    // --- sterowanie: predkosc pozioma ---
    p.vx = 0;
    if (held(LEFT))  p.vx = -WALK;
    if (held(RIGHT)) p.vx = WALK;

    // Skok - twoje zadanie 2 (pressed(A) && p.on_ground -> p.vy = JUMP_V)

    // --- grawitacja ---
    p.vy = p.vy + 1;
    if (p.vy > MAX_FALL) p.vy = MAX_FALL;

    // --- ruch z kolizjami: move_box przesuwa (x, y) o (vx, vy) i zatrzymuje na kafelkach ---
    MoveResult r = move_box(p.x, p.y, PLAYER_W, PLAYER_H, p.vx, p.vy);
    p.on_ground = r.on_ground;

    // Monety - twoje zadanie 3 (map_tile / map_set na kafelku pod srodkiem gracza)

    // --- kamera: gracz w 40% szerokosci ekranu, ale nie poza mapa ---
    cam_x = clamp(p.x - screen_width() * 2 / 5, 0, map_width() - screen_width());

    // --- rysowanie mapy: kafelki wedlug znaku, przesuniete o kamere ---
    draw_tiles('#', DARK_GREEN, cam_x);
    draw_tiles('=', BROWN, cam_x);
    draw_tiles('B', ORANGE, cam_x);
    draw_tiles('o', YELLOW, cam_x);

    rect(p.x - cam_x, p.y, PLAYER_W, PLAYER_H, RED);
    text(20, 8, coins, WHITE, 3);

    watch("x", p.x);
    watch("y", p.y);
    watch("vy", p.vy);
    watch("ground", p.on_ground);
    watch("coins", coins);
}

}  // namespace

CONSOLE_ADD_GAME(mini_mario, "Mini Mario", "Lekcja 12: mapa kafelkow")
