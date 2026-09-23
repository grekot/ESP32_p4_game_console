// Interfejs gry. Kazda gra w konsoli implementuje te metody;
// petla konsoli (app/app.cpp) wola je co klatke.
#pragma once

#include <stddef.h>

#include "gfx/canvas.h"
#include "input/pad.h"

namespace engine {

class Game {
public:
    virtual ~Game() = default;

    // Jednorazowo, przed pierwsza klatka (ladowanie assetow, stan poczatkowy).
    virtual void init(gfx::Canvas& canvas) = 0;

    // Logika; dt w sekundach (ograniczone do 1/30..1/240 s).
    virtual void update(float dt, const input::PadState& pad) = 0;

    // Rysowanie calej klatki na plotnie (plotno NIE jest czyszczone automatycznie).
    // Rozmiar plotna: CANVAS_W x CANVAS_H (800x480), albo PIXEL_CANVAS_W x PIXEL_CANVAS_H (400x240)
    // gdy canvas_scale() zwraca 2 - wtedy konsola powieksza klatke x2 (pixel-art).
    virtual void render(gfx::Canvas& canvas) = 0;

    // 1 = gra rysuje w pelnej rozdzielczosci (domyslnie), 2 = pixel-art na polowie rozdzielczosci.
    virtual int canvas_scale() const { return 1; }

    // Jedna linia stanu do logow i testow skryptowych (emulator: --trace N). Domyslnie pusta.
    virtual void debug_line(char* buf, size_t n) const
    {
        if (n) buf[0] = 0;
    }
};

}  // namespace engine
