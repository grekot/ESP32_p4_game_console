// Emulator: opcjonalny pad USB podlaczony do PC - galka, przyciski i krzyzak.
#pragma once

#include <stdint.h>

namespace sim {

// Stan pada: lewa galka (-1..1), przyciski (bit n = przycisk n+1 wg Windows), krzyzak jako POV
// w setnych stopnia (0 = gora, 9000 = prawo; -1 = puszczony).
struct GamepadState {
    float    x = 0.f, y = 0.f;
    uint32_t buttons = 0;
    int      pov = -1;
};

// false = brak pada, wtedy osie ida z klawiszy.
bool gamepad_read(GamepadState& s);

bool gamepad_present();

}  // namespace sim
