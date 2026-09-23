// Lista lekcji - jedna linia na lekcje, w kolejnosci, w jakiej maja sie pokazac w menu
// (Lake Mario jest zawsze pierwsze). Nazwa w LEKCJA(...) musi byc taka sama jak w CONSOLE_ADD_GAME(...)
// na koncu pliku gra.cpp danej lekcji. Ten plik jest wlaczany dwa razy przez games/registry.cpp
// (raz jako deklaracje, raz jako elementy tablicy), dlatego NIE ma #pragma once.
//
// Dodajesz lekcje? Skopiuj katalog 00_szablon, zmien CONSOLE_ADD_GAME i dopisz tu linie.

LEKCJA(szablon)
LEKCJA(wizytowka)
LEKCJA(pilka)
LEKCJA(lapacz)
LEKCJA(pong)
LEKCJA(inwazja)
LEKCJA(inwazja_strzal)
LEKCJA(waz)
LEKCJA(flappy)
LEKCJA(breakout)
LEKCJA(bohater)
LEKCJA(flappy_pelny)
LEKCJA(mini_mario)

// Nie lekcja: plakat wszystkich funkcji rysowania (docs/API.md). Ostatni w menu.
LEKCJA(api_demo)
