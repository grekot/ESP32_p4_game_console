// Funkcje dostepne tylko w emulatorze (nie ma ich na plytce).
#pragma once

#include <stdint.h>

#include "input/keys.h"

namespace sim {

// Zapisuje ostatnia wyswietlona klatke jako plik BMP (rozmiar plotna, 24 bity).
// Przydatne do zrzutow do dokumentacji i do sprawdzania regresji wygladu UI.
bool save_screenshot(const char* path);

// Sztuczne wcisniecie klawiszy konsoli (bitmaska wg input::Key) - pozwala skryptowac
// scenariusze bez udzialu czlowieka: zrzut ekranu pauzy, pomiar wysokosci skoku itp.
void     set_synthetic_keys(uint16_t mask);
uint16_t synthetic_keys();

}  // namespace sim
