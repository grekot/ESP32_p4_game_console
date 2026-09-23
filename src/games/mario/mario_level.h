// Mapa poziomu jako ASCII. Poziom sklada sie z segmentow o szerokosci jednego ekranu (25 kafelkow),
// kazdy segment ma 15 wierszy (240 px / 16). Segmenty sa sklejane w poziomie przy ladowaniu.
//
// Legenda:
//   ' ' niebo          '#' ziemia (trawa)    '=' ziemia (glebiej)   'B' cegla (rozbijalna od dolu)
//   '?' blok z moneta  'U' blok zuzyty       '[' ']' gora rury      '{' '}' korpus rury
//   'o' moneta         'E' przeciwnik        'S' start gracza       'F' maszt   '^' szczyt masztu
//   'f' flaga (dekoracja, kolumna przed 'F')  'C' chmura (dekoracja)  'h' krzak (dekoracja)
#pragma once

namespace mario {

constexpr int LEVEL_ROWS     = 15;
constexpr int SEG_COLS       = 25;
constexpr int LEVEL_SEGMENTS = 6;
constexpr int LEVEL_MAX_COLS = SEG_COLS * LEVEL_SEGMENTS;

extern const char* const LEVEL1[LEVEL_SEGMENTS][LEVEL_ROWS];

}  // namespace mario
