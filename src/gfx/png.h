// Wczytywanie obrazkow PNG z zasobow (platform::read_file) do sprite'ow RGB565.
// Dekoder: lodepng (src/gfx/lodepng, licencja zlib), kompilowany bez enkodera i bez dostepu do dysku.
// Przezroczystosc: piksel z alfa < 128 staje sie TRANSPARENT (kolor-klucz). Krycie czesciowe jest tracone
// (RGB565 nie ma kanalu alfa) - krawedzie rysuj w PNG "twardo", bez wygladzania.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "gfx/canvas.h"

namespace gfx {

// Wczytuje PNG spod sciezki zasobow (np. "bohater/hero.png"). dst: bufor na piksele o pojemnosci
// dst_capacity pikseli; nullptr = przydziel `new` (zyje do konca programu, jak assety Mario).
// Zwraca pusty Sprite (px == nullptr) przy bledzie - powod w logu.
Sprite load_png(const char* path, uint16_t* dst = nullptr, size_t dst_capacity = 0);

// Sam odczyt rozmiaru (bez dekodowania pikseli). false, gdy pliku nie ma albo nie jest PNG.
bool png_size(const char* path, int& w, int& h);

}  // namespace gfx
