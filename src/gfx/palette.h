// Wspolna paleta konsoli: 19 kolorow ze staly znakami do ASCII-artu ('.' = przezroczysty).
// Uzywa jej Lake Mario (mario_assets.cpp), API ucznia (console) i make_sprite() bez podanej palety.
// Jedno miejsce z wartosciami RGB - kolory w grach ucznia i w Mario sa identyczne.
#pragma once

#include "gfx/canvas.h"
#include "gfx/sprite.h"

namespace gfx {

namespace pal {
constexpr uint16_t BLACK       = rgb565(0, 0, 0);          // k
constexpr uint16_t WHITE       = rgb565(255, 255, 255);    // w
constexpr uint16_t GRAY        = rgb565(165, 165, 175);    // e
constexpr uint16_t DARK_GRAY   = rgb565(85, 85, 95);       // E
constexpr uint16_t RED         = rgb565(225, 45, 45);      // r
constexpr uint16_t DARK_RED    = rgb565(145, 20, 20);      // R
constexpr uint16_t ORANGE      = rgb565(240, 140, 40);     // o
constexpr uint16_t YELLOW      = rgb565(250, 215, 50);     // y
constexpr uint16_t DARK_YELLOW = rgb565(200, 150, 20);     // Y
constexpr uint16_t GREEN       = rgb565(70, 190, 70);      // g
constexpr uint16_t DARK_GREEN  = rgb565(30, 120, 40);      // G
constexpr uint16_t BROWN       = rgb565(155, 95, 40);      // b
constexpr uint16_t DARK_BROWN  = rgb565(85, 50, 20);       // B
constexpr uint16_t BEIGE       = rgb565(205, 175, 125);    // t
constexpr uint16_t SKIN        = rgb565(255, 205, 160);    // s
constexpr uint16_t BLUE        = rgb565(50, 100, 220);     // u
constexpr uint16_t DARK_BLUE   = rgb565(30, 55, 140);      // U
constexpr uint16_t PURPLE      = rgb565(170, 80, 200);     // p
constexpr uint16_t DARK_PURPLE = rgb565(100, 40, 130);     // P
constexpr uint16_t SKY         = rgb565(100, 170, 255);    // tlo Lake Mario (bez znaku w palecie)
}  // namespace pal

extern const PaletteEntry DEFAULT_PALETTE[];
extern const int          DEFAULT_PALETTE_N;

// Kolor dla znaku z domyslnej palety; TRANSPARENT dla nieznanego (z ostrzezeniem w logu).
uint16_t default_palette_lookup(char key);

}  // namespace gfx
