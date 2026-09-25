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

// Wylacza czekanie na 60 FPS w present() - tryb --frames liczy klatki tak szybko, jak sie da.
// Nie zmienia wynikow (krok czasu jest staly), tylko czas trwania testow.
void set_unthrottled(bool on);
bool unthrottled();

// Tryb skryptowany (--frames): okno ukryte, prawdziwa klawiatura i mysz ignorowane - liczy sie tylko --hold/--pause-at.
// Ustawiac PRZED platform::init().
// Bez tego klawisz wcisniety przez osobe pracujaca przy komputerze w trakcie testu (np. Enter = START -> pauza)
// psul slad i zrzut, a wzorzec nagrany w takiej chwili byl zly.
void set_ignore_real_input(bool on);
bool ignore_real_input();

}  // namespace sim
