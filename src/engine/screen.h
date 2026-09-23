// Rozmiar plotna gry - wspolny dla plytki i emulatora.
//
// Plotno 400x240 jest skalowane sprzetowo x2 do logicznego ekranu 800x480 (na plytce panel
// 480x800 obrocony o LAKE_DISPLAY_ROTATION; w emulatorze okno). Cztery razy mniej pikseli do
// narysowania przez CPU i klasyczny wyglad pixel-art.
#pragma once

namespace engine {

constexpr int CANVAS_W = 400;
constexpr int CANVAS_H = 240;
constexpr int CANVAS_SCALE = 2;   // plotno -> logiczny ekran

constexpr int SCREEN_W = CANVAS_W * CANVAS_SCALE;   // 800
constexpr int SCREEN_H = CANVAS_H * CANVAS_SCALE;   // 480

}  // namespace engine
