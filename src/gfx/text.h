// Wygladzany (antyaliasowany) tekst na plotnie gry czcionka wektorowa Montserrat z LVGL.
//
// To NIE jest czcionka 5x7 z gfx/font.h (ta zostaje dla gier pixel-art). Tu glify pochodza
// z bitmap A8 generowanych przez LVGL i sa mieszane z tlem wg krycia - litery wygladaja jak w menu.
// Dostepne rozmiary (px): 12, 14, 16, 20, 24, 28, 32, 40, 48 (patrz LV_FONT_MONTSERRAT_* w lv_conf.h);
// inny rozmiar jest zaokraglany do najblizszego. Znaki: ASCII (Montserrat wbudowany w LVGL nie ma
// polskich liter - sa rysowane jako '?'). Tekst UTF-8.
#pragma once

#include <stdint.h>

#include "gfx/canvas.h"

namespace gfx {

// Rysuje jedna linie tekstu; (x, y) to lewy gorny rog pola linii (wysokosc = text_height_px).
// Zwraca szerokosc narysowanego tekstu w pikselach.
int draw_text_px(Canvas& c, int x, int y, const char* utf8, uint16_t color, int px);

// Szerokosc tekstu w pikselach dla danego rozmiaru czcionki.
int text_width_px(const char* utf8, int px);

// Wysokosc linii dla danego rozmiaru czcionki.
int text_height_px(int px);

// Faktyczny rozmiar czcionki, ktory zostanie uzyty dla px (najblizszy dostepny).
int text_font_px(int px);

}  // namespace gfx
