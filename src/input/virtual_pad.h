// Skladanie stanu pada z dwoch zrodel: klawiatury mechanicznej i ekranu dotykowego.
//
// Wirtualny pad dotykowy (strefy na dole ekranu) jest zapasem na wypadek, gdy klawiatura nie jest
// jeszcze zbudowana albo podlaczona. Po pierwszym uzyciu klawiszy jego podpowiedzi sa chowane.
//
// Strefy dotykowe (wspolrzedne plotna 400x240):
//   dol-lewo:  [<] [>]        dol-prawo:  [B] [A]
#pragma once

#include "gfx/canvas.h"
#include "input/pad.h"

namespace input {

class VirtualPad {
public:
    void begin_frame();
    // Punkty dotyku we wspolrzednych plotna (engine::CANVAS_W x CANVAS_H).
    void feed_touch(const TouchPoint* pts, int n);
    // Stan klawiatury konsoli (platform::keypad()).
    void feed_keys(const PadState& keys);
    void end_frame();

    const PadState& state() const { return out_; }

    // true, gdy od startu uzyto choc raz klawiatury - warto wtedy schowac podpowiedzi dotykowe.
    bool keys_used() const { return keys_used_; }

    // Rysuje obrysy stref dotykowych na plotnie gry.
    void draw(gfx::Canvas& c) const;

private:
    PadState cur_{};
    PadState prev_{};
    PadState out_{};
    bool     touching_      = false;
    bool     touching_prev_ = false;
    bool     keys_used_     = false;
};

}  // namespace input
