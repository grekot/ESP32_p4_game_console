#include "input/pad.h"

namespace input {

bool PadState::held(Key k) const
{
    switch (k) {
        case Key::Up:     return up;
        case Key::Down:   return down;
        case Key::Left:   return left;
        case Key::Right:  return right;
        case Key::A:      return a;
        case Key::B:      return b;
        case Key::X:      return x;
        case Key::Y:      return y;
        case Key::Start:  return start;
        case Key::Select: return select;
        // Kierunki galki nie maja wlasnych pol - ich wplyw widac w up/down/left/right i w osiach.
        default:          return false;
    }
}

bool PadState::pressed(Key k) const
{
    switch (k) {
        case Key::A:      return a_pressed;
        case Key::B:      return b_pressed;
        case Key::X:      return x_pressed;
        case Key::Y:      return y_pressed;
        case Key::Start:  return start_pressed;
        case Key::Select: return select_pressed;
        default:          return false;
    }
}

bool PadState::any_held() const
{
    return up || down || left || right || a || b || x || y || start || select;
}

PadState pad_from_mask(uint16_t m)
{
    PadState p;
    p.up     = (m & key_bit(Key::Up))     != 0;
    p.down   = (m & key_bit(Key::Down))   != 0;
    p.left   = (m & key_bit(Key::Left))   != 0;
    p.right  = (m & key_bit(Key::Right))  != 0;
    p.a      = (m & key_bit(Key::A))      != 0;
    p.b      = (m & key_bit(Key::B))      != 0;
    p.x      = (m & key_bit(Key::X))      != 0;
    p.y      = (m & key_bit(Key::Y))      != 0;
    p.start  = (m & key_bit(Key::Start))  != 0;
    p.select = (m & key_bit(Key::Select)) != 0;
    return p;
}

}  // namespace input
