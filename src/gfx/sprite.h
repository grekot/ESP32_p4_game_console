// Budowanie sprite'ow z ASCII-artu + palety. Pozwala trzymac grafike w kodzie w czytelnej
// formie; docelowo assety beda ladowane z partycji `assets` (PNG -> RGB565).
#pragma once

#include "gfx/canvas.h"

namespace gfx {

struct PaletteEntry {
    char     key;
    uint16_t color;
};

// rows: h wierszy po w znakow ('.' = przezroczysty). Krotsze wiersze sa dopelniane przezroczystoscia.
// Zwrocony bufor pikseli zyje do konca programu (assety).
Sprite make_sprite(const char* const* rows, int w, int h, const PaletteEntry* palette, int palette_size);

}  // namespace gfx
