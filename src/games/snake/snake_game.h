// Snake - pelnowartosciowa gra w weza na plotnie 800x480 z grafika z Gemini (assets/snake/, PNG z alfa).
//
// Pole 25x14 kratek po 32 px pod paskiem HUD (32 px). Tryb Przygoda: 10 poziomow w 5 swiatach (laka, pustynia,
// zima, dzungla, wulkan) - na kazdym trzeba zjesc N jablek, wtedy otwiera sie portal do nastepnego. Tryb Bez konca:
// otwarte pole z zawijaniem krawedzi, swiat zmienia sie co 20 jablek. Waz przyspiesza z dlugoscia i z poziomem.
// Przedmioty: jablko, zlote jablko (znika), grzyb (skraca), klepsydra (spowolnienie), gwiazda (duch - przez
// przeszkody i siebie), serce (+zycie), magnes (przyciaga jablka), klejnot x2 (punkty), kula-tarcza (jedno
// darmowe zderzenie), bomba (od poziomu 3; zjedzona = strata zycia). Combo za szybkie jedzenie.
// Sterowanie: krzyzak (4 kierunki) albo dwa przyciski (LEWO/PRAWO = skret w lewo/prawo wzgledem glowy, jak w starych
// telefonach) - wybor na ekranie tytulowym; A trzymane = turbo, Y = autopilot (demo i testy skryptowe).
//
// Logika w snake_game.cpp (plansze, ruch, przedmioty, autopilot), rysowanie w snake_render.cpp.
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "gfx/canvas.h"
#include "gfx/png.h"

namespace snake {

// Obraz z alfa (RGB565 + krycie) - wlasne kopie obroconych glow i kulek ciala generowanych w kodzie.
struct Pic {
    int             w = 0, h = 0;
    const uint16_t* px    = nullptr;
    const uint8_t*  alpha = nullptr;
};

class SnakeGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

    static constexpr int W = 800, H = 480;
    static constexpr int CELL = 32, COLS = 25, ROWS = 14;
    static constexpr int FIELD_Y = H - ROWS * CELL;   // 32 - pod paskiem HUD
    static constexpr int FIELD_W = COLS * CELL, FIELD_H = ROWS * CELL;
    static constexpr int MAX_LEN = COLS * ROWS;

    enum class State { Title, Intro, Playing, Dying, LevelClear, GameOver, Victory };
    enum class Mode { Adventure, Endless };
    enum Item : uint8_t { APPLE, GOLDEN, MUSHROOM, HOURGLASS, STAR, HEART, MAGNET, GEM, ORB, BOMB, ITEM_COUNT };
    enum Dir : uint8_t { RIGHT, DOWN, LEFT, UP };

private:
    struct Cell   { int8_t x, y; };
    struct Pickup { Item type; int8_t x, y; float fx, fy, life, age; bool alive; };
    struct Part   { float x, y, vx, vy, t, t0; uint16_t color; uint8_t size; bool alive; };
    struct Popup  { float x, y, t; char text[12]; uint16_t color; bool alive; };
    struct Score  { int score; int level; uint8_t mode; };

    static constexpr int MAX_PICKUPS = 10, MAX_PARTS = 160, MAX_POPUPS = 10, QUEUE = 3, TOP = 5;

    // --- stan gry ---
    State state_ = State::Title;
    Mode  mode_  = Mode::Adventure;
    int   skin_  = 0;          // 0 zielony, 1 niebieski, 2 pomaranczowy
    int   title_row_ = 0;      // wybrany wiersz na ekranie tytulowym (tryb / waz / sterowanie)
    bool  two_buttons_ = false;   // sterowanie dwoma przyciskami: LEWO/PRAWO skreca wzgledem kierunku jazdy
    int   level_ = 0;          // 0..9 (Przygoda)
    int   start_level_ = 0;    // wybor na ekranie tytulowym (X)
    int   world_ = 0;          // 0..4 - tlo i przeszkody
    bool  wrap_  = false;      // krawedzie pola zawijaja (inaczej sciana)
    uint8_t solid_[ROWS][COLS]{};   // 0 wolne, 1..3 przeszkoda (wariant grafiki)

    Cell  body_[MAX_LEN]{};    // [0] = glowa
    Cell  prev_[MAX_LEN]{};    // pozycje przed ostatnim krokiem (plynna interpolacja)
    int   len_ = 0, grow_ = 0;
    Dir   dir_ = RIGHT, last_dir_ = RIGHT;
    Dir   queue_[QUEUE]{};
    int   queued_ = 0;
    bool  held_prev_[4]{};
    float step_acc_ = 0;       // czas od ostatniego kroku
    float step_int_ = 0.15f;   // aktualny odstep krokow (do rysowania)
    Cell  start_{ 6, 7 };

    Pickup pickups_[MAX_PICKUPS]{};
    Part   parts_[MAX_PARTS]{};
    Popup  popups_[MAX_POPUPS]{};

    int   score_ = 0, lives_ = 3, apples_ = 0, apples_total_ = 0, need_ = 10;
    bool  portal_open_ = false;
    Cell  portal_{ 0, 0 };
    float level_time_ = 0, state_t_ = 0, anim_ = 0, bonus_t_ = 0, bomb_t_ = 0;
    float slow_t_ = 0, ghost_t_ = 0, magnet_t_ = 0, x2_t_ = 0, flash_t_ = 0;
    bool  shield_ = false;
    int   combo_ = 0;
    float combo_t_ = 0;
    int   dying_i_ = 0;        // ile segmentow juz "wybuchlo" przy smierci
    bool  autopilot_ = false;
    int   clear_bonus_ = 0;
    char  banner_[40] = "";    // krotki napis na srodku (np. "SPOWOLNIENIE")
    float banner_t_ = 0;
    Score top_[TOP]{};
    int   last_rank_ = -1;

    // --- grafika ---
    bool  assets_loaded_ = false;
    int   baked_world_ = -1, baked_level_ = -2;
    uint16_t* bg_   = nullptr;   // tlo swiata (surowe, FIELD_W x FIELD_H)
    uint16_t* bake_ = nullptr;   // tlo + siatka + przeszkody z cieniami (kopiowane co klatke)
    uint8_t*  mask_ = nullptr;   // maska cienia weza
    Pic   item_[ITEM_COUNT]{};
    Pic   icon_[ITEM_COUNT]{};
    Pic   obst_[5][3]{};         // swiat x wariant
    Pic   head_[3][4]{};         // skorka x kierunek
    Pic   ball_[3][2][8]{};      // skorka x ton (pasek) x promien 7..14
    Pic   dark_[8]{};            // ciemna kulka obrysu, promien 9..16
    Pic   portal_img_[8]{};      // portal w 8 fazach obrotu
    gfx::Image title_img_{};

    // logika (snake_game.cpp)
    void start_game();
    void load_level();
    void reset_snake();
    void update_title(const input::PadState& pad);
    void update_playing(float dt, const input::PadState& pad);
    void read_input(const input::PadState& pad);
    void step();
    void crash();
    void eat(Pickup& p);
    void spawn_item(Item type, float life);
    void spawn_bonus();
    bool random_free_cell(int& x, int& y, int margin_from_head);
    bool cell_blocked(int x, int y, bool for_spawn) const;
    bool snake_at(int x, int y, bool skip_tail) const;
    Pickup* pickup_at(int x, int y);
    void wrap_cell(int& x, int& y) const;
    float step_interval(bool turbo) const;
    Dir  autopilot_dir();
    void open_portal();
    void burst(float x, float y, uint16_t color, int count, float speed);
    void popup(float x, float y, const char* text, uint16_t color);
    void add_score(int pts, int cx, int cy);
    void submit_score();
    void set_banner(const char* s);
    const char* world_name(int w) const;

    // rysowanie (snake_render.cpp)
    void load_assets();
    void bake_field();
    void draw_field(gfx::Canvas& c);
    void draw_pickups(gfx::Canvas& c);
    void draw_portal(gfx::Canvas& c);
    void draw_snake(gfx::Canvas& c);
    void draw_effects(gfx::Canvas& c);
    void draw_hud(gfx::Canvas& c);
    void draw_title(gfx::Canvas& c);
    void draw_overlay(gfx::Canvas& c);
    void seg_pos(int i, float& x, float& y) const;
};

}  // namespace snake
