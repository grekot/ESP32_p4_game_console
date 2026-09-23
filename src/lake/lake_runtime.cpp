#include "lake/lake_api.h"
#include "lake/lake_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/rng.h"
#include "gfx/text.h"
#include "platform/platform.h"

namespace lake {

namespace {

const char* TAG = "lekcja";

constexpr size_t ARENA_PIXELS = 32768;   // 64 kB - ok. 128 sprite'ow 16x16

// Litery ASCII-artu -> kolory: wspolna paleta konsoli (gfx/palette.h), ta sama co w Lake Mario.
uint16_t palette_lookup(char ch)
{
    return gfx::default_palette_lookup(ch);
}

inline gfx::Canvas* canvas()
{
    gfx::Canvas* c = detail::rt().canvas;
    if (!c) LAKE_LOGW(TAG, "rysowanie poza frame() nie ma efektu (brak plotna)");
    return c;
}

bool key_now(const input::PadState& p, Key k)
{
    if (k == Key::Start || k == Key::Select) return false;   // naleza do konsoli
    return p.held(k);
}

void add_watch(const char* name, const char* value)
{
    detail::Runtime& r = detail::rt();
    if (r.watch_count >= detail::MAX_WATCH) return;
    detail::WatchEntry& w = r.watches[r.watch_count++];
    snprintf(w.name, sizeof(w.name), "%s", name ? name : "?");
    snprintf(w.value, sizeof(w.value), "%s", value ? value : "");
}

}  // namespace

// ------------------------------------------------------------------ ekran

void clear(Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->clear(c.raw);
}

void pixel(int x, int y, Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->put(x, y, c.raw);
}

void rect(int x, int y, int w, int h, Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->fill_rect(x, y, w, h, c.raw);
}

void rect_outline(int x, int y, int w, int h, Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->draw_rect(x, y, w, h, c.raw);
}

void circle(int cx, int cy, int r, Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->fill_circle(cx, cy, r, c.raw);
}

void circle_outline(int cx, int cy, int r, Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->draw_circle(cx, cy, r, c.raw);
}

void line(int x0, int y0, int x1, int y1, Color c)
{
    if (gfx::Canvas* cv = canvas()) cv->line(x0, y0, x1, y1, c.raw);
}

namespace {
// Rozmiar napisu ucznia (1, 2, 3, 4) -> czcionka w pikselach. Wygladzany Montserrat, jak w menu konsoli.
int text_px(int scale)
{
    static const int PX[] = { 16, 24, 32, 48 };
    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;
    return PX[scale - 1];
}
}  // namespace

void text(int x, int y, const char* s, Color c, int scale)
{
    if (gfx::Canvas* cv = canvas()) gfx::draw_text_px(*cv, x, y, s, c.raw, text_px(scale));
}

void text(int x, int y, int number, Color c, int scale)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", number);
    text(x, y, buf, c, scale);
}

int text_width(const char* s, int scale)
{
    return gfx::text_width_px(s, text_px(scale));
}

int text_height(int scale)
{
    return gfx::text_height_px(text_px(scale));
}

// ------------------------------------------------------------------ sprite'y

namespace detail {

Sprite build_sprite(const char* const* rows, int h)
{
    Sprite s;
    if (!rows || h <= 0) return s;

    int w = 0;
    for (int y = 0; y < h; ++y) {
        const int len = rows[y] ? (int)strlen(rows[y]) : 0;
        if (len > w) w = len;
    }
    if (w == 0) return s;

    Runtime& r = rt();
    if (!r.arena) {
        r.arena     = platform::alloc_pixels(ARENA_PIXELS, /*fast=*/false);
        r.arena_cap = r.arena ? ARENA_PIXELS : 0;
        if (!r.arena) LAKE_LOGE(TAG, "brak pamieci na sprite'y");
    }
    const size_t need = (size_t)w * h;
    if (r.arena_used + need > r.arena_cap) {
        LAKE_LOGE(TAG, "za duzo sprite'ow (%dx%d nie miesci sie w %u px) - load_sprite wolaj w setup(), nie w frame()",
                  w, h, (unsigned)r.arena_cap);
        return s;
    }
    uint16_t* px = r.arena + r.arena_used;
    r.arena_used += need;

    for (int y = 0; y < h; ++y) {
        const char* row = rows[y] ? rows[y] : "";
        const int   len = (int)strlen(row);
        for (int x = 0; x < w; ++x) {
            const char ch = x < len ? row[x] : '.';
            px[(size_t)y * w + x] = (ch == '.' || ch == ' ') ? gfx::TRANSPARENT : palette_lookup(ch);
        }
    }
    s.w  = w;
    s.h  = h;
    s.px = px;
    return s;
}

}  // namespace detail

void sprite(const Sprite& s, int x, int y, bool flip_x, int scale)
{
    if (gfx::Canvas* cv = canvas()) cv->blit_scaled(s, x, y, scale < 1 ? 1 : scale, flip_x);
}

// ------------------------------------------------------------------ wejscie

bool held(Key k)     { return key_now(detail::rt().pad, k); }
bool pressed(Key k)  { const detail::Runtime& r = detail::rt(); return key_now(r.pad, k) && !key_now(r.prev, k); }
bool released(Key k) { const detail::Runtime& r = detail::rt(); return !key_now(r.pad, k) && key_now(r.prev, k); }

// ------------------------------------------------------------------ czas i losowosc

int   frame_count() { return detail::rt().frame; }
float seconds()     { return detail::rt().time; }
float dt()          { return detail::rt().dt; }

int  random(int a, int b)       { return engine::rng().range(a, b); }
bool chance(int percent)        { return engine::rng().range(1, 100) <= percent; }
void random_seed(uint32_t seed) { engine::seed_rng(seed); }

// ------------------------------------------------------------------ kolizje i matematyka

bool overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool overlaps(Rect a, Rect b) { return overlaps(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h); }

int clamp(int v, int lo, int hi)
{
    if (lo > hi) { const int t = lo; lo = hi; hi = t; }
    return v < lo ? lo : (v > hi ? hi : v);
}

int abs(int v)        { return v < 0 ? -v : v; }
int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

// ------------------------------------------------------------------ mapa kafelkow

namespace {

// TileMap chce zwyklej funkcji, wiec lista znakow stalych jest w Runtime, a to jej czyta.
bool solid_by_chars(char tile)
{
    for (const char* p = detail::rt().solid_chars; *p; ++p) {
        if (*p == tile) return true;
    }
    return false;
}

}  // namespace

namespace detail {

void load_map_rows(const char* const* rows, int h, const char* solid_chars)
{
    Runtime& r = rt();
    int w = 0;
    for (int y = 0; y < h; ++y) {
        const int len = rows[y] ? (int)strlen(rows[y]) : 0;
        if (len > w) w = len;
    }
    if (h > engine::TileMap::MAX_ROWS || w > engine::TileMap::MAX_COLS) {
        LAKE_LOGW(TAG, "mapa %dx%d przycieta do %dx%d kafelkow", w, h, engine::TileMap::MAX_COLS, engine::TileMap::MAX_ROWS);
    }
    r.map.reset(w, h, TILE, ' ');
    r.map.set_solid_fn(solid_by_chars);
    snprintf(r.solid_chars, sizeof(r.solid_chars), "%s", solid_chars ? solid_chars : "");
    for (int y = 0; y < r.map.rows(); ++y) {
        const char* row = rows[y] ? rows[y] : "";
        const int   len = (int)strlen(row);
        for (int x = 0; x < r.map.cols(); ++x) {
            r.map.set_tile(x, y, x < len ? row[x] : ' ');
        }
    }
}

}  // namespace detail

int  map_cols()                               { return detail::rt().map.cols(); }
int  map_rows()                               { return detail::rt().map.rows(); }
int  map_width()                              { return detail::rt().map.width_px(); }
char map_tile(int col, int row)               { return detail::rt().map.tile_at(col, row); }
void map_set(int col, int row, char tile)     { detail::rt().map.set_tile(col, row, tile); }
bool map_solid(int col, int row)              { return detail::rt().map.solid_at(col, row); }
int  map_col(int px)                          { return detail::rt().map.col_of((float)px); }
int  map_row(int py)                          { return detail::rt().map.row_of((float)py); }
bool map_solid_at(int px, int py)             { return map_solid(map_col(px), map_row(py)); }

void draw_tiles(char tile, const Sprite& s, int cam_x)
{
    gfx::Canvas* cv = canvas();
    if (!cv) return;
    const engine::TileMap& m = detail::rt().map;
    // Sprite 16x16 na kafelku 32x32 -> powiekszenie x2 (pixel-art zostaje ostry).
    const int scale = (s.w > 0 && TILE % s.w == 0) ? TILE / s.w : 1;
    const int c0 = cam_x / TILE, c1 = c0 + cv->width() / TILE + 1;
    for (int row = 0; row < m.rows(); ++row) {
        for (int col = c0; col <= c1; ++col) {
            if (m.tile_at(col, row) == tile) cv->blit_scaled(s, col * TILE - cam_x, row * TILE, scale);
        }
    }
}

void draw_tiles(char tile, Color c, int cam_x)
{
    gfx::Canvas* cv = canvas();
    if (!cv) return;
    const engine::TileMap& m = detail::rt().map;
    const int c0 = cam_x / TILE, c1 = c0 + cv->width() / TILE + 1;
    for (int row = 0; row < m.rows(); ++row) {
        for (int col = c0; col <= c1; ++col) {
            if (m.tile_at(col, row) == tile) cv->fill_rect(col * TILE - cam_x, row * TILE, TILE, TILE, c.raw);
        }
    }
}

MoveResult move_box(int& x, int& y, int w, int h, int& vx, int& vy)
{
    // Silnik liczy w pikselach na sekunde i float; uczen w pikselach na klatke i int.
    // Przy stalym kroku 1/60 s przeliczenie jest dokladne: vx px/klatke = vx*60 px/s.
    const float dt  = 1.f / 60.f;
    float fx = (float)x, fy = (float)y;
    float fvx = (float)vx * 60.f, fvy = (float)vy * 60.f;

    MoveResult r{};
    const engine::TileMap& m = detail::rt().map;
    m.move_x(fx, fy, fvx, w, h, dt, r.hit_wall);
    r.on_ground = m.move_y(fx, fy, fvy, w, h, dt, r.head_col, r.head_row);
    r.hit_head  = r.head_col >= 0;

    x  = (int)floorf(fx + 0.5f);
    y  = (int)floorf(fy + 0.5f);
    vx = (int)floorf(fvx / 60.f + 0.5f);
    vy = (int)floorf(fvy / 60.f + 0.5f);
    return r;
}

// ------------------------------------------------------------------ sterowanie i diagnostyka

void restart() { detail::rt().restart = true; }

void watch(const char* name, int value)
{
    char buf[detail::WATCH_VALUE];
    snprintf(buf, sizeof(buf), "%d", value);
    add_watch(name, buf);
}

void watch(const char* name, float value)
{
    char buf[detail::WATCH_VALUE];
    snprintf(buf, sizeof(buf), "%.2f", (double)value);
    add_watch(name, buf);
}

void watch(const char* name, double value)      { watch(name, (float)value); }
void watch(const char* name, bool value)        { add_watch(name, value ? "true" : "false"); }
void watch(const char* name, const char* value) { add_watch(name, value); }

void print(const char* s)                          { LAKE_LOGI(TAG, "%s", s ? s : ""); }
void print(const char* label, int value)           { LAKE_LOGI(TAG, "%s %d", label ? label : "", value); }
void print(const char* label, float value)         { LAKE_LOGI(TAG, "%s %.2f", label ? label : "", (double)value); }
void print(const char* label, const char* value)   { LAKE_LOGI(TAG, "%s %s", label ? label : "", value ? value : ""); }

// ------------------------------------------------------------------ srodowisko (dla SimpleGame)

namespace detail {

Runtime& rt()
{
    static Runtime r;
    return r;
}

void reset(gfx::Canvas& c)
{
    Runtime& r     = rt();
    r.canvas       = &c;
    r.pad          = input::PadState{};
    r.prev         = input::PadState{};
    r.dt           = 1.f / 60.f;
    r.time         = 0.f;
    r.frame        = 0;
    r.restart      = false;
    r.watch_count  = 0;
    r.arena_used   = 0;
    r.map.reset(0, 0, TILE, ' ');
    r.solid_chars[0] = '\0';
}

void begin_update(float dt, const input::PadState& pad)
{
    Runtime& r = rt();
    r.prev = r.pad;
    r.pad  = pad;
    r.dt   = dt;
    r.time += dt;
}

void begin_render(gfx::Canvas& c)
{
    Runtime& r    = rt();
    r.canvas      = &c;
    r.watch_count = 0;
}

void end_render()
{
    Runtime& r = rt();
    if (r.watch_count > 0 && r.canvas) {
        // Panel w prawym gornym rogu: "nazwa  wartosc" w kazdej linii, czcionka 16 px, ciemne tlo.
        constexpr int PX = 16;
        const int lh = gfx::text_height_px(PX);
        int maxw = 0;
        char line[WATCH_NAME + WATCH_VALUE + 3];
        for (int i = 0; i < r.watch_count; ++i) {
            snprintf(line, sizeof(line), "%s  %s", r.watches[i].name, r.watches[i].value);
            const int w = gfx::text_width_px(line, PX);
            if (w > maxw) maxw = w;
        }
        const int pad_px = 8;
        const int box_w  = maxw + 2 * pad_px;
        const int box_h  = r.watch_count * lh + 2 * pad_px;
        const int x0     = r.canvas->width() - box_w - 8;
        const int y0     = 8;
        r.canvas->fill_rect(x0, y0, box_w, box_h, gfx::rgb565(15, 23, 42));
        r.canvas->draw_rect(x0, y0, box_w, box_h, gfx::rgb565(51, 65, 85));
        for (int i = 0; i < r.watch_count; ++i) {
            snprintf(line, sizeof(line), "%s  %s", r.watches[i].name, r.watches[i].value);
            gfx::draw_text_px(*r.canvas, x0 + pad_px, y0 + pad_px + i * lh, line, gfx::rgb565(134, 239, 172), PX);
        }
    }
    ++r.frame;
}

void format_debug_line(char* buf, size_t n)
{
    if (!buf || n == 0) return;
    const Runtime& r = rt();
    // r.frame juz zwiekszone w end_render - pokazujemy numer klatki, ktory widzial frame()
    int len = snprintf(buf, n, "F=%d t=%.2f", r.frame > 0 ? r.frame - 1 : 0, (double)r.time);
    for (int i = 0; i < r.watch_count && len >= 0 && (size_t)len < n; ++i) {
        len += snprintf(buf + len, n - (size_t)len, " %s=%s", r.watches[i].name, r.watches[i].value);
    }
}

}  // namespace detail

}  // namespace lake
