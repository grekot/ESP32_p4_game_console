#include "games/mario/mario_level.h"

namespace mario {

// Kazdy wiersz ma dokladnie 25 znakow (SEG_COLS). Krotsze sa dopelniane niebem przy ladowaniu.
//
// Zasady projektowe poziomu 1 wynikajace z fizyki (GRAVITY 1100, JUMP_V -350/-390):
//   skok z chodu:  ~3,5 kafelka w gore, ~4 kafelki w dal
//   skok z biegu:  ~4,3 kafelka w gore, ~7 kafelkow w dal
// Kazda przeszkoda na trasie ma byc do przejscia skokiem z CHODU. Bieg jest tylko do bonusow.
//   - sciany i rury na trasie: max 3 kafelki (wiersze 10-12)
//   - przepascie: max 3 kafelki
//   - platformy do stania: gora max 3 kafelki nad powierzchnia, z ktorej sie skacze
//   - bloki '?' do uderzenia glowa: wiersz >= 9 (gracz stoi w wierszu 12, glowa siega do 9)
// Wiersz 13 to trawa, 14 ziemia. Gracz stojacy na ziemi zajmuje wiersz 12.
const char* const LEVEL1[LEVEL_SEGMENTS][LEVEL_ROWS] = {
    // ---- segment 0: start, nauka skoku i uderzania blokow ----
    {
        "                         ",
        "                         ",
        "    CC                   ",
        "                    CC   ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "           ?             ",
        "       S          B?B    ",
        "                         ",
        "  h                 h    ",
        "#########################",
        "=========================",
    },
    // ---- segment 1: pierwsi przeciwnicy, niska i wysoka rura (3 kafelki - skok z chodu) ----
    {
        "                         ",
        "                         ",
        "  CC                     ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "            oo           ",
        "     B?B   BBBB          ",
        "                    []   ",
        "   E              E {}   ",
        "        []     h    {}   ",
        "#########################",
        "=========================",
    },
    // ---- segment 2: przepasc (3 kafelki) i platformy z monetami ----
    {
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "          o o            ",
        "         BBBBB           ",
        "                         ",
        "   oo           ?        ",
        "  BBBB                   ",
        "                   E     ",
        "     E      []           ",
        "########   ##############",
        "========   ==============",
    },
    // ---- segment 3: platformy i schody przed przepascia ----
    {
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "              oo         ",
        "        oo   UUUU        ",
        "                         ",
        "      BBBB           U   ",
        "   E                UU   ",
        "                   UUU   ",
        "######################   ",
        "======================   ",
    },
    // ---- segment 4: po skoku, niska scianka i bloki z monetami ----
    {
        "                         ",
        "                         ",
        "    CC                   ",
        "                         ",
        "                         ",
        "                         ",
        "                         ",
        "         ooo             ",
        "                         ",
        "        B?B?B            ",
        "              E    E     ",
        "   UU                    ",
        "   UU       []     h     ",
        "#########################",
        "=========================",
    },
    // ---- segment 5: meta ----
    {
        "                         ",
        "                         ",
        "             ^           ",
        "            fF           ",
        "             F           ",
        "             F           ",
        "             F           ",
        "             F           ",
        "             F           ",
        "             F           ",
        "        U    F           ",
        "       UU    F      CC   ",
        "      UUU    F  h        ",
        "#########################",
        "=========================",
    },
};

}  // namespace mario
