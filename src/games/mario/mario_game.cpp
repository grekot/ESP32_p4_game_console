#include "games/mario/mario_game.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"

#include "engine/math2d.h"
#include "engine/screen.h"
#include "engine/stats.h"
#include "games/mario/mario_assets.h"
#include "gfx/font.h"

namespace mario {

namespace {

const char* TAG = "mario";

// Lake Mario to pixel-art: rysuje na 400x240 (canvas_scale() == 2), konsola powieksza x2.
constexpr int VIEW_W = engine::PIXEL_CANVAS_W;   // 400
constexpr int VIEW_H = engine::PIXEL_CANVAS_H;   // 240

// --- parametry fizyki (px, px/s, px/s^2) ---
constexpr float GRAVITY     = 1100.f;
constexpr float MAX_FALL    = 420.f;
constexpr float WALK_SPEED  = 100.f;
constexpr float RUN_SPEED   = 165.f;
constexpr float ACCEL       = 700.f;
constexpr float FRICTION    = 900.f;
constexpr float JUMP_V      = -350.f;
constexpr float JUMP_V_RUN  = -390.f;
// Puszczenie A skraca skok, ale nie do zera: przy -140 krotkie tapniecie dawalo podskok
// na 9 px, czyli ponizej jednego kafelka, i wygladalo jakby gra nie zareagowala.
// -240 to okolo 26 px, czyli 1,6 kafelka - widac, ze postac skoczyla.
constexpr float JUMP_CUT_V  = -240.f;

// Bufor skoku: wcisniecie A tuz PRZED wyladowaniem nie przepada, tylko czeka i odpala sie
// przy zetknieciu z ziemia. Bez tego gracz, ktory nacisnie odrobine za wczesnie, nie skacze.
constexpr float JUMP_BUFFER_TIME = 0.12f;

// Czas kojota: przez chwile PO zejsciu z krawedzi skok nadal dziala. Bez tego skok tuz po
// zbiegnieciu z platformy jest ignorowany, co gracz odbiera jako zgubione wcisniecie.
constexpr float COYOTE_TIME = 0.10f;
constexpr float ENEMY_SPEED = 40.f;
constexpr float STOMP_V     = -230.f;

constexpr int PLAYER_W = 12, PLAYER_H = 16;   // hitbox (sprite ma 16 px; offset x = 2)
constexpr int ENEMY_W  = 14, ENEMY_H  = 13;   // hitbox (sprite 16x16; offset x = 1, y = 3)

constexpr float LEVEL_TIME  = 300.f;
constexpr int   START_LIVES = 3;

constexpr uint16_t HUD_COLOR   = gfx::WHITE;
constexpr uint16_t HUD_SHADOW  = gfx::rgb565(20, 30, 60);
constexpr uint16_t DEBRIS_COLOR = gfx::rgb565(155, 95, 40);

// Matematyka wspolna dla gier jest w engine/math2d.h; tu tylko skroty z kafelkiem 16 px.
inline int   tile_of(float v) { return engine::tile_of(v, TILE); }
inline float fabsf_(float v)  { return engine::absf(v); }
using engine::approach;
using engine::overlap;

}  // namespace

// ============================================================================ cykl zycia

void MarioGame::init(gfx::Canvas&)
{
    mario::load_assets();
    map_.set_solid_fn(mario::tile_is_solid);
    new_game();
    CONSOLE_LOGI(TAG, "poziom: %d kolumn, %d przeciwnikow", map_.cols(), enemy_count_);
}

void MarioGame::new_game()
{
    score_ = 0;
    coins_ = 0;
    lives_ = START_LIVES;
    load_level();
    state_   = State::Title;
    state_t_ = 0;
}

void MarioGame::load_level()
{
    map_.reset(mario::LEVEL_SEGMENTS * mario::SEG_COLS, LEVEL_ROWS, TILE);
    enemy_count_ = 0;
    spawn_x_     = 2 * TILE;
    spawn_y_     = 0;

    for (int r = 0; r < LEVEL_ROWS; ++r) {
        for (int seg = 0; seg < mario::LEVEL_SEGMENTS; ++seg) {
            const char* row = mario::LEVEL1[seg][r] ? mario::LEVEL1[seg][r] : "";
            const int   len = (int)strlen(row);
            for (int c = 0; c < mario::SEG_COLS; ++c) {
                const int col = seg * mario::SEG_COLS + c;
                char t = c < len ? row[c] : ' ';
                if (t == 'S') {
                    spawn_x_ = (float)(col * TILE + 2);
                    spawn_y_ = (float)(r * TILE);
                    t = ' ';
                } else if (t == 'E') {
                    if (enemy_count_ < MAX_ENEMIES) {
                        Enemy& e  = enemies_[enemy_count_++];
                        e.spawn_x = (float)(col * TILE + 1);
                        e.spawn_y = (float)(r * TILE + (TILE - ENEMY_H));
                    }
                    t = ' ';
                }
                map_.set_tile(col, r, t);
            }
        }
    }
    particles_.clear();

    reset_enemies();
    reset_player();
    time_left_ = LEVEL_TIME;
    cam_x_     = 0;
    update_camera(1.f);
}

void MarioGame::reset_player()
{
    player_ = Player{};
    player_.x = spawn_x_;
    player_.y = spawn_y_;
    jump_buffer_ = 0.f;
    coyote_      = 0.f;
}

void MarioGame::reset_enemies()
{
    for (int i = 0; i < enemy_count_; ++i) {
        Enemy& e   = enemies_[i];
        e.x        = e.spawn_x;
        e.y        = e.spawn_y;
        e.vx       = -ENEMY_SPEED;
        e.vy       = 0;
        e.alive    = true;
        e.active   = false;
        e.squash_t = 0;
    }
}

void MarioGame::kill_player()
{
    if (state_ != State::Playing) return;
    state_     = State::Dying;
    state_t_   = 1.6f;
    player_.vx = 0;
    player_.vy = -320.f;
}

void MarioGame::level_clear()
{
    if (state_ != State::Playing) return;
    state_   = State::LevelClear;
    state_t_ = 4.f;
    score_  += (int)time_left_ * 10;
    player_.vx = 0;
}

// ============================================================================ mapa

void MarioGame::bump_block(int col, int row)
{
    const char t = tile_at(col, row);
    const float bx = (float)(col * TILE), by = (float)(row * TILE);
    if (t == '?') {
        set_tile(col, row, 'U');
        ++coins_;
        score_ += 200;
        spawn_particle(bx, by - TILE, 0, -260.f, 1);
    } else if (t == 'B') {
        set_tile(col, row, ' ');
        score_ += 50;
        spawn_particle(bx + 2,  by + 2, -70.f, -240.f, 2);
        spawn_particle(bx + 10, by + 2,  70.f, -240.f, 2);
        spawn_particle(bx + 2,  by + 8, -50.f, -160.f, 2);
        spawn_particle(bx + 10, by + 8,  50.f, -160.f, 2);
    }
}

// ============================================================================ logika
// Kolizje z kafelkami (move_x/move_y) sa w engine::TileMap - patrz engine/tilemap.cpp, tam tez
// wyjasnienie, dlaczego podloze sprawdza sie pod dolna krawedzia (y + h), a nie pod ostatnim pikselem.

void MarioGame::update(float dt, const input::PadState& pad)
{
    anim_t_ += dt;

    switch (state_) {
        case State::Title:
            // START nalezy do konsoli (pauza), wiec ekran tytulowy reaguje na A, B albo dotyk.
            // Gdyby reagowal tez na START, jedno wcisniecie uruchamialoby gre i od razu ja pauzowalo.
            if (pad.a_pressed || pad.b_pressed || (pad.any_pressed && !pad.start_pressed)) {
                state_ = State::Playing;
            }
            break;

        case State::Playing:
            // Pauze obsluguje konsola (app), nie gra - inaczej obie reagowaly na ten sam START
            // i po powrocie z menu pauzy gra zostawala zamrozona we wlasnej pauzie.
            time_left_ -= dt;
            if (time_left_ <= 0) { time_left_ = 0; kill_player(); break; }
            update_player(dt, pad);
            update_enemies(dt);
            update_particles(dt);
            update_camera(dt);
            break;

        case State::Dying:
            state_t_  -= dt;
            player_.vy += GRAVITY * dt;
            player_.y  += player_.vy * dt;
            update_particles(dt);
            if (state_t_ <= 0) {
                --lives_;
                if (lives_ < 0) {
                    state_   = State::GameOver;
                    state_t_ = 3.f;
                } else {
                    reset_player();
                    reset_enemies();
                    time_left_ = LEVEL_TIME;
                    cam_x_     = 0;
                    update_camera(1.f);
                    state_ = State::Playing;
                }
            }
            break;

        case State::LevelClear:
            state_t_ -= dt;
            update_particles(dt);
            if (state_t_ <= 0) {
                load_level();
                state_ = State::Title;
            }
            break;

        case State::GameOver:
            state_t_ -= dt;
            if (state_t_ <= 0) new_game();
            break;
    }
}

void MarioGame::update_player(float dt, const input::PadState& pad)
{
    Player& p = player_;

    // --- poziomo ---
    const float max_speed = pad.b ? RUN_SPEED : WALK_SPEED;
    float target = 0;
    if (pad.left)  target -= max_speed;
    if (pad.right) target += max_speed;

    if (target != 0) {
        const bool turning = (target > 0 && p.vx < 0) || (target < 0 && p.vx > 0);
        p.vx = approach(p.vx, target, (turning ? ACCEL + FRICTION : ACCEL) * dt);
        p.facing_left = target < 0;
    } else {
        p.vx = approach(p.vx, 0, FRICTION * dt);
    }

    // --- skok (z buforem wcisniecia i czasem kojota) ---
    if (pad.a_pressed) {
        jump_buffer_ = JUMP_BUFFER_TIME;
    } else if (jump_buffer_ > 0.f) {
        jump_buffer_ -= dt;
    }

    // p.on_ground pochodzi z poprzedniej klatki - dlatego odliczanie zaczyna sie dopiero
    // po faktycznym oderwaniu sie od podloza.
    if (p.on_ground) {
        coyote_ = COYOTE_TIME;
    } else if (coyote_ > 0.f) {
        coyote_ -= dt;
    }

    if (jump_buffer_ > 0.f && coyote_ > 0.f) {
        p.vy         = fabsf_(p.vx) > WALK_SPEED + 20 ? JUMP_V_RUN : JUMP_V;
        p.on_ground  = false;
        jump_buffer_ = 0.f;
        coyote_      = 0.f;      // jeden skok na jedno oderwanie sie od ziemi
    }

    if (!pad.a && p.vy < JUMP_CUT_V) {
        p.vy = JUMP_CUT_V;
    }

    // --- grawitacja ---
    p.vy += GRAVITY * dt;
    if (p.vy > MAX_FALL) p.vy = MAX_FALL;

    // --- ruch i kolizje z mapa ---
    bool hit_wall = false;
    map_.move_x(p.x, p.y, p.vx, PLAYER_W, PLAYER_H, dt, hit_wall);
    int hit_col, hit_row;
    p.on_ground = map_.move_y(p.x, p.y, p.vy, PLAYER_W, PLAYER_H, dt, hit_col, hit_row);
    if (hit_col >= 0) bump_block(hit_col, hit_row);

    // --- animacja ---
    if (p.on_ground && fabsf_(p.vx) > 10.f) {
        p.anim_t += dt * (fabsf_(p.vx) / WALK_SPEED);
    } else {
        p.anim_t = 0;
    }

    // --- zbieranie / meta / upadek ---
    collect_tiles();
    if (p.y > (float)(LEVEL_ROWS * TILE + 16)) {
        kill_player();
    }
}

void MarioGame::collect_tiles()
{
    const Player& p = player_;
    const int c0 = tile_of(p.x), c1 = tile_of(p.x + PLAYER_W - 1);
    const int r0 = tile_of(p.y), r1 = tile_of(p.y + PLAYER_H - 1);
    for (int r = r0; r <= r1; ++r) {
        for (int c = c0; c <= c1; ++c) {
            const char t = tile_at(c, r);
            if (t == 'o') {
                set_tile(c, r, ' ');
                ++coins_;
                score_ += 100;
                spawn_particle((float)(c * TILE), (float)(r * TILE), 0, -180.f, 1);
            } else if (t == 'F' || t == '^') {
                level_clear();
                return;
            }
        }
    }
}

void MarioGame::update_enemies(float dt)
{
    Player& p = player_;
    for (int i = 0; i < enemy_count_; ++i) {
        Enemy& e = enemies_[i];
        if (!e.alive) continue;

        if (!e.active) {
            if (e.x < cam_x_ + VIEW_W + 32) e.active = true;
            else continue;
        }
        if (e.squash_t > 0) {
            e.squash_t -= dt;
            if (e.squash_t <= 0) e.alive = false;
            continue;
        }

        e.vy += GRAVITY * dt;
        if (e.vy > MAX_FALL) e.vy = MAX_FALL;

        const float dir = e.vx;
        bool hit_wall = false;
        map_.move_x(e.x, e.y, e.vx, ENEMY_W, ENEMY_H, dt, hit_wall);
        if (hit_wall) e.vx = -dir;            // odbicie od sciany
        int hc, hr;
        map_.move_y(e.x, e.y, e.vy, ENEMY_W, ENEMY_H, dt, hc, hr);

        if (e.y > (float)(LEVEL_ROWS * TILE + 32)) { e.alive = false; continue; }

        // kolizja z graczem
        if (state_ == State::Playing &&
            overlap(p.x, p.y, PLAYER_W, PLAYER_H, e.x, e.y, ENEMY_W, ENEMY_H)) {
            const bool stomp = p.vy > 0 && (p.y + PLAYER_H) < e.y + ENEMY_H * 0.6f;
            if (stomp) {
                e.squash_t = 0.6f;
                e.vx       = 0;
                p.vy       = STOMP_V;
                p.y        = e.y - PLAYER_H;
                score_    += 100;
            } else {
                kill_player();
            }
        }
    }
}

void MarioGame::spawn_particle(float x, float y, float vx, float vy, uint8_t kind)
{
    // Moneta (kind 1) zyje krotko i jest lzejsza; odlamek cegly (kind 2) spada z pelna grawitacja.
    particles_.spawn(x, y, vx, vy, kind, kind == 1 ? 0.55f : 1.0f, kind == 1 ? 700.f : GRAVITY);
}

void MarioGame::update_particles(float dt)
{
    particles_.update(dt);
}

void MarioGame::update_camera(float dt)
{
    const float target = player_.x + PLAYER_W * 0.5f - VIEW_W * 0.4f;
    float k = dt * 10.f;
    if (k > 1.f) k = 1.f;
    cam_x_ += (target - cam_x_) * k;
    const float max_cam = (float)(map_.width_px() - VIEW_W);
    if (cam_x_ < 0) cam_x_ = 0;
    if (cam_x_ > max_cam) cam_x_ = max_cam;
}

// ============================================================================ diagnostyka

void MarioGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "PLAY", "DYING", "CLEAR", "OVER" };
    const Player& p = player_;

    // Najblizszy zywy, aktywny przeciwnik - przydaje sie do skryptowania testow zderzen.
    float ex = -1.f, ey = -1.f, best = 1e9f;
    for (int i = 0; i < enemy_count_; ++i) {
        const Enemy& e = enemies_[i];
        if (!e.alive || !e.active || e.squash_t > 0) continue;
        const float d = fabsf_(e.x - p.x);
        if (d < best) { best = d; ex = e.x; ey = e.y; }
    }

    snprintf(buf, n, "%-5s x=%6.1f y=%6.1f vx=%6.1f vy=%7.1f ground=%d score=%d coins=%d lives=%d time=%.0f enemy=%.0f,%.0f",
             NAMES[(int)state_], (double)p.x, (double)p.y, (double)p.vx, (double)p.vy,
             p.on_ground ? 1 : 0, score_, coins_, lives_, (double)time_left_, (double)ex, (double)ey);
}

// ============================================================================ rysowanie

void MarioGame::render(gfx::Canvas& c)
{
    c.clear(mario::SKY);
    draw_tiles(c);
    draw_entities(c);
    draw_hud(c);
    draw_overlay(c);
}

void MarioGame::draw_tiles(gfx::Canvas& c) const
{
    const int coin_frame = (int)(anim_t_ * 4.f) & 1;
    map_.draw(c, (int)cam_x_, mario::tile_sprite, coin_frame);
}

void MarioGame::draw_entities(gfx::Canvas& c) const
{
    const int camx = (int)cam_x_;

    // czasteczki
    for (const auto& p : particles_) {
        if (p.kind == 1) {
            c.blit(spr::coin_a, (int)p.x - camx, (int)p.y);
        } else if (p.kind == 2) {
            c.fill_rect((int)p.x - camx, (int)p.y, 4, 4, DEBRIS_COLOR);
        }
    }

    // przeciwnicy
    const bool enemy_frame = ((int)(anim_t_ * 4.f) & 1) != 0;
    for (int i = 0; i < enemy_count_; ++i) {
        const Enemy& e = enemies_[i];
        if (!e.alive || !e.active) continue;
        const int sx = (int)e.x - 1 - camx;
        const int sy = (int)e.y - (TILE - ENEMY_H);
        if (e.squash_t > 0) {
            c.blit(spr::enemy_squash, sx, sy);
        } else {
            c.blit(enemy_frame ? spr::enemy_b : spr::enemy_a, sx, sy, e.vx > 0);
        }
    }

    // gracz
    const Player& p = player_;
    const gfx::Sprite* ps = &spr::player_idle;
    if (state_ == State::Dying) {
        ps = &spr::player_jump;
    } else if (!p.on_ground) {
        ps = &spr::player_jump;
    } else if (fabsf_(p.vx) > 10.f) {
        ps = ((int)(p.anim_t * 8.f) & 1) ? &spr::player_walk2 : &spr::player_walk1;
    }
    c.blit(*ps, (int)p.x - 2 - camx, (int)p.y, p.facing_left);
}

void MarioGame::draw_hud(gfx::Canvas& c) const
{
    char buf[32];
    snprintf(buf, sizeof(buf), "SCORE %06d", score_);
    gfx::draw_text_shadow(c, 6, 4, buf, HUD_COLOR, HUD_SHADOW);

    snprintf(buf, sizeof(buf), "COINS %02d", coins_);
    gfx::draw_text_shadow(c, 120, 4, buf, HUD_COLOR, HUD_SHADOW);

    snprintf(buf, sizeof(buf), "LIVES %d", lives_ < 0 ? 0 : lives_);
    gfx::draw_text_shadow(c, 210, 4, buf, HUD_COLOR, HUD_SHADOW);

    snprintf(buf, sizeof(buf), "TIME %03d", (int)ceilf(time_left_));
    gfx::draw_text_shadow(c, VIEW_W - 6 - gfx::text_width(buf), 4, buf,
                          time_left_ < 30.f && ((int)(anim_t_ * 4.f) & 1) ? gfx::rgb565(255, 80, 80) : HUD_COLOR,
                          HUD_SHADOW);

#ifdef CONSOLE_SHOW_FPS
    snprintf(buf, sizeof(buf), "%.0f FPS", (double)engine::current_fps());
    gfx::draw_text_shadow(c, VIEW_W - 6 - gfx::text_width(buf), 14, buf, gfx::rgb565(180, 255, 180), HUD_SHADOW);
#endif
}

void MarioGame::draw_overlay(gfx::Canvas& c) const
{
    auto center = [&](int y, const char* text, uint16_t color, int scale) {
        gfx::draw_text_shadow(c, (VIEW_W - gfx::text_width(text, scale)) / 2, y, text, color, gfx::BLACK, scale);
    };
    const bool blink = fmodf(anim_t_, 1.f) < 0.6f;

    switch (state_) {
        case State::Title:
            c.fill_rect(0, 56, VIEW_W, 76, gfx::rgb565(20, 30, 70));
            c.hline(0, 56, VIEW_W, gfx::WHITE);
            c.hline(0, 131, VIEW_W, gfx::WHITE);
            center(64, "LAKE MARIO", gfx::rgb565(255, 220, 60), 3);
            if (blink) center(100, "TAP OR PRESS A", gfx::WHITE, 2);
            break;
        case State::LevelClear:
            center(80, "LEVEL CLEAR!", gfx::rgb565(120, 255, 120), 3);
            center(120, "TIME BONUS ADDED", gfx::WHITE, 1);
            break;
        case State::GameOver:
            center(90, "GAME OVER", gfx::rgb565(255, 80, 80), 3);
            break;
        default:
            break;
    }
}

}  // namespace mario
