// Kart - logika: tor (linia srodkowa, mapa nawierzchni), fizyka gokarta, AI, przedmioty, ranking.
// Rysowanie 3D (siatki, kamera, HUD) w kart_render.cpp.
#include "games/kart/kart_game.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "engine/rng.h"
#include "engine/storage.h"
#include "gfx/palette.h"
#include "gfx/png.h"
#include "platform/platform.h"

namespace kart {

namespace {

const char* TAG = "kart";

constexpr float PI2 = 6.28318530718f;

// Fizyka (jednostki swiata = teksele toru; gokart ma ok. 14 jednostek dlugosci)
constexpr float MAX_SPEED    = 150.f;
constexpr float BOOST_SPEED  = 225.f;
constexpr float ACCEL        = 95.f;
constexpr float BOOST_ACCEL  = 320.f;
constexpr float COAST_DRAG   = 55.f;    // hamowanie bez gazu (u/s^2)
constexpr float BRAKE        = 190.f;
constexpr float OFFROAD_MAX  = 65.f;
constexpr float OFFROAD_DRAG = 160.f;
constexpr float TURN_RATE    = 2.3f;    // rad/s przy pelnej predkosci sterowania
constexpr float DRIFT_TURN   = 1.55f;   // mnoznik skretu w driftcie
constexpr float SPIN_TIME    = 1.1f;
constexpr float KART_RADIUS  = 7.f;
constexpr float ROAD_HALF    = 32.f;    // asfalt
constexpr float CURB_HALF    = 38.f;    // asfalt + krawezniki

// Uklady torow (punkty kontrolne, pola przyspieszenia, skrzynki): kart_tracks.h
constexpr int N_CTRL = 16;

inline float wrap_angle(float a)
{
    while (a > 3.14159265f) a -= PI2;
    while (a < -3.14159265f) a += PI2;
    return a;
}

// Lokalny generator do dekoracji toru - stale ziarno, zeby tor byl zawsze taki sam (niezaleznie od engine::rng).
struct LocalRng {
    uint32_t s = 0x4B415254u;   // "KART"
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float    unit() { return (float)(next() >> 8) / 16777216.f; }
    int      range(int a, int b) { return a + (int)(next() % (uint32_t)(b - a + 1)); }
};

}  // namespace

// ============================================================================ cykl zycia

void KartGame::init(gfx::Canvas&)
{
    if (!assets_loaded_) {
        assets_loaded_ = true;
        banana_    = gfx::load_png_rgba("kart/banana.png");
        shell_     = gfx::load_png_rgba("kart/shell.png");
        mushroom_  = gfx::load_png_rgba("kart/mushroom.png");
        clouds_    = gfx::load_png_rgba("kart/clouds.png");
        smoke_     = gfx::load_png_rgba("kart/smoke.png");
        glow_      = gfx::load_png_rgba("kart/glow.png");
        flame_     = gfx::load_png_rgba("kart/flame.png");
        CONSOLE_LOGI(TAG, "grafika 2D: %s", smoke_.px ? "PNG z assets/kart" : "BRAK");
        float saved[MAX_TRACKS];
        if (engine::load_data("kart_best", saved, sizeof(saved))) memcpy(rec_lap_, saved, sizeof(rec_lap_));
        build_track();   // linia srodkowa, mapa nawierzchni, skrzynki, drzewa
        build_scene();   // siatki 3D (kart_render.cpp)
    }
    state_ = State::Title;
    new_race();
}

void KartGame::new_race()
{
    for (int i = 0; i < N_KARTS; ++i) {
        Kart& k = karts_[i];
        k = Kart{};
        k.color = i;
        k.lane  = (i == 0) ? 0.f : (i == 1 ? -14.f : (i == 2 ? 12.f : -4.f));
    }
    for (Hazard& h : hazards_) h.alive = false;
    for (Shell& s : shells_) s.alive = false;
    for (Spark& s : sparks_) s.t = 0;
    for (Puff& p : puffs_) p.alive = false;
    for (Skid& s : skids_) s.alive = false;
    for (int i = 0; i < N_KARTS; ++i) skid_on_[i][0] = skid_on_[i][1] = false;
    for (ItemBox& b : boxes_) b.respawn = 0;
    place_karts_on_grid();
    for (int i = 0; i < N_KARTS; ++i) { prev_angle_[i] = karts_[i].angle; yaw_rate_[i] = 0.f; }   // bez skoku przechylu na starcie
    countdown_ = 3.2f;
    race_t_    = 0;
    go_t_      = 0;
    best_lap_  = 0;
    lap_start_t_ = 0;
    cam_angle_ = karts_[0].angle;
    for (Kart& k : karts_) update_progress(k);
    update_ranking();
}

void KartGame::place_karts_on_grid()
{
    // Pola startowe: tuz przed linia mety (indeks 0), po dwa w rzedzie
    for (int i = 0; i < N_KARTS; ++i) {
        const int idx  = (N_PATH - 4 - (i / 2) * 5) % N_PATH;
        const int next = (idx + 1) % N_PATH;
        const float tx = path_x_[next] - path_x_[idx], ty = path_y_[next] - path_y_[idx];
        const float len = sqrtf(tx * tx + ty * ty) + 1e-6f;
        const float px = -ty / len, py = tx / len;   // prawo
        const float side = (i % 2 == 0) ? -12.f : 12.f;
        Kart& k  = karts_[i];
        k.x      = path_x_[idx] + px * side;
        k.y      = path_y_[idx] + py * side;
        k.angle  = atan2f(ty, tx);
        k.path_idx = idx;
        k.lap    = 1;
        k.half   = false;
    }
}

// ============================================================================ tor

void KartGame::select_track(int t)
{
    track_ = (t + TRACK_COUNT) % TRACK_COUNT;
    build_track();
    build_track_scene();
    new_race();
    CONSOLE_LOGI(TAG, "tor %d: %s (%s)", track_, TRACKS[track_].name, THEMES[TRACKS[track_].theme].name);
}

void KartGame::build_track()
{
    const TrackDef& T = TRACKS[track_];
    // 1. Linia srodkowa: Catmull-Rom przez punkty kontrolne, N_PATH probek
    const int per_seg = N_PATH / N_CTRL;
    for (int s = 0; s < N_CTRL; ++s) {
        const float* p0 = T.ctrl[(s - 1 + N_CTRL) % N_CTRL];
        const float* p1 = T.ctrl[s];
        const float* p2 = T.ctrl[(s + 1) % N_CTRL];
        const float* p3 = T.ctrl[(s + 2) % N_CTRL];
        for (int j = 0; j < per_seg; ++j) {
            const float t = (float)j / per_seg, t2 = t * t, t3 = t2 * t;
            const int   i = s * per_seg + j;
            path_x_[i] = 0.5f * ((2 * p1[0]) + (-p0[0] + p2[0]) * t + (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t2 +
                                 (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t3);
            path_y_[i] = 0.5f * ((2 * p1[1]) + (-p0[1] + p2[1]) * t + (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t2 +
                                 (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t3);
        }
    }

    memset(surf_, 0, sizeof(surf_));

    auto dir_at = [&](int idx, float& tx, float& ty) {
        const int n = (idx + 1) % N_PATH;
        tx = path_x_[n] - path_x_[idx];
        ty = path_y_[n] - path_y_[idx];
        const float l = sqrtf(tx * tx + ty * ty) + 1e-6f;
        tx /= l; ty /= l;
    };
    // 2. Mapa nawierzchni: komorki 8x8 pod droga i krawtnikami = asfalt (1)
    auto stamp_surf = [&](float cx, float cy, float r, uint8_t v) {
        const int c0 = (int)((cx - r) / 8.f) - 1, c1 = (int)((cx + r) / 8.f) + 1;
        const int r0 = (int)((cy - r) / 8.f) - 1, r1 = (int)((cy + r) / 8.f) + 1;
        for (int row = r0; row <= r1; ++row) {
            for (int col = c0; col <= c1; ++col) {
                if (row < 0 || col < 0 || row >= SURF || col >= SURF) continue;
                const float dx = col * 8.f + 4.f - cx, dy = row * 8.f + 4.f - cy;
                if (dx * dx + dy * dy <= r * r) surf_[row * SURF + col] = v;
            }
        }
    };
    for (int i = 0; i < N_PATH; ++i) {
        const int   n  = (i + 1) % N_PATH;
        const float dx = path_x_[n] - path_x_[i], dy = path_y_[n] - path_y_[i];
        const float len = sqrtf(dx * dx + dy * dy);
        const int   steps = (int)(len / 4.f) + 1;
        for (int st = 0; st < steps; ++st) {
            const float t = (float)st / steps;
            stamp_surf(path_x_[i] + dx * t, path_y_[i] + dy * t, CURB_HALF, 1);
        }
    }
    // 3. Pola przyspieszenia (2): prostokat 28 x 24 jednostki na drodze
    for (int b = 0; b < 2; ++b) {
        const int idx = T.boost[b];
        float tx, ty;
        dir_at(idx, tx, ty);
        const float px = -ty, py = tx;
        for (int u = -14; u <= 14; u += 2) {
            for (int v = -12; v <= 12; v += 2) {
                const float x = path_x_[idx] + tx * u + px * v, y = path_y_[idx] + ty * u + py * v;
                const int col = (int)(x / 8.f), row = (int)(y / 8.f);
                if (row >= 0 && col >= 0 && row < SURF && col < SURF) surf_[row * SURF + col] = 2;
            }
        }
    }

    // 5. Skrzynki z przedmiotami: 3 rzedy po 3
    for (int g = 0; g < 3; ++g) {
        const int idx = T.box[g];
        float tx, ty;
        dir_at(idx, tx, ty);
        for (int j = 0; j < 3; ++j) {
            const float off = (j - 1) * 18.f;
            boxes_[g * 3 + j].x = path_x_[idx] - ty * off;
            boxes_[g * 3 + j].y = path_y_[idx] + tx * off;
            boxes_[g * 3 + j].respawn = 0;
        }
    }

    // 6. Drzewa przy drodze (na trawie, 55-130 jednostek od linii)
    LocalRng lr;
    int placed = 0;
    for (int tries = 0; tries < 4000 && placed < N_TREES; ++tries) {
        const int idx = lr.range(0, N_PATH - 1);
        float tx, ty;
        dir_at(idx, tx, ty);
        const float side = lr.unit() < 0.5f ? -1.f : 1.f;
        const float d = 55.f + lr.unit() * 75.f;
        const float x = path_x_[idx] - ty * side * d, y = path_y_[idx] + tx * side * d;
        if (x < 20 || y < 20 || x > WORLD - 20 || y > WORLD - 20) continue;
        if (surface_at(x, y) != 0) continue;
        bool too_close = false;
        for (int t = 0; t < placed; ++t) {
            const float ddx = trees_[t].x - x, ddy = trees_[t].y - y;
            if (ddx * ddx + ddy * ddy < 28.f * 28.f) { too_close = true; break; }
        }
        if (too_close) continue;
        // drzewo nie moze stac na drodze ani przy sasiednim odcinku toru: sprawdz 3x3 komorki
        bool near_road = false;
        for (int oy = -1; oy <= 1 && !near_road; ++oy)
            for (int ox = -1; ox <= 1; ox++)
                if (surface_at(x + ox * 8.f, y + oy * 8.f) != 0) { near_road = true; break; }
        if (near_road) continue;
        trees_[placed++] = { x, y, lr.range(0, 1) };
    }
    for (int t = placed; t < N_TREES; ++t) trees_[t] = { -1000.f, -1000.f, 0 };
}

int KartGame::surface_at(float x, float y) const
{
    const int cx = ((int)floorf(x) >> 3) & (SURF - 1);
    const int cy = ((int)floorf(y) >> 3) & (SURF - 1);
    return surf_[cy * SURF + cx];
}

// ============================================================================ fizyka

void KartGame::update_kart(Kart& k, float dt, float steer, bool gas, bool brake, bool drift)
{
    if (k.spin_t > 0) {
        k.spin_t -= dt;
        k.speed = engine::approach(k.speed, 0.f, COAST_DRAG * dt);
        gas = brake = drift = false;
        steer = 0;
    }
    if (k.boost_t > 0) k.boost_t -= dt;
    if (k.shield_t > 0) k.shield_t -= dt;

    const int surface = surface_at(k.x, k.y);
    if (surface == 2 && k.boost_t < 0.6f) k.boost_t = 0.9f;   // pole przyspieszenia

    // predkosc
    float max_speed = k.boost_t > 0 ? BOOST_SPEED : MAX_SPEED;
    if (surface == 0 && k.boost_t <= 0) max_speed = OFFROAD_MAX;
    if (k.boost_t > 0) {
        k.speed = engine::approach(k.speed, max_speed, BOOST_ACCEL * dt);
    } else if (brake) {
        k.speed = engine::approach(k.speed, -40.f, BRAKE * dt);
    } else if (gas) {
        const float target = k.drifting ? max_speed * 0.94f : max_speed;
        if (k.speed < target) k.speed = engine::approach(k.speed, target, ACCEL * dt);
        else                  k.speed = engine::approach(k.speed, target, (surface == 0 ? OFFROAD_DRAG : COAST_DRAG) * dt);
    } else {
        k.speed = engine::approach(k.speed, 0.f, (surface == 0 ? OFFROAD_DRAG : COAST_DRAG) * dt);
    }

    // skret: slabszy przy malej predkosci, lekko slabszy przy maksymalnej; drift = ostrzej
    const float abs_v = engine::absf(k.speed);
    float turn = TURN_RATE * (abs_v < 70.f ? abs_v / 70.f : 1.f - 0.2f * (abs_v - 70.f) / (BOOST_SPEED - 70.f));
    const bool want_drift = drift && steer != 0.f && k.speed > 60.f;
    if (want_drift) {
        turn *= DRIFT_TURN;
        k.drift_t += dt;
        k.drifting = true;
    } else {
        if (k.drifting && k.drift_t > 0.7f) k.boost_t = engine::imax(0, 0) + (k.drift_t > 1.4f ? 0.7f : 0.45f);   // mini-turbo
        k.drifting = false;
        k.drift_t  = 0;
    }
    if (k.speed < 0) steer = -steer;
    k.angle = wrap_angle(k.angle + steer * turn * dt);

    k.x += cosf(k.angle) * k.speed * dt;
    k.y += sinf(k.angle) * k.speed * dt;
    k.x = engine::clampf(k.x, 12.f, WORLD - 12.f);
    k.y = engine::clampf(k.y, 12.f, WORLD - 12.f);
}

void KartGame::ai_control(Kart& k, int index, float dt, float& steer, bool& gas, bool& brake, bool& drift)
{
    // Cel: punkt linii srodkowej kilka probek do przodu, przesuniety o "pas" gokarta
    const int ahead = 7 + (int)(k.speed / 50.f);
    const int ti    = (k.path_idx + ahead) % N_PATH;
    const int tn    = (ti + 1) % N_PATH;
    float tx = path_x_[tn] - path_x_[ti], ty = path_y_[tn] - path_y_[ti];
    const float l = sqrtf(tx * tx + ty * ty) + 1e-6f;
    tx /= l; ty /= l;
    const float goal_x = path_x_[ti] - ty * k.lane, goal_y = path_y_[ti] + tx * k.lane;
    const float want   = atan2f(goal_y - k.y, goal_x - k.x);
    const float diff   = wrap_angle(want - k.angle);
    steer = engine::clampf(diff * 3.f, -1.f, 1.f);
    gas   = true;
    brake = false;
    drift = engine::absf(diff) > 0.45f && k.speed > 90.f;

    // Tempo: kazdy rywal inne + guma (wolniejszy, gdy prowadzi; szybszy, gdy daleko za graczem).
    // index 0 = autopilot gracza: stale tempo srednie, bez gumy.
    const float base   = index == 0 ? 0.94f : 0.84f + 0.05f * index;
    const float gap    = index == 0 ? 0.f : karts_[0].progress - k.progress;   // >0: gracz przed nim
    float       factor = base;
    if (gap > 30.f) factor += 0.10f;
    else if (gap < -40.f) factor -= 0.06f;
    const float limit = MAX_SPEED * factor;
    if (k.speed > limit && k.boost_t <= 0) gas = false;

    // Przedmioty: AI czeka 1-3 s i uzywa
    if (k.item != NONE) {
        k.item_t -= dt;
        if (k.item_t <= 0) use_item(k, index);
    }
    (void)dt;
}

void KartGame::update_progress(Kart& k)
{
    // najblizsza probka w oknie wokol poprzedniej (tor jest ciagly, gokart nie teleportuje sie)
    int   best   = k.path_idx;
    float best_d = 1e30f;
    for (int o = -6; o <= 12; ++o) {
        const int   i  = (k.path_idx + o + N_PATH) % N_PATH;
        const float dx = path_x_[i] - k.x, dy = path_y_[i] - k.y;
        const float d  = dx * dx + dy * dy;
        if (d < best_d) { best_d = d; best = i; }
    }
    const int prev = k.path_idx;
    k.path_idx = best;
    if (best > N_PATH / 2 && best < N_PATH - 20) k.half = true;
    if (prev > N_PATH - 20 && best < 20 && k.half) {   // przejazd przez mete
        k.half = false;
        if (&k == &karts_[0]) {
            const float lap_t = race_t_ - lap_start_t_;
            if (best_lap_ <= 0 || lap_t < best_lap_) best_lap_ = lap_t;
            if (&k == &karts_[0] && !autopilot_ && (rec_lap_[track_] <= 0 || lap_t < rec_lap_[track_])) {
                rec_lap_[track_] = lap_t;   // rekord toru tylko z jazdy recznej
                engine::save_data("kart_best", rec_lap_, sizeof(rec_lap_));
            }
            lap_start_t_ = race_t_;
        }
        k.lap++;
        if (k.lap > LAPS && !k.finished) {
            k.finished    = true;
            k.finish_time = race_t_;
        }
    }
    k.progress = (float)(k.lap * N_PATH + best) - sqrtf(best_d) * 0.01f;
}

void KartGame::update_ranking()
{
    for (int i = 0; i < N_KARTS; ++i) {
        int place = 1;
        for (int j = 0; j < N_KARTS; ++j) {
            if (j == i) continue;
            if (karts_[j].finished && karts_[i].finished) { if (karts_[j].finish_time < karts_[i].finish_time) ++place; }
            else if (karts_[j].finished) ++place;
            else if (!karts_[i].finished && karts_[j].progress > karts_[i].progress) ++place;
        }
        karts_[i].place = place;
    }
}

// ============================================================================ przedmioty

void KartGame::hit_kart(Kart& k)
{
    if (k.shield_t > 0 || k.spin_t > 0) return;
    k.spin_t   = SPIN_TIME;
    k.shield_t = SPIN_TIME + 0.8f;
    k.speed   *= 0.35f;
    k.boost_t  = 0;
    k.drifting = false;
    k.drift_t  = 0;
}

void KartGame::use_item(Kart& k, int index)
{
    const float dx = cosf(k.angle), dy = sinf(k.angle);
    switch (k.item) {
        case MUSHROOM:
            k.boost_t = 1.3f;
            break;
        case BANANA:
            for (Hazard& h : hazards_) {
                if (h.alive) continue;
                h.alive = true;
                h.x = k.x - dx * 16.f;
                h.y = k.y - dy * 16.f;
                break;
            }
            break;
        case SHELL:
            for (Shell& s : shells_) {
                if (s.alive) continue;
                s.alive = true;
                s.owner = index;
                s.x  = k.x + dx * 14.f;
                s.y  = k.y + dy * 14.f;
                s.dx = dx * (240.f + engine::absf(k.speed) * 0.5f);
                s.dy = dy * (240.f + engine::absf(k.speed) * 0.5f);
                s.t  = 3.5f;
                break;
            }
            break;
        default: break;
    }
    k.item = NONE;
}

void KartGame::update_items(float dt, bool player_use)
{
    // skrzynki
    for (ItemBox& b : boxes_) {
        if (b.respawn > 0) { b.respawn -= dt; continue; }
        for (int i = 0; i < N_KARTS; ++i) {
            Kart& k = karts_[i];
            const float dx = k.x - b.x, dy = k.y - b.y;
            if (dx * dx + dy * dy > 10.f * 10.f) continue;
            b.respawn = 4.f;
            if (k.item != NONE) break;
            // losowanie zalezne od miejsca: lider dostaje banany, ostatni grzyby
            const int r = engine::rng().range(0, 99);
            if (k.place == 1)      k.item = r < 55 ? BANANA : (r < 90 ? SHELL : MUSHROOM);
            else if (k.place == 2) k.item = r < 35 ? BANANA : (r < 75 ? SHELL : MUSHROOM);
            else                   k.item = r < 15 ? BANANA : (r < 50 ? SHELL : MUSHROOM);
            k.item_t = 1.f + engine::rng().unit() * 2.5f;
            break;
        }
    }
    if (player_use && karts_[0].item != NONE && karts_[0].spin_t <= 0) use_item(karts_[0], 0);

    // skorupy
    for (Shell& s : shells_) {
        if (!s.alive) continue;
        s.t -= dt;
        s.x += s.dx * dt;
        s.y += s.dy * dt;
        if (s.t <= 0 || s.x < 8 || s.y < 8 || s.x > WORLD - 8 || s.y > WORLD - 8) { s.alive = false; continue; }
        for (int i = 0; i < N_KARTS; ++i) {
            if (i == s.owner && s.t > 3.1f) continue;   // przez chwile nie trafia wlasciciela
            const float dx = karts_[i].x - s.x, dy = karts_[i].y - s.y;
            if (dx * dx + dy * dy < 9.f * 9.f) {
                hit_kart(karts_[i]);
                s.alive = false;
                break;
            }
        }
    }
    // banany
    for (Hazard& h : hazards_) {
        if (!h.alive) continue;
        for (int i = 0; i < N_KARTS; ++i) {
            const float dx = karts_[i].x - h.x, dy = karts_[i].y - h.y;
            if (dx * dx + dy * dy < 8.f * 8.f && karts_[i].shield_t <= 0) {
                hit_kart(karts_[i]);
                h.alive = false;
                break;
            }
        }
    }
}

void KartGame::add_obstacle(float x, float y, float r)
{
    if (n_obst_ < N_OBST) obst_[n_obst_++] = { x, y, r };
}

void KartGame::obstacle_collisions(Kart& k)
{
    // wypchniecie z okregu przeszkody + utrata predkosci (drzewa, stosy opon, slupy, trybuna)
    for (int i = 0; i < n_obst_; ++i) {
        const Obstacle& o = obst_[i];
        const float dx = k.x - o.x, dy = k.y - o.y;
        const float min_d = o.r + KART_RADIUS;
        const float d2 = dx * dx + dy * dy;
        if (d2 >= min_d * min_d) continue;
        const float d = d2 > 1e-4f ? sqrtf(d2) : 1e-2f;
        const float nx = dx / d, ny = dy / d;
        k.x = o.x + nx * min_d;
        k.y = o.y + ny * min_d;
        // skladowa predkosci w strone przeszkody znika (odbicie z tlumieniem), reszta zostaje
        const float into = -(cosf(k.angle) * nx + sinf(k.angle) * ny);   // 1 = jedzie prosto w przeszkode
        if (into > 0.f) k.speed *= 1.f - 0.75f * into;
    }
}

void KartGame::kart_collisions()
{
    for (int i = 0; i < N_KARTS; ++i) {
        for (int j = i + 1; j < N_KARTS; ++j) {
            Kart& a = karts_[i];
            Kart& b = karts_[j];
            const float dx = b.x - a.x, dy = b.y - a.y;
            const float d2 = dx * dx + dy * dy;
            const float min_d = KART_RADIUS * 2.f;
            if (d2 >= min_d * min_d || d2 < 1e-4f) continue;
            const float d = sqrtf(d2);
            const float push = (min_d - d) * 0.5f;
            const float nx = dx / d, ny = dy / d;
            a.x -= nx * push; a.y -= ny * push;
            b.x += nx * push; b.y += ny * push;
            // Kara predkosci tylko przy ZBLIZANIU sie, proporcjonalna do predkosci zblizania. Wczesniej *0.92 co klatke
            // styku: dwa gokarty na tym samym torze jazdy (autopilot i rywal) sklejaly sie i pelzly ~19 km/h
            // (rownowaga ACCEL*dt = 0.08*v) - wykryte na torze Kanion (25.09.2026).
            const float vax = cosf(a.angle) * a.speed, vay = sinf(a.angle) * a.speed;
            const float vbx = cosf(b.angle) * b.speed, vby = sinf(b.angle) * b.speed;
            const float closing = (vax - vbx) * nx + (vay - vby) * ny;   // >0: a nachodzi na b
            if (closing > 0.f) {
                const float k = 1.f - engine::clampf(closing / 250.f, 0.f, 0.15f);
                a.speed *= k;
                b.speed *= k;
            }
        }
    }
}

void KartGame::add_spark(float x, float y, float vx, float vy, float t, uint16_t color)
{
    for (Spark& s : sparks_) {
        if (s.t > 0) continue;
        s = { x, y, vx, vy, t, color };
        return;
    }
}

void KartGame::add_puff(float x, float y, float size, float life, int kind)
{
    for (Puff& p : puffs_) {
        if (p.alive) continue;
        p = { x, y, 0.f, life, size, true, (uint8_t)kind };
        return;
    }
}

void KartGame::update_skids(float dt)
{
    // slady opon: tylne kola przy drifcie i wirowaniu na asfalcie; segment co ~1.5 jednostki, bufor cykliczny
    for (int i = 0; i < N_KARTS; ++i) {
        const Kart& k = karts_[i];
        const bool mark = (k.drifting || k.spin_t > 0) && k.speed > 30.f && surface_at(k.x, k.y) != 0;
        const float fx = cosf(k.angle), fy = sinf(k.angle), rx = -fy, ry = fx;
        for (int w = 0; w < 2; ++w) {
            const float side = w ? 6.6f : -6.6f;
            const float wx = k.x - fx * 6.4f + rx * side, wy = k.y - fy * 6.4f + ry * side;
            float& px = skid_prev_[i][w][0];
            float& py = skid_prev_[i][w][1];
            if (mark && skid_on_[i][w]) {
                const float dx = wx - px, dy = wy - py;
                const float l = sqrtf(dx * dx + dy * dy);
                if (l > 1.5f) {
                    const float nx = -dy / l * 0.9f, ny = dx / l * 0.9f;
                    Skid& s = skids_[skid_next_];
                    skid_next_ = (skid_next_ + 1) % N_SKIDS;
                    s = { { px - nx, px + nx, wx + nx, wx - nx }, { py - ny, py + ny, wy + ny, wy - ny }, { 0, 0, 0, 0 }, 0.f, true };
                    for (int c = 0; c < 4; ++c) s.h[c] = surface_height(s.x[c], s.y[c]) + 0.08f;
                    px = wx; py = wy;
                }
            } else {
                px = wx; py = wy;
            }
            skid_on_[i][w] = mark;
        }
    }
    for (Skid& s : skids_) {
        if (!s.alive) continue;
        s.age += dt;
        if (s.age > 30.f) s.alive = false;
    }
}

void KartGame::update_effects(float dt)
{
    update_skids(dt);
    for (Spark& s : sparks_) {
        if (s.t <= 0) continue;
        s.t -= dt; s.x += s.vx * dt; s.y += s.vy * dt; s.vy += 400.f * dt;
    }
    for (Puff& p : puffs_) {
        if (!p.alive) continue;
        p.t += dt;
        p.size += 9.f * dt;
        if (p.t >= p.life) p.alive = false;
    }
    // obrot kol i tempo skretu (do przechylu nadwozia) - tylko wizualne
    for (int i = 0; i < N_KARTS; ++i) {
        const Kart& k = karts_[i];
        wheel_spin_[i] = fmodf(wheel_spin_[i] + k.speed * dt / 2.6f, 6.2831853f);
        const float yaw = wrap_angle(k.angle - prev_angle_[i]) / (dt > 1e-4f ? dt : 1e-4f);
        prev_angle_[i]  = k.angle;
        yaw_rate_[i]    = yaw_rate_[i] * 0.85f + yaw * 0.15f;
    }
    // dym spod kol przy driftcie, kurz na trawie
    ++puff_phase_;
    for (const Kart& k : karts_) {
        if (k.speed < 45.f) continue;
        const int surf = surface_at(k.x, k.y);
        const bool drift = k.drifting, dust = surf == 0;
        if (!drift && !dust) continue;
        if ((puff_phase_ & 1) && !drift) continue;
        const float bx = k.x - cosf(k.angle) * 6.f, by = k.y - sinf(k.angle) * 6.f;
        const float px = -sinf(k.angle) * 5.f, py = cosf(k.angle) * 5.f;
        const float side = (puff_phase_ & 2) ? 1.f : -1.f;
        add_puff(bx + px * side, by + py * side, drift ? 5.f : 4.f, drift ? 0.7f : 0.5f, drift ? 0 : 1);
    }
}

// ============================================================================ glowna petla

void KartGame::update(float dt, const input::PadState& pad)
{
    anim_ += dt;
    update_effects(dt);

    switch (state_) {
        case State::Title: {
            // LEWO/PRAWO = wybor toru (przebudowa sceny), A/B albo dotyk = start
            const bool l = pad.left, r = pad.right;
            const bool le = l && !title_lr_[0], re = r && !title_lr_[1];
            title_lr_[0] = l; title_lr_[1] = r;
            if (le || re) { select_track(track_ + (re ? 1 : -1)); break; }
            const bool dir = pad.up || pad.down || l || r;
            if (pad.a_pressed || pad.b_pressed || (pad.any_pressed && !pad.start_pressed && !dir)) {
                new_race();
                state_ = State::Countdown;
            }
            break;
        }

        case State::Countdown:
            countdown_ -= dt;
            if (countdown_ <= 0) {
                state_ = State::Racing;
                go_t_  = 0.9f;
                // start z gazem = lekki "rocket start"
                if (pad.a || pad.up) karts_[0].boost_t = 0.5f;
            }
            break;

        case State::Racing:
        case State::Finished: {
            race_t_ += dt;
            if (go_t_ > 0) go_t_ -= dt;

            // gracz
            Kart& p = karts_[0];
            autopilot_ = pad.y && !p.finished;
            if (autopilot_ || p.finished) {   // po mecie gokart jedzie dalej sam (jak w Mario Kart)
                float steer; bool gas, brake, drift;
                ai_control(p, 0, dt, steer, gas, brake, drift);
                steer_vis_[0] = steer;
                update_kart(p, dt, steer, gas, brake, drift);
            } else {
                const float steer = (pad.right ? 1.f : 0.f) - (pad.left ? 1.f : 0.f) +
                                    (engine::absf(pad.stick_x) > 0.5f ? 0.f : pad.stick_x);
                steer_vis_[0] = engine::clampf(steer, -1.f, 1.f);
                update_kart(p, dt, engine::clampf(steer, -1.f, 1.f), pad.a || pad.up, pad.down, pad.b);
            }
            // rywale
            for (int i = 1; i < N_KARTS; ++i) {
                Kart& k = karts_[i];
                float steer; bool gas, brake, drift;
                ai_control(k, i, dt, steer, gas, brake, drift);
                steer_vis_[i] = steer;
                update_kart(k, dt, steer, gas, brake, drift);
            }
            kart_collisions();
            for (Kart& k : karts_) obstacle_collisions(k);
            update_items(dt, pad.x_pressed && !p.finished);
            for (Kart& k : karts_) update_progress(k);
            update_ranking();

            if (p.finished && state_ == State::Racing) state_ = State::Finished;
            if (state_ == State::Finished && pad.a_pressed && race_t_ - p.finish_time > 1.f) {
                new_race();
                state_ = State::Countdown;
            }

            // kamera: podaza za katem gracza z opoznieniem (widac skret gokarta)
            const float da = wrap_angle(p.angle - cam_angle_);
            cam_angle_ = wrap_angle(cam_angle_ + da * engine::clampf(7.f * dt, 0.f, 1.f));
            break;
        }
    }
    cam_x_ = karts_[0].x - cosf(cam_angle_) * 44.f;
    cam_y_ = karts_[0].y - sinf(cam_angle_) * 44.f;
}

void KartGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "COUNT", "RACE", "FINISH" };
    static const char* const ITEMS[] = { "-", "mushroom", "banana", "shell" };
    const Kart& p = karts_[0];
    int hazards = 0, shells = 0;
    for (const Hazard& h : hazards_) if (h.alive) ++hazards;
    for (const Shell& s : shells_) if (s.alive) ++shells;
    snprintf(buf, n,
             "%-6s lap=%d/%d place=%d x=%.0f y=%.0f ang=%.2f spd=%.0f idx=%d item=%s surf=%d t=%.1f boost=%.1f spin=%.1f "
             "ai=%d.%d/%d.%d/%d.%d haz=%d sh=%d",
             NAMES[(int)state_], p.lap > LAPS ? LAPS : p.lap, LAPS, p.place, (double)p.x, (double)p.y, (double)p.angle,
             (double)p.speed, p.path_idx, ITEMS[p.item], surface_at(p.x, p.y), (double)race_t_, (double)p.boost_t,
             (double)p.spin_t, karts_[1].lap, karts_[1].path_idx, karts_[2].lap, karts_[2].path_idx, karts_[3].lap,
             karts_[3].path_idx, hazards, shells);
}

}  // namespace kart
