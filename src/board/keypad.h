// Klawiatura mechaniczna konsoli: 10 przelacznikow podlaczonych bezposrednio do GPIO na JP1.
//
// Odpytywanie i odklocanie (debounce) dzieje sie w osobnym zadaniu o czestotliwosci 1 kHz,
// zeby krotkie drgania stykow przelacznika mechanicznego (1-5 ms) nie dawaly falszywych
// wcisniec niezaleznie od tego, jak dlugo trwa klatka gry.
#pragma once

#include <stdint.h>

#include "esp_err.h"

namespace board::keypad {

// Konfiguruje piny i uruchamia zadanie odpytujace.
esp_err_t init();

// Bitmaska wcisnietych klawiszy (bity wg input::Key). 0 = nic nie wcisniete.
uint16_t held();

// false, jesli init() nie powiodl sie (konsola dziala wtedy na samym dotyku).
bool available();

}  // namespace board::keypad
