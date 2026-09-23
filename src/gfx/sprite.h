// Budowanie sprite'ow z ASCII-artu + palety. Pozwala trzymac grafike w kodzie w czytelnej
// formie; docelowo assety beda ladowane z partycji `assets` (PNG -> RGB565).
#pragma once

#include <string.h>

#include "gfx/canvas.h"

namespace gfx {

struct PaletteEntry {
    char     key;
    uint16_t color;
};

// rows: h wierszy po w znakow ('.' i ' ' = przezroczysty). Krotsze wiersze sa dopelniane przezroczystoscia.
// Zwrocony bufor pikseli zyje do konca programu (assety). Wolac raz, przy ladowaniu.
Sprite make_sprite(const char* const* rows, int w, int h, const PaletteEntry* palette, int palette_size);

// To samo z domyslna paleta konsoli (gfx/palette.h).
Sprite make_sprite(const char* const* rows, int w, int h);

// Rozmiar z tablicy: h = liczba wierszy, w = najdluzszy wiersz. Domyslna paleta.
template <int H>
Sprite make_sprite(const char* const (&rows)[H])
{
    int w = 0;
    for (int y = 0; y < H; ++y) {
        const int len = rows[y] ? (int)strlen(rows[y]) : 0;
        if (len > w) w = len;
    }
    return make_sprite(rows, w, H);
}

}  // namespace gfx
