// Generator liczb losowych konsoli (xorshift32).
//
// Wlasny, a nie rand(): ten sam ciag na plytce i w emulatorze, a po ustawieniu ziarna
// (seed_rng) scenariusze testowe --frames daja identyczny wynik za kazdym razem.
#pragma once

#include <stdint.h>

namespace engine {

struct Rng {
    uint32_t state = 0x4C414B45u;   // "LAKE"

    uint32_t next();                 // 32 bity
    int      range(int a, int b);    // a..b wlacznie (kolejnosc a/b dowolna)
    float    unit();                 // 0..1
};

// Globalny generator konsoli - z niego korzystaja gry (console::random).
Rng& rng();

// Ustawia ziarno (0 jest zamieniane na stala, xorshift nie moze startowac od zera).
void seed_rng(uint32_t seed);

// Tryb powtarzalny (staly krok czasu w testach --frames): gry nie moga wtedy zmieniac wygladu zaleznie od zegara
// (np. adaptacyjna jakosc Karta po czasie renderu) - inaczej zrzuty roznia sie pod obciazeniem CPU.
void set_deterministic(bool on);
bool deterministic();

}  // namespace engine
