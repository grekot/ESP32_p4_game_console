#include "gfx/canvas.h"

namespace gfx {

namespace {
inline int imax(int a, int b) { return a > b ? a : b; }
inline int imin(int a, int b) { return a < b ? a : b; }
inline int iabs(int a)        { return a < 0 ? -a : a; }

// Pierwiastek calkowity (najwiekszy s, dla ktorego s*s <= v) - bez float, do wypelniania kol.
int isqrt(int v)
{
    if (v <= 0) return 0;
    int s = 0;
    while ((s + 1) * (s + 1) <= v) ++s;
    return s;
}
}  // namespace

void Canvas::clear(uint16_t color)
{
    const uint32_t v = ((uint32_t)color << 16) | color;
    uint32_t*      p = reinterpret_cast<uint32_t*>(px_);
    const size_t   n = (size_t)w_ * h_ / 2;
    for (size_t i = 0; i < n; ++i) {
        p[i] = v;
    }
    if (((size_t)w_ * h_) & 1) {
        px_[(size_t)w_ * h_ - 1] = color;
    }
}

void Canvas::put(int x, int y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    px_[(size_t)y * w_ + x] = color;
}

void Canvas::fill_rect(int x, int y, int w, int h, uint16_t color)
{
    const int x0 = imax(x, 0), y0 = imax(y, 0);
    const int x1 = imin(x + w, w_), y1 = imin(y + h, h_);
    if (x0 >= x1 || y0 >= y1) return;
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t* row = px_ + (size_t)yy * w_;
        for (int xx = x0; xx < x1; ++xx) {
            row[xx] = color;
        }
    }
}

void Canvas::hline(int x, int y, int w, uint16_t color) { fill_rect(x, y, w, 1, color); }
void Canvas::vline(int x, int y, int h, uint16_t color) { fill_rect(x, y, 1, h, color); }

void Canvas::draw_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    hline(x, y, w, color);
    hline(x, y + h - 1, w, color);
    vline(x, y, h, color);
    vline(x + w - 1, y, h, color);
}

void Canvas::line(int x0, int y0, int x1, int y1, uint16_t color)
{
    // Szybkie sciezki dla odcinkow prostych
    if (y0 == y1) { hline(imin(x0, x1), y0, iabs(x1 - x0) + 1, color); return; }
    if (x0 == x1) { vline(x0, imin(y0, y1), iabs(y1 - y0) + 1, color); return; }

    const int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Canvas::fill_circle(int cx, int cy, int r, uint16_t color)
{
    if (r < 0) return;
    if (r == 0) { put(cx, cy, color); return; }
    for (int dy = -r; dy <= r; ++dy) {
        const int dx = isqrt(r * r - dy * dy);
        hline(cx - dx, cy + dy, 2 * dx + 1, color);
    }
}

void Canvas::draw_circle(int cx, int cy, int r, uint16_t color)
{
    if (r < 0) return;
    if (r == 0) { put(cx, cy, color); return; }
    // Algorytm punktu srodkowego - 8 symetrycznych punktow na krok
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        put(cx + x, cy + y, color); put(cx - x, cy + y, color);
        put(cx + x, cy - y, color); put(cx - x, cy - y, color);
        put(cx + y, cy + x, color); put(cx - y, cy + x, color);
        put(cx + y, cy - x, color); put(cx - y, cy - x, color);
        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
}

void Canvas::blit(const Sprite& s, int x, int y, bool flip_x)
{
    if (!s.px) return;
    const int x0 = imax(x, 0), y0 = imax(y, 0);
    const int x1 = imin(x + s.w, w_), y1 = imin(y + s.h, h_);
    if (x0 >= x1 || y0 >= y1) return;

    for (int yy = y0; yy < y1; ++yy) {
        const uint16_t* srow = s.px + (size_t)(yy - y) * s.w;
        uint16_t*       drow = px_ + (size_t)yy * w_;
        if (!flip_x) {
            for (int xx = x0; xx < x1; ++xx) {
                const uint16_t c = srow[xx - x];
                if (c != TRANSPARENT) drow[xx] = c;
            }
        } else {
            for (int xx = x0; xx < x1; ++xx) {
                const uint16_t c = srow[s.w - 1 - (xx - x)];
                if (c != TRANSPARENT) drow[xx] = c;
            }
        }
    }
}

void Canvas::blit_scaled(const Sprite& s, int x, int y, int scale, bool flip_x)
{
    if (!s.px || scale < 1) return;
    if (scale == 1) { blit(s, x, y, flip_x); return; }
    const int dw = s.w * scale, dh = s.h * scale;
    const int x0 = imax(x, 0), y0 = imax(y, 0);
    const int x1 = imin(x + dw, w_), y1 = imin(y + dh, h_);
    if (x0 >= x1 || y0 >= y1) return;

    for (int yy = y0; yy < y1; ++yy) {
        const uint16_t* srow = s.px + (size_t)((yy - y) / scale) * s.w;
        uint16_t*       drow = px_ + (size_t)yy * w_;
        for (int xx = x0; xx < x1; ++xx) {
            int sx = (xx - x) / scale;
            if (flip_x) sx = s.w - 1 - sx;
            const uint16_t c = srow[sx];
            if (c != TRANSPARENT) drow[xx] = c;
        }
    }
}

void Canvas::blit_upscale2x(const uint16_t* src, int src_w, int src_h)
{
    if (!src || src_w <= 0 || src_h <= 0) return;
    const int rows = imin(src_h, h_ / 2);
    const int cols = imin(src_w, w_ / 2);
    for (int y = 0; y < rows; ++y) {
        const uint16_t* srow = src + (size_t)y * src_w;
        // Dwa docelowe wiersze naraz; para pikseli zapisywana jako jedno slowo 32-bitowe.
        uint32_t* d0 = reinterpret_cast<uint32_t*>(px_ + (size_t)(2 * y) * w_);
        uint32_t* d1 = reinterpret_cast<uint32_t*>(px_ + (size_t)(2 * y + 1) * w_);
        for (int x = 0; x < cols; ++x) {
            const uint32_t c = srow[x];
            const uint32_t cc = c | (c << 16);
            d0[x] = cc;
            d1[x] = cc;
        }
    }
}

void Canvas::blit_tinted(const Sprite& s, int x, int y, uint16_t color, bool flip_x)
{
    if (!s.px) return;
    const int x0 = imax(x, 0), y0 = imax(y, 0);
    const int x1 = imin(x + s.w, w_), y1 = imin(y + s.h, h_);
    if (x0 >= x1 || y0 >= y1) return;

    for (int yy = y0; yy < y1; ++yy) {
        const uint16_t* srow = s.px + (size_t)(yy - y) * s.w;
        uint16_t*       drow = px_ + (size_t)yy * w_;
        for (int xx = x0; xx < x1; ++xx) {
            const int sx = flip_x ? (s.w - 1 - (xx - x)) : (xx - x);
            if (srow[sx] != TRANSPARENT) drow[xx] = color;
        }
    }
}

}  // namespace gfx
