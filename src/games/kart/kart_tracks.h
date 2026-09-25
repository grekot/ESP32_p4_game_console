// Tory Karta: uklad (16 punktow kontrolnych linii srodkowej, zamknieta petla Catmull-Rom w swiecie 1024x1024),
// pola przyspieszenia, skrzynki, motyw graficzny i wzgorza.
//
// Nowy tor: dopisz wpis do TRACKS i sprawdz go:  python tools/kart_tracks_check.py --png podglad.png
// (margines 90, odstep miedzy odcinkami >= 120, zakret nie ostrzejszy niz na torze 0, miejsce na trybune przy starcie).
// Tor 0 ("Jezioro") to pierwotny tor gry - na nim ida testy regresji (tests/scenarios.txt: kart_*); nie zmieniac.
#pragma once

#include <stdint.h>

namespace kart {

// Motyw: tekstury (assets/kart/atlas_<id>.png - ten sam uklad kafelkow dla kazdego motywu), panorama gor
// (assets/kart/mountains_<id>.png), kolory nieba i mgly (RGB).
struct ThemeDef {
    const char* id;
    const char* name;
    uint8_t sky_top[3], sky_mid[3], sky_horiz[3], fog[3];
};

// Kolejnosc = indeks motywu w TrackDef::theme. Kolory mgly musza zgadzac sie z THEMES w tools/gen_kart_gemini.py
// (dol panoramy gor przechodzi w mgle horyzontu).
const ThemeDef THEMES[] = {
    { "jezioro", "\u0142\u0105ka", { 30, 86, 204 }, { 104, 164, 238 }, { 206, 222, 242 }, { 206, 222, 242 } },
    { "kanion", "pustynia", { 40, 110, 200 }, { 130, 180, 230 }, { 240, 214, 178 }, { 238, 212, 176 } },
    { "zima", "\u015bnieg", { 60, 110, 190 }, { 150, 186, 226 }, { 226, 234, 244 }, { 226, 234, 244 } },
    { "jesien", "jesie\u0144", { 50, 96, 180 }, { 150, 170, 210 }, { 232, 214, 196 }, { 230, 212, 194 } },
};
constexpr int THEME_COUNT = (int)(sizeof(THEMES) / sizeof(THEMES[0]));

struct TrackDef {
    const char* id;
    const char* name;
    int         theme;
    float       ctrl[16][2];
    int         boost[2];      // indeksy probek (0..255) z polami przyspieszenia
    int         box[3];        // indeksy probek z rzedami skrzynek
    float       hills;         // skala wysokosci wzgorz (1 = pierwotny tor)
    float       phase;         // przesuniecie fazy sinusow terenu (inny ksztalt wzgorz)
};

const TrackDef TRACKS[] = {
    { "jezioro", "Jezioro", 0,
      { { 200, 180 }, { 420, 130 }, { 640, 190 }, { 830, 150 }, { 910, 330 }, { 800, 470 }, { 620, 420 }, { 500, 560 },
        { 570, 720 }, { 760, 750 }, { 890, 880 }, { 640, 930 }, { 380, 880 }, { 200, 770 }, { 130, 560 }, { 230, 390 } },
      { 30, 150 }, { 44, 128, 212 }, 1.0f, 0.0f },
    { "kanion", "Kanion", 1,
      { { 170, 200 }, { 380, 130 }, { 600, 170 }, { 760, 290 }, { 895, 260 }, { 905, 450 }, { 800, 560 }, { 870, 740 },
        { 780, 910 }, { 590, 880 }, { 470, 740 }, { 340, 860 }, { 160, 850 }, { 110, 660 }, { 260, 520 }, { 120, 360 } },
      { 40, 170 }, { 60, 120, 214 }, 0.7f, 1.3f },
    { "zima", "Zimowa prze\u0142\u0119cz", 2,
      { { 180, 150 }, { 500, 130 }, { 820, 160 }, { 910, 340 }, { 720, 430 }, { 500, 380 }, { 320, 450 }, { 350, 620 },
        { 620, 580 }, { 860, 620 }, { 900, 860 }, { 620, 930 }, { 390, 830 }, { 200, 910 }, { 100, 700 }, { 130, 400 } },
      { 20, 200 }, { 50, 136, 222 }, 1.4f, 2.7f },
    { "jesien", "Jesienny las", 3,
      { { 180, 190 }, { 500, 150 }, { 820, 170 }, { 900, 360 }, { 760, 470 }, { 560, 440 }, { 400, 520 }, { 540, 630 },
        { 790, 610 }, { 900, 780 }, { 780, 920 }, { 500, 900 }, { 270, 925 }, { 110, 760 }, { 170, 520 }, { 110, 340 } },
      { 36, 176 }, { 70, 144, 220 }, 1.1f, 4.1f },
};
constexpr int TRACK_COUNT = (int)(sizeof(TRACKS) / sizeof(TRACKS[0]));

}  // namespace kart
