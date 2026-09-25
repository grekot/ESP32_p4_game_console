// Snake - logika: plansze, ruch po kratkach, przedmioty, punkty, stany gry, autopilot.
#include "games/snake/snake_game.h"

#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "engine/rng.h"
#include "gfx/palette.h"

namespace snake {

namespace {

constexpr int   DX[4] = { 1, 0, -1, 0 };
constexpr int   DY[4] = { 0, 1, 0, -1 };
constexpr int   START_LEN   = 4;
constexpr int   MAX_LIVES   = 5;
constexpr float MIN_STEP    = 0.055f;   // najszybszy waz: ~18 kratek/s
constexpr float TURBO_MUL   = 0.6f;     // A trzymane
constexpr float SLOW_MUL    = 1.6f;     // klepsydra
constexpr float COMBO_TIME  = 3.0f;     // okno na kolejne jablko
constexpr int   ENDLESS_WORLD_APPLES = 20;

// Plansze 25x14: '.' wolne, '#' przeszkoda, 'S' glowa na starcie (waz lezy w lewo od niej, jedzie w prawo).
struct LevelDef {
    const char* name;
    int         world;
    int         need;       // jablek do otwarcia portalu
    float       step;       // odstep krokow na starcie poziomu [s]
    bool        wrap;       // krawedzie zawijaja
    bool        bombs;
    const char* rows[14];
};

const LevelDef LEVELS[10] = {
    { "Pierwsze kroki", 0, 8, 0.150f, true, false, {
        ".........................",
        ".........................",
        ".........................",
        ".........................",
        ".........................",
        ".........................",
        ".........................",
        "......S..................",
        ".........................",
        ".........................",
        ".........................",
        ".........................",
        ".........................",
        "........................." } },
    { "Ogrod", 0, 10, 0.145f, false, false, {
        ".........................",
        ".........................",
        "...##...............##...",
        "...#.................#...",
        ".........................",
        "..........#...#..........",
        ".........................",
        ".....S...................",
        "..........#...#..........",
        ".........................",
        "...#.................#...",
        "...##...............##...",
        ".........................",
        "........................." } },
    { "Oaza", 1, 12, 0.140f, false, true, {
        ".........................",
        ".........................",
        "......#...........#......",
        ".........................",
        "...#......#####......#...",
        "..........#...#..........",
        ".........................",
        "....S.....#...#..........",
        "..........#####..........",
        "...#.................#...",
        ".........................",
        "......#...........#......",
        ".........................",
        "........................." } },
    { "Kaniony", 1, 12, 0.135f, true, true, {
        ".........................",
        "#########.......#########",
        ".........................",
        ".........................",
        ".........................",
        ".......###########.......",
        ".........................",
        "...S.....................",
        ".......###########.......",
        ".........................",
        ".........................",
        ".........................",
        "#########.......#########",
        "........................." } },
    { "Lodowisko", 2, 14, 0.130f, false, true, {
        ".........................",
        ".........................",
        "..##....##.....##....##..",
        "..##....##.....##....##..",
        ".........................",
        ".........................",
        ".........................",
        "..S......................",
        ".........................",
        ".........................",
        "..##....##.....##....##..",
        "..##....##.....##....##..",
        ".........................",
        "........................." } },
    { "Zamiec", 2, 14, 0.125f, true, true, {
        "#...........#...........#",
        ".........................",
        "....#.....#.....#....#...",
        ".........................",
        "..#....#.....#.....#.....",
        ".........................",
        "#.....................#..",
        "....S....................",
        ".........#.......#.......",
        "...#.................#...",
        ".........................",
        "......#.....#......#.....",
        ".........................",
        "#...........#...........#" } },
    { "Labirynt", 3, 15, 0.120f, false, true, {
        ".........................",
        ".######.....#.....######.",
        ".#..........#..........#.",
        ".#..........#..........#.",
        ".........................",
        ".....#####.....#####.....",
        ".........................",
        "....S....................",
        ".....#####.....#####.....",
        ".........................",
        ".#..........#..........#.",
        ".#..........#..........#.",
        ".######.....#.....######.",
        "........................." } },
    { "Pnacza", 3, 16, 0.115f, true, true, {
        "....#.......#.......#....",
        "....#.......#.......#....",
        "....#...............#....",
        ".........................",
        "........#.......#........",
        "........#.......#........",
        ".........................",
        "...S.....................",
        "........#.......#........",
        "........#.......#........",
        ".........................",
        "....#...............#....",
        "....#.......#.......#....",
        "....#.......#.......#...." } },
    { "Krater", 4, 18, 0.110f, false, true, {
        ".........................",
        ".........................",
        "........#########........",
        "......##.........##......",
        ".....#.............#.....",
        "....#...............#....",
        ".........................",
        "..S......................",
        "....#...............#....",
        ".....#.............#.....",
        "......##.........##......",
        "........####.####........",
        ".........................",
        "........................." } },
    { "Serce wulkanu", 4, 20, 0.105f, false, true, {
        ".........................",
        ".###.......#.#.......###.",
        ".#.........#.#.........#.",
        ".....#.............#.....",
        ".....#...#######...#.....",
        ".....#.............#.....",
        ".........................",
        "..S......................",
        ".....#.............#.....",
        ".....#...#######...#.....",
        ".....#.............#.....",
        ".#.........#.#.........#.",
        ".###.......#.#.......###.",
        "........................." } },
};

// Waga losowania bonusow (poziom 1-2 bez bomb, bomby maja osobny zegar).
struct BonusW { SnakeGame::Item type; int w; };
const BonusW BONUS[] = {
    { SnakeGame::GOLDEN, 26 }, { SnakeGame::MUSHROOM, 14 }, { SnakeGame::HOURGLASS, 12 }, { SnakeGame::STAR, 10 },
    { SnakeGame::MAGNET, 12 }, { SnakeGame::GEM, 12 }, { SnakeGame::ORB, 8 }, { SnakeGame::HEART, 6 },
};

uint16_t item_color(SnakeGame::Item t)
{
    switch (t) {
    case SnakeGame::APPLE:     return gfx::rgb565(235, 60, 50);
    case SnakeGame::GOLDEN:    return gfx::rgb565(255, 210, 60);
    case SnakeGame::MUSHROOM:  return gfx::rgb565(190, 110, 230);
    case SnakeGame::HOURGLASS: return gfx::rgb565(90, 170, 255);
    case SnakeGame::STAR:      return gfx::rgb565(255, 230, 90);
    case SnakeGame::HEART:     return gfx::rgb565(255, 80, 110);
    case SnakeGame::MAGNET:    return gfx::rgb565(230, 70, 70);
    case SnakeGame::GEM:       return gfx::rgb565(90, 230, 110);
    case SnakeGame::ORB:       return gfx::rgb565(80, 200, 255);
    default:                   return gfx::rgb565(255, 140, 40);
    }
}

}  // namespace

// ============================================================================ cykl zycia

void SnakeGame::init(gfx::Canvas&)
{
    if (!assets_loaded_) {
        assets_loaded_ = true;
        load_assets();
    }
    state_   = State::Title;
    state_t_ = 0;
    autopilot_ = false;
    world_ = 0;
    level_ = 0;
    wrap_  = true;
    memset(solid_, 0, sizeof(solid_));
    baked_world_ = -1;
    baked_level_ = -2;
}

const char* SnakeGame::world_name(int w) const
{
    static const char* const N[5] = { "Laka", "Pustynia", "Zima", "Dzungla", "Wulkan" };
    return N[engine::iclamp(w, 0, 4)];
}

void SnakeGame::start_game()
{
    score_ = 0;
    lives_ = 3;
    apples_total_ = 0;
    level_ = mode_ == Mode::Adventure ? start_level_ : 0;
    last_rank_ = -1;
    load_level();
}

void SnakeGame::load_level()
{
    memset(solid_, 0, sizeof(solid_));
    start_ = { 6, 7 };
    if (mode_ == Mode::Adventure) {
        const LevelDef& L = LEVELS[level_];
        world_ = L.world;
        wrap_  = L.wrap;
        need_  = L.need;
        for (int y = 0; y < ROWS; ++y) {
            const char* row = L.rows[y];
            for (int x = 0; x < COLS && row[x]; ++x) {
                if (row[x] == '#') solid_[y][x] = 1;
                else if (row[x] == 'S') start_ = { (int8_t)x, (int8_t)y };
            }
        }
        // wariant grafiki: sciana (przeszkoda z sasiadem) = glowny material swiata, pojedyncza = losowo z 3
        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                if (!solid_[y][x]) continue;
                const bool nb = (x > 0 && solid_[y][x - 1]) || (x < COLS - 1 && solid_[y][x + 1]) || (y > 0 && solid_[y - 1][x]) ||
                                (y < ROWS - 1 && solid_[y + 1][x]);
                solid_[y][x] = nb ? 1 : (uint8_t)(1 + ((x * 7 + y * 13 + x * y) % 3));
            }
        }
    } else {
        world_ = (apples_total_ / ENDLESS_WORLD_APPLES) % 5;
        wrap_  = true;
        need_  = 0;
    }
    apples_ = 0;
    level_time_ = 0;
    portal_open_ = false;
    for (Pickup& p : pickups_) p.alive = false;
    for (Part& p : parts_) p.alive = false;
    for (Popup& p : popups_) p.alive = false;
    reset_snake();
    state_   = State::Intro;
    state_t_ = 0;
}

void SnakeGame::reset_snake()
{
    len_  = START_LEN;
    grow_ = 0;
    for (int i = 0; i < len_; ++i) {
        body_[i] = { (int8_t)(start_.x - i), (int8_t)start_.y };
        prev_[i] = body_[i];
    }
    dir_ = last_dir_ = RIGHT;
    queued_  = 0;
    step_acc_ = 0;
    slow_t_ = ghost_t_ = magnet_t_ = x2_t_ = 0;
    shield_ = false;
    combo_  = 0;
    combo_t_ = 0;
    bonus_t_ = 5.f;
    bomb_t_  = 9.f;
    banner_t_ = 0;
    // jedno jablko zawsze na planszy; stare bonusy znikaja (po smierci tez)
    for (Pickup& p : pickups_) p.alive = false;
    spawn_item(APPLE, -1);
    if (portal_open_) open_portal();
}

// ============================================================================ pomocnicze

void SnakeGame::wrap_cell(int& x, int& y) const
{
    if (x < 0) x += COLS;
    if (x >= COLS) x -= COLS;
    if (y < 0) y += ROWS;
    if (y >= ROWS) y -= ROWS;
}

bool SnakeGame::snake_at(int x, int y, bool skip_tail) const
{
    const int n = skip_tail ? len_ - 1 : len_;
    for (int i = 0; i < n; ++i)
        if (body_[i].x == x && body_[i].y == y) return true;
    return false;
}

SnakeGame::Pickup* SnakeGame::pickup_at(int x, int y)
{
    for (Pickup& p : pickups_)
        if (p.alive && p.x == x && p.y == y) return &p;
    return nullptr;
}

bool SnakeGame::cell_blocked(int x, int y, bool for_spawn) const
{
    if (x < 0 || y < 0 || x >= COLS || y >= ROWS) return true;
    if (solid_[y][x]) return true;
    if (snake_at(x, y, false)) return true;
    if (portal_open_ && portal_.x == x && portal_.y == y) return true;
    if (for_spawn) {
        for (const Pickup& p : pickups_)
            if (p.alive && p.x == x && p.y == y) return true;
    }
    return false;
}

bool SnakeGame::random_free_cell(int& x, int& y, int margin)
{
    for (int attempt = 0; attempt < 400; ++attempt) {
        x = engine::rng().range(0, COLS - 1);
        y = engine::rng().range(0, ROWS - 1);
        if (cell_blocked(x, y, true)) continue;
        // nie tuz przed glowa - nieuczciwe bomby i "darmowe" jablka
        const int hx = body_[0].x, hy = body_[0].y;
        if (engine::iabs(x - hx) + engine::iabs(y - hy) < margin) continue;
        // nie przy krawedzi, gdy krawedz zabija (trudne do wziecia)
        return true;
    }
    return false;
}

void SnakeGame::spawn_item(Item type, float life)
{
    int x, y;
    if (!random_free_cell(x, y, type == BOMB ? 5 : 3)) return;
    for (Pickup& p : pickups_) {
        if (p.alive) continue;
        p.alive = true;
        p.type  = type;
        p.x = (int8_t)x;
        p.y = (int8_t)y;
        p.fx = (float)x;
        p.fy = (float)y;
        p.life = life;
        p.age  = 0;
        return;
    }
}

void SnakeGame::spawn_bonus()
{
    int bonuses = 0;
    for (const Pickup& p : pickups_)
        if (p.alive && p.type != APPLE && p.type != BOMB) ++bonuses;
    if (bonuses >= 2) return;
    int total = 0;
    for (const BonusW& b : BONUS) total += b.w;
    int r = engine::rng().range(0, total - 1);
    Item t = GOLDEN;
    for (const BonusW& b : BONUS) {
        if (r < b.w) { t = b.type; break; }
        r -= b.w;
    }
    if (t == HEART && lives_ >= MAX_LIVES) t = GOLDEN;
    if (t == MUSHROOM && len_ < 7) t = GEM;
    spawn_item(t, t == GOLDEN ? 6.f : 9.f);
}

float SnakeGame::step_interval(bool turbo) const
{
    float base = mode_ == Mode::Adventure ? LEVELS[level_].step : 0.15f - 0.004f * (float)(apples_total_ / 5);
    // przyspieszanie z dlugoscia: -1% na segment ponad startowe, maks. -40%
    const float grow = engine::clampf(1.f - 0.01f * (float)(len_ - START_LEN), 0.6f, 1.f);
    float t = base * grow;
    if (t < MIN_STEP) t = MIN_STEP;
    if (slow_t_ > 0) t *= SLOW_MUL;
    if (turbo) t *= TURBO_MUL;
    return t;
}

void SnakeGame::burst(float x, float y, uint16_t color, int count, float speed)
{
    for (int i = 0; i < count; ++i) {
        for (Part& p : parts_) {
            if (p.alive) continue;
            const float a = engine::rng().unit() * 6.2832f;
            const float v = speed * (0.35f + 0.65f * engine::rng().unit());
            p.alive = true;
            p.x = x; p.y = y;
            p.vx = cosf(a) * v;
            p.vy = sinf(a) * v;
            p.t0 = p.t = 0.35f + 0.4f * engine::rng().unit();
            p.color = color;
            p.size  = (uint8_t)engine::rng().range(2, 4);
            break;
        }
    }
}

void SnakeGame::popup(float x, float y, const char* text, uint16_t color)
{
    Popup* best = &popups_[0];
    for (Popup& p : popups_) {
        if (!p.alive) { best = &p; break; }
        if (p.t < best->t) best = &p;
    }
    best->alive = true;
    best->x = x;
    best->y = y;
    best->t = 1.0f;
    best->color = color;
    snprintf(best->text, sizeof(best->text), "%s", text);
}

void SnakeGame::add_score(int pts, int cx, int cy)
{
    if (x2_t_ > 0) pts *= 2;
    score_ += pts;
    char b[12];
    snprintf(b, sizeof(b), "+%d", pts);
    popup((float)(cx * CELL + CELL / 2), (float)(FIELD_Y + cy * CELL), b, x2_t_ > 0 ? gfx::rgb565(120, 255, 140) : gfx::WHITE);
}

void SnakeGame::set_banner(const char* s)
{
    snprintf(banner_, sizeof(banner_), "%s", s);
    banner_t_ = 1.6f;
}

void SnakeGame::open_portal()
{
    int x, y;
    if (!random_free_cell(x, y, 6)) { x = start_.x; y = start_.y; }
    portal_ = { (int8_t)x, (int8_t)y };
    portal_open_ = true;
    // jablka i bonusy na miejscu portalu nie zostaja
    if (Pickup* p = pickup_at(x, y)) p->alive = false;
}

void SnakeGame::submit_score()
{
    last_rank_ = -1;
    for (int i = 0; i < TOP; ++i) {
        if (score_ > top_[i].score) {
            for (int j = TOP - 1; j > i; --j) top_[j] = top_[j - 1];
            top_[i] = { score_, mode_ == Mode::Adventure ? level_ + 1 : apples_total_, (uint8_t)mode_ };
            last_rank_ = i;
            return;
        }
    }
}

// ============================================================================ sterowanie

void SnakeGame::read_input(const input::PadState& pad)
{
    const bool held[4] = { pad.right, pad.down, pad.left, pad.up };
    for (int d = 0; d < 4; ++d) {
        const bool edge = held[d] && !held_prev_[d];
        const Dir  last = queued_ ? queue_[queued_ - 1] : dir_;
        // nowy wcisniety kierunek zawsze trafia do kolejki; trzymany - tylko gdy kolejka pusta i rozny od biezacego
        if ((edge || (held[d] && queued_ == 0)) && d != last && d != ((last + 2) & 3) && queued_ < QUEUE)
            queue_[queued_++] = (Dir)d;
        held_prev_[d] = held[d];
    }
}

// ============================================================================ logika

void SnakeGame::update(float dt, const input::PadState& pad)
{
    anim_    += dt;
    state_t_ += dt;
    if (banner_t_ > 0) banner_t_ -= dt;
    if (flash_t_ > 0) flash_t_ -= dt;

    for (Part& p : parts_) {
        if (!p.alive) continue;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx *= 1.f - 2.5f * dt;
        p.vy *= 1.f - 2.5f * dt;
        p.t -= dt;
        if (p.t <= 0) p.alive = false;
    }
    for (Popup& p : popups_) {
        if (!p.alive) continue;
        p.y -= 40.f * dt;
        p.t -= dt;
        if (p.t <= 0) p.alive = false;
    }

    switch (state_) {
    case State::Title:
        update_title(pad);
        break;
    case State::Intro:
        read_input(pad);   // kierunek wcisniety w odliczaniu zadziala od razu po starcie
        if (pad.y_pressed) autopilot_ = !autopilot_;
        if (state_t_ >= 2.0f || (state_t_ > 0.6f && pad.a_pressed)) {
            state_ = State::Playing;
            state_t_ = 0;
        }
        break;
    case State::Playing:
        update_playing(dt, pad);
        break;
    case State::Dying: {
        // segmenty wybuchaja od glowy co 25 ms, potem chwila przerwy
        const int target = engine::imin(len_, (int)(state_t_ / 0.025f));
        while (dying_i_ < target) {
            float x, y;
            seg_pos(dying_i_, x, y);
            static const uint16_t SKIN[3] = { gfx::rgb565(110, 200, 70), gfx::rgb565(70, 150, 235), gfx::rgb565(245, 145, 50) };
            burst(x, y, SKIN[skin_], 4, 140.f);
            ++dying_i_;
        }
        if (state_t_ > 0.025f * len_ + 0.9f) {
            if (lives_ <= 0) {
                submit_score();
                state_ = State::GameOver;
                state_t_ = 0;
            } else {
                reset_snake();
                state_ = State::Intro;
                state_t_ = 0.8f;   // krotsze odliczanie po smierci
            }
        }
        break;
    }
    case State::LevelClear:
        // autopilot (demo) sam przechodzi dalej po 2,5 s
        if ((state_t_ > 1.0f && pad.a_pressed) || (autopilot_ && state_t_ > 2.5f)) {
            if (level_ + 1 >= 10) {
                submit_score();
                state_ = State::Victory;
                state_t_ = 0;
            } else {
                ++level_;
                load_level();
            }
        }
        break;
    case State::GameOver:
    case State::Victory:
        if (state_t_ > 1.2f && pad.a_pressed) {
            state_ = State::Title;
            state_t_ = 0;
        }
        break;
    }
}

void SnakeGame::update_title(const input::PadState& pad)
{
    static bool prev_ud[2] = {}, prev_lr[2] = {};
    const bool up_e = pad.up && !prev_ud[0], down_e = pad.down && !prev_ud[1];
    const bool left_e = pad.left && !prev_lr[0], right_e = pad.right && !prev_lr[1];
    prev_ud[0] = pad.up; prev_ud[1] = pad.down; prev_lr[0] = pad.left; prev_lr[1] = pad.right;
    if (up_e || down_e) title_row_ ^= 1;
    if (left_e || right_e) {
        const int d = right_e ? 1 : -1;
        if (title_row_ == 0) mode_ = mode_ == Mode::Adventure ? Mode::Endless : Mode::Adventure;
        else skin_ = (skin_ + 3 + d) % 3;
    }
    if (pad.x_pressed) start_level_ = (start_level_ + 1) % 10;   // wybor poziomu startowego (trening)
    if (state_t_ > 0.03f && pad.a_pressed) start_game();
}

void SnakeGame::update_playing(float dt, const input::PadState& pad)
{
    if (pad.y_pressed) {
        autopilot_ = !autopilot_;
        set_banner(autopilot_ ? "AUTOPILOT" : "STEROWANIE RECZNE");
    }
    if (!autopilot_) read_input(pad);

    level_time_ += dt;
    const float timers_dt = dt;
    if (slow_t_ > 0) slow_t_ -= timers_dt;
    if (ghost_t_ > 0) ghost_t_ -= timers_dt;
    if (magnet_t_ > 0) magnet_t_ -= timers_dt;
    if (x2_t_ > 0) x2_t_ -= timers_dt;
    if (combo_t_ > 0) { combo_t_ -= timers_dt; if (combo_t_ <= 0) combo_ = 0; }

    // bonusy i bomby na zegarze
    bonus_t_ -= dt;
    if (bonus_t_ <= 0) {
        spawn_bonus();
        bonus_t_ = 6.f + 5.f * engine::rng().unit();
    }
    const bool bombs = mode_ == Mode::Adventure ? LEVELS[level_].bombs : apples_total_ >= 10;
    if (bombs) {
        bomb_t_ -= dt;
        if (bomb_t_ <= 0) {
            int count = 0;
            for (const Pickup& p : pickups_) if (p.alive && p.type == BOMB) ++count;
            const int max_bombs = mode_ == Mode::Adventure ? 1 + level_ / 3 : 1 + apples_total_ / 25;
            if (count < max_bombs) spawn_item(BOMB, 12.f);
            bomb_t_ = 7.f + 5.f * engine::rng().unit();
        }
    }
    for (Pickup& p : pickups_) {
        if (!p.alive) continue;
        p.age += dt;
        // plynne dojscie do kratki (magnes przesuwa przedmioty)
        p.fx += ((float)p.x - p.fx) * engine::clampf(12.f * dt, 0.f, 1.f);
        p.fy += ((float)p.y - p.fy) * engine::clampf(12.f * dt, 0.f, 1.f);
        if (p.life > 0) {
            p.life -= dt;
            if (p.life <= 0) {
                p.alive = false;
                const float cx = (float)(p.x * CELL + CELL / 2), cy = (float)(FIELD_Y + p.y * CELL + CELL / 2);
                if (p.type == BOMB) {   // bomba wybucha sama, bez szkody - efekt ostrzegawczy
                    burst(cx, cy, gfx::rgb565(255, 160, 40), 18, 200.f);
                    burst(cx, cy, gfx::rgb565(90, 90, 90), 10, 120.f);
                } else {
                    burst(cx, cy, gfx::rgb565(230, 230, 230), 8, 80.f);
                }
            }
        }
    }

    const bool turbo = pad.a && !autopilot_;
    step_int_ = step_interval(turbo);
    step_acc_ += dt;
    while (state_ == State::Playing && step_acc_ >= step_int_) {
        step_acc_ -= step_int_;
        if (autopilot_) {
            const Dir d = autopilot_dir();
            queued_ = 0;
            if (d != dir_) queue_[queued_++] = d;
        }
        step();
        if (turbo && state_ == State::Playing) score_ += 1;
    }
    if (state_ != State::Playing) step_acc_ = 0;
}

void SnakeGame::step()
{
    if (queued_) {
        dir_ = queue_[0];
        for (int i = 1; i < queued_; ++i) queue_[i - 1] = queue_[i];
        --queued_;
    }
    int nx = body_[0].x + DX[dir_], ny = body_[0].y + DY[dir_];
    const bool ghost = ghost_t_ > 0;
    if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS) {
        if (wrap_ || ghost) wrap_cell(nx, ny);
        else { crash(); return; }
    }
    // zderzenie: przeszkoda albo wlasne cialo (ogon sie odsunie, jesli waz nie rosnie)
    if (!ghost && (solid_[ny][nx] || snake_at(nx, ny, grow_ == 0))) { crash(); return; }
    Pickup* p = pickup_at(nx, ny);
    if (p && p->type == BOMB && !ghost) {
        p->alive = false;
        burst((float)(nx * CELL + 16), (float)(FIELD_Y + ny * CELL + 16), gfx::rgb565(255, 160, 40), 24, 240.f);
        crash();
        return;
    }

    // ruch
    for (int i = 0; i < len_; ++i) prev_[i] = body_[i];
    if (grow_ > 0 && len_ < MAX_LEN) {
        prev_[len_] = body_[len_ - 1];
        ++len_;
        --grow_;
    }
    for (int i = len_ - 1; i > 0; --i) body_[i] = body_[i - 1];
    body_[0] = { (int8_t)nx, (int8_t)ny };
    last_dir_ = dir_;

    if (portal_open_ && nx == portal_.x && ny == portal_.y) {
        // koniec poziomu: premia za czas i dlugosc
        const int par = 25 + need_ * 4;
        clear_bonus_ = engine::imax(0, par - (int)level_time_) * 10 + len_ * 5 + lives_ * 50;
        score_ += clear_bonus_;
        burst((float)(nx * CELL + 16), (float)(FIELD_Y + ny * CELL + 16), gfx::rgb565(180, 120, 255), 40, 260.f);
        state_ = State::LevelClear;
        state_t_ = 0;
        return;
    }
    if (p) eat(*p);

    // magnes: jablka w promieniu 5 kratek podchodza o jedna kratke do glowy
    if (magnet_t_ > 0) {
        for (Pickup& q : pickups_) {
            if (!q.alive || (q.type != APPLE && q.type != GOLDEN)) continue;
            const int dx = body_[0].x - q.x, dy = body_[0].y - q.y;
            if (engine::iabs(dx) > 5 || engine::iabs(dy) > 5) continue;
            int tx = q.x, ty = q.y;
            if (engine::iabs(dx) >= engine::iabs(dy)) tx += dx > 0 ? 1 : -1;
            else ty += dy > 0 ? 1 : -1;
            if (tx == body_[0].x && ty == body_[0].y) {   // wpadlo prosto w paszcze
                q.x = (int8_t)tx; q.y = (int8_t)ty;
                eat(q);
                continue;
            }
            if (cell_blocked(tx, ty, true)) continue;
            q.x = (int8_t)tx;
            q.y = (int8_t)ty;
        }
    }
}

void SnakeGame::crash()
{
    if (shield_) {
        // tarcza: waz stoi w miejscu, chwila "ducha" na wyjscie z opresji
        shield_  = false;
        ghost_t_ = 1.5f;
        flash_t_ = 0.2f;
        set_banner("TARCZA!");
        burst((float)(body_[0].x * CELL + 16), (float)(FIELD_Y + body_[0].y * CELL + 16), gfx::rgb565(80, 200, 255), 20, 180.f);
        return;
    }
    --lives_;
    flash_t_ = 0.35f;
    dying_i_ = 0;
    state_   = State::Dying;
    state_t_ = 0;
}

void SnakeGame::eat(Pickup& p)
{
    p.alive = false;
    const int cx = p.x, cy = p.y;
    const float px = (float)(cx * CELL + CELL / 2), py = (float)(FIELD_Y + cy * CELL + CELL / 2);
    burst(px, py, item_color(p.type), p.type == APPLE ? 10 : 18, 170.f);

    // combo: kolejne jablko w ciagu 3 s
    auto combo_hit = [&]() {
        combo_ = combo_t_ > 0 ? engine::imin(combo_ + 1, 5) : 1;
        combo_t_ = COMBO_TIME;
        if (combo_ >= 2) {
            char b[24];
            snprintf(b, sizeof(b), "COMBO x%d", combo_);
            set_banner(b);
        }
    };

    switch (p.type) {
    case APPLE:
        combo_hit();
        grow_ += 1;
        ++apples_;
        ++apples_total_;
        add_score(10 * combo_, cx, cy);
        spawn_item(APPLE, -1);
        if (mode_ == Mode::Adventure && !portal_open_ && apples_ >= need_) {
            open_portal();
            set_banner("PORTAL OTWARTY!");
        }
        if (mode_ == Mode::Endless && apples_total_ % ENDLESS_WORLD_APPLES == 0) {
            world_ = (apples_total_ / ENDLESS_WORLD_APPLES) % 5;
            char b[32];
            snprintf(b, sizeof(b), "SWIAT: %s", world_name(world_));
            set_banner(b);
        }
        break;
    case GOLDEN:
        combo_hit();
        grow_ += 3;
        ++apples_;
        ++apples_total_;
        add_score(50 * combo_, cx, cy);
        if (mode_ == Mode::Adventure && !portal_open_ && apples_ >= need_) {
            open_portal();
            set_banner("PORTAL OTWARTY!");
        }
        break;
    case MUSHROOM: {
        const int cut = engine::imin(3, len_ - 3);
        for (int i = 0; i < cut; ++i) {
            float x, y;
            seg_pos(len_ - 1, x, y);
            burst(x, y, gfx::rgb565(190, 110, 230), 5, 90.f);
            --len_;
        }
        grow_ = 0;
        add_score(25, cx, cy);
        set_banner("KROTSZY WAZ");
        break;
    }
    case HOURGLASS: slow_t_ = 8.f;    add_score(15, cx, cy); set_banner("SPOWOLNIENIE"); break;
    case STAR:      ghost_t_ = 6.f;   add_score(15, cx, cy); set_banner("DUCH - PRZEZ WSZYSTKO!"); break;
    case MAGNET:    magnet_t_ = 10.f; add_score(15, cx, cy); set_banner("MAGNES"); break;
    case GEM:       x2_t_ = 12.f;     add_score(20, cx, cy); set_banner("PUNKTY x2"); break;
    case ORB:       shield_ = true;   add_score(15, cx, cy); set_banner("TARCZA"); break;
    case HEART:
        if (lives_ < MAX_LIVES) ++lives_;
        add_score(30, cx, cy);
        set_banner("+1 ZYCIE");
        break;
    case BOMB:   // tylko w trybie ducha (inaczej crash() w step)
        add_score(40, cx, cy);
        burst(px, py, gfx::rgb565(255, 160, 40), 20, 220.f);
        break;
    default: break;
    }
}

// ============================================================================ autopilot (BFS)

SnakeGame::Dir SnakeGame::autopilot_dir()
{
    // BFS od glowy do najblizszego celu (portal > jablko > bonus), omija cialo, przeszkody i bomby.
    // Brak drogi: kierunek z najwieksza wolna przestrzenia (zalewanie). Deterministyczny.
    static int16_t from[ROWS * COLS];
    static uint8_t blocked[ROWS * COLS];
    static int16_t queue[ROWS * COLS];
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) blocked[y * COLS + x] = solid_[y][x] ? 1 : 0;
    for (int i = 0; i < len_ - 1; ++i) blocked[body_[i].y * COLS + body_[i].x] = 1;
    for (const Pickup& p : pickups_)
        if (p.alive && (p.type == BOMB || p.type == MUSHROOM)) blocked[p.y * COLS + p.x] = 1;

    auto neighbor = [&](int c, int d, int& out) -> bool {
        int x = c % COLS + DX[d], y = c / COLS + DY[d];
        if (x < 0 || y < 0 || x >= COLS || y >= ROWS) {
            if (!wrap_) return false;
            wrap_cell(x, y);
        }
        out = y * COLS + x;
        return !blocked[out];
    };
    auto is_target = [&](int c) -> bool {
        const int x = c % COLS, y = c / COLS;
        if (portal_open_) return x == portal_.x && y == portal_.y;
        for (const Pickup& p : pickups_)
            if (p.alive && p.x == x && p.y == y && p.type != BOMB && p.type != MUSHROOM) return true;
        return false;
    };
    auto flood = [&](int start) -> int {   // ile kratek osiagalnych od start
        static uint8_t seen[ROWS * COLS];
        memset(seen, 0, sizeof(seen));
        int head = 0, tail = 0, n = 0;
        queue[tail++] = (int16_t)start;
        seen[start] = 1;
        while (head < tail) {
            const int c = queue[head++];
            ++n;
            for (int d = 0; d < 4; ++d) {
                int o;
                if (neighbor(c, d, o) && !seen[o]) { seen[o] = 1; queue[tail++] = (int16_t)o; }
            }
        }
        return n;
    };

    const int start = body_[0].y * COLS + body_[0].x;
    for (int i = 0; i < ROWS * COLS; ++i) from[i] = -1;
    int head = 0, tail = 0, found = -1;
    queue[tail++] = (int16_t)start;
    from[start] = (int16_t)start;
    while (head < tail && found < 0) {
        const int c = queue[head++];
        for (int k = 0; k < 4; ++k) {
            const int d = (dir_ + k) & 3;   // najpierw prosto - mniej zakretow
            if (c == start && d == ((dir_ + 2) & 3)) continue;
            int o;
            if (!neighbor(c, d, o) || from[o] >= 0) continue;
            from[o] = (int16_t)c;
            if (is_target(o)) { found = o; break; }
            queue[tail++] = (int16_t)o;
        }
    }
    if (found >= 0) {
        int c = found;
        while (from[c] != start) c = from[c];
        for (int d = 0; d < 4; ++d) {
            int o;
            if (neighbor(start, d, o) && o == c) {
                // czy po tym ruchu zostanie dosc miejsca? (unika wjazdu w slepy zaulek)
                blocked[start] = 1;
                const int space = flood(o);
                blocked[start] = 0;
                if (space >= len_ || space > 40) return (Dir)d;
                break;
            }
        }
    }
    int best = -1, best_d = dir_;
    for (int k = 0; k < 4; ++k) {
        const int d = (dir_ + k) & 3;
        if (d == ((dir_ + 2) & 3)) continue;
        int o;
        if (!neighbor(start, d, o)) continue;
        blocked[start] = 1;
        const int space = flood(o);
        blocked[start] = 0;
        if (space > best) { best = space; best_d = d; }
    }
    return (Dir)best_d;
}

// ============================================================================ slad do testow

void SnakeGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "INTRO", "PLAY", "DYING", "CLEAR", "OVER", "WIN" };
    static const char DIRS[] = "RDLU";
    int items = 0;
    for (const Pickup& p : pickups_) if (p.alive) ++items;
    snprintf(buf, n, "%-5s lvl=%d len=%d head=%d,%d dir=%c apples=%d/%d score=%d lives=%d items=%d portal=%d pw=%c%c%c%c%c ap=%d",
             NAMES[(int)state_], mode_ == Mode::Adventure ? level_ + 1 : 0, len_, body_[0].x, body_[0].y, DIRS[dir_], apples_, need_,
             score_, lives_, items, portal_open_ ? 1 : 0, slow_t_ > 0 ? 'S' : '-', ghost_t_ > 0 ? 'G' : '-', magnet_t_ > 0 ? 'M' : '-',
             x2_t_ > 0 ? 'X' : '-', shield_ ? 'O' : '-', autopilot_ ? 1 : 0);
}

}  // namespace snake
