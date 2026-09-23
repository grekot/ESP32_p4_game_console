// Rejestr gier konsoli. Definiuje engine::GAMES (zadeklarowane w engine/game_registry.h),
// dlatego jest w przestrzeni engine, choc lezy w games/.
//
// Lekcje (src/games/lekcje/) trafiaja tu automatycznie z listy lista.h: kazdy wpis LEKCJA(id)
// odpowiada makru LAKE_GAME(id, ...) na koncu pliku lekcji. Jawna lista zamiast "samorejestracji"
// przez statyczne inicjalizatory, bo ESP-IDF linkuje komponent jako biblioteke statyczna
// i plik, do ktorego nikt sie nie odwoluje, wypadlby z programu razem ze swoja gra.
#include "engine/game_registry.h"

#include <stdlib.h>
#include <string.h>

#include "games/mario/mario_game.h"

// Deklaracje wpisow lekcji (definicje sa w plikach lekcji, przez LAKE_GAME).
#define LEKCJA(id) extern const engine::GameEntry lake_entry_##id;
#include "games/lekcje/lista.h"
#undef LEKCJA

namespace engine {

namespace {

Game* create_mario()
{
    static mario::MarioGame game;
    return &game;
}

char upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }

bool equal_ignore_case(const char* a, const char* b)
{
    if (!a || !b) return false;
    while (*a && *b) {
        if (upper(*a) != upper(*b)) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

}  // namespace

const GameEntry GAMES[] = {
    { "mario", "Lake Mario", "Platformowka 2D", create_mario },
#define LEKCJA(id) lake_entry_##id,
#include "games/lekcje/lista.h"
#undef LEKCJA
};

const int GAME_COUNT = (int)(sizeof(GAMES) / sizeof(GAMES[0]));

int find_game(const char* what)
{
    if (!what || !*what) return -1;

    // same cyfry -> indeks
    bool digits = true;
    for (const char* p = what; *p; ++p) {
        if (*p < '0' || *p > '9') { digits = false; break; }
    }
    if (digits) {
        const int idx = atoi(what);
        return idx < GAME_COUNT ? idx : -1;
    }

    // sciezka -> ostatni segment (bez koncowych separatorow)
    char seg[64];
    size_t len = strlen(what);
    while (len > 0 && (what[len - 1] == '/' || what[len - 1] == '\\')) --len;
    size_t start = len;
    while (start > 0 && what[start - 1] != '/' && what[start - 1] != '\\') --start;
    size_t n = len - start;
    if (n >= sizeof(seg)) n = sizeof(seg) - 1;
    memcpy(seg, what + start, n);
    seg[n] = '\0';

    // "02_pilka" -> "pilka"
    const char* stripped = seg;
    while (*stripped >= '0' && *stripped <= '9') ++stripped;
    if (stripped != seg && *stripped == '_') ++stripped;
    else stripped = seg;

    for (int i = 0; i < GAME_COUNT; ++i) {
        if (equal_ignore_case(seg, GAMES[i].id) || equal_ignore_case(stripped, GAMES[i].id)) return i;
    }
    for (int i = 0; i < GAME_COUNT; ++i) {
        if (equal_ignore_case(seg, GAMES[i].name) || equal_ignore_case(what, GAMES[i].name)) return i;
    }
    return -1;
}

}  // namespace engine
