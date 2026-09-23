// Drobna matematyka 2D wspolna dla gier: min/max/abs, dojscie do wartosci, nachodzenie prostokatow,
// numer kafelka. Wszystko inline - zero kosztu, jedna definicja (wczesniej kopie w canvas.cpp i mario_game.cpp).
#pragma once

#include <math.h>

namespace engine {

inline int imin(int a, int b) { return a < b ? a : b; }
inline int imax(int a, int b) { return a > b ? a : b; }
inline int iabs(int a)        { return a < 0 ? -a : a; }
inline int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline float absf(float v) { return v < 0 ? -v : v; }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Przesuwa v w strone target o co najwyzej step (bez przeskoczenia celu). Do przyspieszania/hamowania.
inline float approach(float v, float target, float step)
{
    if (v < target) { v += step; return v > target ? target : v; }
    if (v > target) { v -= step; return v < target ? target : v; }
    return v;
}

// Czy prostokaty (lewy gorny rog + rozmiar) nachodza na siebie.
inline bool overlap(float ax, float ay, int aw, int ah, float bx, float by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

// Numer kafelka (kolumny/wiersza) dla wspolrzednej w pikselach; ujemne wspolrzedne daja ujemne numery.
inline int tile_of(float v, int tile) { return (int)floorf(v / (float)tile); }

}  // namespace engine
