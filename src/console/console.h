// Jedyny naglowek, ktory wlacza plik gry ucznia: #include "console/console.h"
//
// Daje: funkcje rysowania i wejscia (console_api.h) bez przedrostka console::, oraz makro CONSOLE_ADD_GAME,
// ktore rejestruje gre w konsoli. Uklad pliku gry:
//
//   #include "console/console.h"
//   namespace {                 // "pudelko" - zmienne i funkcje tej gry nie klóca sie z innymi lekcjami
//   int x = 0;
//   void setup() { x = 100; }   // raz, na starcie
//   void frame()  { clear(BLACK); rect(x, 100, 20, 20, RED); }   // 60 razy na sekunde
//   }  // namespace
//   CONSOLE_ADD_GAME(moja_gra, "Moja gra", "Opis w menu")
//
// Nazwa w CONSOLE_ADD_GAME (tu: moja_gra) musi trafic do src/games/lekcje/lista.h jako LEKCJA(moja_gra).
#pragma once

#include "engine/game_registry.h"
#include "console/console_api.h"
#include "console/simple_game.h"

using namespace console;   // celowo: uczen pisze rect(...), nie console::rect(...)

// Rejestracja gry. Tworzy fabryke ze statyczna instancja adaptera i staly wpis engine::GameEntry
// o nazwie console_entry_<id>, do ktorego odwoluje sie games/registry.cpp (przez lista.h).
// `extern` jest potrzebny, bo stala na poziomie pliku ma domyslnie widocznosc tylko w tym pliku.
#define CONSOLE_ADD_GAME(id, name_, desc_)                                                     \
    static engine::Game* console_create_##id()                                             \
    {                                                                                   \
        static console::SimpleGame game(setup, frame, #id);                                \
        return &game;                                                                   \
    }                                                                                   \
    extern const engine::GameEntry console_entry_##id;                                     \
    const engine::GameEntry        console_entry_##id = { #id, name_, desc_, console_create_##id, nullptr, true };
