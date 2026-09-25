// Space Invaders - logika: formacja, nurkowania, pociski, bunkry, UFO, boss, bonusy, punkty, autopilot.
#include "games/invaders/invaders_game.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "engine/rng.h"
#include "engine/storage.h"

namespace invaders {

namespace {

const char* TAG = "invaders";

using G = InvadersGame;

constexpr int W = G::W, H = G::H, FCOLS = G::FCOLS, FROWS = G::FROWS, ALIENS = G::ALIENS;
constexpr int CELL_W = G::CELL_W, CELL_H = G::CELL_H;

constexpr float READY_TIME = 2.0f, DIE_TIME = 1.7f, CLEAR_TIME = 2.6f, INVULN_TIME = 2.2f;
constexpr float SHIP_SPEED = 330.f, SHIP_MIN_X = 36.f, SHIP_MAX_X = (float)W - 36.f;
constexpr float BOLT_SPEED = 640.f, LASER_SPEED = 820.f;
constexpr float FIRE_CD = 0.27f, LASER_CD = 0.085f;
constexpr float EDGE = 14.f, DROP = 16.f, DROP_SPEED = 90.f;
constexpr float HALF_A = 20.f;                   // polowa hitboxu obcego (sprite 44)
constexpr float SHIP_HW = 22.f, SHIP_HH = 20.f;  // polowa hitboxu statku
constexpr float PICKUP_SPEED = 100.f;
constexpr float COMBO_WINDOW = 0.7f;
constexpr float PW_DURATION[G::POWERS] = { 12.f, 7.f, 25.f, 8.f, 0.f };
constexpr int   ALIEN_PTS[G::TYPES] = { 40, 20, 10, 30 };   // kalmar, krab, osmiornica, meduza
constexpr int   ROW_TYPE[G::FROWS] = { 0, 3, 1, 1, 2 };     // typ obcego w wierszu formacji
constexpr int   UFO_PTS[4] = { 50, 100, 150, 300 };
constexpr int   MAX_START_LEVEL = 10;
constexpr float UFO_Y = 62.f;

// kolory iskier wg typu obcego (kalmar fiolet, krab zielen, osmiornica pomarancz, meduza blekit)
constexpr uint16_t TYPE_COLOR[G::TYPES] = { gfx::rgb565(200, 110, 255), gfx::rgb565(110, 240, 110),
                                            gfx::rgb565(255, 150, 50), gfx::rgb565(90, 230, 255) };
constexpr uint16_t COL_GOLD = gfx::rgb565(255, 214, 60);

inline float frand(float a, float b) { return a + (b - a) * engine::rng().unit(); }
inline bool is_boss_level(int lvl) { return lvl % 5 == 0; }

}  // namespace

// ============================================================================ pomocnicze

float InvadersGame::alien_x(int i) const
{
    if (alien_[i].mode != IN_FORMATION) return alien_[i].x;
    return fx_ + (float)(i % FCOLS) * CELL_W + CELL_W * 0.5f;
}

float InvadersGame::alien_y(int i) const
{
    if (alien_[i].mode != IN_FORMATION) return alien_[i].y;
    return fy_ + (float)(i / FCOLS) * CELL_H + CELL_H * 0.5f;
}

float InvadersGame::formation_speed() const
{
    const float base = 26.f + 4.f * (float)engine::imin(level_ - 1, 10);
    const float gone = total_ > 0 ? 1.f - (float)alive_ / (float)total_ : 0.f;
    return base * (1.f + 4.f * gone * gone);
}

void InvadersGame::popup(float x, float y, const char* text, uint16_t color)
{
    for (Popup& p : popup_) {
        if (p.alive) continue;
        p.alive = true;
        p.x = x;
        p.y = y;
        p.t = 1.0f;
        p.color = color;
        snprintf(p.text, sizeof(p.text), "%s", text);
        return;
    }
}

void InvadersGame::add_score(int pts, float x, float y)
{
    score_ += pts;
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", pts);
    popup(x, y, buf, combo_ > 1 ? COL_GOLD : gfx::rgb565(235, 240, 255));
    if (score_ >= next_life_) {
        next_life_ += 30000;
        ++lives_;
        popup(ship_x_, (float)G::SHIP_Y - 40.f, "+1 ZYCIE", gfx::rgb565(120, 255, 140));
    }
}

void InvadersGame::save_record()
{
    if (score_ <= top_.score) return;
    top_ = { 1, score_, level_ };
    engine::save_data("invaders_top", &top_, sizeof(top_));
}

void InvadersGame::explode(float x, float y, float scale, uint16_t color, int sparks)
{
    for (Boom& b : boom_) {
        if (b.alive) continue;
        b.alive = true;
        b.x = x;
        b.y = y;
        b.t = 0;
        b.dur = 0.42f + 0.1f * scale;
        b.scale = scale;
        break;
    }
    for (int k = 0; k < sparks; ++k) {
        for (Spark& s : spark_) {
            if (s.alive) continue;
            const float a = frand(0.f, 6.2832f), v = frand(60.f, 260.f) * (0.6f + 0.4f * scale);
            s.alive = true;
            s.x = x;
            s.y = y;
            s.vx = cosf(a) * v;
            s.vy = sinf(a) * v;
            s.max = s.life = frand(0.35f, 0.8f);
            s.color = (k & 3) == 0 ? gfx::rgb565(255, 250, 220) : color;
            break;
        }
    }
}

// ============================================================================ start / poziom

void InvadersGame::init(gfx::Canvas&)
{
    Record saved{};
    if (engine::load_data("invaders_top", &saved, sizeof(saved)) && saved.version == 1) top_ = saved;
    else top_ = { 1, 0, 0 };
    // gwiazdy paralaksy: wlasny prosty generator (nie ruszamy engine::rng - ciag gry zalezy tylko od rozgrywki)
    uint32_t s = 12345u;
    for (Star& st : star_) {
        s = s * 1664525u + 1013904223u;
        st.x = (int16_t)((s >> 8) % W);
        s = s * 1664525u + 1013904223u;
        st.y = (int16_t)((s >> 8) % H);
        s = s * 1664525u + 1013904223u;
        st.layer = (uint8_t)((s >> 12) % 3);
        st.phase = (uint8_t)(s >> 24);
    }
    load_assets();
    state_ = State::Title;
    state_t_ = 0;
    CONSOLE_LOGI(TAG, "Space Invaders gotowe, rekord %d", top_.score);
}

void InvadersGame::start_game()
{
    score_ = 0;
    lives_ = 3;
    next_life_ = 20000;
    level_ = start_level_;
    kills_ = 0;
    for (float& t : pw_time_) t = 0;
    load_level();
}

void InvadersGame::reset_bunkers()
{
    for (int b = 0; b < BUNKERS; ++b) {
        if (!bunker_mask_[b]) continue;
        if (bunker_img_.alpha && bunker_img_.w == BUNKER_W && bunker_img_.h == BUNKER_H) {
            memcpy(bunker_mask_[b], bunker_img_.alpha, (size_t)BUNKER_W * BUNKER_H);
        } else {
            // bez PNG: luk z prostokata ze scietymi rogami
            for (int y = 0; y < BUNKER_H; ++y)
                for (int x = 0; x < BUNKER_W; ++x) {
                    const bool cut = (y < 12 && (x < 12 - y || x > BUNKER_W - 13 + y)) || (y > 36 && x > 34 && x < BUNKER_W - 34);
                    bunker_mask_[b][y * BUNKER_W + x] = cut ? 0 : 255;
                }
        }
    }
}

void InvadersGame::load_level()
{
    world_ = (level_ - 1) % WORLDS;
    boss_level_ = is_boss_level(level_);
    for (Alien& a : alien_) a = Alien{};
    total_ = 0;
    const int rows = boss_level_ ? 1 : FROWS;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < FCOLS; ++c) {
            Alien& a = alien_[r * FCOLS + c];
            a.alive = true;
            a.type = (uint8_t)(boss_level_ ? 3 : ROW_TYPE[r]);
            ++total_;
        }
    }
    alive_ = total_;
    fx_ = (float)(W - FCOLS * CELL_W) / 2.f;
    fy_ = boss_level_ ? 190.f : 64.f + 10.f * (float)engine::imin((level_ - 1) % 5, 4);
    fdir_ = 1;
    fdrop_ = 0;
    fanim_t_ = 0;
    fframe_ = 0;
    bomb_t_ = 1.2f;
    dive_t_ = 5.f;
    for (Shot& s : bolt_) s.alive = false;
    for (Shot& s : bomb_) s.alive = false;
    for (Pickup& p : pickup_) p.alive = false;
    for (Popup& p : popup_) p.alive = false;
    ufo_alive_ = false;
    ufo_t_ = frand(12.f, 20.f);
    boss_alive_ = boss_level_;
    boss_hp_ = boss_max_ = 40 + 15 * (level_ / 5 - 1);
    boss_t_ = 0;
    boss_fire_t_ = 2.5f;
    boss_flash_ = 0;
    boss_die_t_ = 0;
    boss_x_ = W / 2;
    boss_y_ = 128;
    combo_ = 0;
    combo_t_ = 0;
    invuln_ = 0;
    level_time_ = 0;
    ship_x_ = W / 2;
    reset_bunkers();
    load_background();
    state_ = State::Ready;
    state_t_ = 0;
    CONSOLE_LOGI(TAG, "fala %d: obcych %d, swiat %d%s", level_, total_, world_, boss_level_ ? ", BOSS" : "");
}

// ============================================================================ petla

void InvadersGame::update(float dt, const input::PadState& pad)
{
    anim_ += dt;
    update_effects(dt);

    switch (state_) {
    case State::Title:
        update_title(pad);
        break;
    case State::Ready: {
        state_t_ += dt;
        if (pad.y_pressed) autopilot_ = !autopilot_;
        float mv = 0;
        if (pad.left) mv -= 1;
        if (pad.right) mv += 1;
        if (autopilot_) mv = (float)autopilot_move();
        ship_x_ = engine::clampf(ship_x_ + mv * SHIP_SPEED * dt, SHIP_MIN_X, SHIP_MAX_X);
        if (state_t_ >= READY_TIME) {
            state_ = State::Playing;
            state_t_ = 0;
        }
        break;
    }
    case State::Playing:
        update_playing(dt, pad);
        break;
    case State::Dying:
        state_t_ += dt;
        if (pad.y_pressed) autopilot_ = !autopilot_;
        if (state_t_ >= DIE_TIME) {
            if (lives_ <= 0) {
                save_record();
                state_ = State::GameOver;
                state_t_ = 0;
            } else {
                state_ = State::Playing;
                state_t_ = 0;
                invuln_ = INVULN_TIME;
                ship_x_ = W / 2;
            }
        }
        break;
    case State::Clear:
        state_t_ += dt;
        if (state_t_ >= CLEAR_TIME) {
            ++level_;
            load_level();
        }
        break;
    case State::GameOver:
        state_t_ += dt;
        if (state_t_ > 1.0f) {
            if (pad.a_pressed) start_game();
            else if (pad.x_pressed) {
                state_ = State::Title;
                state_t_ = 0;
            }
        }
        break;
    }
}

void InvadersGame::update_title(const input::PadState& pad)
{
    if (pad.x_pressed) start_level_ = start_level_ % MAX_START_LEVEL + 1;
    if (pad.y_pressed) autopilot_ = !autopilot_;
    if (pad.a_pressed || pad.b_pressed) start_game();
}

void InvadersGame::update_playing(float dt, const input::PadState& pad)
{
    level_time_ += dt;
    if (pad.y_pressed) autopilot_ = !autopilot_;
    const float slow = pw_time_[PW_SLOW] > 0 ? 0.45f : 1.f;
    for (int p = 0; p < POWERS; ++p) {
        if (p == PW_SHIELD || p == PW_LIFE) continue;
        if (pw_time_[p] > 0) pw_time_[p] = fmaxf(0.f, pw_time_[p] - dt);
    }
    if (pw_time_[PW_SHIELD] > 0) pw_time_[PW_SHIELD] = fmaxf(0.f, pw_time_[PW_SHIELD] - dt);
    if (invuln_ > 0) invuln_ = fmaxf(0.f, invuln_ - dt);
    if (combo_t_ > 0) {
        combo_t_ -= dt;
        if (combo_t_ <= 0) combo_ = 0;
    }

    // --- statek ---
    float mv = 0;
    if (autopilot_) {
        mv = (float)autopilot_move();
    } else {
        if (pad.left) mv -= 1;
        if (pad.right) mv += 1;
        if (engine::absf(pad.stick_x) > 0.2f) mv = pad.stick_x;
    }
    ship_x_ = engine::clampf(ship_x_ + mv * SHIP_SPEED * dt, SHIP_MIN_X, SHIP_MAX_X);
    if (fire_cd_ > 0) fire_cd_ -= dt;
    bool fire = pad.a || pad.b;
    if (autopilot_) fire = !bunker_above(ship_x_);   // strzela ciagle, chyba ze nad nim bunkier
    if (fire && fire_cd_ <= 0) player_fire();

    update_formation(dt, slow);
    update_dives(dt, slow);
    update_ufo(dt * slow);
    update_boss(dt, slow);
    update_shots(dt, slow);

    // bonusy spadaja
    for (Pickup& p : pickup_) {
        if (!p.alive) continue;
        p.y += PICKUP_SPEED * dt;
        if (p.y > H + 20) p.alive = false;
        else if (engine::absf(p.x - ship_x_) < 30.f && engine::absf(p.y - (float)SHIP_Y) < 28.f && state_ == State::Playing) {
            p.alive = false;
            take_pickup(p.kind);
        }
    }

    if (state_ != State::Playing) return;
    // koniec fali: wszyscy obcy (i boss) pokonani
    if (alive_ == 0 && !boss_alive_) {
        const int bonus = 500 + 250 * level_;
        score_ += bonus;
        char buf[12];
        snprintf(buf, sizeof(buf), "+%d", bonus);
        popup(W / 2, 300, buf, COL_GOLD);
        save_record();
        for (Shot& s : bomb_) s.alive = false;
        state_ = State::Clear;
        state_t_ = 0;
    }
}

void InvadersGame::update_formation(float dt, float slow)
{
    // Ready: formacja zjezdza z gory na miejsce (tylko wizualnie, bez strzalow)
    if (alive_ == 0) return;
    float minx = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (int i = 0; i < ALIENS; ++i) {
        if (!alien_[i].alive) continue;
        const float sx = fx_ + (float)(i % FCOLS) * CELL_W + CELL_W * 0.5f;
        minx = fminf(minx, sx);
        maxx = fmaxf(maxx, sx);
        if (alien_[i].mode == IN_FORMATION) maxy = fmaxf(maxy, fy_ + (float)(i / FCOLS) * CELL_H + CELL_H * 0.5f);
    }
    const float speed = formation_speed() * slow;
    if (fdrop_ > 0) {
        const float d = fminf(fdrop_, DROP_SPEED * slow * dt);
        fy_ += d;
        fdrop_ -= d;
    } else {
        fx_ += fdir_ * speed * dt;
        if (fdir_ > 0 && maxx + HALF_A + 2.f >= (float)W - EDGE) {
            fdir_ = -1;
            fdrop_ = boss_level_ ? 0.f : DROP;
        } else if (fdir_ < 0 && minx - HALF_A - 2.f <= EDGE) {
            fdir_ = 1;
            fdrop_ = boss_level_ ? 0.f : DROP;
        }
    }
    // animacja krokow: szybciej, gdy formacja szybsza
    fanim_t_ += dt * slow * (1.2f + speed / 40.f);
    if (fanim_t_ >= 1.f) {
        fanim_t_ -= 1.f;
        fframe_ ^= 1;
    }
    for (Alien& a : alien_)
        if (a.flash > 0) a.flash -= dt;

    // obcy kruszy bunkry, przez ktore przechodzi; inwazja = koniec gry
    if (maxy > -1e8f) {
        if (maxy + HALF_A >= (float)BUNKER_Y) {
            for (int i = 0; i < ALIENS; ++i) {
                if (!alien_[i].alive || alien_[i].mode != IN_FORMATION) continue;
                const float x = alien_x(i), y = alien_y(i);
                if (y + HALF_A >= (float)BUNKER_Y) bunker_erase_rect(x - HALF_A, y - HALF_A, x + HALF_A, y + HALF_A);
            }
        }
        if (maxy + HALF_A >= (float)INVADE_Y) {
            explode(ship_x_, (float)SHIP_Y, 1.6f, gfx::rgb565(140, 200, 255), 40);
            lives_ = 0;
            shake_ = 0.8f;
            popup(W / 2, 250, "INWAZJA!", gfx::rgb565(255, 80, 70));
            state_ = State::Dying;
            state_t_ = 0;
            return;
        }
    }

    // strzaly obcych
    bomb_t_ -= dt * slow;
    if (bomb_t_ <= 0) {
        alien_fire(slow);
        const float base = fmaxf(0.32f, 1.25f - 0.08f * (float)(level_ - 1));
        bomb_t_ = base * frand(0.6f, 1.4f);
    }
}

void InvadersGame::alien_fire(float)
{
    // losowa kolumna z zywym obcym w formacji, strzela najnizszy
    int cols[FCOLS], n = 0;
    for (int c = 0; c < FCOLS; ++c) {
        for (int r = FROWS - 1; r >= 0; --r) {
            const Alien& a = alien_[r * FCOLS + c];
            if (a.alive && a.mode == IN_FORMATION) {
                cols[n++] = r * FCOLS + c;
                break;
            }
        }
    }
    if (!n) return;
    const int i = cols[engine::rng().range(0, n - 1)];
    for (Shot& b : bomb_) {
        if (b.alive) continue;
        b = Shot{};
        b.alive = true;
        b.x = alien_x(i);
        b.y = alien_y(i) + 18.f;
        b.vy = 170.f + 10.f * (float)engine::imin(level_, 10);
        if (level_ >= 3 && engine::rng().unit() < 0.35f) {
            const float t = ((float)SHIP_Y - b.y) / b.vy;
            b.vx = engine::clampf((ship_x_ - b.x) / fmaxf(t, 0.3f) * 0.8f, -90.f, 90.f);
        }
        return;
    }
}

void InvadersGame::start_dive()
{
    int cand[ALIENS], n = 0, diving = 0;
    for (int i = 0; i < ALIENS; ++i) {
        if (!alien_[i].alive) continue;
        if (alien_[i].mode != IN_FORMATION) ++diving;
        else cand[n++] = i;
    }
    const int max_div = engine::imin(1 + level_ / 3, 3);
    if (!n || diving >= max_div || n < 3) return;
    const int i = cand[engine::rng().range(0, n - 1)];
    Alien& a = alien_[i];
    a.x = alien_x(i);
    a.y = alien_y(i);
    a.mode = DIVING;
    a.t = 0;
    a.dive_x = a.x;
    a.dive_amp = frand(60.f, 110.f) * (engine::rng().unit() < 0.5f ? -1.f : 1.f);
    a.dive_w = frand(1.8f, 2.6f);
    a.dive_shot = false;
}

void InvadersGame::update_dives(float dt, float slow)
{
    if (level_ >= 2 && !boss_level_) {
        dive_t_ -= dt * slow;
        if (dive_t_ <= 0) {
            start_dive();
            dive_t_ = fmaxf(1.6f, 6.f - 0.4f * (float)level_) * frand(0.7f, 1.3f);
        }
    }
    for (int i = 0; i < ALIENS; ++i) {
        Alien& a = alien_[i];
        if (!a.alive || a.mode == IN_FORMATION) continue;
        if (a.mode == DIVING) {
            a.t += dt * slow;
            a.dive_x = engine::approach(a.dive_x, ship_x_, 55.f * dt * slow);
            // start: krotki luk w gore, potem opadanie z wezykiem
            const float vy = a.t < 0.35f ? -60.f : 150.f + 8.f * (float)engine::imin(level_, 10);
            a.y += vy * dt * slow;
            a.x = a.dive_x + a.dive_amp * sinf(a.t * a.dive_w);
            if (!a.dive_shot && a.y > 230.f) {
                a.dive_shot = true;
                for (Shot& b : bomb_) {
                    if (b.alive) continue;
                    b = Shot{};
                    b.alive = true;
                    b.x = a.x;
                    b.y = a.y + 16.f;
                    b.vy = 230.f;
                    b.vx = engine::clampf((ship_x_ - a.x) * 0.9f, -120.f, 120.f);
                    break;
                }
            }
            if (a.y + HALF_A >= (float)BUNKER_Y && a.y - HALF_A < (float)(BUNKER_Y + BUNKER_H))
                bunker_erase_rect(a.x - HALF_A, a.y - HALF_A, a.x + HALF_A, a.y + HALF_A);
            // zderzenie z graczem
            if (state_ == State::Playing && engine::absf(a.x - ship_x_) < HALF_A + SHIP_HW - 6.f &&
                engine::absf(a.y - (float)SHIP_Y) < HALF_A + SHIP_HH - 6.f) {
                kill_alien(i);
                hit_player();
                continue;
            }
            if (a.y > (float)H + 30.f) {
                a.mode = RETURNING;
                a.y = -30.f;
                a.x = fx_ + (float)(i % FCOLS) * CELL_W + CELL_W * 0.5f;
            }
        } else {   // RETURNING: lot do swojego miejsca w formacji
            const float tx = fx_ + (float)(i % FCOLS) * CELL_W + CELL_W * 0.5f;
            const float ty = fy_ + (float)(i / FCOLS) * CELL_H + CELL_H * 0.5f;
            const float dx = tx - a.x, dy = ty - a.y, d = sqrtf(dx * dx + dy * dy);
            const float step = 230.f * dt * slow;
            if (d <= step + 1.f) a.mode = IN_FORMATION;
            else {
                a.x += dx / d * step;
                a.y += dy / d * step;
            }
        }
    }
}

void InvadersGame::update_ufo(float dt)
{
    if (!ufo_alive_) {
        if (boss_level_ || alive_ < 4) return;
        ufo_t_ -= dt;
        if (ufo_t_ <= 0) {
            ufo_alive_ = true;
            ufo_dir_ = engine::rng().unit() < 0.5f ? 1.f : -1.f;
            ufo_x_ = ufo_dir_ > 0 ? -44.f : (float)W + 44.f;
        }
        return;
    }
    ufo_x_ += ufo_dir_ * 140.f * dt;
    if (ufo_x_ < -60.f || ufo_x_ > (float)W + 60.f) {
        ufo_alive_ = false;
        ufo_t_ = frand(16.f, 24.f);
    }
}

void InvadersGame::update_boss(float dt, float slow)
{
    if (!boss_alive_) return;
    if (boss_flash_ > 0) boss_flash_ -= dt;
    if (boss_die_t_ > 0) {   // seria wybuchow, potem koniec
        boss_die_t_ -= dt;
        if (engine::rng().unit() < 0.35f)
            explode(boss_x_ + frand(-60.f, 60.f), boss_y_ + frand(-55.f, 55.f), frand(0.8f, 1.6f), gfx::rgb565(255, 170, 60), 10);
        shake_ = fmaxf(shake_, 0.25f);
        if (boss_die_t_ <= 0) {
            boss_alive_ = false;
            explode(boss_x_, boss_y_, 2.6f, gfx::rgb565(255, 200, 90), 60);
            shake_ = 0.9f;
            add_score(5000 * (level_ / 5), boss_x_, boss_y_);
            spawn_pickup(boss_x_, boss_y_, PW_LIFE);
            for (int i = 0; i < ALIENS; ++i)   // eskorta ginie razem z bossem
                if (alien_[i].alive) kill_alien(i);
        }
        return;
    }
    boss_t_ += dt * slow;
    boss_x_ = (float)W / 2 + 250.f * sinf(boss_t_ * 0.55f);
    boss_y_ = 128.f + 18.f * sinf(boss_t_ * 1.3f);
    boss_fire_t_ -= dt * slow;
    if (boss_fire_t_ <= 0 && state_ == State::Playing) {
        const int n = level_ / 5;
        const bool fan = ((int)(boss_t_ * 0.7f) & 1) == 0;
        if (fan) {   // wachlarz 5-7 pociskow
            const int k = 5 + engine::imin(n - 1, 2);
            for (int j = 0; j < k; ++j) {
                const float ang = -0.7f + 1.4f * (float)j / (float)(k - 1);
                for (Shot& b : bomb_) {
                    if (b.alive) continue;
                    b = Shot{};
                    b.alive = true;
                    b.kind = 1;
                    b.x = boss_x_;
                    b.y = boss_y_ + 60.f;
                    b.vx = sinf(ang) * 190.f;
                    b.vy = cosf(ang) * 190.f;
                    break;
                }
            }
        } else {     // trzy celowane
            for (int j = -1; j <= 1; ++j) {
                for (Shot& b : bomb_) {
                    if (b.alive) continue;
                    b = Shot{};
                    b.alive = true;
                    b.kind = 1;
                    b.x = boss_x_ + (float)j * 40.f;
                    b.y = boss_y_ + 55.f;
                    b.vy = 250.f;
                    b.vx = engine::clampf((ship_x_ - b.x) * 0.8f, -140.f, 140.f);
                    break;
                }
            }
        }
        boss_fire_t_ = fmaxf(0.9f, 1.8f - 0.15f * (float)n) * frand(0.8f, 1.2f);
    }
}

void InvadersGame::player_fire()
{
    int alive = 0;
    for (const Shot& s : bolt_) alive += s.alive;
    const bool laser = pw_time_[PW_LASER] > 0, triple = pw_time_[PW_TRIPLE] > 0;
    const int limit = laser ? MAX_BOLTS : triple ? 9 : 3;
    const int need = triple ? 3 : 1;
    if (alive + need > limit) return;
    fire_cd_ = laser ? LASER_CD : FIRE_CD;
    for (int k = 0; k < need; ++k) {
        for (Shot& s : bolt_) {
            if (s.alive) continue;
            s = Shot{};
            s.alive = true;
            s.x = ship_x_;
            s.y = (float)SHIP_Y - 26.f;
            s.vy = -(laser ? LASER_SPEED : BOLT_SPEED);
            s.vx = triple ? (float)(k - 1) * 150.f : 0.f;
            s.pierce = laser;
            break;
        }
    }
}

bool InvadersGame::bunker_hit(float x, float y, int radius)
{
    if (y < (float)BUNKER_Y || y >= (float)(BUNKER_Y + BUNKER_H)) return false;
    for (int b = 0; b < BUNKERS; ++b) {
        uint8_t* m = bunker_mask_[b];
        if (!m) continue;
        const int bx0 = W * (b + 1) / (BUNKERS + 1) - BUNKER_W / 2;
        const int lx = (int)x - bx0, ly = (int)y - BUNKER_Y;
        if (lx < 0 || lx >= BUNKER_W) continue;
        if (m[ly * BUNKER_W + lx] < 90) return false;
        // wykruszenie: kolo z postrzepiona krawedzia
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                const int px = lx + dx, py = ly + dy;
                if (px < 0 || py < 0 || px >= BUNKER_W || py >= BUNKER_H) continue;
                const int d2 = dx * dx + dy * dy;
                if (d2 > radius * radius) continue;
                if (d2 > (radius - 2) * (radius - 2) && (engine::rng().next() & 1)) continue;
                m[py * BUNKER_W + px] = 0;
            }
        }
        return true;
    }
    return false;
}

void InvadersGame::bunker_erase_rect(float x0, float y0, float x1, float y1)
{
    for (int b = 0; b < BUNKERS; ++b) {
        uint8_t* m = bunker_mask_[b];
        if (!m) continue;
        const int bx0 = W * (b + 1) / (BUNKERS + 1) - BUNKER_W / 2;
        const int ax = engine::imax((int)x0 - bx0, 0), bx = engine::imin((int)x1 - bx0, BUNKER_W);
        const int ay = engine::imax((int)y0 - BUNKER_Y, 0), by = engine::imin((int)y1 - BUNKER_Y, BUNKER_H);
        for (int y = ay; y < by; ++y)
            for (int x = ax; x < bx; ++x) m[y * BUNKER_W + x] = 0;
    }
}

void InvadersGame::kill_alien(int i)
{
    Alien& a = alien_[i];
    if (!a.alive) return;
    const float x = alien_x(i), y = alien_y(i);
    a.alive = false;
    --alive_;
    ++kills_;
    explode(x, y, 1.f, TYPE_COLOR[a.type], 14);
    const int prev = combo_;
    combo_ = combo_t_ > 0 ? engine::imin(combo_ + 1, 4) : 1;
    combo_t_ = COMBO_WINDOW;
    int pts = ALIEN_PTS[a.type] * combo_;
    if (a.mode == DIVING) pts *= 2;   // nurkujacy wart podwojnie
    add_score(pts, x, y - 10.f);
    if (combo_ >= 2 && combo_ > prev) {   // napis tylko przy wzroscie mnoznika, stan combo pokazuje HUD
        char buf[12];
        snprintf(buf, sizeof(buf), "COMBO x%d", combo_);
        popup(x, y + 12.f, buf, gfx::rgb565(255, 120, 220));
    }
    if (engine::rng().unit() < 0.035f) spawn_pickup(x, y, engine::rng().range(0, 3));
}

void InvadersGame::hit_player()
{
    if (state_ != State::Playing || invuln_ > 0) return;
    if (pw_time_[PW_SHIELD] > 0) {
        pw_time_[PW_SHIELD] = 0;
        invuln_ = 0.6f;
        explode(ship_x_, (float)SHIP_Y - 6.f, 0.8f, gfx::rgb565(90, 170, 255), 18);
        shake_ = fmaxf(shake_, 0.2f);
        return;
    }
    --lives_;
    explode(ship_x_, (float)SHIP_Y, 1.8f, gfx::rgb565(140, 200, 255), 50);
    shake_ = 0.7f;
    for (Shot& s : bomb_) s.alive = false;
    for (Shot& s : bolt_) s.alive = false;
    pw_time_[PW_TRIPLE] = pw_time_[PW_LASER] = pw_time_[PW_SLOW] = 0;
    combo_ = 0;
    // nurkujacy wracaja do formacji
    for (int i = 0; i < ALIENS; ++i)
        if (alien_[i].alive && alien_[i].mode == DIVING) {
            alien_[i].mode = RETURNING;
            alien_[i].y = fminf(alien_[i].y, -30.f);
            alien_[i].x = alien_x(i);
        }
    state_ = State::Dying;
    state_t_ = 0;
}

void InvadersGame::spawn_pickup(float x, float y, int kind)
{
    for (Pickup& p : pickup_) {
        if (p.alive) continue;
        p.alive = true;
        p.x = x;
        p.y = y;
        p.kind = (uint8_t)kind;
        return;
    }
}

void InvadersGame::take_pickup(int kind)
{
    static const char* const NAMES[POWERS] = { "POTROJNY!", "LASER!", "OSLONA!", "SPOWOLNIENIE!", "+1 ZYCIE" };
    static const uint16_t COLS[POWERS] = { gfx::rgb565(255, 90, 80), gfx::rgb565(255, 220, 60), gfx::rgb565(90, 170, 255),
                                           gfx::rgb565(200, 120, 255), gfx::rgb565(120, 255, 140) };
    if (kind == PW_LIFE) ++lives_;
    else pw_time_[kind] = PW_DURATION[kind];
    score_ += 100;
    popup(ship_x_, (float)SHIP_Y - 44.f, NAMES[kind], COLS[kind]);
    explode(ship_x_, (float)SHIP_Y - 10.f, 0.5f, COLS[kind], 16);
}

void InvadersGame::update_shots(float dt, float slow)
{
    // --- pociski gracza ---
    for (Shot& s : bolt_) {
        if (!s.alive) continue;
        const float y0 = s.y;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        if (s.y < (float)HUD_H - 10.f || s.x < -10.f || s.x > (float)W + 10.f) {
            s.alive = false;
            continue;
        }
        // bunkry od dolu: sprawdzamy odcinek przebyty w tej klatce co 3 px (pocisk leci ~11 px/klatke)
        bool hit = false;
        for (float yy = y0 - 18.f; yy >= s.y - 18.f && !hit; yy -= 3.f) hit = bunker_hit(s.x, yy, 5);
        if (hit) {
            s.alive = false;
            explode(s.x, s.y - 18.f, 0.25f, gfx::rgb565(120, 255, 240), 5);
            continue;
        }
        // pocisk vs pocisk obcych
        for (Shot& b : bomb_) {
            if (!b.alive || b.kind == 1) continue;
            if (engine::absf(b.x - s.x) < 10.f && s.y - 20.f < b.y + 14.f && y0 + 10.f > b.y - 14.f) {
                b.alive = false;
                explode(b.x, b.y, 0.35f, gfx::rgb565(200, 255, 120), 6);
                if (!s.pierce) s.alive = false;
                add_score(5, b.x, b.y);
                break;
            }
        }
        if (!s.alive) continue;
        // obcy
        for (int i = 0; i < ALIENS && s.alive; ++i) {
            if (!alien_[i].alive) continue;
            const float ax = alien_x(i), ay = alien_y(i);
            if (engine::absf(ax - s.x) < HALF_A && s.y - 16.f < ay + HALF_A - 2.f && y0 > ay - HALF_A) {
                kill_alien(i);
                if (!s.pierce) s.alive = false;
            }
        }
        if (!s.alive) continue;
        if (ufo_alive_ && engine::absf(ufo_x_ - s.x) < 38.f && engine::absf(UFO_Y - s.y) < 22.f) {
            ufo_alive_ = false;
            ufo_t_ = frand(16.f, 24.f);
            s.alive = false;
            explode(ufo_x_, UFO_Y, 1.4f, gfx::rgb565(255, 90, 80), 30);
            combo_ = combo_t_ > 0 ? engine::imin(combo_ + 1, 4) : 1;
            combo_t_ = COMBO_WINDOW;
            add_score(UFO_PTS[engine::rng().range(0, 3)] * combo_, ufo_x_, UFO_Y);
            spawn_pickup(ufo_x_, UFO_Y, engine::rng().unit() < 0.15f ? PW_LIFE : engine::rng().range(0, 3));
            continue;
        }
        if (boss_alive_ && boss_die_t_ <= 0 && engine::absf(boss_x_ - s.x) < 62.f && engine::absf(boss_y_ - s.y) < 58.f) {
            s.alive = false;
            boss_flash_ = 0.08f;
            explode(s.x, s.y, 0.35f, gfx::rgb565(255, 200, 120), 4);
            if (--boss_hp_ <= 0) {
                boss_hp_ = 0;
                boss_die_t_ = 2.0f;
                for (Shot& b : bomb_) b.alive = false;
            } else {
                score_ += 10;
            }
        }
    }

    // --- pociski obcych ---
    for (Shot& b : bomb_) {
        if (!b.alive) continue;
        const float y0 = b.y;
        b.x += b.vx * dt * slow;
        b.y += b.vy * dt * slow;
        if (b.y > (float)H + 20.f || b.x < -20.f || b.x > (float)W + 20.f) {
            b.alive = false;
            continue;
        }
        bool hit = false;
        for (float yy = y0 + 14.f; yy <= b.y + 14.f && !hit; yy += 3.f) hit = bunker_hit(b.x, yy, 7);
        if (hit) {
            b.alive = false;
            explode(b.x, b.y + 14.f, 0.3f, gfx::rgb565(200, 255, 120), 5);
            continue;
        }
        if (state_ == State::Playing && engine::absf(b.x - ship_x_) < SHIP_HW - 4.f && engine::absf(b.y - (float)SHIP_Y) < SHIP_HH) {
            b.alive = false;
            hit_player();
        }
    }
}

void InvadersGame::update_effects(float dt)
{
    if (shake_ > 0) shake_ = fmaxf(0.f, shake_ - dt);
    for (Boom& b : boom_) {
        if (!b.alive) continue;
        b.t += dt;
        if (b.t >= b.dur) b.alive = false;
    }
    for (Spark& s : spark_) {
        if (!s.alive) continue;
        s.life -= dt;
        if (s.life <= 0) {
            s.alive = false;
            continue;
        }
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.vx *= 1.f - 2.2f * dt;
        s.vy = s.vy * (1.f - 2.2f * dt) + 60.f * dt;
    }
    for (Popup& p : popup_) {
        if (!p.alive) continue;
        p.t -= dt;
        p.y -= 26.f * dt;
        if (p.t <= 0) p.alive = false;
    }
}

// ============================================================================ autopilot

// Czy nad statkiem w kolumnie x stoi jeszcze bunkier (strzal trafilby we wlasna oslone).
bool InvadersGame::bunker_above(float x) const
{
    for (int b = 0; b < BUNKERS; ++b) {
        const uint8_t* m = bunker_mask_[b];
        const int lx = (int)x - (W * (b + 1) / (BUNKERS + 1) - BUNKER_W / 2);
        if (!m || lx < 0 || lx >= BUNKER_W) continue;
        for (int y = 0; y < BUNKER_H; y += 2)
            if (m[y * BUNKER_W + lx] >= 90) return true;
    }
    return false;
}

// Wybiera kierunek ruchu (-1, 0, 1): cel = bonus, UFO, boss albo najnizszy obcy (z wyprzedzeniem ruchu formacji),
// koszt pozycji = zagrozenie od pociskow i nurkujacych (przewidywane polozenie za chwile) + odleglosc od celu.
int InvadersGame::autopilot_move()
{
    float target = ship_x_;
    bool have = false;
    for (const Pickup& p : pickup_) {
        if (p.alive && p.y > 150.f) {
            target = p.x;
            have = true;
            break;
        }
    }
    if (!have && boss_alive_ && boss_die_t_ <= 0) {
        target = boss_x_;
        have = true;
    }
    if (!have && ufo_alive_ && ufo_x_ > 40.f && ufo_x_ < (float)W - 40.f && alive_ < 20) {
        target = ufo_x_ + ufo_dir_ * 70.f;
        have = true;
    }
    if (!have) {
        float best = 1e9f;
        const float lead = ((float)SHIP_Y - 200.f) / BOLT_SPEED;
        for (int i = 0; i < ALIENS; ++i) {
            if (!alien_[i].alive) continue;
            float x = alien_x(i);
            if (alien_[i].mode == IN_FORMATION && fdrop_ <= 0) x += fdir_ * formation_speed() * lead;
            else if (alien_[i].mode != IN_FORMATION) continue;
            // preferuj niskich (grozniejszych) i bliskich
            const float cost = engine::absf(x - ship_x_) - alien_y(i) * 0.8f;
            if (cost < best) {
                best = cost;
                target = x;
            }
        }
    }
    target = engine::clampf(target, SHIP_MIN_X, SHIP_MAX_X);

    auto danger = [&](float x) -> float {
        float d = 0;
        for (const Shot& b : bomb_) {
            if (!b.alive || b.vy <= 0) continue;
            const float tt = ((float)SHIP_Y - SHIP_HH - b.y) / b.vy;
            if (tt < -0.05f || tt > 0.85f) continue;
            const float bx = b.x + b.vx * fmaxf(tt, 0.f);
            if (engine::absf(bx - x) < SHIP_HW + 14.f) d += 1.5f - tt;
        }
        for (int i = 0; i < ALIENS; ++i) {
            const Alien& a = alien_[i];
            if (!a.alive || a.mode != DIVING || a.y < 220.f) continue;
            if (engine::absf(a.x - x) < 56.f) d += 1.f;
        }
        return d;
    };
    float best_cost = 1e18f;
    float best_x = ship_x_;
    for (int k = -24; k <= 24; ++k) {
        const float x = engine::clampf(ship_x_ + (float)k * 10.f, SHIP_MIN_X, SHIP_MAX_X);
        // droga przez niebezpieczenstwo tez sie liczy (probkujemy polowe drogi)
        const float mid = (x + ship_x_) * 0.5f;
        const float cost = danger(x) * 1000.f + danger(mid) * 300.f + engine::absf(x - target) + engine::absf(x - ship_x_) * 0.3f +
                           (bunker_above(x) ? 120.f : 0.f);
        if (cost < best_cost) {
            best_cost = cost;
            best_x = x;
        }
    }
    if (best_x > ship_x_ + 3.f) return 1;
    if (best_x < ship_x_ - 3.f) return -1;
    return 0;
}

// ============================================================================ debug

void InvadersGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "READY", "PLAY", "DYING", "CLEAR", "OVER" };
    int bolts = 0, bombs = 0, divers = 0;
    for (const Shot& s : bolt_) bolts += s.alive;
    for (const Shot& s : bomb_) bombs += s.alive;
    for (const Alien& a : alien_) divers += a.alive && a.mode != IN_FORMATION;
    char pw[6] = "-----";
    static const char PWC[] = "TLSV";
    for (int i = 0; i < 4; ++i)
        if (pw_time_[i] > 0) pw[i] = PWC[i];
    snprintf(buf, n, "%-5s lvl=%d score=%d lives=%d alive=%d f=%d,%d ship=%d bolts=%d bombs=%d div=%d ufo=%d boss=%d/%d pw=%s combo=%d ap=%d",
             NAMES[(int)state_], level_, score_, lives_, alive_, (int)fx_, (int)fy_, (int)ship_x_, bolts, bombs, divers,
             ufo_alive_ ? (int)ufo_x_ : -1, boss_alive_ ? boss_hp_ : 0, boss_max_, pw, combo_, autopilot_ ? 1 : 0);
}

}  // namespace invaders
