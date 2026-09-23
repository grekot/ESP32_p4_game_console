#include "console/simple_game.h"

#include "console/console_internal.h"

namespace console {

void SimpleGame::init(gfx::Canvas& canvas)
{
    detail::reset(canvas, id_);
    if (setup_) setup_();
}

void SimpleGame::update(float dt, const input::PadState& pad)
{
    detail::begin_update(dt, pad);
}

void SimpleGame::render(gfx::Canvas& canvas)
{
    detail::begin_render(canvas);
    if (frame_) frame_();
    detail::end_render();

    if (detail::rt().restart) {
        // Uczen zawolal restart(): od nastepnej klatki gra startuje od setup().
        init(canvas);
    }
}

void SimpleGame::debug_line(char* buf, size_t n) const
{
    detail::format_debug_line(buf, n);
}

}  // namespace console
