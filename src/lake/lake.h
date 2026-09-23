// Jedyny naglowek, ktory wlacza plik gry ucznia: #include "lake/lake.h"
//
// Daje: funkcje rysowania i wejscia (lake_api.h) bez przedrostka lake::, oraz makro LAKE_GAME,
// ktore rejestruje gre w konsoli. Uklad pliku gry:
//
//   #include "lake/lake.h"
//   namespace {                 // "pudelko" - zmienne i funkcje tej gry nie klóca sie z innymi lekcjami
//   int x = 0;
//   void setup() { x = 100; }   // raz, na starcie
//   void frame()  { clear(BLACK); rect(x, 100, 20, 20, RED); }   // 60 razy na sekunde
//   }  // namespace
//   LAKE_GAME(moja_gra, "Moja gra", "Opis w menu")
//
// Nazwa w LAKE_GAME (tu: moja_gra) musi trafic do src/games/lekcje/lista.h jako LEKCJA(moja_gra).
#pragma once

#include "engine/game_registry.h"
#include "lake/lake_api.h"
#include "lake/simple_game.h"

using namespace lake;   // celowo: uczen pisze rect(...), nie lake::rect(...)

// Rejestracja gry. Tworzy fabryke ze statyczna instancja adaptera i staly wpis engine::GameEntry
// o nazwie lake_entry_<id>, do ktorego odwoluje sie games/registry.cpp (przez lista.h).
// `extern` jest potrzebny, bo stala na poziomie pliku ma domyslnie widocznosc tylko w tym pliku.
#define LAKE_GAME(id, name_, desc_)                                                     \
    static engine::Game* lake_create_##id()                                             \
    {                                                                                   \
        static lake::SimpleGame game(setup, frame);                                     \
        return &game;                                                                   \
    }                                                                                   \
    extern const engine::GameEntry lake_entry_##id;                                     \
    const engine::GameEntry        lake_entry_##id = { #id, name_, desc_, lake_create_##id };
