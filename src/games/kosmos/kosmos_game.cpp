#include "games/kosmos/kosmos_game.h"

#include <math.h>
#include <stdio.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "engine/rng.h"
#include "gfx/palette.h"
#include "gfx/png.h"
#include "gfx/text.h"

namespace kosmos {

namespace {

const char* TAG = "kosmos";

constexpr float SHIP_SPEED   = 300.f;   // px/s
constexpr float BULLET_SPEED = 700.f;
constexpr float FIRE_DELAY   = 0.18f;
constexpr float STAR_SPEED[3] = { 25.f, 60.f, 130.f };
const uint16_t  STAR_COLOR[3] = { gfx::rgb565(90, 95, 120), gfx::rgb565(160, 165, 190), gfx::rgb565(235, 235, 255) };
constexpr int   ASTEROID_PX[3] = { 24, 40, 64 };
constexpr int   ASTEROID_SCORE[3] = { 30, 20, 10 };

float frand(float a, float b) { return a + engine::rng().unit() * (b - a); }

}  // namespace

// ============================================================================ cykl zycia

void KosmosGame::init(gfx::Canvas&)
{
    if (!assets_loaded_) {
        assets_loaded_ = true;
        ship_        = gfx::load_png("kosmos/ship.png");
        asteroid_[0] = gfx::load_png("kosmos/asteroid_s.png");
        asteroid_[1] = gfx::load_png("kosmos/asteroid_m.png");
        asteroid_[2] = gfx::load_png("kosmos/asteroid_l.png");
        bullet_      = gfx::load_png("kosmos/bullet.png");
        for (int i = 0; i < 4; ++i) {
            char name[32];
            snprintf(name, sizeof(name), "kosmos/boom%d.png", i);
            boom_[i] = gfx::load_png(name);
        }
        CONSOLE_LOGI(TAG, "grafika: %s", ship_.px ? "PNG z assets/kosmos" : "BRAK - figury zastepcze");
    }
    for (int i = 0; i < STARS; ++i) {
        stars_[i].x     = frand(0, W);
        stars_[i].y     = frand(0, H);
        stars_[i].layer = (uint8_t)(i % 3);
    }
    state_ = State::Title;
    new_round();
}

void KosmosGame::new_round()
{
    ship_x_ = 120;
    ship_y_ = H / 2;
    score_  = 0;
    lives_  = 3;
    fire_cd_ = 0;
    spawn_t_ = 0.5f;
    inv_t_   = 0;
    difficulty_ = 1.f;
    for (Bullet& b : bullets_) b.alive = false;
    for (Asteroid& a : asteroids_) a.alive = false;
    for (Boom& b : booms_) b.alive = false;
    for (Spark& s : sparks_) s.alive = false;
}

void KosmosGame::spawn_asteroid(int size, float x, float y)
{
    for (Asteroid& a : asteroids_) {
        if (a.alive) continue;
        a.alive = true;
        a.size  = size;
        a.x     = x;
        a.y     = y;
        a.vx    = -frand(70.f, 140.f) * (1.f + 0.15f * (2 - size)) * difficulty_;
        a.vy    = frand(-40.f, 40.f);
        return;
    }
}

void KosmosGame::explode(float x, float y, int count)
{
    for (Boom& b : booms_) {
        if (b.alive) continue;
        b.alive = true; b.x = x; b.y = y; b.t = 0;
        break;
    }
    for (int i = 0; i < count; ++i) {
        for (Spark& s : sparks_) {
            if (s.alive) continue;
            const float a = frand(0, 6.2832f), v = frand(60.f, 260.f);
            s.alive = true; s.x = x; s.y = y; s.vx = cosf(a) * v; s.vy = sinf(a) * v; s.t = frand(0.3f, 0.7f);
            break;
        }
    }
}

void KosmosGame::fire()
{
    for (Bullet& b : bullets_) {
        if (b.alive) continue;
        b.alive = true;
        b.x     = ship_x_ + 44;
        b.y     = ship_y_ + 14;
        fire_cd_ = FIRE_DELAY;
        return;
    }
}

// ============================================================================ logika

void KosmosGame::update(float dt, const input::PadState& pad)
{
    anim_ += dt;
    // Gwiazdy przesuwaja sie zawsze - tlo zyje takze na tytule
    for (Star& s : stars_) {
        s.x -= STAR_SPEED[s.layer] * dt;
        if (s.x < 0) { s.x += W; s.y = frand(0, H); }
    }

    switch (state_) {
        case State::Title:
            if (pad.a_pressed || pad.b_pressed || (pad.any_pressed && !pad.start_pressed)) {
                new_round();
                state_ = State::Playing;
            }
            break;
        case State::Playing:
            update_playing(dt, pad);
            break;
        case State::GameOver:
            for (Boom& b : booms_) if (b.alive) { b.t += dt; if (b.t > 0.6f) b.alive = false; }
            for (Spark& s : sparks_) if (s.alive) { s.x += s.vx * dt; s.y += s.vy * dt; s.t -= dt; if (s.t <= 0) s.alive = false; }
            if (pad.a_pressed) { new_round(); state_ = State::Playing; }
            break;
    }
}

void KosmosGame::update_playing(float dt, const input::PadState& pad)
{
    // --- statek ---
    float mx = 0, my = 0;
    if (pad.left)  mx -= 1;
    if (pad.right) mx += 1;
    if (pad.up)    my -= 1;
    if (pad.down)  my += 1;
    ship_x_ = engine::clampf(ship_x_ + mx * SHIP_SPEED * dt, 0, W - 48);
    ship_y_ = engine::clampf(ship_y_ + my * SHIP_SPEED * dt, 0, H - 32);
    if (fire_cd_ > 0) fire_cd_ -= dt;
    if (pad.a && fire_cd_ <= 0) fire();
    if (inv_t_ > 0) inv_t_ -= dt;

    // iskry z silnika
    if (fmodf(anim_, 0.03f) < dt) {
        for (Spark& s : sparks_) {
            if (s.alive) continue;
            s.alive = true; s.x = ship_x_ + 2; s.y = ship_y_ + 16 + frand(-3, 3);
            s.vx = -frand(120.f, 220.f) - mx * 60.f; s.vy = frand(-25.f, 25.f); s.t = frand(0.15f, 0.3f);
            break;
        }
    }

    // --- pociski ---
    for (Bullet& b : bullets_) {
        if (!b.alive) continue;
        b.x += BULLET_SPEED * dt;
        if (b.x > W) b.alive = false;
    }

    // --- asteroidy ---
    difficulty_ += dt * 0.012f;
    spawn_t_ -= dt;
    if (spawn_t_ <= 0) {
        spawn_t_ = frand(0.5f, 1.1f) / difficulty_;
        const int size = engine::rng().range(0, 9) < 4 ? 2 : (engine::rng().range(0, 1) ? 1 : 0);
        spawn_asteroid(size, W + 40, frand(20, H - 60));
    }
    for (Asteroid& a : asteroids_) {
        if (!a.alive) continue;
        a.x += a.vx * dt;
        a.y += a.vy * dt;
        const int s = ASTEROID_PX[a.size];
        if (a.y < -s) a.y = H;
        if (a.y > H) a.y = -s;
        if (a.x < -s) { a.alive = false; continue; }

        // trafienie pociskiem
        for (Bullet& b : bullets_) {
            if (!b.alive) continue;
            if (engine::overlap(b.x, b.y, 12, 4, a.x + s * 0.12f, a.y + s * 0.12f, (int)(s * 0.76f), (int)(s * 0.76f))) {
                // Kopie przed spawn_asteroid: moze ono ponownie uzyc TEGO slotu `a` i nadpisac size/x/y.
                const int   size = a.size;
                const float cx = a.x + s / 2, cy = a.y + s / 2;
                b.alive = false;
                a.alive = false;
                score_ += ASTEROID_SCORE[size];
                explode(cx, cy, 6 + size * 4);
                if (size > 0) {   // rozpad na dwie mniejsze
                    spawn_asteroid(size - 1, cx - 10, cy - 14);
                    spawn_asteroid(size - 1, cx - 10, cy + 6);
                }
                break;
            }
        }
        if (!a.alive) continue;

        // zderzenie ze statkiem (hitbox statku nieco mniejszy niz obrazek)
        if (inv_t_ <= 0 &&
            engine::overlap(ship_x_ + 6, ship_y_ + 8, 34, 16, a.x + s * 0.15f, a.y + s * 0.15f, (int)(s * 0.7f), (int)(s * 0.7f))) {
            a.alive = false;
            explode(ship_x_ + 24, ship_y_ + 16, 16);
            --lives_;
            inv_t_ = 1.5f;   // chwila nietykalnosci po trafieniu
            if (lives_ <= 0) {
                if (score_ > best_) best_ = score_;
                state_ = State::GameOver;
            }
        }
    }

    // --- efekty ---
    for (Boom& b : booms_) if (b.alive) { b.t += dt; if (b.t > 0.4f) b.alive = false; }
    for (Spark& s : sparks_) {
        if (!s.alive) continue;
        s.x += s.vx * dt; s.y += s.vy * dt; s.t -= dt;
        if (s.t <= 0) s.alive = false;
    }
}

// ============================================================================ rysowanie

void KosmosGame::draw_world(gfx::Canvas& c)
{
    c.clear(gfx::rgb565(6, 8, 18));
    for (const Star& s : stars_) {
        if (s.layer == 2) c.fill_rect((int)s.x, (int)s.y, 2, 2, STAR_COLOR[2]);
        else              c.put((int)s.x, (int)s.y, STAR_COLOR[s.layer]);
    }
    for (const Spark& s : sparks_) {
        if (!s.alive) continue;
        const uint16_t col = s.t > 0.2f ? gfx::rgb565(255, 200, 80) : gfx::rgb565(220, 80, 30);
        c.fill_rect((int)s.x, (int)s.y, 3, 3, col);
    }
    for (const Asteroid& a : asteroids_) {
        if (!a.alive) continue;
        if (asteroid_[a.size].px) c.blit(asteroid_[a.size], (int)a.x, (int)a.y);
        else c.fill_circle((int)a.x + ASTEROID_PX[a.size] / 2, (int)a.y + ASTEROID_PX[a.size] / 2, ASTEROID_PX[a.size] / 2, gfx::pal::GRAY);
    }
    for (const Bullet& b : bullets_) {
        if (!b.alive) continue;
        if (bullet_.px) c.blit(bullet_, (int)b.x, (int)b.y);
        else c.fill_rect((int)b.x, (int)b.y, 12, 4, gfx::pal::YELLOW);
    }
    if (state_ != State::GameOver && (inv_t_ <= 0 || fmodf(anim_, 0.16f) < 0.08f)) {   // miganie po trafieniu
        if (ship_.px) c.blit(ship_, (int)ship_x_, (int)ship_y_);
        else c.fill_rect((int)ship_x_, (int)ship_y_, 48, 32, gfx::pal::WHITE);
    }
    for (const Boom& b : booms_) {
        if (!b.alive) continue;
        const int frame = engine::iclamp((int)(b.t / 0.1f), 0, 3);
        if (boom_[frame].px) c.blit_scaled(boom_[frame], (int)b.x - 48, (int)b.y - 48, 2);
        else c.fill_circle((int)b.x, (int)b.y, 10 + frame * 8, gfx::pal::ORANGE);
    }
}

void KosmosGame::draw_hud(gfx::Canvas& c)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "%d", score_);
    gfx::draw_text_px(c, 20, 12, buf, gfx::pal::WHITE, 32);
    for (int i = 0; i < lives_; ++i) {
        c.fill_rect(W - 30 - i * 22, 18, 16, 6, gfx::pal::RED);
        c.fill_rect(W - 30 - i * 22 + 4, 14, 8, 14, gfx::pal::RED);
    }

    auto center = [&](int y, const char* s, uint16_t col, int px) {
        gfx::draw_text_px(c, (W - gfx::text_width_px(s, px)) / 2, y, s, col, px);
    };
    if (state_ == State::Title) {
        c.fill_rect(0, 150, W, 190, gfx::rgb565(6, 8, 18));
        center(160, "KOSMOS", gfx::pal::WHITE, 48);
        center(230, "Strzalki - lot,  A - strzal.  Rozbijaj asteroidy, unikaj zderzen.", gfx::pal::GRAY, 16);
        if (fmodf(anim_, 1.f) < 0.6f) center(280, "Wcisnij A", gfx::pal::WHITE, 24);
        if (best_ > 0) {
            snprintf(buf, sizeof(buf), "Rekord %d", best_);
            center(320, buf, gfx::pal::YELLOW, 16);
        }
    } else if (state_ == State::GameOver) {
        c.fill_rect(200, 150, 400, 180, gfx::rgb565(6, 8, 18));
        c.draw_rect(200, 150, 400, 180, gfx::rgb565(60, 70, 100));
        center(170, "Koniec gry", gfx::pal::RED, 32);
        snprintf(buf, sizeof(buf), "Wynik %d    Rekord %d", score_, best_);
        center(230, buf, gfx::pal::WHITE, 20);
        center(280, "A - jeszcze raz", gfx::pal::GRAY, 16);
    }
}

void KosmosGame::render(gfx::Canvas& c)
{
    draw_world(c);
    draw_hud(c);
}

void KosmosGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "PLAY", "OVER" };
    int asteroids = 0, bullets = 0;
    for (const Asteroid& a : asteroids_) if (a.alive) ++asteroids;
    for (const Bullet& b : bullets_) if (b.alive) ++bullets;
    snprintf(buf, n, "%-5s ship=%.0f,%.0f score=%d lives=%d asteroids=%d bullets=%d", NAMES[(int)state_], (double)ship_x_,
             (double)ship_y_, score_, lives_, asteroids, bullets);
}

}  // namespace kosmos
