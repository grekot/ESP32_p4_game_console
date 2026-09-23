// Stan srodowiska uruchomieniowego Lake - wspolny dla funkcji API (lake_runtime.cpp)
// i adaptera SimpleGame. Nie dla ucznia.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "engine/tilemap.h"
#include "gfx/canvas.h"
#include "input/pad.h"

namespace lake::detail {

constexpr int MAX_WATCH   = 8;
constexpr int WATCH_NAME  = 12;
constexpr int WATCH_VALUE = 16;

struct WatchEntry {
    char name[WATCH_NAME];
    char value[WATCH_VALUE];
};

struct Runtime {
    gfx::Canvas*    canvas = nullptr;
    input::PadState pad{};
    input::PadState prev{};
    float           dt          = 1.f / 60.f;
    float           time        = 0.f;
    int             frame       = 0;      // numer klatki widziany przez frame() ucznia
    bool            restart     = false;

    WatchEntry watches[MAX_WATCH]{};
    int        watch_count = 0;

    // Arena pikseli na sprite'y ucznia - zerowana przy kazdym setup(), zeby nic nie wyciekalo.
    uint16_t* arena      = nullptr;
    size_t    arena_used = 0;
    size_t    arena_cap  = 0;

    // Mapa kafelkow (API poziom 2, lekcja 12) i znaki uznawane za stale.
    engine::TileMap map;
    char            solid_chars[32] = {};
};

Runtime& rt();

// Wolane przez SimpleGame.
void reset(gfx::Canvas& c);                                 // przed setup()
void begin_update(float dt, const input::PadState& pad);    // co klatke, przed frame()
void begin_render(gfx::Canvas& c);                          // ustawia plotno, czysci watch
void end_render();                                          // panel watch, licznik klatek
void format_debug_line(char* buf, size_t n);                // "F=12 t=0.20 x=100 ..."

}  // namespace lake::detail
