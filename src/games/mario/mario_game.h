// Lake Mario - platformowka 2D (pierwsza gra konsoli).
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "games/mario/mario_level.h"

class MarioGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

private:
    enum class State { Title, Playing, Dying, LevelClear, GameOver };   // pauza nalezy do konsoli

    struct Player {
        float x = 0, y = 0;      // lewy gorny rog hitboxa (px)
        float vx = 0, vy = 0;    // px/s
        bool  on_ground   = false;
        bool  facing_left = false;
        float anim_t      = 0;
    };
    struct Enemy {
        float x = 0, y = 0, vx = 0, vy = 0;
        float spawn_x = 0, spawn_y = 0;
        bool  alive    = false;
        bool  active   = false;   // aktywuje sie, gdy wejdzie w kadr
        float squash_t = 0;       // >0: zgnieciony, znika po czasie
    };
    struct Particle {
        float   x = 0, y = 0, vx = 0, vy = 0, t = 0;
        uint8_t kind = 0;         // 0 wolny, 1 moneta, 2 odlamek cegly
    };

    static constexpr int MAX_ENEMIES   = 32;
    static constexpr int MAX_PARTICLES = 48;

    // --- stan poziomu ---
    char  tiles_[mario::LEVEL_ROWS][mario::LEVEL_MAX_COLS] = {};
    int   cols_ = 0;
    float spawn_x_ = 0, spawn_y_ = 0;

    Player   player_{};
    Enemy    enemies_[MAX_ENEMIES]{};
    int      enemy_count_ = 0;
    Particle particles_[MAX_PARTICLES]{};

    // Ulatwienia sterowania skokiem - patrz JUMP_BUFFER_TIME / COYOTE_TIME w mario_game.cpp.
    float jump_buffer_ = 0;   // pamiec wcisniecia A tuz przed ladowaniem
    float coyote_      = 0;   // pozostaly czas na skok po zejsciu z krawedzi

    float cam_x_     = 0;
    State state_     = State::Title;
    float state_t_   = 0;
    float time_left_ = 0;
    int   score_ = 0, coins_ = 0, lives_ = 0;
    float anim_t_ = 0;

    // --- cykl zycia ---
    void new_game();
    void load_level();
    void reset_player();
    void reset_enemies();
    void kill_player();
    void level_clear();

    // --- mapa ---
    char tile_at(int col, int row) const;
    bool solid_at(int col, int row) const;
    void set_tile(int col, int row, char t);
    void bump_block(int col, int row);

    // --- fizyka ---
    void move_x(float& x, float y, float& vx, int w, int h, float dt, bool& hit_wall);
    bool move_y(float x, float& y, float& vy, int w, int h, float dt, int& hit_col, int& hit_row);

    // --- logika ---
    void update_player(float dt, const input::PadState& pad);
    void update_enemies(float dt);
    void update_particles(float dt);
    void update_camera(float dt);
    void collect_tiles();
    void spawn_particle(float x, float y, float vx, float vy, uint8_t kind);

    // --- rysowanie ---
    void draw_tiles(gfx::Canvas& c) const;
    void draw_entities(gfx::Canvas& c) const;
    void draw_hud(gfx::Canvas& c) const;
    void draw_overlay(gfx::Canvas& c) const;
};
