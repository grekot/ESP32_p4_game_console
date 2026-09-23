// Rozmiar plotna konsoli - wspolny dla plytki i emulatora.
//
// Plotno 800x480 to pelna, natywna rozdzielczosc logicznego ekranu (panel 480x800 obrocony
// o CONSOLE_DISPLAY_ROTATION; w emulatorze okno 1:1). Menu, pauza i gry ucznia rysuja w niej wprost -
// czcionki sa gladkie, bez schodkow po skalowaniu. Gra, ktora chce wygladu pixel-art (Lake Mario,
// sprite'y 16x16), deklaruje engine::Game::canvas_scale() == 2: rysuje na 400x240, a konsola
// powieksza klatke najblizszym sasiadem.
//
// Historycznie (do 23.09.2026) cale plotno mialo 400x240 i bylo skalowane x2 sprzetowo (PPA);
// stad w kodzie miejsca, gdzie rozmiar liczy sie z CANVAS_W/H zamiast stalych.
#pragma once

namespace engine {

constexpr int CANVAS_W = 800;
constexpr int CANVAS_H = 480;
constexpr int CANVAS_SCALE = 1;   // plotno -> logiczny ekran (PPA robi tylko obrot)

constexpr int SCREEN_W = CANVAS_W * CANVAS_SCALE;   // 800
constexpr int SCREEN_H = CANVAS_H * CANVAS_SCALE;   // 480

// Plotno gier pixel-art (canvas_scale() == 2)
constexpr int PIXEL_CANVAS_W = CANVAS_W / 2;   // 400
constexpr int PIXEL_CANVAS_H = CANVAS_H / 2;   // 240

}  // namespace engine
