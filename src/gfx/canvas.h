// Prosty software'owy renderer 2D na buforze RGB565 (little-endian, R w najstarszych bitach).
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace gfx {

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Kolor-klucz przezroczystosci w sprite'ach (czysta magenta)
constexpr uint16_t TRANSPARENT = rgb565(255, 0, 255);

constexpr uint16_t BLACK = rgb565(0, 0, 0);
constexpr uint16_t WHITE = rgb565(255, 255, 255);

struct Sprite {
    int             w  = 0;
    int             h  = 0;
    const uint16_t* px = nullptr;   // w*h pikseli, TRANSPARENT = przezroczysty
};

class Canvas {
public:
    Canvas(uint16_t* buf, int w, int h) : px_(buf), w_(w), h_(h) {}

    int       width() const  { return w_; }
    int       height() const { return h_; }
    uint16_t* data()         { return px_; }

    void clear(uint16_t color);
    void put(int x, int y, uint16_t color);
    void fill_rect(int x, int y, int w, int h, uint16_t color);
    void draw_rect(int x, int y, int w, int h, uint16_t color);   // obrys 1 px
    void hline(int x, int y, int w, uint16_t color);
    void vline(int x, int y, int h, uint16_t color);

    // Odcinek 1 px (Bresenham), konce wlacznie.
    void line(int x0, int y0, int x1, int y1, uint16_t color);

    // Kolo wypelnione / okrag 1 px o srodku (cx, cy) i promieniu r.
    void fill_circle(int cx, int cy, int r, uint16_t color);
    void draw_circle(int cx, int cy, int r, uint16_t color);

    // Rysuje sprite z przezroczystoscia; flip_x = odbicie lustrzane w poziomie.
    void blit(const Sprite& s, int x, int y, bool flip_x = false);

    // Rysuje sprite, zastepujac wszystkie nieprzezroczyste piksele jednym kolorem (np. cien, migotanie).
    void blit_tinted(const Sprite& s, int x, int y, uint16_t color, bool flip_x = false);

    // Rysuje sprite powiekszony `scale` razy (najblizszy sasiad, pixel-art na plotnie 800x480).
    void blit_scaled(const Sprite& s, int x, int y, int scale, bool flip_x = false);

    // Kopiuje mniejszy bufor src_w x src_h powiekszony x2 najblizszym sasiadem od (0,0) - klatka gry
    // pixel-art (400x240) na plotno 800x480. Przycina do rozmiaru plotna.
    void blit_upscale2x(const uint16_t* src, int src_w, int src_h);

private:
    uint16_t* px_;
    int       w_;
    int       h_;
};

}  // namespace gfx
