// Snake - rysowanie: grafika z assets/snake/ (PNG z alfa z Gemini, obrobione tools/gen_snake_assets.py),
// cialo weza z cieniowanych kulek generowanych w kodzie, HUD i ekrany.
//
// Koszt klatki: tlo pola (tlo swiata + siatka + przeszkody z cieniami) jest "wypalane" raz na poziom do bake_
// i kopiowane jednym memcpy. Waz: na kazdy segment 2 probki (segment + polowa drogi do nastepnego), kazda to
// obrys (kulka o 2 px wieksza) + kulka koloru; cien przez maske (bez podwojnego przyciemnienia).
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "games/snake/snake_game.h"
#include "gfx/palette.h"
#include "gfx/png.h"
#include "gfx/text.h"
#include "platform/platform.h"

namespace snake {

namespace {

const char* TAG = "snake";

using G = SnakeGame;

constexpr int W = G::W, H = G::H, CELL = G::CELL, FIELD_Y = G::FIELD_Y, FIELD_W = G::FIELD_W, FIELD_H = G::FIELD_H;
constexpr int R_MIN = 7, R_MAX = 14;   // promienie kulek ciala

inline uint16_t pack565(int r, int g, int b) { return (uint16_t)((r << 11) | (g << 5) | b); }
inline uint16_t blend565(uint16_t dst, uint16_t src, int a)   // a = 0..255 krycie src
{
    const int dr = (dst >> 11) & 31, dg = (dst >> 5) & 63, db = dst & 31;
    const int sr = (src >> 11) & 31, sg = (src >> 5) & 63, sb = src & 31;
    return pack565(dr + (((sr - dr) * a) >> 8), dg + (((sg - dg) * a) >> 8), db + (((sb - db) * a) >> 8));
}
inline uint16_t scale565(uint16_t c, int k)   // k/256 jasnosci
{
    const int r = ((c >> 11) & 31) * k >> 8, g = ((c >> 5) & 63) * k >> 8, b = (c & 31) * k >> 8;
    return pack565(r > 31 ? 31 : r, g > 63 ? 63 : g, b > 31 ? 31 : b);
}

// Obszar przycinania rysowania obrazkow (pole gry albo caly ekran).
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

// Ksztalt obrazka jednym kolorem (obrys ciala, cien).
void blit_pic_color(gfx::Canvas& c, const Pic& p, int x, int y, uint16_t color, int alpha_mul = 256)
{
    if (!p.px) return;
    const int cw = c.width();
    const int x0 = engine::imax(x, engine::imax(g_clip.x0, 0)), y0 = engine::imax(y, engine::imax(g_clip.y0, 0));
    const int x1 = engine::imin(x + p.w, engine::imin(g_clip.x1, cw)), y1 = engine::imin(y + p.h, engine::imin(g_clip.y1, c.height()));
    uint16_t* px = c.data();
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t*      drow = px + (size_t)yy * cw;
        const uint8_t* arow = p.alpha + (size_t)(yy - y) * p.w - x;
        for (int xx = x0; xx < x1; ++xx) {
            int a = arow[xx];
            if (!a) continue;
            a = a * alpha_mul >> 8;
            drow[xx] = a >= 255 ? color : blend565(drow[xx], color, a);
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

// Wypelniona elipsa z miekka krawedzia (cienie pod przedmiotami, poswiata).
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

void panel(gfx::Canvas& c, int x, int y, int w, int h, int a = 200)
{
    fill_round_rect_alpha(c, x + 3, y + 5, w, h, 16, gfx::BLACK, a / 3);
    fill_round_rect_alpha(c, x, y, w, h, 16, gfx::rgb565(18, 28, 30), a);
    fill_rect_alpha(c, x + 16, y + 1, w - 32, 1, gfx::rgb565(150, 220, 170), 90);
}

void text_shadow(gfx::Canvas& c, int x, int y, const char* s, uint16_t col, int px)
{
    gfx::draw_text_px(c, x + 2, y + 2, s, gfx::rgb565(4, 10, 6), px);
    gfx::draw_text_px(c, x, y, s, col, px);
}

void text_outline(gfx::Canvas& c, int x, int y, const char* s, uint16_t col, uint16_t outline, int px, int t)
{
    for (int dy = -t; dy <= t; dy += t)
        for (int dx = -t; dx <= t; dx += t)
            if (dx || dy) gfx::draw_text_px(c, x + dx, y + dy, s, outline, px);
    gfx::draw_text_px(c, x, y, s, col, px);
}

int center_x(const char* s, int px, int cx = W / 2) { return cx - gfx::text_width_px(s, px) / 2; }

// Bufory obrazkow generowanych w kodzie (~180 kB razem) - w PSRAM, jak obrazy z PNG; zyja do konca programu.
uint16_t* alloc_px(int n) { return platform::alloc_pixels((size_t)n, false); }
uint8_t*  alloc_a(int n) { return (uint8_t*)platform::alloc_pixels((size_t)(n + 1) / 2, false); }

Pic from_image(const gfx::Image& im)
{
    Pic p;
    p.w = im.w;
    p.h = im.h;
    p.px = im.px;
    p.alpha = im.alpha;
    return p;
}

Pic load_pic(const char* name)
{
    char path[64];
    snprintf(path, sizeof(path), "snake/%s.png", name);
    return from_image(gfx::load_png_rgba(path));
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
            case 1: sx = y; sy = s.h - 1 - x; break;          // 90 w prawo
            case 2: sx = s.w - 1 - x; sy = s.h - 1 - y; break;
            default: sx = s.w - 1 - y; sy = x; break;         // 90 w lewo
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

// Obrot o dowolny kat (najblizszy sasiad) - klatki portalu.
Pic rotate_any(const Pic& s, float ang)
{
    Pic d;
    if (!s.px) return d;
    const int n = s.w * s.h;
    uint16_t* px = alloc_px(n);
    uint8_t*  al = alloc_a(n);
    d.w = s.w;
    d.h = s.h;
    const float ca = cosf(ang), sa = sinf(ang), cx = (float)s.w * 0.5f, cy = (float)s.h * 0.5f;
    for (int y = 0; y < s.h; ++y) {
        for (int x = 0; x < s.w; ++x) {
            const float fx = (float)x + 0.5f - cx, fy = (float)y + 0.5f - cy;
            const int sx = (int)floorf(ca * fx + sa * fy + cx), sy = (int)floorf(-sa * fx + ca * fy + cy);
            const bool in = sx >= 0 && sy >= 0 && sx < s.w && sy < s.h;
            px[y * s.w + x] = in ? s.px[sy * s.w + sx] : 0;
            al[y * s.w + x] = in ? s.alpha[sy * s.w + sx] : 0;
        }
    }
    d.px = px;
    d.alpha = al;
    return d;
}

// Kulka cieniowana (Lambert + odblask), swiatlo z lewej-gory; alfa wygladzona na krawedzi.
Pic make_ball(int r, int br, int bg, int bb, bool flat)
{
    Pic p;
    const int s = 2 * r + 2;
    uint16_t* px = alloc_px(s * s);
    uint8_t*  al = alloc_a(s * s);
    const float c = (float)s * 0.5f;
    const float lx = -0.45f, ly = -0.6f, lz = 0.66f;   // znormalizowane ~1
    const float hx = lx, hy = ly, hz = lz + 1.f, hl = sqrtf(hx * hx + hy * hy + hz * hz);
    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            const float dx = (float)x + 0.5f - c, dy = (float)y + 0.5f - c;
            const float d = sqrtf(dx * dx + dy * dy);
            const float a = engine::clampf((float)r - d + 0.5f, 0.f, 1.f);
            al[y * s + x] = (uint8_t)(a * 255.f);
            if (flat) {
                px[y * s + x] = gfx::rgb565((uint8_t)br, (uint8_t)bg, (uint8_t)bb);
                continue;
            }
            const float nx = dx / (float)r, ny = dy / (float)r;
            const float nz = sqrtf(fmaxf(0.f, 1.f - nx * nx - ny * ny));
            const float lam = fmaxf(0.f, nx * lx + ny * ly + nz * lz);
            const float sp = powf(fmaxf(0.f, (nx * hx + ny * hy + nz * hz) / hl), 28.f);
            const float k = 0.5f + 0.62f * lam;
            const int rr = engine::iclamp((int)((float)br * k + 190.f * sp), 0, 255);
            const int gg = engine::iclamp((int)((float)bg * k + 190.f * sp), 0, 255);
            const int bb2 = engine::iclamp((int)((float)bb * k + 190.f * sp), 0, 255);
            px[y * s + x] = gfx::rgb565((uint8_t)rr, (uint8_t)gg, (uint8_t)bb2);
        }
    }
    p.w = p.h = s;
    p.px = px;
    p.alpha = al;
    return p;
}

struct Skin { uint8_t r, g, b, r2, g2, b2; };
const Skin SKINS[3] = {
    { 120, 205, 60, 88, 168, 44 },     // zielony (jak glowa z Gemini)
    { 70, 150, 235, 45, 112, 200 },    // niebieski
    { 250, 145, 45, 222, 110, 30 },    // pomaranczowy
};
const char* const SKIN_NAMES[3] = { "Zielony", "Niebieski", "Pomaranczowy" };

const char* const ITEM_FILES[G::ITEM_COUNT] = { "apple", "golden", "mushroom", "hourglass", "star", "heart", "magnet", "gem", "orb", "bomb" };
const char* const OBST_FILES[5][3] = {
    { "bush", "rock", "stump" },          // [0] = material scian (przeszkody z sasiadem)
    { "sandstone", "cactus", "rock" },
    { "ice", "snowrock", "snowrock" },
    { "crate", "bush", "stump" },
    { "lavarock", "rock", "lavarock" },
};
const char* const BG_FILES[5] = { "bg_meadow", "bg_desert", "bg_snow", "bg_jungle", "bg_volcano" };
// Kolory zastepcze tla (gdy brak PNG) i kolor ramki-sciany
const uint16_t BG_FALLBACK[5] = { gfx::rgb565(78, 150, 60), gfx::rgb565(214, 170, 100), gfx::rgb565(215, 228, 240),
                                  gfx::rgb565(30, 80, 50), gfx::rgb565(60, 40, 40) };

}  // namespace

// ============================================================================ zasoby

void SnakeGame::load_assets()
{
    for (int i = 0; i < ITEM_COUNT; ++i) {
        item_[i] = load_pic(ITEM_FILES[i]);
        char n[32];
        snprintf(n, sizeof(n), "icon_%s", ITEM_FILES[i]);
        char path[64];
        snprintf(path, sizeof(path), "snake/%s.png", n);
        int w, h;
        if (gfx::png_size(path, w, h)) icon_[i] = load_pic(n);
    }
    // przeszkody: ten sam plik uzywany w kilku swiatach - wczytany raz
    for (int w = 0; w < 5; ++w) {
        for (int v = 0; v < 3; ++v) {
            Pic found;
            for (int pw = 0; pw <= w && !found.px; ++pw)
                for (int pv = 0; pv < 3 && !found.px; ++pv)
                    if ((pw < w || pv < v) && strcmp(OBST_FILES[pw][pv], OBST_FILES[w][v]) == 0) found = obst_[pw][pv];
            obst_[w][v] = found.px ? found : load_pic(OBST_FILES[w][v]);
        }
    }
    static const char* const HEADS[3] = { "head_green", "head_blue", "head_orange" };
    for (int s = 0; s < 3; ++s) {
        const Pic src = load_pic(HEADS[s]);
        // grafika patrzy w prawo; w lewo = lustro (nie do gory nogami), gora/dol = obrot
        head_[s][RIGHT] = src;
        head_[s][DOWN]  = rotate90(src, 1, false);
        head_[s][LEFT]  = rotate90(src, 0, true);
        head_[s][UP]    = rotate90(src, 3, false);
    }
    const Pic portal = load_pic("portal");
    for (int i = 0; i < 8; ++i) portal_img_[i] = i == 0 ? portal : rotate_any(portal, -(float)i * 0.7854f / 2.f);

    for (int s = 0; s < 3; ++s) {
        for (int r = R_MIN; r <= R_MAX; ++r) {
            ball_[s][0][r - R_MIN] = make_ball(r, SKINS[s].r, SKINS[s].g, SKINS[s].b, false);
            ball_[s][1][r - R_MIN] = make_ball(r, SKINS[s].r2, SKINS[s].g2, SKINS[s].b2, false);
        }
    }
    for (int r = R_MIN; r <= R_MAX; ++r) dark_[r - R_MIN] = make_ball(r + 2, 255, 255, 255, true);

    title_img_ = gfx::load_png_rgba("snake/title.png");
    bg_   = platform::alloc_pixels((size_t)FIELD_W * FIELD_H, false);
    bake_ = platform::alloc_pixels((size_t)FIELD_W * FIELD_H, false);
    mask_ = (uint8_t*)platform::alloc_pixels((size_t)FIELD_W * FIELD_H / 2 + 1, false);
    memset(mask_, 0, (size_t)FIELD_W * FIELD_H);
    CONSOLE_LOGI(TAG, "grafika: %s, glowa %s, tytul %s", item_[APPLE].px ? "PNG z assets/snake" : "BRAK - figury zastepcze",
                 head_[0][0].px ? "tak" : "nie", title_img_.px ? "tak" : "nie");
}

void SnakeGame::bake_field()
{
    baked_world_ = world_;
    baked_level_ = mode_ == Mode::Adventure ? level_ : -1;
    char path[48];
    snprintf(path, sizeof(path), "snake/%s.png", BG_FILES[world_]);
    const gfx::Sprite s = gfx::load_png(path, bg_, (size_t)FIELD_W * FIELD_H);
    if (!s.px || s.w != FIELD_W || s.h != FIELD_H) {
        for (int i = 0; i < FIELD_W * FIELD_H; ++i) bg_[i] = BG_FALLBACK[world_];
    }
    memcpy(bake_, bg_, (size_t)FIELD_W * FIELD_H * 2);
    gfx::Canvas c(bake_, FIELD_W, FIELD_H);
    const Clip saved = g_clip;
    g_clip = { 0, 0, FIELD_W, FIELD_H };

    // delikatna szachownica kratek - czytelnosc ruchu po siatce
    const bool light_world = world_ == 1 || world_ == 2;
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x)
            if ((x + y) & 1) fill_rect_alpha(c, x * CELL, y * CELL, CELL, CELL, light_world ? gfx::rgb565(120, 90, 60) : gfx::BLACK, light_world ? 12 : 13);
    // winieta: ciemniejsze brzegi
    for (int y = 0; y < FIELD_H; ++y) {
        uint16_t* row = bake_ + (size_t)y * FIELD_W;
        const float fy = ((float)y - FIELD_H * 0.5f) / (FIELD_H * 0.5f);
        for (int x = 0; x < FIELD_W; ++x) {
            const float fx = ((float)x - FIELD_W * 0.5f) / (FIELD_W * 0.5f);
            const float d = fx * fx * 0.6f + fy * fy * 0.8f;
            if (d > 0.35f) row[x] = scale565(row[x], 256 - (int)engine::clampf((d - 0.35f) * 90.f, 0.f, 70.f));
        }
    }
    // krawedz: sciana (czerwonawa poswiata) albo zawijanie (jasna przerywana linia)
    if (!wrap_) {
        for (int i = 0; i < 6; ++i) {
            const int a = 120 - i * 20;
            const uint16_t col = gfx::rgb565(120, 30, 20);
            fill_rect_alpha(c, 0, i, FIELD_W, 1, col, a);
            fill_rect_alpha(c, 0, FIELD_H - 1 - i, FIELD_W, 1, col, a);
            fill_rect_alpha(c, i, 0, 1, FIELD_H, col, a);
            fill_rect_alpha(c, FIELD_W - 1 - i, 0, 1, FIELD_H, col, a);
        }
    } else {
        for (int x = 0; x < FIELD_W; x += 16) {
            fill_rect_alpha(c, x, 0, 8, 2, gfx::WHITE, 70);
            fill_rect_alpha(c, x, FIELD_H - 2, 8, 2, gfx::WHITE, 70);
        }
        for (int y = 0; y < FIELD_H; y += 16) {
            fill_rect_alpha(c, 0, y, 2, 8, gfx::WHITE, 70);
            fill_rect_alpha(c, FIELD_W - 2, y, 2, 8, gfx::WHITE, 70);
        }
    }
    // przeszkody: cien, potem obrazek (36 px na kratce 32 - lekko wystaje do gory)
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            if (!solid_[y][x]) continue;
            const int cx = x * CELL + CELL / 2, cy = y * CELL + CELL / 2;
            fill_ellipse_alpha(c, cx + 3, cy + 10, 15, 7, gfx::BLACK, 110);
            const Pic& p = obst_[world_][solid_[y][x] - 1];
            if (p.px) blit_pic(c, p, cx - p.w / 2, cy - p.h / 2 - 3);
            else fill_round_rect_alpha(c, x * CELL + 2, y * CELL + 2, CELL - 4, CELL - 4, 6, gfx::rgb565(110, 110, 120), 255);
        }
    }
    g_clip = saved;
}

// ============================================================================ pozycje

void SnakeGame::seg_pos(int i, float& x, float& y) const
{
    float f = 1.f;
    if (state_ == State::Playing && step_int_ > 0) f = engine::clampf(step_acc_ / step_int_, 0.f, 1.f);
    int dx = body_[i].x - prev_[i].x, dy = body_[i].y - prev_[i].y;
    // przejscie przez krawedz (zawijanie): ruch o jedna kratke "na zewnatrz"
    if (dx > 1) dx -= COLS;
    if (dx < -1) dx += COLS;
    if (dy > 1) dy -= ROWS;
    if (dy < -1) dy += ROWS;
    float gx = (float)prev_[i].x + (float)dx * f, gy = (float)prev_[i].y + (float)dy * f;
    if (gx < -0.5f) gx += COLS;
    if (gx > COLS - 0.5f) gx -= COLS;
    if (gy < -0.5f) gy += ROWS;
    if (gy > ROWS - 0.5f) gy -= ROWS;
    x = gx * CELL + CELL / 2;
    y = FIELD_Y + gy * CELL + CELL / 2;
}

// ============================================================================ rysowanie

void SnakeGame::draw_field(gfx::Canvas& c)
{
    const int want_level = mode_ == Mode::Adventure ? level_ : -1;
    if (baked_world_ != world_ || baked_level_ != want_level) bake_field();
    memcpy(c.data() + (size_t)FIELD_Y * W, bake_, (size_t)FIELD_W * FIELD_H * 2);
}

void SnakeGame::draw_pickups(gfx::Canvas& c)
{
    for (const Pickup& p : pickups_) {
        if (!p.alive) continue;
        // miganie przed zniknieciem
        if (p.life > 0 && p.life < 2.f && fmodf(p.life, 0.24f) < 0.1f) continue;
        const int cx = (int)(p.fx * CELL) + CELL / 2, cy = FIELD_Y + (int)(p.fy * CELL) + CELL / 2;
        const float bob = sinf(anim_ * 4.f + (float)(p.x * 3 + p.y)) * 2.5f;
        // pojawienie: wyskok ze skali (tu: z przesuniecia) w pierwszej 0,2 s
        const float pop = p.age < 0.2f ? (0.2f - p.age) * 60.f : 0.f;
        fill_ellipse_alpha(c, cx + 2, cy + 11, 13 - (int)(bob * 0.6f), 5, gfx::BLACK, 90);
        if (p.type == GOLDEN || p.type == STAR || p.type == GEM || p.type == ORB) {
            const int glow = 90 + (int)(40.f * sinf(anim_ * 6.f));
            static const uint16_t GL[4] = { gfx::rgb565(255, 230, 120), gfx::rgb565(255, 240, 150), gfx::rgb565(150, 255, 170), gfx::rgb565(140, 220, 255) };
            const int gi = p.type == GOLDEN ? 0 : p.type == STAR ? 1 : p.type == GEM ? 2 : 3;
            fill_ellipse_alpha(c, cx, cy - 4 + (int)bob, 22, 22, GL[gi], glow / 2);
        }
        if (p.type == BOMB && p.life > 0 && p.life < 4.f && fmodf(p.life, 0.4f) < 0.2f)
            fill_ellipse_alpha(c, cx, cy - 4, 22, 22, gfx::rgb565(255, 60, 30), 110);
        const Pic& pic = item_[p.type];
        if (pic.px) blit_pic(c, pic, cx - pic.w / 2, cy - pic.h / 2 - 4 + (int)(bob - pop));
        else c.fill_circle(cx, cy, 12, gfx::pal::RED);
        // iskra na lontcie bomby
        if (p.type == BOMB) {
            const int sx = cx + 12, sy = cy - 20 + (int)bob;
            const int fl = (int)(anim_ * 20.f) & 3;
            fill_ellipse_alpha(c, sx, sy, 4 + fl, 4 + fl, gfx::rgb565(255, 220, 90), 170);
        }
    }
}

void SnakeGame::draw_portal(gfx::Canvas& c)
{
    if (!portal_open_) return;
    const int cx = portal_.x * CELL + CELL / 2, cy = FIELD_Y + portal_.y * CELL + CELL / 2;
    const float pulse = 0.5f + 0.5f * sinf(anim_ * 5.f);
    fill_ellipse_alpha(c, cx, cy, 30 + (int)(pulse * 6), 30 + (int)(pulse * 6), gfx::rgb565(170, 90, 255), 60 + (int)(pulse * 40));
    const Pic& p = portal_img_[(int)(anim_ * 14.f) & 7];
    if (p.px) blit_pic(c, p, cx - p.w / 2, cy - p.h / 2);
    else c.fill_circle(cx, cy, 16, gfx::rgb565(160, 80, 255));
}

void SnakeGame::draw_snake(gfx::Canvas& c)
{
    if (len_ <= 0) return;
    const int skip = state_ == State::Dying ? dying_i_ : 0;   // przy smierci segmenty znikaja od glowy
    if (skip >= len_) return;

    // probki: segmenty od ogona + polowa drogi do poprzednika; promien zwezany na ostatnich 5 segmentach
    struct S { int x, y; uint8_t r, tone; };
    static S pts[MAX_LEN * 2 + 2];
    int n = 0;
    for (int i = len_ - 1; i >= skip; --i) {
        float x, y;
        seg_pos(i, x, y);
        const int from_tail = len_ - 1 - i;
        const int r = i == 0 ? R_MAX : engine::imin(R_MAX - 1, R_MIN + 2 + from_tail);
        const uint8_t tone = (uint8_t)((i / 2) & 1);
        if (n > 0) {
            const S& q = pts[n - 1];
            const float mx = (x + (float)q.x) * 0.5f, my = (y + (float)q.y) * 0.5f;
            if (fabsf(x - (float)q.x) < CELL * 1.5f && fabsf(y - (float)q.y) < CELL * 1.5f)
                pts[n++] = { (int)mx, (int)my, (uint8_t)((r + q.r) / 2), q.tone };
        }
        pts[n++] = { (int)x, (int)y, (uint8_t)r, tone };
    }

    const bool ghost = ghost_t_ > 0 && (ghost_t_ > 1.5f || fmodf(ghost_t_, 0.2f) < 0.12f);
    const int  amul = ghost ? 150 : 256;

    // 1) cien przez maske (maks. krycie na piksel, bez sumowania nakladajacych sie kulek)
    int bx0 = FIELD_W, by0 = FIELD_H, bx1 = 0, by1 = 0;
    for (int k = 0; k < n; ++k) {
        const Pic& d = dark_[pts[k].r - R_MIN];
        const int x0 = pts[k].x - d.w / 2 + 4, y0 = pts[k].y - FIELD_Y - d.h / 2 + 6;
        for (int yy = 0; yy < d.h; ++yy) {
            const int fy = y0 + yy;
            if (fy < 0 || fy >= FIELD_H) continue;
            uint8_t* mrow = mask_ + (size_t)fy * FIELD_W;
            const uint8_t* arow = d.alpha + yy * d.w;
            for (int xx = 0; xx < d.w; ++xx) {
                const int fx = x0 + xx;
                if (fx < 0 || fx >= FIELD_W) continue;
                if (arow[xx] > mrow[fx]) mrow[fx] = arow[xx];
            }
        }
        bx0 = engine::imin(bx0, x0); by0 = engine::imin(by0, y0);
        bx1 = engine::imax(bx1, x0 + d.w); by1 = engine::imax(by1, y0 + d.h);
    }
    bx0 = engine::imax(bx0, 0); by0 = engine::imax(by0, 0);
    bx1 = engine::imin(bx1, FIELD_W); by1 = engine::imin(by1, FIELD_H);
    const int shadow_a = ghost ? 40 : 85;
    for (int y = by0; y < by1; ++y) {
        uint8_t*  mrow = mask_ + (size_t)y * FIELD_W;
        uint16_t* crow = c.data() + (size_t)(y + FIELD_Y) * W;
        for (int x = bx0; x < bx1; ++x) {
            if (!mrow[x]) continue;
            crow[x] = blend565(crow[x], gfx::BLACK, mrow[x] * shadow_a >> 8);
            mrow[x] = 0;
        }
    }

    // 2) obrys, 3) kulki koloru - osobne przejscia, zeby obrys nie przecinal ciala
    const uint16_t outline = pack565(SKINS[skin_].r2 >> 5, SKINS[skin_].g2 >> 4, SKINS[skin_].b2 >> 5);
    for (int k = 0; k < n; ++k) {
        const Pic& d = dark_[pts[k].r - R_MIN];
        blit_pic_color(c, d, pts[k].x - d.w / 2, pts[k].y - d.h / 2, outline, amul);
    }
    for (int k = 0; k < n; ++k) {
        const Pic& b = ball_[skin_][pts[k].tone][pts[k].r - R_MIN];
        blit_pic(c, b, pts[k].x - b.w / 2, pts[k].y - b.h / 2, amul);
    }

    // glowa
    if (skip == 0) {
        float hx, hy;
        seg_pos(0, hx, hy);
        const int d = last_dir_;
        static const int FX[4] = { 1, 0, -1, 0 }, FY[4] = { 0, 1, 0, -1 };
        // jezyk: co ~1,6 s na 0,25 s
        const float tph = fmodf(anim_, 1.6f);
        if (tph < 0.25f && state_ != State::Dying) {
            const float ext = sinf(tph / 0.25f * 3.1416f);
            const int len = 6 + (int)(ext * 10.f);
            const int x0 = (int)hx + FX[d] * 16, y0 = (int)hy + FY[d] * 16;
            const int x1 = x0 + FX[d] * len, y1 = y0 + FY[d] * len;
            const uint16_t red = gfx::rgb565(230, 40, 60);
            for (int t = -1; t <= 1; ++t) {
                c.line(x0 + FY[d] * t, y0 + FX[d] * t, x1 + FY[d] * t, y1 + FX[d] * t, red);
            }
            // rozwidlenie
            c.line(x1, y1, x1 + FX[d] * 4 + FY[d] * 4, y1 + FY[d] * 4 + FX[d] * 4, red);
            c.line(x1, y1, x1 + FX[d] * 4 - FY[d] * 4, y1 + FY[d] * 4 - FX[d] * 4, red);
        }
        const Pic& h = head_[skin_][d];
        if (h.px) {
            blit_pic(c, h, (int)hx - h.w / 2 + FX[d] * 3, (int)hy - h.h / 2 + FY[d] * 3, amul);
        } else {
            c.fill_circle((int)hx, (int)hy, 14, gfx::rgb565(SKINS[skin_].r, SKINS[skin_].g, SKINS[skin_].b));
            c.fill_circle((int)hx + FX[d] * 5 - FY[d] * 6, (int)hy + FY[d] * 5 - FX[d] * 6, 4, gfx::WHITE);
            c.fill_circle((int)hx + FX[d] * 5 + FY[d] * 6, (int)hy + FY[d] * 5 + FX[d] * 6, 4, gfx::WHITE);
        }
        if (shield_) {
            const int rr = 21 + (int)(2.f * sinf(anim_ * 6.f));
            c.draw_circle((int)hx, (int)hy, rr, gfx::rgb565(120, 220, 255));
            c.draw_circle((int)hx, (int)hy, rr - 1, gfx::rgb565(60, 170, 255));
        }
    }
}

void SnakeGame::draw_effects(gfx::Canvas& c)
{
    for (const Part& p : parts_) {
        if (!p.alive) continue;
        const int a = (int)(255.f * engine::clampf(p.t / p.t0, 0.f, 1.f));
        fill_ellipse_alpha(c, (int)p.x, (int)p.y, p.size, p.size, p.color, a);
    }
    for (const Popup& p : popups_) {
        if (!p.alive) continue;
        text_shadow(c, (int)p.x - gfx::text_width_px(p.text, 20) / 2, (int)p.y - 10, p.text, p.color, 20);
    }
    if (flash_t_ > 0) {
        const int a = (int)(flash_t_ * 300.f);
        fill_rect_alpha(c, 0, FIELD_Y, FIELD_W, FIELD_H, gfx::rgb565(255, 40, 30), engine::imin(a, 110));
    }
    if (banner_t_ > 0 && banner_[0]) {
        const int y = FIELD_Y + 40 - (int)((1.6f - banner_t_) * 12.f);
        text_outline(c, center_x(banner_, 32), y, banner_, gfx::rgb565(255, 235, 120), gfx::rgb565(40, 20, 0), 32, 2);
    }
}

void SnakeGame::draw_hud(gfx::Canvas& c)
{
    // pasek: ciemny gradient + jasna kreska u dolu
    for (int y = 0; y < FIELD_Y; ++y) c.hline(0, y, W, gfx::rgb565((uint8_t)(16 + y / 3), (uint8_t)(34 + y / 2), (uint8_t)(30 + y / 3)));
    c.hline(0, FIELD_Y - 1, W, gfx::rgb565(90, 160, 110));
    char buf[48];

    // lewo: poziom i swiat
    if (mode_ == Mode::Adventure) snprintf(buf, sizeof(buf), "POZIOM %d", level_ + 1);
    else snprintf(buf, sizeof(buf), "BEZ KONCA");
    int x = 12 + gfx::draw_text_px(c, 12, 6, buf, gfx::WHITE, 20);
    gfx::draw_text_px(c, x + 8, 9, world_name(world_), gfx::rgb565(150, 200, 170), 16);

    // jablka i postep
    const int ax = 230;
    if (icon_[APPLE].px) blit_pic(c, icon_[APPLE], ax, 4);
    if (mode_ == Mode::Adventure) {
        snprintf(buf, sizeof(buf), "%d/%d", engine::imin(apples_, need_), need_);
        gfx::draw_text_px(c, ax + 30, 6, buf, gfx::WHITE, 20);
        const int bx = ax + 90, bw = 90;
        fill_round_rect_alpha(c, bx, 12, bw, 9, 4, gfx::BLACK, 140);
        const int fw = engine::imax(0, engine::imin(bw, bw * apples_ / engine::imax(1, need_)));
        if (fw > 0) fill_round_rect_alpha(c, bx, 12, fw, 9, 4, portal_open_ ? gfx::rgb565(190, 120, 255) : gfx::rgb565(120, 220, 90), 255);
    } else {
        snprintf(buf, sizeof(buf), "%d", apples_total_);
        gfx::draw_text_px(c, ax + 30, 6, buf, gfx::WHITE, 20);
    }

    // wynik
    snprintf(buf, sizeof(buf), "%d", score_);
    const int sx = 470 - gfx::text_width_px(buf, 24) / 2;
    text_shadow(c, sx, 3, buf, gfx::rgb565(255, 230, 120), 24);
    if (combo_ >= 2) {
        snprintf(buf, sizeof(buf), "x%d", combo_);
        gfx::draw_text_px(c, sx + gfx::text_width_px("000000", 24) / 2 + 30, 8, buf, gfx::rgb565(255, 150, 60), 16);
    }

    // prawo: zycia
    int rx = W - 10;
    for (int i = 0; i < lives_; ++i) {
        rx -= 26;
        if (icon_[HEART].px) blit_pic(c, icon_[HEART], rx, 4);
        else c.fill_circle(rx + 12, 16, 9, gfx::pal::RED);
    }
    // aktywne moce: ikona + pasek pozostalego czasu
    struct PW { Item it; float t, max; };
    const PW pws[5] = { { HOURGLASS, slow_t_, 8.f }, { STAR, ghost_t_, 6.f }, { MAGNET, magnet_t_, 10.f }, { GEM, x2_t_, 12.f },
                        { ORB, shield_ ? 1.f : 0.f, 1.f } };
    rx -= 10;
    for (const PW& p : pws) {
        if (p.t <= 0) continue;
        rx -= 30;
        if (icon_[p.it].px) blit_pic(c, icon_[p.it], rx, 2);
        const int bw = (int)(24.f * engine::clampf(p.t / p.max, 0.f, 1.f));
        fill_rect_alpha(c, rx, 27, 24, 3, gfx::BLACK, 150);
        fill_rect_alpha(c, rx, 27, bw, 3, gfx::rgb565(255, 230, 120), 255);
    }
    if (autopilot_) {
        rx -= 50;
        gfx::draw_text_px(c, rx, 9, "AUTO", gfx::rgb565(120, 220, 255), 16);
    }
}

void SnakeGame::draw_title(gfx::Canvas& c)
{
    if (title_img_.px && title_img_.w == W && title_img_.h == H) {
        memcpy(c.data(), title_img_.px, (size_t)W * H * 2);
    } else {
        for (int y = 0; y < H; ++y) c.hline(0, y, W, gfx::rgb565((uint8_t)(30 + y / 8), (uint8_t)(90 + y / 6), (uint8_t)(50 + y / 10)));
    }
    // logo
    // uklad: ilustracja ma weza po lewej i portal w prawym dolnym rogu - napisy i menu w prawej polowie, nad portalem
    const int ux = 560;   // os interfejsu
    const char* logo = "SNAKE";
    const int lx = center_x(logo, 48, ux);
    const int ly = 26 + (int)(sinf(anim_ * 2.f) * 3.f);
    text_outline(c, lx, ly, logo, gfx::rgb565(255, 225, 90), gfx::rgb565(20, 60, 10), 48, 3);
    const char* sub = "Przygoda w pieciu swiatach";
    text_shadow(c, center_x(sub, 20, ux), ly + 58, sub, gfx::WHITE, 20);

    // menu wyboru
    const int pw = 420, px = ux - pw / 2, py = 150, ph = 104;
    panel(c, px, py, pw, ph, 200);
    char buf[48];
    for (int row = 0; row < 2; ++row) {
        const int y = py + 12 + row * 44;
        const bool sel = title_row_ == row;
        if (sel) fill_round_rect_alpha(c, px + 10, y - 4, pw - 20, 38, 10, gfx::rgb565(80, 150, 90), 150);
        gfx::draw_text_px(c, px + 26, y + 4, row == 0 ? "Tryb" : "Waz", gfx::rgb565(170, 210, 180), 20);
        if (row == 0) snprintf(buf, sizeof(buf), "%s", mode_ == Mode::Adventure ? "Przygoda (10 poziomow)" : "Bez konca");
        else snprintf(buf, sizeof(buf), "%s", SKIN_NAMES[skin_]);
        const int tx = px + 240 - gfx::text_width_px(buf, 20) / 2;
        gfx::draw_text_px(c, tx, y + 4, buf, gfx::WHITE, 20);
        if (sel) {
            gfx::draw_text_px(c, px + 90, y + 4, "<", gfx::rgb565(255, 225, 90), 20);
            gfx::draw_text_px(c, px + pw - 30, y + 4, ">", gfx::rgb565(255, 225, 90), 20);
        }
        if (row == 1 && head_[skin_][RIGHT].px) blit_pic(c, head_[skin_][RIGHT], tx - 48, y - 6);
    }
    if (mode_ == Mode::Adventure && start_level_ > 0) {
        snprintf(buf, sizeof(buf), "Start od poziomu %d (X - zmiana)", start_level_ + 1);
        text_shadow(c, center_x(buf, 16, ux), py + ph + 50, buf, gfx::rgb565(150, 230, 255), 16);
    }
    if (fmodf(anim_, 1.f) < 0.65f) {
        const char* s = "Wcisnij A, aby grac";
        text_outline(c, center_x(s, 24, ux), py + ph + 14, s, gfx::WHITE, gfx::rgb565(10, 40, 10), 24, 2);
    }
    fill_round_rect_alpha(c, ux - 170, py + ph + 44, 340, 72, 12, gfx::rgb565(10, 30, 16), 120);
    const char* hint1 = "Krzyzak - ruch    A - turbo    Y - autopilot";
    const char* hint2 = "X - poziom startowy    START - pauza";
    text_shadow(c, center_x(hint1, 14, ux), py + ph + 76, hint1, gfx::rgb565(235, 245, 235), 14);
    text_shadow(c, center_x(hint2, 14, ux), py + ph + 94, hint2, gfx::rgb565(235, 245, 235), 14);

    // rekordy
    if (top_[0].score > 0) {
        const int rx = 14, ry = 14, rw = 170, rh = 34 + TOP * 24;
        panel(c, rx, ry, rw, rh, 190);
        gfx::draw_text_px(c, rx + 16, ry + 8, "REKORDY", gfx::rgb565(255, 225, 90), 16);
        for (int i = 0; i < TOP && top_[i].score > 0; ++i) {
            snprintf(buf, sizeof(buf), "%d.", i + 1);
            gfx::draw_text_px(c, rx + 16, ry + 32 + i * 24, buf, gfx::rgb565(170, 210, 180), 16);
            snprintf(buf, sizeof(buf), "%d", top_[i].score);
            gfx::draw_text_px(c, rx + 44, ry + 32 + i * 24, buf, gfx::WHITE, 16);
            snprintf(buf, sizeof(buf), top_[i].mode == 0 ? "poz. %d" : "%d jabl.", top_[i].level);
            gfx::draw_text_px(c, rx + rw - 16 - gfx::text_width_px(buf, 14), ry + 33 + i * 24, buf, gfx::rgb565(150, 180, 160), 14);
        }
    }
}

void SnakeGame::draw_overlay(gfx::Canvas& c)
{
    char buf[64];
    const int cx = W / 2;
    if (state_ == State::Intro) {
        const int pw = 460, ph = 190, px = cx - pw / 2, py = FIELD_Y + 110;
        panel(c, px, py, pw, ph, 205);
        if (mode_ == Mode::Adventure) {
            snprintf(buf, sizeof(buf), "POZIOM %d / 10  -  %s", level_ + 1, world_name(world_));
            gfx::draw_text_px(c, center_x(buf, 16), py + 16, buf, gfx::rgb565(150, 210, 170), 16);
            static const char* const NAMES[10] = { "Pierwsze kroki", "Ogrod", "Oaza", "Kaniony", "Lodowisko", "Zamiec", "Labirynt",
                                                   "Pnacza", "Krater", "Serce wulkanu" };
            text_shadow(c, center_x(NAMES[level_], 40), py + 40, NAMES[level_], gfx::rgb565(255, 225, 90), 40);
            snprintf(buf, sizeof(buf), "Zjedz %d jablek i wejdz do portalu", need_);
            gfx::draw_text_px(c, center_x(buf, 20), py + 96, buf, gfx::WHITE, 20);
            gfx::draw_text_px(c, center_x(wrap_ ? "Krawedzie pola sa otwarte" : "Uwaga: krawedzie to sciana!", 16), py + 124,
                              wrap_ ? "Krawedzie pola sa otwarte" : "Uwaga: krawedzie to sciana!", wrap_ ? gfx::rgb565(150, 220, 255) : gfx::rgb565(255, 140, 110), 16);
        } else {
            text_shadow(c, center_x("BEZ KONCA", 40), py + 30, "BEZ KONCA", gfx::rgb565(255, 225, 90), 40);
            gfx::draw_text_px(c, center_x("Jedz, rosnij, przyspieszaj. Swiat zmienia sie co 20 jablek.", 16), py + 96,
                              "Jedz, rosnij, przyspieszaj. Swiat zmienia sie co 20 jablek.", gfx::WHITE, 16);
        }
        const int n = 3 - (int)(state_t_ / 0.67f);
        snprintf(buf, sizeof(buf), n > 0 ? "%d" : "START!", n);
        text_outline(c, center_x(buf, 28), py + ph - 44, buf, gfx::WHITE, gfx::rgb565(20, 60, 10), 28, 2);
    } else if (state_ == State::LevelClear) {
        const int pw = 440, ph = 230, px = cx - pw / 2, py = FIELD_Y + 90;
        panel(c, px, py, pw, ph, 215);
        text_shadow(c, center_x("POZIOM UKONCZONY!", 32), py + 18, "POZIOM UKONCZONY!", gfx::rgb565(255, 225, 90), 32);
        const int t = (int)level_time_;
        snprintf(buf, sizeof(buf), "Czas  %d:%02d", t / 60, t % 60);
        gfx::draw_text_px(c, center_x(buf, 20), py + 76, buf, gfx::WHITE, 20);
        snprintf(buf, sizeof(buf), "Dlugosc weza  %d", len_);
        gfx::draw_text_px(c, center_x(buf, 20), py + 104, buf, gfx::WHITE, 20);
        snprintf(buf, sizeof(buf), "Premia  +%d", clear_bonus_);
        gfx::draw_text_px(c, center_x(buf, 24), py + 136, buf, gfx::rgb565(120, 230, 130), 24);
        snprintf(buf, sizeof(buf), "Wynik  %d", score_);
        gfx::draw_text_px(c, center_x(buf, 20), py + 170, buf, gfx::rgb565(255, 230, 120), 20);
        if (state_t_ > 1.f && fmodf(anim_, 1.f) < 0.65f)
            gfx::draw_text_px(c, center_x("A - dalej", 16), py + ph - 26, "A - dalej", gfx::rgb565(170, 210, 180), 16);
    } else if (state_ == State::GameOver || state_ == State::Victory) {
        const bool win = state_ == State::Victory;
        const int pw = 460, ph = 220, px = cx - pw / 2, py = FIELD_Y + 100;
        fill_rect_alpha(c, 0, FIELD_Y, W, FIELD_H, gfx::BLACK, 90);
        panel(c, px, py, pw, ph, 225);
        const char* title = win ? "ZWYCIESTWO!" : "KONIEC GRY";
        text_outline(c, center_x(title, 40), py + 18, title, win ? gfx::rgb565(255, 225, 90) : gfx::rgb565(255, 110, 90),
                     gfx::rgb565(30, 10, 5), 40, 2);
        if (win) gfx::draw_text_px(c, center_x("Wszystkie 10 poziomow ukonczone!", 16), py + 72, "Wszystkie 10 poziomow ukonczone!",
                                   gfx::rgb565(170, 210, 180), 16);
        snprintf(buf, sizeof(buf), "Wynik  %d", score_);
        text_shadow(c, center_x(buf, 32), py + 98, buf, gfx::WHITE, 32);
        if (last_rank_ >= 0) {
            snprintf(buf, sizeof(buf), last_rank_ == 0 ? "NOWY REKORD!" : "Miejsce %d w rekordach", last_rank_ + 1);
            gfx::draw_text_px(c, center_x(buf, 20), py + 144, buf, gfx::rgb565(255, 225, 90), 20);
        }
        if (state_t_ > 1.2f && fmodf(anim_, 1.f) < 0.65f)
            gfx::draw_text_px(c, center_x("A - menu", 16), py + ph - 28, "A - menu", gfx::rgb565(170, 210, 180), 16);
    }
}

void SnakeGame::render(gfx::Canvas& c)
{
    if (state_ == State::Title) {
        g_clip = { 0, 0, W, H };
        draw_title(c);
        return;
    }
    draw_field(c);
    g_clip = { 0, FIELD_Y, W, H };
    draw_portal(c);
    draw_pickups(c);
    draw_snake(c);
    draw_effects(c);
    g_clip = { 0, 0, W, H };
    draw_hud(c);
    draw_overlay(c);
}

}  // namespace snake
