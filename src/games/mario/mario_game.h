// Lake Mario - platformowka 2D (pierwsza gra konsoli).
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "engine/particles.h"
#include "engine/tilemap.h"
#include "games/mario/mario_level.h"

namespace mario {

class MarioGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    int  canvas_scale() const override { return 2; }   // pixel-art 400x240 powiekszany x2
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
    static constexpr int MAX_ENEMIES   = 32;
    static constexpr int MAX_PARTICLES = 48;   // czasteczki: kind 1 = moneta, 2 = odlamek cegly

    // --- stan poziomu ---
    engine::TileMap map_;                 // kafelki + kolizje + rysowanie (engine/tilemap.h)
    float spawn_x_ = 0, spawn_y_ = 0;

    Player   player_{};
    Enemy    enemies_[MAX_ENEMIES]{};
    int      enemy_count_ = 0;
    engine::ParticlePool<MAX_PARTICLES> particles_;

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

    // --- mapa (skroty do map_) ---
    char tile_at(int col, int row) const { return map_.tile_at(col, row); }
    void set_tile(int col, int row, char t) { map_.set_tile(col, row, t); }
    void bump_block(int col, int row);

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

}  // namespace mario
