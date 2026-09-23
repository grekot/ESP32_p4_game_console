// Kosmos - strzelanka 2D w pelnej rozdzielczosci 800x480: statek i asteroidy z PNG (assets/kosmos/),
// gwiazdy w trzech warstwach paralaksy, wybuchy animowane klatkami PNG, czasteczki silnika.
// Pokazuje, jak wyglada gra na pelnym silniku (engine::Game, fizyka na dt) z grafika z plikow.
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "gfx/canvas.h"

namespace kosmos {

class KosmosGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

private:
    enum class State { Title, Playing, GameOver };

    struct Bullet   { float x, y; bool alive; };
    struct Asteroid { float x, y, vx, vy; int size; bool alive; };   // size: 0 maly, 1 sredni, 2 duzy
    struct Boom     { float x, y, t; bool alive; };
    struct Spark    { float x, y, vx, vy, t; bool alive; };
    struct Star     { float x, y; uint8_t layer; };

    static constexpr int W = 800, H = 480;
    static constexpr int MAX_BULLETS = 16, MAX_ASTEROIDS = 24, MAX_BOOMS = 8, MAX_SPARKS = 64, STARS = 120;

    State    state_ = State::Title;
    float    ship_x_ = 120, ship_y_ = 240;
    float    fire_cd_ = 0, spawn_t_ = 0, anim_ = 0, inv_t_ = 0;
    int      score_ = 0, best_ = 0, lives_ = 3;
    float    difficulty_ = 1.f;
    Bullet   bullets_[MAX_BULLETS]{};
    Asteroid asteroids_[MAX_ASTEROIDS]{};
    Boom     booms_[MAX_BOOMS]{};
    Spark    sparks_[MAX_SPARKS]{};
    Star     stars_[STARS]{};

    gfx::Sprite ship_, asteroid_[3], bullet_, boom_[4];
    bool        assets_loaded_ = false;

    void new_round();
    void spawn_asteroid(int size, float x, float y);
    void explode(float x, float y, int count);
    void fire();
    void update_playing(float dt, const input::PadState& pad);
    void draw_world(gfx::Canvas& c);
    void draw_hud(gfx::Canvas& c);
};

}  // namespace kosmos
