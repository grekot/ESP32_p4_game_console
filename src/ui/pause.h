// Ekran pauzy (LVGL) rysowany NA WIERZCHU zatrzymanej klatki gry - demonstruje tryb nakladki.
#pragma once

namespace ui::pause {

enum class Result { None, Resume, Exit };

void show(const char* game_name);
void hide();

// Wybor uzytkownika albo None. Odczyt kasuje wybor.
Result take_result();

}  // namespace ui::pause
