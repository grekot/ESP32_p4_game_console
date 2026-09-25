// Pacman - rysowanie: labirynt "wypalany" raz na poziom (tlo z Gemini + swiecace sciany rysowane w kodzie przez
// maske), kulki, postacie z PNG z alfa (assets/pacman/), panel z punktami, ekrany.
//
// Sciany: kazda kratka sciany laczy sie z sasiadem-sciana grubym odcinkiem przez srodki kratek (zaokraglone konce
// z definicji), w trzech warstwach: poswiata (szeroka, polprzezroczysta), korpus, jasna krawedz przesunieta w lewo-gore.
// Warstwa idzie do maski (maks. z pokrycia), potem jedno mieszanie z tlem - nakladajace sie odcinki nie sciemniaja
// sie podwojnie. Bloki scian 2x2 dostaja ciemna plyte miedzy srodkami (wygladaja na pelne).
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "games/pacman/pacman_game.h"
#include "gfx/png.h"
#include "gfx/text.h"
#include "platform/platform.h"

namespace pacman {

namespace {

const char* TAG = "pacman";

using G = PacmanGame;

constexpr int W = G::W, H = G::H, CELL = G::CELL, COLS = G::COLS, ROWS = G::ROWS;
constexpr int MAZE_X = G::MAZE_X, MAZE_Y = G::MAZE_Y, MAZE_W = G::MAZE_W, MAZE_H = G::MAZE_H;
constexpr int PANEL_X = G::PANEL_X, PANEL_W = G::PANEL_W, SPR = G::SPR;
constexpr float HALF = (float)CELL * 0.5f;
constexpr float DIE_TIME = 1.7f, CLEAR_TIME = 2.6f;   // jak w pacman_game.cpp

inline uint16_t pack565(int r, int g, int b) { return (uint16_t)((r << 11) | (g << 5) | b); }
inline uint16_t blend565(uint16_t dst, uint16_t src, int a)
{
    const int dr = (dst >> 11) & 31, dg = (dst >> 5) & 63, db = dst & 31;
    const int sr = (src >> 11) & 31, sg = (src >> 5) & 63, sb = src & 31;
    return pack565(dr + (((sr - dr) * a) >> 8), dg + (((sg - dg) * a) >> 8), db + (((sb - db) * a) >> 8));
}

struct Clip { int x0, y0, x1, y1; };
Clip g_clip = { 0, 0, W, H };

void blit_pic(gfx::Canvas& c, const Pic& p, int x, int y, int alpha_mul = 256)
{
    if (!p.px) return;
    const int cw = c.width();
    const int x0 = engine::imax(x, engine::imax(g_clip.x0, 0)), y0 = engine::imax(y, engine::imax(g_clip.y0, 0));
    const int x1 = engine::imin(x + p.w, engine::imin(g_clip.x1, cw)), y1 = engine::imin(y + p.h, engine::imin(g_clip.y1, c.height()));
    uint16_t* px = c.data();
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t*       drow = px + (size_t)yy * cw;
        const uint16_t* srow = p.px + (size_t)(yy - y) * p.w - x;
        const uint8_t*  arow = p.alpha + (size_t)(yy - y) * p.w - x;
        for (int xx = x0; xx < x1; ++xx) {
            int a = arow[xx];
            if (!a) continue;
            if (alpha_mul < 256) a = a * alpha_mul >> 8;
            drow[xx] = a >= 255 ? srow[xx] : blend565(drow[xx], srow[xx], a);
        }
    }
}

void fill_rect_alpha(gfx::Canvas& c, int x, int y, int w, int h, uint16_t color, int a)
{
    const int x0 = engine::imax(x, 0), y0 = engine::imax(y, 0);
    const int x1 = engine::imin(x + w, c.width()), y1 = engine::imin(y + h, c.height());
    uint16_t* px = c.data();
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t* row = px + (size_t)yy * c.width();
        for (int xx = x0; xx < x1; ++xx) row[xx] = blend565(row[xx], color, a);
    }
}

void fill_round_rect_alpha(gfx::Canvas& c, int x, int y, int w, int h, int r, uint16_t color, int a)
{
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int yy = 0; yy < h; ++yy) {
        float d = 0.f;
        if (yy < r) d = (float)(r - yy) - 0.5f;
        else if (yy >= h - r) d = (float)(yy - (h - r)) + 0.5f;
        int inset = 0;
        if (d > 0.f) inset = r - (int)floorf(sqrtf(fmaxf((float)r * r - d * d, 0.f)) + 0.5f);
        fill_rect_alpha(c, x + inset, y + yy, w - 2 * inset, 1, color, a);
    }
}

// Wypelniona elipsa z miekka krawedzia (cienie, poswiaty), przycinana do g_clip.
void fill_ellipse_alpha(gfx::Canvas& c, int cx, int cy, int rx, int ry, uint16_t color, int a)
{
    if (rx <= 0 || ry <= 0) return;
    const int y0 = engine::imax(cy - ry, engine::imax(g_clip.y0, 0)), y1 = engine::imin(cy + ry + 1, engine::imin(g_clip.y1, c.height()));
    const int x0 = engine::imax(cx - rx, engine::imax(g_clip.x0, 0)), x1 = engine::imin(cx + rx + 1, engine::imin(g_clip.x1, c.width()));
    uint16_t* px = c.data();
    const float irx = 1.f / (float)rx, iry = 1.f / (float)ry;
    for (int y = y0; y < y1; ++y) {
        const float fy = ((float)y - (float)cy) * iry;
        uint16_t* row = px + (size_t)y * c.width();
        for (int x = x0; x < x1; ++x) {
            const float fx = ((float)x - (float)cx) * irx;
            const float d = fx * fx + fy * fy;
            if (d >= 1.f) continue;
            const int aa = (int)((float)a * (d > 0.6f ? (1.f - d) / 0.4f : 1.f));
            row[x] = blend565(row[x], color, aa);
        }
    }
}

void text_shadow(gfx::Canvas& c, int x, int y, const char* s, uint16_t col, int px)
{
    gfx::draw_text_px(c, x + 2, y + 2, s, gfx::rgb565(4, 6, 12), px);
    gfx::draw_text_px(c, x, y, s, col, px);
}

void text_outline(gfx::Canvas& c, int x, int y, const char* s, uint16_t col, uint16_t outline, int px, int t)
{
    for (int dy = -t; dy <= t; dy += t)
        for (int dx = -t; dx <= t; dx += t)
            if (dx || dy) gfx::draw_text_px(c, x + dx, y + dy, s, outline, px);
    gfx::draw_text_px(c, x, y, s, col, px);
}

int center_x(const char* s, int px, int cx) { return cx - gfx::text_width_px(s, px) / 2; }

uint16_t* alloc_px(int n) { return platform::alloc_pixels((size_t)n, false); }
uint8_t*  alloc_a(int n) { return (uint8_t*)platform::alloc_pixels((size_t)(n + 1) / 2, false); }

Pic load_pic(const char* name)
{
    char path[64];
    snprintf(path, sizeof(path), "pacman/%s.png", name);
    const gfx::Image im = gfx::load_png_rgba(path);
    Pic p;
    p.w = im.w;
    p.h = im.h;
    p.px = im.px;
    p.alpha = im.alpha;
    return p;
}

// Obrot o k*90 stopni w prawo (k = 0..3), z opcjonalnym odbiciem w poziomie przed obrotem.
Pic rotate90(const Pic& s, int k, bool mirror)
{
    Pic d;
    if (!s.px) return d;
    const int n = s.w * s.h;
    uint16_t* px = alloc_px(n);
    uint8_t*  al = alloc_a(n);
    d.w = (k & 1) ? s.h : s.w;
    d.h = (k & 1) ? s.w : s.h;
    for (int y = 0; y < d.h; ++y) {
        for (int x = 0; x < d.w; ++x) {
            int sx, sy;
            switch (k & 3) {
            case 0: sx = x; sy = y; break;
            case 1: sx = y; sy = s.h - 1 - x; break;
            case 2: sx = s.w - 1 - x; sy = s.h - 1 - y; break;
            default: sx = s.w - 1 - y; sy = x; break;
            }
            if (mirror) sx = s.w - 1 - sx;
            px[y * d.w + x] = s.px[sy * s.w + sx];
            al[y * d.w + x] = s.alpha[sy * s.w + sx];
        }
    }
    d.px = px;
    d.alpha = al;
    return d;
}

struct Theme {
    const char* bg;
    const char* name;
    uint8_t body[3], glow[3], hi[3], plate[3];
    uint16_t fallback;
};
const Theme THEMES[G::WORLDS] = {
    { "bg_neon",   "Neon",     { 40, 110, 255 }, { 20, 70, 255 },  { 180, 225, 255 }, { 14, 30, 90 },  gfx::rgb565(8, 12, 40) },
    { "bg_candy",  "Cukierki", { 255, 80, 170 }, { 255, 40, 140 }, { 255, 205, 235 }, { 90, 20, 70 },  gfx::rgb565(40, 10, 40) },
    { "bg_jungle", "Dzungla",  { 50, 205, 95 },  { 20, 160, 60 },  { 190, 255, 205 }, { 14, 70, 35 },  gfx::rgb565(8, 30, 14) },
    { "bg_lava",   "Lawa",     { 255, 120, 30 }, { 255, 70, 0 },   { 255, 225, 150 }, { 95, 35, 10 },  gfx::rgb565(40, 14, 8) },
};
const char* const GHOST_FILES[G::GHOSTS] = { "red", "pink", "cyan", "orange" };
const char* const FRUIT_FILES[G::FRUITS] = { "cherry", "strawberry", "orange", "apple", "melon", "bell", "key", "star" };

constexpr uint16_t COL_BG    = gfx::rgb565(7, 8, 16);
constexpr uint16_t COL_PANEL = gfx::rgb565(18, 20, 36);
constexpr uint16_t COL_TEXT  = gfx::rgb565(235, 238, 245);
constexpr uint16_t COL_MUTED = gfx::rgb565(140, 148, 175);
constexpr uint16_t COL_YELLOW = gfx::rgb565(255, 214, 40);
constexpr uint16_t COL_PELLET = gfx::rgb565(255, 226, 172);
constexpr uint16_t COL_POWER  = gfx::rgb565(255, 244, 210);

}  // namespace

// ============================================================================ zasoby

void PacmanGame::load_assets()
{
    if (assets_loaded_) return;
    assets_loaded_ = true;
    bake_ = alloc_px(MAZE_W * MAZE_H);
    mask_ = alloc_a(MAZE_W * MAZE_H);
    title_ = alloc_px(W * H);
    const gfx::Sprite t = gfx::load_png("pacman/title.png", title_, (size_t)W * H);
    if (!t.px || t.w != W || t.h != H) {
        CONSOLE_LOGW(TAG, "brak pacman/title.png (albo zly rozmiar) - tytul bez ilustracji");
        memset(title_, 0, (size_t)W * H * 2);
    }
    for (int f = 0; f < 4; ++f) {
        char n[16];
        snprintf(n, sizeof(n), "hero%d", f);
        const Pic src = load_pic(n);
        hero_[RIGHT][f] = src;
        hero_[LEFT][f]  = rotate90(src, 0, true);
        hero_[UP][f]    = rotate90(src, 3, false);
        hero_[DOWN][f]  = rotate90(src, 1, false);
        snprintf(n, sizeof(n), "die%d", f);
        die_[f] = load_pic(n);
    }
    icon_hero_ = load_pic("icon_hero");
    for (int i = 0; i < GHOSTS; ++i) {
        for (int f = 0; f < 2; ++f) {
            char n[24];
            snprintf(n, sizeof(n), "ghost_%s%d", GHOST_FILES[i], f);
            ghost_img_[i][f][0] = load_pic(n);
            ghost_img_[i][f][1] = rotate90(ghost_img_[i][f][0], 0, true);
        }
    }
    for (int f = 0; f < 2; ++f) {
        char n[16];
        snprintf(n, sizeof(n), "scared%d", f);
        scared_[f][0] = load_pic(n);
        scared_[f][1] = rotate90(scared_[f][0], 0, true);
    }
    eyes_[0] = load_pic("eyes");
    eyes_[1] = rotate90(eyes_[0], 0, true);
    for (int i = 0; i < FRUITS; ++i) {
        char n[24];
        fruit_img_[i] = load_pic(FRUIT_FILES[i]);
        snprintf(n, sizeof(n), "icon_%s", FRUIT_FILES[i]);
        fruit_icon_[i] = load_pic(n);
    }
    CONSOLE_LOGI(TAG, "assety: bohater %s, duchy %s, owoce %s", hero_[0][0].px ? "tak" : "nie",
                 ghost_img_[0][0][0].px ? "tak" : "nie", fruit_img_[0].px ? "tak" : "nie");
}

void PacmanGame::bake_maze()
{
    if (!bake_) return;
    const Theme& th = THEMES[world_];
    char path[48];
    snprintf(path, sizeof(path), "pacman/%s.png", th.bg);
    const gfx::Sprite bg = gfx::load_png(path, bake_, (size_t)MAZE_W * MAZE_H);
    if (!bg.px || bg.w != MAZE_W || bg.h != MAZE_H) {
        for (int i = 0; i < MAZE_W * MAZE_H; ++i) bake_[i] = th.fallback;
    }
    // Sciany z pola odleglosci: dla piksela w kratce sciany liczymy odleglosc d do najblizszego korytarza (sasiedzi
    // 4-kierunkowi = odleglosc do krawedzi kratki, sasiedzi po skosie = odleglosc do naroznika). Rurka neonowa
    // swieci przy d = INSET (stala odleglosc od korytarza, wiec naroznik wypukly jest zaokraglony automatycznie),
    // dalej w glab bloku ciemna plyta. Poza labiryntem jest "korytarz" - ramka zewnetrzna ma rurke z obu stron
    // (podwojna linia jak w klasyku). Maska rurki zostaje w mask_ do migania na koniec poziomu.
    auto open_at = [&](int c, int r) -> bool {
        if (r < 0 || r >= ROWS) return true;
        if (c < 0 || c >= COLS) return true;
        return cell_[r][c] != '#';
    };
    constexpr float INSET = 5.0f, TUBE = 2.3f, GLOW = 7.5f, HI_OFF = 1.1f;
    const uint16_t plate = gfx::rgb565(th.plate[0], th.plate[1], th.plate[2]);
    const uint16_t body  = gfx::rgb565(th.body[0], th.body[1], th.body[2]);
    const uint16_t glow  = gfx::rgb565(th.glow[0], th.glow[1], th.glow[2]);
    const uint16_t hi    = gfx::rgb565(th.hi[0], th.hi[1], th.hi[2]);
    memset(mask_, 0, (size_t)MAZE_W * MAZE_H);
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (cell_[r][c] != '#') continue;
            const bool oL = open_at(c - 1, r), oR = open_at(c + 1, r), oT = open_at(c, r - 1), oB = open_at(c, r + 1);
            const bool oTL = open_at(c - 1, r - 1), oTR = open_at(c + 1, r - 1), oBL = open_at(c - 1, r + 1), oBR = open_at(c + 1, r + 1);
            for (int v = 0; v < CELL; ++v) {
                uint16_t* row = bake_ + (size_t)(r * CELL + v) * MAZE_W + c * CELL;
                uint8_t*  mrow = mask_ + (size_t)(r * CELL + v) * MAZE_W + c * CELL;
                const float fy = (float)v + 0.5f, fyb = (float)CELL - fy;
                for (int u = 0; u < CELL; ++u) {
                    const float fx = (float)u + 0.5f, fxr = (float)CELL - fx;
                    float d = 1e9f;
                    if (oL) d = fminf(d, fx);
                    if (oR) d = fminf(d, fxr);
                    if (oT) d = fminf(d, fy);
                    if (oB) d = fminf(d, fyb);
                    if (oTL) d = fminf(d, sqrtf(fx * fx + fy * fy));
                    if (oTR) d = fminf(d, sqrtf(fxr * fxr + fy * fy));
                    if (oBL) d = fminf(d, sqrtf(fx * fx + fyb * fyb));
                    if (oBR) d = fminf(d, sqrtf(fxr * fxr + fyb * fyb));
                    uint16_t px = row[u];
                    if (d > INSET + TUBE) px = blend565(px, plate, 175);
                    const float e = engine::absf(d - INSET);
                    if (e < GLOW) {
                        const float g = 1.f - e / GLOW;
                        px = blend565(px, glow, (int)(g * g * 150.f));
                    }
                    const float t = engine::clampf(TUBE + 0.5f - e, 0.f, 1.f);
                    if (t > 0.f) {
                        const float h = engine::clampf(1.f - engine::absf(d - (INSET - HI_OFF)) / 1.3f, 0.f, 1.f);
                        const uint16_t col = h > 0.f ? blend565(body, hi, (int)(h * 235.f)) : body;
                        px = blend565(px, col, (int)(t * 255.f));
                        mrow[u] = (uint8_t)(t * 255.f);
                    }
                    row[u] = px;
                }
            }
        }
    }
    // drzwi domu duchow
    gfx::Canvas bc(bake_, MAZE_W, MAZE_H);
    fill_rect_alpha(bc, door_x_ * CELL + 2, door_y_ * CELL + CELL / 2 - 3, CELL - 4, 6, gfx::rgb565(255, 170, 205), 255);
    baked_level_ = level_;
}

// ============================================================================ rysowanie

void PacmanGame::render(gfx::Canvas& c)
{
    if (state_ == State::Title) {
        draw_title(c);
        return;
    }
    c.clear(COL_BG);
    draw_maze(c);
    g_clip = { MAZE_X, MAZE_Y, MAZE_X + MAZE_W, MAZE_Y + MAZE_H };
    draw_pellets(c);
    draw_fruit(c);
    draw_ghosts(c);
    draw_pac(c);
    draw_popups(c);
    g_clip = { 0, 0, W, H };
    draw_panel(c);
    draw_overlay(c);
}

void PacmanGame::draw_maze(gfx::Canvas& c)
{
    if (!bake_) return;
    uint16_t* px = c.data();
    for (int y = 0; y < MAZE_H; ++y)
        memcpy(px + (size_t)(MAZE_Y + y) * W + MAZE_X, bake_ + (size_t)y * MAZE_W, (size_t)MAZE_W * 2);
    // koniec poziomu: sciany migaja na bialo (maska korpusu z bake_maze)
    if (state_ == State::LevelClear && mask_ && ((int)(state_t_ * 5.f) & 1)) {
        for (int y = 0; y < MAZE_H; ++y) {
            uint16_t* row = px + (size_t)(MAZE_Y + y) * W + MAZE_X;
            const uint8_t* mrow = mask_ + (size_t)y * MAZE_W;
            for (int x = 0; x < MAZE_W; ++x) {
                if (!mrow[x]) continue;
                row[x] = blend565(row[x], gfx::WHITE, mrow[x] * 230 >> 8);
            }
        }
    }
}

void PacmanGame::draw_pellets(gfx::Canvas& c)
{
    const int pulse = (int)(sinf(anim_ * 6.f) * 1.5f + 1.5f);
    for (int r = 0; r < ROWS; ++r) {
        for (int col = 0; col < COLS; ++col) {
            const uint8_t ch = cell_[r][col];
            if (ch != '.' && ch != 'o') continue;
            const int cx = MAZE_X + col * CELL + CELL / 2, cy = MAZE_Y + r * CELL + CELL / 2;
            if (ch == '.') {
                c.fill_circle(cx, cy, 3, COL_PELLET);
                c.put(cx - 1, cy - 1, gfx::WHITE);
            } else {
                fill_ellipse_alpha(c, cx, cy, 12 + pulse, 12 + pulse, gfx::rgb565(255, 200, 120), 90);
                c.fill_circle(cx, cy, 6 + pulse, COL_POWER);
                c.fill_circle(cx - 2, cy - 2, 2, gfx::WHITE);
            }
        }
    }
}

void PacmanGame::draw_fruit(gfx::Canvas& c)
{
    if (fruit_t_ <= 0) return;
    if (fruit_t_ < 2.f && ((int)(anim_ * 8.f) & 1)) return;   // miga przed zniknieciem
    const Pic& p = fruit_img_[fruit_kind_];
    const int cx = MAZE_X + fruit_x_ * CELL + CELL / 2, cy = MAZE_Y + fruit_y_ * CELL + CELL / 2;
    const int bob = (int)(sinf(anim_ * 4.f) * 2.f);
    fill_ellipse_alpha(c, cx, cy + 14, 12, 4, gfx::BLACK, 90);
    blit_pic(c, p, cx - p.w / 2, cy - p.h / 2 - 2 + bob);
}

void PacmanGame::draw_ghosts(gfx::Canvas& c)
{
    const int frame = ((int)(anim_ * 8.f)) & 1;
    const bool flash = fright_t_ > 0 && fright_t_ < 1.6f && ((int)(anim_ * 8.f) & 1);
    for (int i = 0; i < GHOSTS; ++i) {
        const Ghost& g = ghost_[i];
        if (pause_t_ > 0 && i == eaten_ghost_) continue;
        const int side = g.a.dir == LEFT ? 1 : 0;
        const Pic* p;
        if (g.mode == EYES || g.mode == ENTERING) p = &eyes_[side];
        else if (g.mode == FRIGHTENED) p = &scared_[flash ? 1 : 0][side];
        else p = &ghost_img_[i][frame][side];
        const int bob = (g.mode == IN_HOUSE) ? (int)g.bob : 0;
        for (int k = -1; k <= 1; ++k) {   // kopie za krawedzia tunelu
            const float x = g.a.x + (float)k * MAZE_W;
            if (x < -SPR || x > MAZE_W + SPR) continue;
            const int sx = MAZE_X + (int)(x + 0.5f), sy = MAZE_Y + (int)(g.a.y + 0.5f) + bob;
            if (g.mode != EYES && g.mode != ENTERING) fill_ellipse_alpha(c, sx, sy + 18, 14, 5, gfx::BLACK, 80);
            blit_pic(c, *p, sx - p->w / 2, sy - p->h / 2);
        }
    }
}

void PacmanGame::draw_pac(gfx::Canvas& c)
{
    if (pause_t_ > 0) return;
    if (state_ == State::Dying) {
        const float t = state_t_ / DIE_TIME;
        const int f = engine::iclamp((int)(t * 4.2f), 0, 3);
        const int alpha = t > 0.85f ? (int)(256.f * (1.f - t) / 0.15f) : 256;
        const Pic& p = die_[f];
        const int sx = MAZE_X + (int)(pac_.x + 0.5f), sy = MAZE_Y + (int)(pac_.y + 0.5f);
        blit_pic(c, p, sx - p.w / 2, sy - p.h / 2, engine::iclamp(alpha, 0, 256));
        return;
    }
    static const int SEQ[6] = { 0, 1, 2, 3, 2, 1 };
    const bool anim = pac_.moving && state_ == State::Playing;
    const int f = anim ? SEQ[(int)(anim_ * 22.f) % 6] : 1;
    const Pic& p = hero_[pac_.dir][f];
    for (int k = -1; k <= 1; ++k) {
        const float x = pac_.x + (float)k * MAZE_W;
        if (x < -SPR || x > MAZE_W + SPR) continue;
        const int sx = MAZE_X + (int)(x + 0.5f), sy = MAZE_Y + (int)(pac_.y + 0.5f);
        fill_ellipse_alpha(c, sx, sy + 18, 14, 5, gfx::BLACK, 90);
        blit_pic(c, p, sx - p.w / 2, sy - p.h / 2);
    }
}

void PacmanGame::draw_popups(gfx::Canvas& c)
{
    for (const Popup& p : popups_) {
        if (!p.alive) continue;
        const int x = MAZE_X + (int)p.x, y = MAZE_Y + (int)p.y;
        text_outline(c, center_x(p.text, 16, x), y - 8, p.text, p.color, gfx::rgb565(4, 6, 12), 16, 1);
    }
}

void PacmanGame::draw_panel(gfx::Canvas& c)
{
    const int x = PANEL_X, y = MAZE_Y;
    fill_round_rect_alpha(c, x, y, PANEL_W, MAZE_H, 14, COL_PANEL, 235);
    const Theme& th = THEMES[world_];
    fill_rect_alpha(c, x + 14, y + 1, PANEL_W - 28, 1, gfx::rgb565(th.hi[0], th.hi[1], th.hi[2]), 120);
    char buf[32];
    int ty = y + 16;
    gfx::draw_text_px(c, x + 12, ty, "PUNKTY", COL_MUTED, 14);
    snprintf(buf, sizeof(buf), "%d", score_);
    text_shadow(c, x + 12, ty + 16, buf, COL_TEXT, 24);
    ty += 60;
    gfx::draw_text_px(c, x + 12, ty, "REKORD", COL_MUTED, 14);
    snprintf(buf, sizeof(buf), "%d", engine::imax(top_.score, score_));
    text_shadow(c, x + 12, ty + 16, buf, COL_YELLOW, 20);
    ty += 56;
    gfx::draw_text_px(c, x + 12, ty, "POZIOM", COL_MUTED, 14);
    snprintf(buf, sizeof(buf), "%d", level_);
    text_shadow(c, x + 12, ty + 16, buf, COL_TEXT, 24);
    gfx::draw_text_px(c, x + 12, ty + 46, th.name, COL_MUTED, 14);
    gfx::draw_text_px(c, x + 12, ty + 62, MAZES[maze_].name, COL_MUTED, 14);
    ty += 100;
    gfx::draw_text_px(c, x + 12, ty, "ZYCIA", COL_MUTED, 14);
    // ikony 24 px co 27 px, 4 w rzedzie (panel ma 128 px), piate i kolejne zycia w drugim rzedzie
    for (int i = 0; i < engine::imin(lives_, 8); ++i) blit_pic(c, icon_hero_, x + 10 + (i % 4) * 27, ty + 18 + (i / 4) * 27);
    ty += 78;
    gfx::draw_text_px(c, x + 12, ty, "OWOCE", COL_MUTED, 14);
    const int shown = engine::imin(fruits_taken_, 8);
    for (int i = 0; i < shown; ++i) blit_pic(c, fruit_icon_[fruit_hist_[i]], x + 10 + (i % 4) * 27, ty + 18 + (i / 4) * 27);
    ty += 80;
    snprintf(buf, sizeof(buf), "KULKI %d", pellets_left_);
    gfx::draw_text_px(c, x + 12, ty, buf, COL_MUTED, 14);
    if (autopilot_) {
        fill_round_rect_alpha(c, x + 8, y + MAZE_H - 40, PANEL_W - 16, 26, 8, gfx::rgb565(60, 120, 60), 220);
        gfx::draw_text_px(c, center_x("AUTOPILOT", 14, x + PANEL_W / 2), y + MAZE_H - 34, "AUTOPILOT", COL_TEXT, 14);
    }
}

void PacmanGame::draw_title(gfx::Canvas& c)
{
    if (title_) memcpy(c.data(), title_, (size_t)W * H * 2);
    else c.clear(COL_BG);
    // logo w ciemniejszej gornej czesci ilustracji
    fill_round_rect_alpha(c, 220, 22, 360, 78, 20, gfx::rgb565(4, 6, 16), 120);
    text_outline(c, center_x("PACMAN", 48, W / 2), 30, "PACMAN", COL_YELLOW, gfx::rgb565(40, 20, 0), 48, 3);
    char buf[48];
    snprintf(buf, sizeof(buf), "REKORD %d", top_.score);
    text_outline(c, center_x(buf, 20, W / 2), 96, buf, COL_TEXT, gfx::rgb565(4, 6, 16), 20, 2);
    // panel dolny
    fill_round_rect_alpha(c, 150, H - 96, W - 300, 78, 16, gfx::rgb565(6, 8, 20), 190);
    const bool blink = ((int)(anim_ * 2.f) & 1) == 0;
    if (blink) text_outline(c, center_x("NACISNIJ A", 24, W / 2), H - 88, "NACISNIJ A", COL_YELLOW, gfx::rgb565(4, 6, 16), 24, 2);
    snprintf(buf, sizeof(buf), "X: poziom startowy %d      Y: autopilot %s", start_level_, autopilot_ ? "wl." : "wyl.");
    gfx::draw_text_px(c, center_x(buf, 16, W / 2), H - 50, buf, COL_MUTED, 16);
    gfx::draw_text_px(c, center_x("Krzyzak: kierunek (mozna wcisnac przed skrzyzowaniem)", 14, W / 2), H - 30,
                      "Krzyzak: kierunek (mozna wcisnac przed skrzyzowaniem)", COL_MUTED, 14);
}

void PacmanGame::draw_overlay(gfx::Canvas& c)
{
    const int cx = MAZE_X + MAZE_W / 2;
    const int cy = MAZE_Y + fruit_y_ * CELL + CELL / 2;   // klasycznie: napis pod domem duchow
    switch (state_) {
    case State::Ready:
        text_outline(c, center_x("GOTOWY!", 28, cx), cy - 14, "GOTOWY!", COL_YELLOW, gfx::rgb565(20, 10, 0), 28, 2);
        break;
    case State::LevelClear: {
        char buf[40];
        snprintf(buf, sizeof(buf), "PLANSZA %d UKONCZONA", level_);
        fill_round_rect_alpha(c, cx - 190, cy - 26, 380, 52, 14, gfx::rgb565(4, 6, 16), 170);
        text_outline(c, center_x(buf, 24, cx), cy - 12, buf, COL_YELLOW, gfx::rgb565(20, 10, 0), 24, 2);
        break;
    }
    case State::GameOver:
        fill_round_rect_alpha(c, cx - 190, cy - 34, 380, 84, 14, gfx::rgb565(4, 6, 16), 190);
        text_outline(c, center_x("KONIEC GRY", 32, cx), cy - 30, "KONIEC GRY", gfx::rgb565(255, 70, 60), gfx::rgb565(20, 0, 0), 32, 2);
        gfx::draw_text_px(c, center_x("A: jeszcze raz    X: tytul", 16, cx), cy + 14, "A: jeszcze raz    X: tytul", COL_TEXT, 16);
        break;
    default:
        break;
    }
    (void)CLEAR_TIME;
}

}  // namespace pacman
