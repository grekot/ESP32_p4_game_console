#include "gfx/canvas.h"

namespace gfx {

namespace {
inline int imax(int a, int b) { return a > b ? a : b; }
inline int imin(int a, int b) { return a < b ? a : b; }
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
