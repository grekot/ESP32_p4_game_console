// Stan "pada" widziany przez gre - niezalezny od zrodla (klawiatura mechaniczna, dotyk, klawiatura PC).
#pragma once

#include <stdint.h>

#include "input/keys.h"

namespace input {

// Punkt dotyku we wspolrzednych plotna gry (engine::CANVAS_W x CANVAS_H).
struct TouchPoint {
    int16_t x;
    int16_t y;
};

constexpr int MAX_TOUCH_POINTS = 5;

struct PadState {
    // stany ciagle (trzymane)
    bool up     = false;
    bool down   = false;
    bool left   = false;
    bool right  = false;
    bool a      = false;   // skok
    bool b      = false;   // bieg / akcja
    bool x      = false;   // wolne dla gier
    bool y      = false;   // wolne dla gier
    bool start  = false;   // pauza / menu
    bool select = false;

    // zbocza (true tylko w klatce wcisniecia)
    bool a_pressed      = false;
    bool b_pressed      = false;
    bool x_pressed      = false;
    bool y_pressed      = false;
    bool start_pressed  = false;
    bool select_pressed = false;
    bool any_pressed    = false;   // dowolny klawisz albo dotkniecie ekranu (np. "TAP TO START")

    // Galka analogowa, -1..1 (0 = srodek albo brak galki). Kierunki z galki sa juz wlaczone
    // w pola up/down/left/right powyzej, wiec gry cyfrowe nie musza o niej wiedziec.
    // Te wartosci sa dla gier, ktore chca plynnego ruchu.
    float stick_x = 0.f;
    float stick_y = 0.f;

    bool held(Key k) const;
    // Zbocze wcisniecia dla A/B/X/Y/Start/Select (pola *_pressed). Kierunki nie maja zbocza - false.
    bool pressed(Key k) const;
    bool any_held() const;
};

// Buduje stan z bitmaski klawiszy (bity wg input::Key). Zbocza wylicza VirtualPad.
PadState pad_from_mask(uint16_t held_mask);

}  // namespace input
