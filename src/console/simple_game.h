// Adapter: dwie funkcje ucznia (setup, frame) -> engine::Game, ktore rozumie petla konsoli.
// Dzieki temu lekcje dostaja menu, pauze i testy skryptowane bez zadnej wiedzy o silniku.
#pragma once

#include "engine/game.h"

namespace console {

class SimpleGame final : public engine::Game {
public:
    using Fn = void (*)();

    // id: identyfikator z CONSOLE_ADD_GAME - wyznacza katalog plikow gry (assets/<id>/).
    SimpleGame(Fn setup, Fn frame, const char* id) : setup_(setup), frame_(frame), id_(id) {}

    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

private:
    Fn          setup_;
    Fn          frame_;
    const char* id_;
};

}  // namespace console
