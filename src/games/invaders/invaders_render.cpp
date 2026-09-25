// Space Invaders - rysowanie: tlo swiata z Gemini + gwiazdy paralaksy, krysztalowe bunkry z maska kruszenia, obcy, UFO,
// boss, statek z plomieniem i oslona, pociski z poswiata (mieszanie addytywne), wybuchy z klatek PNG + iskry,
// HUD, ekran tytulowy i napisy.
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "games/invaders/invaders_game.h"
#include "gfx/png.h"
#include "gfx/text.h"
#include "platform/platform.h"

namespace invaders {

namespace {

const char* TAG = "invaders";

using G = InvadersGame;

constexpr int W = G::W, H = G::H, FCOLS = G::FCOLS, CELL_H = G::CELL_H;
constexpr int BUNKERS = G::BUNKERS, BUNKER_W = G::BUNKER_W, BUNKER_H = G::BUNKER_H, BUNKER_Y = G::BUNKER_Y;
constexpr int SHIP_Y = G::SHIP_Y, HUD_H = G::HUD_H;
constexpr float READY_TIME = 2.0f, INVULN_TIME = 2.2f;   // jak w invaders_game.cpp
constexpr float UFO_Y = 62.f;

inline uint16_t pack565(int r, int g, int b) { return (uint16_t)((r << 11) | (g << 5) | b); }
inline uint16_t blend565(uint16_t dst, uint16_t src, int a)
{
    const int dr = (dst >> 11) & 31, dg = (dst >> 5) & 63, db = dst & 31;
    const int sr = (src >> 11) & 31, sg = (src >> 5) & 63, sb = src & 31;
    return pack565(dr + (((sr - dr) * a) >> 8), dg + (((sg - dg) * a) >> 8), db + (((sb - db) * a) >> 8));
}
// dodawanie z nasyceniem: dst + src * a/256 (poswiaty, iskry)
inline uint16_t add565(uint16_t dst, uint16_t src, int a)
{
    int r = ((dst >> 11) & 31) + ((((src >> 11) & 31) * a) >> 8);
    int g = ((dst >> 5) & 63) + ((((src >> 5) & 63) * a) >> 8);
    int b = (dst & 31) + (((src & 31) * a) >> 8);
    return pack565(r > 31 ? 31 : r, g > 63 ? 63 : g, b > 31 ? 31 : b);
}

void blit_pic(gfx::Canvas& c, const Pic& p, int x, int y, int alpha_mul = 256)
{
    if (!p.px) return;
    const int cw = c.width(), ch = c.height();
    const int x0 = engine::imax(x, 0), y0 = engine::imax(y, 0);
    const int x1 = engine::imin(x + p.w, cw), y1 = engine::imin(y + p.h, ch);
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

// Obraz z przyciemnionym/rozjasnionym kolorem: kolor pikseli mieszany z `tint` w stopniu k/256 (blysk trafienia).
void blit_pic_tint(gfx::Canvas& c, const Pic& p, int x, int y, uint16_t tint, int k)
{
    if (!p.px) return;
    const int cw = c.width(), ch = c.height();
    const int x0 = engine::imax(x, 0), y0 = engine::imax(y, 0);
    const int x1 = engine::imin(x + p.w, cw), y1 = engine::imin(y + p.h, ch);
    uint16_t* px = c.data();
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t*       drow = px + (size_t)yy * cw;
        const uint16_t* srow = p.px + (size_t)(yy - y) * p.w - x;
        const uint8_t*  arow = p.alpha + (size_t)(yy - y) * p.w - x;
        for (int xx = x0; xx < x1; ++xx) {
            const int a = arow[xx];
            if (!a) continue;
            drow[xx] = blend565(drow[xx], blend565(srow[xx], tint, k), a);
        }
    }
}

// Skalowanie najblizszym sasiadem (wybuchy o roznej wielkosci), srodek w (cx, cy).
void blit_pic_scaled(gfx::Canvas& c, const Pic& p, int cx, int cy, float scale, int alpha_mul = 256)
{
    if (!p.px || scale <= 0.f) return;
    const int dw = (int)((float)p.w * scale), dh = (int)((float)p.h * scale);
    if (dw < 1 || dh < 1) return;
    const int x = cx - dw / 2, y = cy - dh / 2;
    const int x0 = engine::imax(x, 0), y0 = engine::imax(y, 0);
    const int x1 = engine::imin(x + dw, c.width()), y1 = engine::imin(y + dh, c.height());
    const int step_x = (p.w << 16) / dw, step_y = (p.h << 16) / dh;
    uint16_t* px = c.data();
    for (int yy = y0; yy < y1; ++yy) {
        const int sy = ((yy - y) * step_y) >> 16;
        uint16_t* drow = px + (size_t)yy * c.width();
        const uint16_t* srow = p.px + (size_t)sy * p.w;
        const uint8_t*  arow = p.alpha + (size_t)sy * p.w;
        for (int xx = x0; xx < x1; ++xx) {
            const int sx = ((xx - x) * step_x) >> 16;
            int a = arow[sx];
            if (!a) continue;
            if (alpha_mul < 256) a = a * alpha_mul >> 8;
            drow[xx] = a >= 255 ? srow[sx] : blend565(drow[xx], srow[sx], a);
        }
    }
}

// Poswiata: elipsa dodawana do tla z zanikiem (1-r^2)^2 od srodka (bez pierwiastka - tanio na P4).
void glow(gfx::Canvas& c, int cx, int cy, int rx, int ry, uint16_t color, int a)
{
    if (rx <= 0 || ry <= 0 || a <= 0) return;
    const int y0 = engine::imax(cy - ry, 0), y1 = engine::imin(cy + ry + 1, c.height());
    const int x0 = engine::imax(cx - rx, 0), x1 = engine::imin(cx + rx + 1, c.width());
    uint16_t* px = c.data();
    const float irx = 1.f / (float)rx, iry = 1.f / (float)ry;
    for (int y = y0; y < y1; ++y) {
        const float fy = ((float)y - (float)cy) * iry;
        uint16_t* row = px + (size_t)y * c.width();
        for (int x = x0; x < x1; ++x) {
            const float fx = ((float)x - (float)cx) * irx;
            const float d = fx * fx + fy * fy;
            if (d >= 1.f) continue;
            const float k = 1.f - d;
            row[x] = add565(row[x], color, (int)((float)a * k * k));
        }
    }
}

// Pierscien (oslona): krycie najwieksze na promieniu, miekko do srodka i na zewnatrz.
void ring(gfx::Canvas& c, int cx, int cy, int rx, int ry, uint16_t color, int a)
{
    const int y0 = engine::imax(cy - ry - 3, 0), y1 = engine::imin(cy + ry + 4, c.height());
    const int x0 = engine::imax(cx - rx - 3, 0), x1 = engine::imin(cx + rx + 4, c.width());
    uint16_t* px = c.data();
    const float irx = 1.f / (float)rx, iry = 1.f / (float)ry;
    for (int y = y0; y < y1; ++y) {
        const float fy = ((float)y - (float)cy) * iry;
        uint16_t* row = px + (size_t)y * c.width();
        for (int x = x0; x < x1; ++x) {
            const float fx = ((float)x - (float)cx) * irx;
            const float d2 = fx * fx + fy * fy;   // r^2; przy krawedzi r - 1 ~ (r^2 - 1) / 2
            float k;
            if (d2 > 1.17f) continue;
            if (d2 > 1.f) k = 1.f - (d2 - 1.f) / 0.17f;
            else k = 0.18f + 0.82f * d2 * d2 * d2;   // lekko wypelnione wnetrze, jasna krawedz (r^6)
            row[x] = add565(row[x], color, (int)((float)a * k));
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

void text_outline(gfx::Canvas& c, int x, int y, const char* s, uint16_t col, uint16_t outline, int px, int t)
{
    for (int dy = -t; dy <= t; dy += t)
        for (int dx = -t; dx <= t; dx += t)
            if (dx || dy) gfx::draw_text_px(c, x + dx, y + dy, s, outline, px);
    gfx::draw_text_px(c, x, y, s, col, px);
}

int center_x(const char* s, int px, int cx) { return cx - gfx::text_width_px(s, px) / 2; }

Pic load_pic(const char* name)
{
    char path[64];
    snprintf(path, sizeof(path), "invaders/%s.png", name);
    const gfx::Image im = gfx::load_png_rgba(path);
    Pic p;
    p.w = im.w;
    p.h = im.h;
    p.px = im.px;
    p.alpha = im.alpha;
    return p;
}

const char* const ALIEN_FILES[G::TYPES] = { "squid", "crab", "octo", "jelly" };
const char* const POWER_FILES[G::POWERS] = { "pw_triple", "pw_laser", "pw_shield", "pw_slow", "life" };
struct World { const char* bg; const char* name; uint16_t fallback; };
const World WORLDS_DEF[G::WORLDS] = {
    { "bg_planet", "Orbita", gfx::rgb565(8, 8, 28) },
    { "bg_rings", "Pierscienie", gfx::rgb565(24, 8, 8) },
    { "bg_asteroids", "Pas asteroid", gfx::rgb565(4, 20, 18) },
    { "bg_blackhole", "Czarna dziura", gfx::rgb565(6, 8, 20) },
};
constexpr uint16_t TYPE_GLOW[G::TYPES] = { gfx::rgb565(150, 60, 255), gfx::rgb565(60, 220, 80),
                                           gfx::rgb565(255, 120, 30), gfx::rgb565(40, 200, 255) };
constexpr uint16_t POWER_GLOW[G::POWERS] = { gfx::rgb565(255, 60, 50), gfx::rgb565(255, 200, 40), gfx::rgb565(60, 140, 255),
                                             gfx::rgb565(170, 80, 255), gfx::rgb565(60, 255, 100) };

constexpr uint16_t COL_TEXT  = gfx::rgb565(235, 240, 250);
constexpr uint16_t COL_MUTED = gfx::rgb565(150, 160, 190);
constexpr uint16_t COL_GOLD  = gfx::rgb565(255, 214, 60);
constexpr uint16_t COL_CYAN  = gfx::rgb565(90, 230, 255);
constexpr uint16_t COL_DARK  = gfx::rgb565(4, 6, 16);

}  // namespace

// ============================================================================ zasoby

void InvadersGame::load_assets()
{
    if (assets_loaded_) return;
    assets_loaded_ = true;
    bg_ = platform::alloc_pixels((size_t)W * H, false);
    title_ = platform::alloc_pixels((size_t)W * H, false);
    const gfx::Sprite t = gfx::load_png("invaders/title.png", title_, (size_t)W * H);
    if (!t.px || t.w != W || t.h != H) {
        CONSOLE_LOGW(TAG, "brak invaders/title.png (albo zly rozmiar) - tytul bez ilustracji");
        for (int i = 0; i < W * H; ++i) title_[i] = gfx::rgb565(8, 8, 28);
    }
    for (int k = 0; k < TYPES; ++k)
        for (int f = 0; f < 2; ++f) {
            char n[16];
            snprintf(n, sizeof(n), "%s%d", ALIEN_FILES[k], f);
            alien_img_[k][f] = load_pic(n);
        }
    ufo_img_ = load_pic("ufo");
    boss_img_[0] = load_pic("boss0");
    boss_img_[1] = load_pic("boss1");
    life_img_ = load_pic("life");
    ship_img_ = load_pic("ship");
    icon_ship_ = load_pic("icon_ship");
    bunker_img_ = load_pic("bunker");
    bomb_img_ = load_pic("bomb");
    bolt_img_ = load_pic("bolt");
    for (int i = 0; i < POWERS; ++i) power_img_[i] = load_pic(POWER_FILES[i]);
    for (int f = 0; f < BOOM_FRAMES; ++f) {
        char n[16];
        snprintf(n, sizeof(n), "boom%d", f);
        boom_img_[f] = load_pic(n);
    }
    for (int b = 0; b < BUNKERS; ++b)
        bunker_mask_[b] = (uint8_t*)platform::alloc_pixels((size_t)(BUNKER_W * BUNKER_H + 1) / 2, false);
    CONSOLE_LOGI(TAG, "assety: obcy %s, statek %s, bunkier %s, wybuchy %s", alien_img_[0][0].px ? "tak" : "nie",
                 ship_img_.px ? "tak" : "nie", bunker_img_.px ? "tak" : "nie", boom_img_[0].px ? "tak" : "nie");
}

void InvadersGame::load_background()
{
    if (!bg_ || bg_world_ == world_) return;
    char path[48];
    snprintf(path, sizeof(path), "invaders/%s.png", WORLDS_DEF[world_].bg);
    const gfx::Sprite bg = gfx::load_png(path, bg_, (size_t)W * H);
    if (!bg.px || bg.w != W || bg.h != H)
        for (int i = 0; i < W * H; ++i) bg_[i] = WORLDS_DEF[world_].fallback;
    bg_world_ = world_;
}

// ============================================================================ rysowanie

void InvadersGame::render(gfx::Canvas& c)
{
    if (state_ == State::Title) {
        draw_title(c);
        return;
    }
    int ox = 0, oy = 0;
    if (shake_ > 0) {
        const float a = fminf(shake_, 0.6f) * 12.f;
        ox = (int)(sinf(anim_ * 91.f) * a);
        oy = (int)(cosf(anim_ * 73.f) * a * 0.7f);
    }
    draw_background(c);
    draw_bunkers(c);
    draw_boss(c, ox, oy);
    draw_aliens(c, ox, oy);
    draw_ufo(c, ox, oy);
    draw_shots(c, ox, oy);
    draw_ship(c, ox, oy);
    draw_effects(c, ox, oy);
    draw_hud(c);
    draw_overlay(c);
}

void InvadersGame::draw_background(gfx::Canvas& c)
{
    uint16_t* px = c.data();
    if (bg_) memcpy(px, bg_, (size_t)W * H * 2);
    else c.clear(COL_DARK);
    // trzy warstwy gwiazd przesuwane w dol (paralaksa), migotanie z fazy gwiazdy
    static const float SPEED[3] = { 6.f, 15.f, 32.f };
    for (const Star& s : star_) {
        const int y = (int)((float)s.y + anim_ * SPEED[s.layer]) % H;
        const float tw = 0.55f + 0.45f * sinf(anim_ * (1.5f + (float)(s.phase & 7) * 0.4f) + (float)s.phase);
        const int a = (int)((80.f + 70.f * (float)s.layer) * tw);
        uint16_t* row = px + (size_t)y * W;
        const uint16_t col = s.layer == 2 ? gfx::rgb565(200, 230, 255) : gfx::rgb565(160, 170, 220);
        row[s.x] = add565(row[s.x], col, a);
        if (s.layer == 2) {
            if (s.x + 1 < W) row[s.x + 1] = add565(row[s.x + 1], col, a / 2);
            if (y + 1 < H) row[W + s.x] = add565(row[W + s.x], col, a / 2);
        }
    }
}

void InvadersGame::draw_bunkers(gfx::Canvas& c)
{
    if (!bunker_img_.px) {
        // bez PNG: jednolity turkus z maski
        for (int b = 0; b < BUNKERS; ++b) {
            const uint8_t* m = bunker_mask_[b];
            if (!m) continue;
            const int bx0 = W * (b + 1) / (BUNKERS + 1) - BUNKER_W / 2;
            for (int y = 0; y < BUNKER_H; ++y) {
                uint16_t* row = c.data() + (size_t)(BUNKER_Y + y) * W + bx0;
                for (int x = 0; x < BUNKER_W; ++x)
                    if (m[y * BUNKER_W + x]) row[x] = blend565(row[x], gfx::rgb565(60, 220, 210), m[y * BUNKER_W + x]);
            }
        }
        return;
    }
    // krysztal: kolor z PNG, krycie z maski (kruszonej), jasny pas przesuwajacy sie po powierzchni
    const int band = (int)(fmodf(anim_ * 60.f, (float)(BUNKER_W + 80))) - 40;
    for (int b = 0; b < BUNKERS; ++b) {
        const uint8_t* m = bunker_mask_[b];
        if (!m) continue;
        const int bx0 = W * (b + 1) / (BUNKERS + 1) - BUNKER_W / 2;
        for (int y = 0; y < BUNKER_H; ++y) {
            uint16_t* row = c.data() + (size_t)(BUNKER_Y + y) * W + bx0;
            const uint16_t* src = bunker_img_.px + (size_t)y * BUNKER_W;
            const uint8_t* mr = m + (size_t)y * BUNKER_W;
            for (int x = 0; x < BUNKER_W; ++x) {
                const int a = mr[x];
                if (!a) continue;
                uint16_t col = src[x];
                const int d = engine::iabs(x - y / 2 - band);
                if (d < 10) col = add565(col, gfx::WHITE, (10 - d) * 9);
                row[x] = blend565(row[x], col, a * 230 >> 8);
            }
        }
    }
}

void InvadersGame::draw_aliens(gfx::Canvas& c, int ox, int oy)
{
    // Ready: formacja zjezdza z gory
    float slide = 0;
    if (state_ == State::Ready) {
        const float t = 1.f - engine::clampf(state_t_ / (READY_TIME * 0.8f), 0.f, 1.f);
        slide = t * t * 340.f;
    }
    for (int i = 0; i < ALIENS; ++i) {
        const Alien& a = alien_[i];
        if (!a.alive) continue;
        const int col = i % FCOLS;
        float x = alien_x(i), y = alien_y(i);
        if (a.mode == IN_FORMATION) {
            y += sinf(anim_ * 3.f + (float)col * 0.6f + (float)(i / FCOLS) * 0.9f) * 2.5f - slide;
        } else {
            glow(c, (int)x + ox, (int)y + oy, 34, 30, TYPE_GLOW[a.type], 70);
        }
        const Pic& p = alien_img_[a.type][fframe_];
        const int sx = (int)x - p.w / 2 + ox, sy = (int)y - p.h / 2 + oy;
        if (sy > H || sy + p.h < 0) continue;
        blit_pic(c, p, sx, sy);
    }
    (void)CELL_H;
}

void InvadersGame::draw_boss(gfx::Canvas& c, int ox, int oy)
{
    if (!boss_alive_) return;
    float slide = 0;
    if (state_ == State::Ready) {
        const float t = 1.f - engine::clampf(state_t_ / (READY_TIME * 0.8f), 0.f, 1.f);
        slide = t * t * 320.f;
    }
    const int x = (int)boss_x_ + ox, y = (int)(boss_y_ - slide) + oy;
    const bool hurt = boss_hp_ * 2 < boss_max_;
    const float pulse = 0.5f + 0.5f * sinf(anim_ * 4.f);
    glow(c, x, y + 10, 110, 95, hurt ? gfx::rgb565(255, 60, 40) : gfx::rgb565(150, 60, 255), (int)(40.f + 40.f * pulse));
    const Pic& p = boss_img_[hurt ? 1 : 0];
    if (boss_flash_ > 0) blit_pic_tint(c, p, x - p.w / 2, y - p.h / 2, gfx::WHITE, 170);
    else blit_pic(c, p, x - p.w / 2, y - p.h / 2);
    // pasek zycia
    if (state_ != State::Ready) {
        const int bw = 360, bx = (W - bw) / 2, by = HUD_H + 8;
        fill_round_rect_alpha(c, bx - 4, by - 4, bw + 8, 18, 8, COL_DARK, 200);
        const int fillw = bw * boss_hp_ / engine::imax(boss_max_, 1);
        for (int k = 0; k < fillw; ++k) {
            const uint16_t col = blend565(gfx::rgb565(255, 60, 60), gfx::rgb565(255, 210, 60), k * 255 / bw);
            c.vline(bx + k, by, 10, col);
        }
        gfx::draw_text_px(c, bx, by + 14, "BOSS", COL_GOLD, 14);
    }
}

void InvadersGame::draw_ufo(gfx::Canvas& c, int ox, int oy)
{
    if (!ufo_alive_) return;
    const int x = (int)ufo_x_ + ox, y = (int)(UFO_Y + sinf(anim_ * 5.f) * 3.f) + oy;
    glow(c, x, y + 14, 48, 22, gfx::rgb565(255, 60, 60), (int)(60.f + 40.f * sinf(anim_ * 12.f)));
    blit_pic(c, ufo_img_, x - ufo_img_.w / 2, y - ufo_img_.h / 2);
}

void InvadersGame::draw_ship(gfx::Canvas& c, int ox, int oy)
{
    if (state_ == State::Dying || state_ == State::GameOver) return;
    if (invuln_ > 0 && invuln_ < INVULN_TIME && ((int)(invuln_ * 12.f) & 1)) return;
    const int x = (int)ship_x_ + ox, y = SHIP_Y + oy;
    const float fl = 0.7f + 0.3f * sinf(anim_ * 40.f) * sinf(anim_ * 17.f);
    glow(c, x, y + 30, 14, (int)(18.f * fl) + 6, gfx::rgb565(60, 170, 255), 150);
    blit_pic(c, ship_img_, x - ship_img_.w / 2, y - ship_img_.h / 2);
    if (pw_time_[PW_LASER] > 0) glow(c, x, y - 28, 10, 10, gfx::rgb565(255, 210, 60), 120);
    if (pw_time_[PW_SHIELD] > 0) {
        const bool ending = pw_time_[PW_SHIELD] < 3.f && ((int)(anim_ * 8.f) & 1);
        if (!ending) ring(c, x, y - 2, 40, 38, gfx::rgb565(70, 150, 255), (int)(150.f + 50.f * sinf(anim_ * 5.f)));
    }
}

void InvadersGame::draw_shots(gfx::Canvas& c, int ox, int oy)
{
    const bool laser = pw_time_[PW_LASER] > 0;
    for (const Shot& s : bolt_) {
        if (!s.alive) continue;
        const int x = (int)s.x + ox, y = (int)s.y + oy;
        glow(c, x, y, 8, 24, laser ? gfx::rgb565(255, 200, 60) : gfx::rgb565(40, 170, 255), 130);
        blit_pic(c, bolt_img_, x - bolt_img_.w / 2, y - bolt_img_.h / 2);
    }
    for (const Shot& b : bomb_) {
        if (!b.alive) continue;
        const int x = (int)b.x + ox, y = (int)b.y + oy;
        const uint16_t g = b.kind == 1 ? gfx::rgb565(255, 60, 200) : gfx::rgb565(120, 255, 60);
        glow(c, x, y + 6, 16, 18, g, 110);
        if (b.kind == 1) blit_pic_tint(c, bomb_img_, x - bomb_img_.w / 2, y - bomb_img_.h / 2, gfx::rgb565(255, 80, 220), 150);
        else blit_pic(c, bomb_img_, x - bomb_img_.w / 2, y - bomb_img_.h / 2);
    }
    for (const Pickup& p : pickup_) {
        if (!p.alive) continue;
        const int x = (int)p.x + ox, y = (int)(p.y + sinf(anim_ * 6.f) * 3.f) + oy;
        glow(c, x, y, 30, 30, POWER_GLOW[p.kind], (int)(90.f + 50.f * sinf(anim_ * 8.f)));
        const Pic& im = power_img_[p.kind];
        blit_pic(c, im, x - im.w / 2, y - im.h / 2);
    }
}

void InvadersGame::draw_effects(gfx::Canvas& c, int ox, int oy)
{
    for (const Boom& b : boom_) {
        if (!b.alive) continue;
        const float t = b.t / b.dur;
        const int f = engine::iclamp((int)(t * (float)BOOM_FRAMES), 0, BOOM_FRAMES - 1);
        const int alpha = t > 0.75f ? (int)(256.f * (1.f - t) / 0.25f) : 256;
        const int x = (int)b.x + ox, y = (int)b.y + oy;
        if (t < 0.5f) glow(c, x, y, (int)(50.f * b.scale), (int)(50.f * b.scale), gfx::rgb565(255, 150, 60), (int)(160.f * (1.f - t * 2.f)));
        blit_pic_scaled(c, boom_img_[f], x, y, b.scale * (0.8f + 0.4f * t), engine::iclamp(alpha, 0, 256));
    }
    uint16_t* px = c.data();
    for (const Spark& s : spark_) {
        if (!s.alive) continue;
        const int x = (int)s.x + ox, y = (int)s.y + oy;
        if (x < 0 || y < 0 || x >= W - 1 || y >= H - 1) continue;
        const int a = (int)(255.f * s.life / s.max);
        uint16_t* row = px + (size_t)y * W + x;
        row[0] = add565(row[0], s.color, a);
        row[1] = add565(row[1], s.color, a * 2 / 3);
        row[W] = add565(row[W], s.color, a * 2 / 3);
        row[W + 1] = add565(row[W + 1], s.color, a / 3);
    }
    for (const Popup& p : popup_) {
        if (!p.alive) continue;
        const int alpha = p.t < 0.3f ? 1 : 0;
        if (alpha && ((int)(p.t * 30.f) & 1)) continue;
        text_outline(c, center_x(p.text, 16, (int)p.x), (int)p.y - 8, p.text, p.color, COL_DARK, 16, 1);
    }
}

void InvadersGame::draw_hud(gfx::Canvas& c)
{
    fill_rect_alpha(c, 0, 0, W, HUD_H, COL_DARK, 170);
    fill_rect_alpha(c, 0, HUD_H, W, 1, COL_CYAN, 90);
    char buf[32];
    gfx::draw_text_px(c, 12, 9, "PUNKTY", COL_MUTED, 14);
    snprintf(buf, sizeof(buf), "%d", score_);
    gfx::draw_text_px(c, 78, 5, buf, COL_TEXT, 20);
    gfx::draw_text_px(c, 200, 9, "REKORD", COL_MUTED, 14);
    snprintf(buf, sizeof(buf), "%d", engine::imax(top_.score, score_));
    gfx::draw_text_px(c, 266, 5, buf, COL_GOLD, 20);
    snprintf(buf, sizeof(buf), "FALA %d", level_);
    gfx::draw_text_px(c, center_x(buf, 20, W / 2 + 40), 5, buf, COL_CYAN, 20);
    if (combo_ >= 2) {
        snprintf(buf, sizeof(buf), "x%d", combo_);
        fill_round_rect_alpha(c, 132, HUD_H + 4, 52, 24, 8, gfx::rgb565(120, 30, 110), 210);
        gfx::draw_text_px(c, center_x(buf, 20, 158), HUD_H + 5, buf, gfx::rgb565(255, 150, 235), 20);
    }
    // aktywne bonusy: ikona + pasek czasu
    int bx = 500;
    for (int p = 0; p < 4; ++p) {
        if (pw_time_[p] <= 0) continue;
        static const float DUR[4] = { 12.f, 7.f, 25.f, 8.f };
        const Pic& im = power_img_[p];
        blit_pic_scaled(c, im, bx + 12, 15, 0.6f);
        const int w = (int)(22.f * pw_time_[p] / DUR[p]);
        c.fill_rect(bx + 1, 29, w, 3, POWER_GLOW[p]);
        bx += 30;
    }
    // zycia
    for (int i = 0; i < engine::imin(lives_, 6); ++i) blit_pic(c, icon_ship_, W - 30 - i * 26, 5);
    if (autopilot_) {
        fill_round_rect_alpha(c, W - 120, HUD_H + 6, 108, 22, 8, gfx::rgb565(40, 120, 70), 200);
        gfx::draw_text_px(c, center_x("AUTOPILOT", 14, W - 66), HUD_H + 10, "AUTOPILOT", COL_TEXT, 14);
    }
}

void InvadersGame::draw_title(gfx::Canvas& c)
{
    if (title_) memcpy(c.data(), title_, (size_t)W * H * 2);
    else c.clear(COL_DARK);
    // logo z pulsujaca poswiata w pustej gornej czesci ilustracji
    const float pulse = 0.5f + 0.5f * sinf(anim_ * 2.5f);
    glow(c, W / 2, 62, 250, 58, gfx::rgb565(60, 120, 255), (int)(60.f + 50.f * pulse));
    text_outline(c, center_x("SPACE INVADERS", 48, W / 2), 26, "SPACE INVADERS", COL_CYAN, gfx::rgb565(10, 20, 70), 48, 3);
    text_outline(c, center_x("KOSMICZNI NAJEZDZCY", 20, W / 2), 84, "KOSMICZNI NAJEZDZCY", COL_TEXT, COL_DARK, 20, 2);
    char buf[64];
    snprintf(buf, sizeof(buf), "REKORD %d", top_.score);
    text_outline(c, center_x(buf, 16, W / 2), 112, buf, COL_GOLD, COL_DARK, 16, 1);
    // panel dolny
    fill_round_rect_alpha(c, 150, H - 96, W - 300, 80, 16, COL_DARK, 190);
    if (((int)(anim_ * 2.f) & 1) == 0)
        text_outline(c, center_x("NACISNIJ A", 24, W / 2), H - 90, "NACISNIJ A", COL_GOLD, COL_DARK, 24, 2);
    snprintf(buf, sizeof(buf), "X: fala startowa %d      Y: autopilot %s", start_level_, autopilot_ ? "wl." : "wyl.");
    gfx::draw_text_px(c, center_x(buf, 16, W / 2), H - 54, buf, COL_MUTED, 16);
    const char* help = "LEWO/PRAWO: ruch    A: strzal (przytrzymaj = seria)";
    gfx::draw_text_px(c, center_x(help, 14, W / 2), H - 33, help, COL_MUTED, 14);
}

void InvadersGame::draw_overlay(gfx::Canvas& c)
{
    char buf[40];
    switch (state_) {
    case State::Ready: {
        snprintf(buf, sizeof(buf), "FALA %d", level_);
        const int y = 214;
        fill_round_rect_alpha(c, W / 2 - 170, y - 10, 340, boss_level_ ? 100 : 80, 16, COL_DARK, 170);
        text_outline(c, center_x(buf, 40, W / 2), y - 4, buf, COL_CYAN, gfx::rgb565(10, 20, 70), 40, 2);
        gfx::draw_text_px(c, center_x(WORLDS_DEF[world_].name, 16, W / 2), y + 42, WORLDS_DEF[world_].name, COL_MUTED, 16);
        if (boss_level_ && ((int)(anim_ * 4.f) & 1))
            text_outline(c, center_x("UWAGA: BOSS!", 20, W / 2), y + 62, "UWAGA: BOSS!", gfx::rgb565(255, 80, 70), COL_DARK, 20, 1);
        break;
    }
    case State::Clear:
        snprintf(buf, sizeof(buf), "FALA %d ZALICZONA", level_);
        fill_round_rect_alpha(c, W / 2 - 200, 200, 400, 56, 16, COL_DARK, 170);
        text_outline(c, center_x(buf, 28, W / 2), 210, buf, COL_GOLD, gfx::rgb565(40, 20, 0), 28, 2);
        break;
    case State::GameOver:
        fill_round_rect_alpha(c, W / 2 - 200, 190, 400, 96, 16, COL_DARK, 200);
        text_outline(c, center_x("KONIEC GRY", 40, W / 2), 198, "KONIEC GRY", gfx::rgb565(255, 70, 60), gfx::rgb565(30, 0, 0), 40, 2);
        snprintf(buf, sizeof(buf), "Wynik %d", score_);
        gfx::draw_text_px(c, center_x(buf, 16, W / 2), 244, buf, COL_TEXT, 16);
        gfx::draw_text_px(c, center_x("A: jeszcze raz    X: tytul", 16, W / 2), 264, "A: jeszcze raz    X: tytul", COL_MUTED, 16);
        break;
    default:
        break;
    }
}

}  // namespace invaders
