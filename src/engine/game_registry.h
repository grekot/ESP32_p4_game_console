// Lista gier dostepnych w konsoli. Nowa gra = jeden wpis w src/games/registry.cpp
// (lekcje: jedna linia w src/games/lekcje/lista.h).
#pragma once

#include "engine/game.h"

namespace engine {

struct GameEntry {
    const char* id;            // krotki identyfikator bez spacji, np. "mario", "pilka" (--game pilka)
    const char* name;          // nazwa w menu
    const char* description;
    Game*       (*create)();   // zwraca instancje o czasie zycia programu
};

extern const GameEntry GAMES[];
extern const int       GAME_COUNT;

// Szuka gry po tym, co podal uzytkownik: same cyfry = indeks w GAMES; inaczej id albo nazwa
// (bez rozrozniania wielkosci liter). Sciezka katalogu ("src/games/lekcje/02_pilka") jest
// skracana do ostatniego segmentu bez numeru ("pilka"). Zwraca indeks albo -1.
int find_game(const char* what);

}  // namespace engine
