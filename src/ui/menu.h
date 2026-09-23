// Menu konsoli (LVGL): wybor gry. Rysowane na pelnym plotnie, bez nakladki.
#pragma once

namespace ui::menu {

void show();
void hide();

// Indeks gry wybranej przez uzytkownika albo -1. Odczyt kasuje wybor.
int take_selection();

}  // namespace ui::menu
