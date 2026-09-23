#include "engine/game_registry.h"

#include "games/mario/mario_game.h"

namespace engine {

namespace {

Game* create_mario()
{
    static MarioGame game;
    return &game;
}

}  // namespace

const GameEntry GAMES[] = {
    { "Lake Mario", "Platformowka 2D", create_mario },
};

const int GAME_COUNT = (int)(sizeof(GAMES) / sizeof(GAMES[0]));

}  // namespace engine
