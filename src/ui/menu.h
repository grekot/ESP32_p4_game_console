// Menu konsoli (LVGL): karuzela gier, lekcje, ustawienia, informacje. Rysowane na pelnym plotnie.
#pragma once

#include "input/pad.h"

namespace ui::menu {

void show();
void hide();

// Indeks gry wybranej przez uzytkownika albo -1. Odczyt kasuje wybor.
int take_selection();

// Klawisze konsoli (stan trzymany, zbocza liczy menu) + animacje. Wolac co klatke przed ui::tick().
void update(const input::PadState& pad);

// Linia stanu do --trace: zakladka, zaznaczenia.
void debug_line(char* buf, int n);

}  // namespace ui::menu
