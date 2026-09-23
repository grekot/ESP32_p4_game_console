// Wbudowana czcionka bitmapowa 5x7 (wielkie litery, cyfry, podstawowa interpunkcja).
// Male litery sa rysowane jak wielkie. Znak zajmuje 6*scale px w poziomie, 8*scale w pionie.
#pragma once

#include "gfx/canvas.h"

namespace gfx {

constexpr int FONT_W = 5;
constexpr int FONT_H = 7;
constexpr int FONT_ADVANCE = 6;

void draw_text(Canvas& c, int x, int y, const char* text, uint16_t color, int scale = 1);

// Tekst z 1-pikselowym cieniem (czytelny na kazdym tle)
void draw_text_shadow(Canvas& c, int x, int y, const char* text, uint16_t color, uint16_t shadow, int scale = 1);

int text_width(const char* text, int scale = 1);

}  // namespace gfx
