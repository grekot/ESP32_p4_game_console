// Lista gier dostepnych w konsoli. Nowa gra = jeden wpis w src/games/registry.cpp.
#pragma once

#include "engine/game.h"

namespace engine {

struct GameEntry {
    const char* name;
    const char* description;
    Game*       (*create)();   // zwraca instancje o czasie zycia programu
};

extern const GameEntry GAMES[];
extern const int       GAME_COUNT;

}  // namespace engine
