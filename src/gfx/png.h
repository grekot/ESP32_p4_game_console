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

// Obraz z kanalem alfa: kolor RGB565 + krycie 0..255 na piksel (bez koloru-klucza). Do sprite'ow z wygladzonymi
// krawedziami, rysowanych z mieszaniem i filtrowaniem (np. gra Kart). Bufor w PSRAM (platform::alloc_pixels),
// zyje do konca programu. Pusty (px == nullptr) przy bledzie.
struct Image {
    int            w     = 0;
    int            h     = 0;
    const uint16_t* px   = nullptr;
    const uint8_t*  alpha = nullptr;
};
Image load_png_rgba(const char* path);

// Obraz indeksowany (PNG z paleta, 8 bit): indeksy pikseli w PSRAM + paleta 256 kolorow RGB565 - atlas tekstur
// dla gfx3d (indeks 0 = przezroczysty przy alpha_test). Pusty (idx == nullptr) przy bledzie albo gdy PNG nie ma palety.
struct IndexedImage {
    int            w = 0, h = 0;
    const uint8_t* idx = nullptr;
    uint16_t       palette[256] = {};
};
IndexedImage load_png_indexed(const char* path);

// Sam odczyt rozmiaru (bez dekodowania pikseli). false, gdy pliku nie ma albo nie jest PNG.
bool png_size(const char* path, int& w, int& h);

}  // namespace gfx
