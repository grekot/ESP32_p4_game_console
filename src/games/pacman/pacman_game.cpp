// Pacman - logika: plansze, ruch po kratkach, AI duchow, tryby, punkty, autopilot.
#include "games/pacman/pacman_game.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "engine/rng.h"
#include "engine/storage.h"

namespace pacman {

namespace {

const char* TAG = "pacman";

using G = PacmanGame;

constexpr int DX[4] = { 1, 0, -1, 0 };
constexpr int DY[4] = { 0, 1, 0, -1 };
constexpr int CELL = G::CELL, COLS = G::COLS, ROWS = G::ROWS;
constexpr float HALF = (float)CELL * 0.5f;

// Predkosci na poziomie 1 [px/s] (kratka 24 px) - mnozone przez speed_mult() na wyzszych poziomach.
constexpr float PAC_SPEED     = 6.2f * CELL;
constexpr float PAC_FRIGHT    = 6.8f * CELL;    // gracz szybszy, gdy duchy uciekaja
constexpr float GHOST_SPEED   = 5.7f * CELL;
constexpr float GHOST_FRIGHT  = 3.2f * CELL;
constexpr float GHOST_TUNNEL  = 3.0f * CELL;
constexpr float GHOST_HOUSE   = 2.4f * CELL;    // wychodzenie z domu
constexpr float EYES_SPEED    = 11.f * CELL;
constexpr float CORNER        = 6.f;            // skret "przed czasem": tyle px przed/za srodkiem kratki

constexpr float READY_TIME = 2.2f, DIE_TIME = 1.7f, CLEAR_TIME = 2.6f, FRUIT_TIME = 9.5f, EAT_PAUSE = 0.55f;
constexpr int   FRUIT_PTS[G::FRUITS] = { 100, 300, 500, 700, 1000, 2000, 3000, 5000 };
constexpr float SCATTER_T[4] = { 7.f, 7.f, 5.f, 5.f };
constexpr float CHASE_T = 20.f;
constexpr float HOUSE_T[G::GHOSTS]  = { 0.f, 1.0f, 5.0f, 10.f };   // po tylu sekundach duch wychodzi z domu...
constexpr int   HOUSE_PEL[G::GHOSTS] = { 0, 0, 30, 60 };          // ...albo po tylu zjedzonych kulkach
constexpr int   CORNER_X[G::GHOSTS] = { COLS - 2, 1, COLS - 2, 1 };   // kat rozproszenia: czerwony PG, rozowy LG,
constexpr int   CORNER_Y[G::GHOSTS] = { 0, 0, ROWS - 1, ROWS - 1 };   // blekitny PD, pomaranczowy LD
constexpr int   EXTRA_LIFE = 10000;
constexpr int   MAX_LEVEL_SELECT = 8;

inline G::Dir opposite(G::Dir d) { return (G::Dir)((d + 2) & 3); }
inline bool perpendicular(G::Dir a, G::Dir b) { return ((a ^ b) & 1) != 0; }

// Odleglosc do srodka biezacej kratki wzdluz kierunku ruchu (dodatnia = srodek przed nami).
inline float to_center(float x, float y, float cx, float cy, G::Dir d)
{
    switch (d) {
    case G::RIGHT: return cx - x;
    case G::LEFT:  return x - cx;
    case G::DOWN:  return cy - y;
    default:       return y - cy;
    }
}

inline void advance(float& x, float& y, G::Dir d, float m)
{
    x += (float)DX[d] * m;
    y += (float)DY[d] * m;
}

}  // namespace

// ============================================================================ pomocnicze

void PacmanGame::cell_of(const Actor& a, int& col, int& row) const
{
    col = (int)floorf(a.x / (float)CELL);
    row = (int)floorf(a.y / (float)CELL);
}

bool PacmanGame::walkable(int col, int row, bool ghost_door) const
{
    if (row < 0 || row >= ROWS) return false;
    if (col < 0 || col >= COLS) return tunnel_row_[row];
    const uint8_t c = cell_[row][col];
    if (c == '#') return false;
    if (c == '-' || c == 'G') return ghost_door;
    return true;
}

float PacmanGame::speed_mult() const { return engine::clampf(1.f + 0.05f * (float)(level_ - 1), 1.f, 1.35f); }
float PacmanGame::fright_time() const { return fmaxf(1.5f, 6.5f - 0.6f * (float)(level_ - 1)); }

void PacmanGame::add_score(int pts)
{
    score_ += pts;
    if (!extra_life_ && score_ >= EXTRA_LIFE) {
        extra_life_ = true;
        ++lives_;
        popup(pac_.x, pac_.y - 20.f, "+1 UP", gfx::rgb565(120, 255, 140));
    }
}

void PacmanGame::popup(float x, float y, const char* text, uint16_t color)
{
    for (Popup& p : popups_) {
        if (p.alive) continue;
        p.alive = true;
        p.x = x;
        p.y = y;
        p.t = 1.1f;
        p.color = color;
        snprintf(p.text, sizeof(p.text), "%s", text);
        return;
    }
}

void PacmanGame::save_record()
{
    if (score_ <= top_.score) return;
    top_ = { 1, score_, level_ };
    engine::save_data("pacman_top", &top_, sizeof(top_));
}

// ============================================================================ start / poziom

void PacmanGame::init(gfx::Canvas&)
{
    Record saved{};
    if (engine::load_data("pacman_top", &saved, sizeof(saved)) && saved.version == 1) top_ = saved;
    else top_ = { 1, 0, 0 };
    load_assets();
    state_ = State::Title;
    state_t_ = 0;
    // plansza tytulowa: pierwszy labirynt w tle nie jest potrzebny - bake dopiero w load_level
    CONSOLE_LOGI(TAG, "Pacman gotowy, rekord %d", top_.score);
}

void PacmanGame::start_game()
{
    score_ = 0;
    lives_ = 3;
    level_ = start_level_;
    extra_life_ = false;
    fruits_taken_ = 0;
    deaths_ = 0;
    memset(fruit_hist_, 0, sizeof(fruit_hist_));
    for (Popup& p : popups_) p.alive = false;
    load_level();
}

void PacmanGame::load_level()
{
    maze_ = (level_ - 1) % MAZE_COUNT;
    world_ = (level_ - 1 + (level_ - 1) / MAZE_COUNT) % WORLDS;   // po pelnym cyklu plansz inne pary plansza-swiat
    const MazeDef& m = MAZES[maze_];
    pellets_total_ = 0;
    int gx = 0, gy = 0, gn = 0;
    for (int r = 0; r < ROWS; ++r) {
        const char* row = m.rows[r];
        tunnel_row_[r] = row[0] != '#';
        for (int c = 0; c < COLS; ++c) {
            char ch = row[c];
            switch (ch) {
            case 'P': start_x_ = c; start_y_ = r; ch = ' '; break;
            case 'F': fruit_x_ = c; fruit_y_ = r; ch = ' '; break;
            case '-': door_x_ = c; door_y_ = r; break;
            case 'G': gx += c; gy += r; ++gn; break;
            case '.': ++pellets_total_; break;
            case 'o': ++pellets_total_; break;
            default: break;
            }
            cell_[r][c] = (uint8_t)ch;
        }
    }
    if (gn) { house_x_ = gx / gn; house_y_ = gy / gn; }
    pellets_left_ = pellets_total_;
    pellets_eaten_ = 0;
    fruit_shown_[0] = fruit_shown_[1] = false;
    fruit_t_ = 0;
    level_time_ = 0;
    for (Popup& p : popups_) p.alive = false;
    reset_actors();
    bake_maze();
    state_ = State::Ready;
    state_t_ = 0;
    CONSOLE_LOGI(TAG, "poziom %d: plansza %s, swiat %d, kulek %d", level_, m.name, world_, pellets_total_);
}

void PacmanGame::reset_actors()
{
    pac_ = Actor{};
    pac_.x = (float)start_x_ * CELL + HALF;
    pac_.y = (float)start_y_ * CELL + HALF;
    pac_.dir = LEFT;
    pac_.moving = true;
    wanted_ = LEFT;
    for (int i = 0; i < GHOSTS; ++i) {
        Ghost& g = ghost_[i];
        g = Ghost{};
        g.a.x = (float)house_x_ * CELL + HALF + (float)((i == 2) ? -CELL : (i == 3) ? CELL : 0);
        g.a.y = (float)house_y_ * CELL + HALF;
        g.a.dir = (i & 1) ? RIGHT : LEFT;
        g.mode = IN_HOUSE;
    }
    // czerwony zaczyna nad drzwiami, juz w grze
    ghost_[0].a.x = (float)door_x_ * CELL + HALF;
    ghost_[0].a.y = (float)(door_y_ - 1) * CELL + HALF;
    ghost_[0].a.dir = LEFT;
    ghost_[0].mode = NORMAL;
    fright_t_ = 0;
    ghost_chain_ = 0;
    eaten_ghost_ = -1;
    pause_t_ = 0;
    phase_ = SCATTER;
    phase_idx_ = 0;
    phase_t_ = 0;
}

// ============================================================================ petla

void PacmanGame::update(float dt, const input::PadState& pad)
{
    anim_ += dt;
    for (Popup& p : popups_) {
        if (!p.alive) continue;
        p.t -= dt;
        p.y -= 18.f * dt;
        if (p.t <= 0) p.alive = false;
    }

    switch (state_) {
    case State::Title:
        update_title(pad);
        break;
    case State::Ready:
        state_t_ += dt;
        if (pad.y_pressed) autopilot_ = !autopilot_;
        if (state_t_ >= READY_TIME) {
            state_ = State::Playing;
            state_t_ = 0;
        }
        break;
    case State::Playing:
        update_playing(dt, pad);
        break;
    case State::Dying:
        state_t_ += dt;
        if (state_t_ >= DIE_TIME) {
            --lives_;
            if (lives_ < 0) {
                save_record();
                state_ = State::GameOver;
                state_t_ = 0;
            } else {
                reset_actors();
                state_ = State::Ready;
                state_t_ = 0;
            }
        }
        break;
    case State::LevelClear:
        state_t_ += dt;
        if (state_t_ >= CLEAR_TIME) {
            ++level_;
            load_level();
        }
        break;
    case State::GameOver:
        state_t_ += dt;
        if (pad.y_pressed) autopilot_ = !autopilot_;
        if ((state_t_ > 1.0f && (pad.a_pressed || pad.b_pressed)) || (autopilot_ && state_t_ > 4.f)) start_game();
        else if (state_t_ > 1.0f && pad.x_pressed) {
            state_ = State::Title;
            state_t_ = 0;
        }
        break;
    }
}

void PacmanGame::update_title(const input::PadState& pad)
{
    state_t_ += 1.f / 60.f;
    if (pad.y_pressed) autopilot_ = !autopilot_;
    if (pad.x_pressed) start_level_ = start_level_ % MAX_LEVEL_SELECT + 1;
    if (state_t_ > 0.03f && (pad.a_pressed || pad.b_pressed)) start_game();
}

void PacmanGame::update_playing(float dt, const input::PadState& pad)
{
    level_time_ += dt;

    // kierunek zyczony: przy dwoch klawiszach naraz wygrywa ten inny niz obecny ruch (skret ma pierwszenstwo)
    Dir w = NONE;
    const bool held[4] = { pad.right, pad.down, pad.left, pad.up };
    for (int d = 0; d < 4; ++d) {
        if (!held[d]) continue;
        if (w == NONE || (Dir)d != pac_.dir) w = (Dir)d;
    }
    if (w != NONE) wanted_ = w;
    if (pad.y_pressed) autopilot_ = !autopilot_;

    // chwila zatrzymania po zjedzeniu ducha
    if (pause_t_ > 0) {
        pause_t_ -= dt;
        if (pause_t_ > 0) return;
        pause_t_ = 0;
        eaten_ghost_ = -1;
    }

    // zegary: strach zatrzymuje zegar faz (jak w oryginale)
    if (fright_t_ > 0) {
        fright_t_ -= dt;
        if (fright_t_ <= 0) {
            fright_t_ = 0;
            ghost_chain_ = 0;
            for (Ghost& g : ghost_) if (g.mode == FRIGHTENED) g.mode = NORMAL;
        }
    } else {
        phase_t_ += dt;
        const bool scatter = (phase_idx_ & 1) == 0;
        const float dur = scatter ? SCATTER_T[engine::imin(phase_idx_ / 2, 3)] : (phase_idx_ >= 7 ? 1e9f : CHASE_T);
        if (phase_t_ >= dur) next_phase();
    }
    if (fruit_t_ > 0) {
        fruit_t_ -= dt;
        if (fruit_t_ < 0) fruit_t_ = 0;
    }

    move_pac(dt);
    for (int i = 0; i < GHOSTS; ++i) move_ghost(ghost_[i], dt, i);

    // zderzenia
    for (int i = 0; i < GHOSTS; ++i) {
        Ghost& g = ghost_[i];
        if (g.mode != NORMAL && g.mode != FRIGHTENED) continue;
        const float dx = g.a.x - pac_.x, dy = g.a.y - pac_.y;
        if (dx * dx + dy * dy > (0.62f * CELL) * (0.62f * CELL)) continue;
        if (g.mode == FRIGHTENED) eat_ghost(i);
        else {
            kill_pac();
            return;
        }
    }
    if (fruit_t_ > 0) {
        const float fx = (float)fruit_x_ * CELL + HALF, fy = (float)fruit_y_ * CELL + HALF;
        const float dx = fx - pac_.x, dy = fy - pac_.y;
        if (dx * dx + dy * dy < (0.6f * CELL) * (0.6f * CELL)) {
            add_score(FRUIT_PTS[fruit_kind_]);
            char txt[8];
            snprintf(txt, sizeof(txt), "%d", FRUIT_PTS[fruit_kind_]);
            popup(fx, fy - 16.f, txt, gfx::rgb565(255, 220, 120));
            fruit_hist_[fruits_taken_ % 8] = (uint8_t)fruit_kind_;
            ++fruits_taken_;
            fruit_t_ = 0;
        }
    }
}

void PacmanGame::next_phase()
{
    ++phase_idx_;
    phase_t_ = 0;
    phase_ = (phase_idx_ & 1) ? CHASE : SCATTER;
    // zmiana fazy = duchy zawracaja
    for (Ghost& g : ghost_) {
        if (g.mode != NORMAL && g.mode != FRIGHTENED) continue;
        g.a.dir = opposite(g.a.dir);
        g.a.dec_col = -99;
    }
}

// ============================================================================ ruch gracza

void PacmanGame::move_pac(float dt)
{
    const float speed = (fright_t_ > 0 ? PAC_FRIGHT : PAC_SPEED) * speed_mult();
    int col, row;
    cell_of(pac_, col, row);
    float cx = (float)col * CELL + HALF, cy = (float)row * CELL + HALF;

    // zawrocenie: w kazdej chwili, chyba ze stoimy w srodku kratki, a za nami sciana
    if (wanted_ == opposite(pac_.dir)) {
        const bool at_center = engine::absf(pac_.x - cx) < 0.5f && engine::absf(pac_.y - cy) < 0.5f;
        if (!at_center || walkable(col + DX[wanted_], row + DY[wanted_], false)) {
            pac_.dir = wanted_;
            pac_.moving = true;
            pac_.dec_col = -99;
        }
    }

    float step = speed * dt;
    for (int guard = 0; guard < 6 && step > 0.f; ++guard) {
        cell_of(pac_, col, row);
        cx = (float)col * CELL + HALF;
        cy = (float)row * CELL + HALF;
        float d = to_center(pac_.x, pac_.y, cx, cy, pac_.dir);

        // skret przed czasem: kierunek prostopadly, wolna kratka, blisko srodka -> przyciagniecie do srodka
        if (wanted_ != pac_.dir && perpendicular(wanted_, pac_.dir) && engine::absf(d) <= CORNER &&
            walkable(col + DX[wanted_], row + DY[wanted_], false)) {
            pac_.x = cx;
            pac_.y = cy;
            pac_.dir = wanted_;
            pac_.dec_col = -99;
            d = 0;
        }
        if (!pac_.moving) {
            // stoimy przy scianie: ruszamy tylko w wolnym kierunku
            if (wanted_ != pac_.dir && walkable(col + DX[wanted_], row + DY[wanted_], false)) {
                pac_.dir = wanted_;
                pac_.dec_col = -99;
                d = 0;
            } else if (!walkable(col + DX[pac_.dir], row + DY[pac_.dir], false)) {
                break;
            }
            pac_.moving = true;
        }
        if (d > 0.01f) {
            const float m = fminf(d, step);
            advance(pac_.x, pac_.y, pac_.dir, m);
            step -= m;
            continue;
        }
        // w srodku kratki (albo tuz za nim po zawinieciu tunelu): decyzja raz na kratke
        if (col != pac_.dec_col || row != pac_.dec_row) {
            pac_.dec_col = col;
            pac_.dec_row = row;
            if (col >= 0 && col < COLS) eat_at(col, row);
            if (state_ != State::Playing) return;   // koniec poziomu przez ostatnia kulke
            if (autopilot_) wanted_ = autopilot_dir(col, row);
            if (wanted_ != pac_.dir && walkable(col + DX[wanted_], row + DY[wanted_], false)) pac_.dir = wanted_;
            if (!walkable(col + DX[pac_.dir], row + DY[pac_.dir], false)) {
                pac_.x = cx;
                pac_.y = cy;
                pac_.moving = false;
                break;
            }
            d = 0;
        }
        const float m = fminf(step, (float)CELL + d);
        if (m <= 0.f) break;
        advance(pac_.x, pac_.y, pac_.dir, m);
        step -= m;
        // tunel
        if (pac_.x < -HALF) { pac_.x += (float)MAZE_W; pac_.dec_col = -99; }
        else if (pac_.x >= (float)MAZE_W + HALF) { pac_.x -= (float)MAZE_W; pac_.dec_col = -99; }
    }
}

void PacmanGame::eat_at(int col, int row)
{
    uint8_t& c = cell_[row][col];
    if (c != '.' && c != 'o') return;
    const bool power = c == 'o';
    c = ' ';
    --pellets_left_;
    ++pellets_eaten_;
    add_score(power ? 50 : 10);
    if (power) set_fright();
    if ((pellets_eaten_ == 70 && !fruit_shown_[0]) || (pellets_eaten_ == 170 && !fruit_shown_[1])) {
        fruit_shown_[pellets_eaten_ == 70 ? 0 : 1] = true;
        fruit_t_ = FRUIT_TIME;
        fruit_kind_ = engine::imin(level_ - 1, FRUITS - 1);
    }
    if (pellets_left_ <= 0) {
        save_record();
        state_ = State::LevelClear;
        state_t_ = 0;
        fruit_t_ = 0;
    }
}

void PacmanGame::set_fright()
{
    fright_t_ = fright_time();
    ghost_chain_ = 0;
    for (Ghost& g : ghost_) {
        if (g.mode == NORMAL) {
            g.mode = FRIGHTENED;
            g.a.dir = opposite(g.a.dir);
            g.a.dec_col = -99;
        }
    }
}

void PacmanGame::kill_pac()
{
    state_ = State::Dying;
    state_t_ = 0;
    ++deaths_;
    fruit_t_ = 0;
}

void PacmanGame::eat_ghost(int idx)
{
    Ghost& g = ghost_[idx];
    const int pts = 200 << engine::imin(ghost_chain_, 3);
    ++ghost_chain_;
    add_score(pts);
    char txt[8];
    snprintf(txt, sizeof(txt), "%d", pts);
    popup(g.a.x, g.a.y - 4.f, txt, gfx::rgb565(120, 230, 255));
    g.mode = EYES;
    g.a.dec_col = -99;
    pause_t_ = EAT_PAUSE;
    eaten_ghost_ = idx;
}

// ============================================================================ duchy

void PacmanGame::ghost_target(const Ghost& g, int idx, int& tx, int& ty) const
{
    if (g.mode == EYES) {
        tx = door_x_;
        ty = door_y_ - 1;
        return;
    }
    if (phase_ == SCATTER) {
        tx = CORNER_X[idx];
        ty = CORNER_Y[idx];
        return;
    }
    int pc, pr;
    cell_of(pac_, pc, pr);
    const Dir pd = pac_.dir;
    switch (idx) {
    case 0:   // czerwony: prosto na gracza
        tx = pc; ty = pr;
        break;
    case 1:   // rozowy: 4 kratki przed graczem
        tx = pc + 4 * DX[pd]; ty = pr + 4 * DY[pd];
        break;
    case 2: {  // blekitny: punkt 2 kratki przed graczem odbity wzgledem czerwonego
        int rc, rr;
        cell_of(ghost_[0].a, rc, rr);
        const int ax = pc + 2 * DX[pd], ay = pr + 2 * DY[pd];
        tx = ax + (ax - rc); ty = ay + (ay - rr);
        break;
    }
    default: {  // pomaranczowy: goni z daleka, z bliska (< 8 kratek) ucieka do swojego kata
        int gc, gr;
        cell_of(g.a, gc, gr);
        const int dx = gc - pc, dy = gr - pr;
        if (dx * dx + dy * dy > 64) { tx = pc; ty = pr; }
        else { tx = CORNER_X[idx]; ty = CORNER_Y[idx]; }
        break;
    }
    }
}

void PacmanGame::ghost_decide(Ghost& g, int idx, int col, int row)
{
    const bool door_ok = g.mode == EYES;
    const Dir opp = opposite(g.a.dir);
    static const Dir ORDER[4] = { UP, LEFT, DOWN, RIGHT };   // kolejnosc rozstrzygania remisow jak w oryginale
    Dir cand[4];
    int n = 0;
    for (Dir d : ORDER) {
        if (d == opp) continue;
        if (walkable(col + DX[d], row + DY[d], door_ok)) cand[n++] = d;
    }
    if (n == 0) {
        g.a.dir = opp;
        return;
    }
    if (g.mode == FRIGHTENED) {
        g.a.dir = cand[engine::rng().range(0, n - 1)];
        return;
    }
    int tx, ty;
    ghost_target(g, idx, tx, ty);
    g.target_x = tx;
    g.target_y = ty;
    int best = 0;
    long best_d = 0x7fffffff;
    for (int i = 0; i < n; ++i) {
        const long dx = col + DX[cand[i]] - tx, dy = row + DY[cand[i]] - ty;
        const long dd = dx * dx + dy * dy;
        if (dd < best_d) { best_d = dd; best = i; }
    }
    g.a.dir = cand[best];
}

void PacmanGame::move_ghost(Ghost& g, float dt, int idx)
{
    const float door_cx = (float)door_x_ * CELL + HALF;
    const float above_cy = (float)(door_y_ - 1) * CELL + HALF;
    const float house_cy = (float)house_y_ * CELL + HALF;

    switch (g.mode) {
    case IN_HOUSE: {
        g.house_t += dt;
        g.bob = sinf(g.house_t * 5.f) * 4.f;
        bool leaving = false;
        for (const Ghost& o : ghost_) if (o.mode == LEAVING) leaving = true;
        // wyjscie z domu po czasie albo po kulkach (blekitny i pomaranczowy), po jednym duchu naraz
        const bool by_time = g.house_t >= HOUSE_T[idx];
        const bool by_pellets = idx >= 2 && pellets_eaten_ >= HOUSE_PEL[idx];
        if (!leaving && (by_time || by_pellets)) {
            g.mode = LEAVING;
            g.bob = 0;
        }
        return;
    }
    case LEAVING: {
        const float sp = GHOST_HOUSE * dt;
        if (engine::absf(g.a.x - door_cx) > 0.5f) {
            g.a.x = engine::approach(g.a.x, door_cx, sp);
            g.a.dir = g.a.x < door_cx ? RIGHT : LEFT;
        } else if (g.a.y > above_cy + 0.5f) {
            g.a.x = door_cx;
            g.a.y = engine::approach(g.a.y, above_cy, sp);
            g.a.dir = UP;
        } else {
            g.a.x = door_cx;
            g.a.y = above_cy;
            g.mode = NORMAL;
            g.a.dir = (idx & 1) ? RIGHT : LEFT;
            g.a.dec_col = -99;
        }
        return;
    }
    case ENTERING: {
        const float sp = EYES_SPEED * dt;
        g.a.x = door_cx;
        g.a.dir = DOWN;
        g.a.y = engine::approach(g.a.y, house_cy, sp);
        if (g.a.y >= house_cy - 0.5f) {
            g.a.y = house_cy;
            g.mode = IN_HOUSE;
            g.house_t = fmaxf(HOUSE_T[idx] - 1.0f, 0.f);   // sekunda w domu i z powrotem do gry
        }
        return;
    }
    default:
        break;
    }

    // NORMAL / FRIGHTENED / EYES: ruch po kratkach
    int col, row;
    cell_of(g.a, col, row);
    float speed;
    if (g.mode == EYES) speed = EYES_SPEED;
    else if (g.mode == FRIGHTENED) speed = GHOST_FRIGHT * speed_mult();
    else {
        speed = GHOST_SPEED * speed_mult();
        if (tunnel_row_[row] && (col < 2 || col >= COLS - 2)) speed = GHOST_TUNNEL;
        else if (idx == 0 && pellets_left_ <= 20) speed *= pellets_left_ <= 10 ? 1.2f : 1.1f;   // "Cruise Elroy"
    }

    float step = speed * dt;
    for (int guard = 0; guard < 8 && step > 0.f; ++guard) {
        cell_of(g.a, col, row);
        const float cx = (float)col * CELL + HALF, cy = (float)row * CELL + HALF;
        float d = to_center(g.a.x, g.a.y, cx, cy, g.a.dir);
        if (d > 0.01f) {
            const float m = fminf(d, step);
            advance(g.a.x, g.a.y, g.a.dir, m);
            step -= m;
            continue;
        }
        if (col != g.a.dec_col || row != g.a.dec_row) {
            g.a.dec_col = col;
            g.a.dec_row = row;
            if (g.mode == EYES && col == door_x_ && row == door_y_ - 1) {
                g.a.x = cx;
                g.a.y = cy;
                g.mode = ENTERING;
                return;
            }
            ghost_decide(g, idx, col, row);
            d = 0;
        }
        const float m = fminf(step, (float)CELL + d);
        if (m <= 0.f) break;
        advance(g.a.x, g.a.y, g.a.dir, m);
        step -= m;
        if (g.a.x < -HALF) { g.a.x += (float)MAZE_W; g.a.dec_col = -99; }
        else if (g.a.x >= (float)MAZE_W + HALF) { g.a.x -= (float)MAZE_W; g.a.dec_col = -99; }
    }
}

// ============================================================================ autopilot

// BFS po kratkach od pozycji gracza z omijaniem okolic duchow; cel: przestraszony duch w zasiegu, owoc, najblizsza
// kulka. Bez celu - kierunek najdalej od najblizszego ducha. Deterministyczny (uzywany w testach skryptowych).
PacmanGame::Dir PacmanGame::autopilot_dir(int pcol, int prow)
{
    static int16_t dist[ROWS][COLS];
    static uint8_t first[ROWS][COLS];
    static uint8_t danger[ROWS][COLS];
    static int16_t qx[ROWS * COLS], qy[ROWS * COLS];

    memset(danger, 0, sizeof(danger));
    // niebezpieczne: kratki w promieniu 2 krokow od zwyklego ducha (i przestraszonego, ktoremu konczy sie czas)
    for (const Ghost& g : ghost_) {
        const bool bad = g.mode == NORMAL || (g.mode == FRIGHTENED && fright_t_ < 0.6f);
        if (!bad) continue;
        int gc, gr;
        cell_of(g.a, gc, gr);
        gc = (gc + COLS) % COLS;
        int head = 0, tail = 0;
        static int16_t gd[ROWS][COLS];
        memset(gd, -1, sizeof(gd));
        if (gr < 0 || gr >= ROWS) continue;
        gd[gr][gc] = 0;
        qx[tail] = (int16_t)gc; qy[tail] = (int16_t)gr; ++tail;
        while (head < tail) {
            const int x = qx[head], y = qy[head]; ++head;
            danger[y][x] = 1;
            if (gd[y][x] >= 2) continue;
            for (int d = 0; d < 4; ++d) {
                const int nx = (x + DX[d] + COLS) % COLS, ny = y + DY[d];
                if (!walkable(nx, ny, false) || gd[ny][nx] >= 0) continue;
                gd[ny][nx] = (int16_t)(gd[y][x] + 1);
                qx[tail] = (int16_t)nx; qy[tail] = (int16_t)ny; ++tail;
            }
        }
    }

    memset(dist, -1, sizeof(dist));
    pcol = (pcol + COLS) % COLS;
    dist[prow][pcol] = 0;
    int head = 0, tail = 0;
    qx[tail] = (int16_t)pcol; qy[tail] = (int16_t)prow; ++tail;
    while (head < tail) {
        const int x = qx[head], y = qy[head]; ++head;
        for (int d = 0; d < 4; ++d) {
            const int nx = (x + DX[d] + COLS) % COLS, ny = y + DY[d];
            if (!walkable(nx, ny, false) || dist[ny][nx] >= 0 || danger[ny][nx]) continue;
            dist[ny][nx] = (int16_t)(dist[y][x] + 1);
            first[ny][nx] = dist[y][x] == 0 ? (uint8_t)d : first[y][x];
            qx[tail] = (int16_t)nx; qy[tail] = (int16_t)ny; ++tail;
        }
    }

    // cel 1: przestraszony duch (jest jeszcze czas), w zasiegu 10 krokow
    int best_x = -1, best_y = -1, best_d = 0x7fff;
    if (fright_t_ > 1.0f) {
        for (const Ghost& g : ghost_) {
            if (g.mode != FRIGHTENED) continue;
            int gc, gr;
            cell_of(g.a, gc, gr);
            gc = (gc + COLS) % COLS;
            if (gr < 0 || gr >= ROWS || dist[gr][gc] < 0 || dist[gr][gc] > 10) continue;
            if (dist[gr][gc] < best_d) { best_d = dist[gr][gc]; best_x = gc; best_y = gr; }
        }
    }
    // cel 2: owoc w zasiegu 12
    if (best_x < 0 && fruit_t_ > 0 && dist[fruit_y_][fruit_x_] >= 0 && dist[fruit_y_][fruit_x_] <= 12) {
        best_x = fruit_x_;
        best_y = fruit_y_;
    }
    // cel 3: najblizsza kulka (duza kulka liczy sie jak blizsza, gdy duch jest w poblizu)
    if (best_x < 0) {
        bool ghost_near = false;
        for (const Ghost& g : ghost_) {
            if (g.mode != NORMAL) continue;
            const float dx = g.a.x - pac_.x, dy = g.a.y - pac_.y;
            if (dx * dx + dy * dy < (6.f * CELL) * (6.f * CELL)) ghost_near = true;
        }
        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                const uint8_t c = cell_[y][x];
                if ((c != '.' && c != 'o') || dist[y][x] < 0) continue;
                int dd = dist[y][x];
                if (c == 'o' && ghost_near) dd -= 4;
                if (dd < best_d) { best_d = dd; best_x = x; best_y = y; }
            }
        }
    }
    if (best_x >= 0 && !(best_x == pcol && best_y == prow)) return (Dir)first[best_y][best_x];

    // bez celu: kierunek najdalej od najblizszego zwyklego ducha (wolna kratka, w miare mozliwosci bezpieczna)
    Dir best_dir = pac_.dir;
    float best_score = -1e9f;
    for (int d = 0; d < 4; ++d) {
        const int nx = (pcol + DX[d] + COLS) % COLS, ny = prow + DY[d];
        if (!walkable(nx, ny, false)) continue;
        float nearest = 1e9f;
        for (const Ghost& g : ghost_) {
            if (g.mode != NORMAL) continue;
            const float dx = (g.a.x - HALF) / CELL - (float)nx, dy = (g.a.y - HALF) / CELL - (float)ny;
            nearest = fminf(nearest, dx * dx + dy * dy);
        }
        float s = nearest - (danger[ny][nx] ? 1000.f : 0.f) - ((Dir)d == opposite(pac_.dir) ? 0.5f : 0.f);
        if (s > best_score) { best_score = s; best_dir = (Dir)d; }
    }
    return best_dir;
}

// ============================================================================ debug

void PacmanGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "READY", "PLAY", "DYING", "CLEAR", "OVER" };
    static const char DIRS[] = "RDLU-";
    static const char MODES[] = "HLNFEI";   // dom, wychodzi, zwykly, przestraszony, oczy, wchodzi
    int pc, pr;
    cell_of(pac_, pc, pr);
    snprintf(buf, n, "%-5s lvl=%d maze=%d pac=%d,%d dir=%c pel=%d/%d score=%d lives=%d g=%c%c%c%c ph=%c fr=%.1f fruit=%d ap=%d",
             NAMES[(int)state_], level_, maze_, pc, pr, DIRS[pac_.dir], pellets_eaten_, pellets_total_, score_, lives_,
             MODES[ghost_[0].mode], MODES[ghost_[1].mode], MODES[ghost_[2].mode], MODES[ghost_[3].mode],
             phase_ == SCATTER ? 'S' : 'C', fright_t_, fruit_t_ > 0 ? fruit_kind_ + 1 : 0, autopilot_ ? 1 : 0);
}

}  // namespace pacman
