// PLAKAT API: jedna klatka pokazujaca kazda funkcje rysowania z podpisem. Zrodlo obrazka
// docs/images/api_plakat.png (docs/API.md). Nie jest lekcja - to sciagawka do ogladania.
//   console_sim.exe --game api_demo --hold X 0 1 --frames 5 --shot docs/images/api_plakat.bmp
// (--hold X: jedno wcisniecie klawisza chowa podpowiedzi wirtualnego pada na dole ekranu)
// Czcionka 16 px ma ok. 9,5 px na znak - podpisy planowane na max 20 znakow w kolumnie 195 px.
#include "console/console.h"

namespace {

const char* const STAR_ROWS[8] = {
    "...yy...",
    "...yy...",
    "yyyYYyyy",
    ".yyYYyy.",
    "..yYYy..",
    ".yy..yy.",
    "yy....yy",
    "........",
};

Sprite star;
Sprite hero_png;   // z pliku assets/api_demo/hero32.png

void label(int x, int y, const char* s)
{
    text(x, y, s, GRAY, 1);
}

void setup()
{
    star     = load_sprite(STAR_ROWS);
    hero_png = load_image("hero32.png");
}

void frame()
{
    clear(rgb(15, 23, 42));

    text(30, 12, "Console API - plakat", WHITE, 3);
    label(30, 50, "ekran 800 x 480 px, (0,0) w lewym gornym rogu, y rosnie w dol, kolor zawsze ostatni");

    // --- figury: 4 kolumny co 195 px, podpis w dwu liniach ---
    rect(30, 78, 140, 70, RED);
    label(30, 152, "rect(x, y, w, h,");
    label(30, 170, "     RED)");

    rect_outline(225, 78, 140, 70, YELLOW);
    label(225, 152, "rect_outline(x, y,");
    label(225, 170, "     w, h, YELLOW)");

    circle(490, 113, 35, GREEN);
    label(420, 152, "circle(sx, sy, r,");
    label(420, 170, "     GREEN)");

    circle_outline(685, 113, 35, SKY);
    label(615, 152, "circle_outline(sx, sy,");
    label(615, 170, "     r, SKY)");

    // --- tekst ---
    text(30, 196, "text rozmiar 1 (16 px)", WHITE, 1);
    text(30, 214, "text rozmiar 2 (24 px)", WHITE, 2);
    text(330, 198, "rozmiar 3", WHITE, 3);
    text(540, 186, "rozmiar 4", WHITE, 4);
    label(30, 246, "text(x, y, \"napis\", KOLOR, rozmiar 1-4)   text_width(napis, rozmiar)   text_height(rozmiar)");

    // --- kolory: 5 kolumn co 154 px, 4 rzedy ---
    const Color cols[] = { BLACK, WHITE, GRAY, DARK_GRAY, RED, DARK_RED, ORANGE, YELLOW, DARK_YELLOW,
                           GREEN, DARK_GREEN, BROWN, DARK_BROWN, BEIGE, SKIN, BLUE, DARK_BLUE, PURPLE, DARK_PURPLE, SKY };
    const char* names[] = { "BLACK", "WHITE", "GRAY", "DARK_GRAY", "RED", "DARK_RED", "ORANGE", "YELLOW", "DARK_YELLOW",
                            "GREEN", "DARK_GREEN", "BROWN", "DARK_BROWN", "BEIGE", "SKIN", "BLUE", "DARK_BLUE", "PURPLE",
                            "DARK_PURPLE", "SKY" };
    for (int i = 0; i < 20; i++) {
        int x = 30 + (i % 5) * 154;
        int y = 270 + (i / 5) * 36;
        rect(x, y, 140, 16, cols[i]);
        rect_outline(x, y, 140, 16, DARK_GRAY);
        label(x, y + 17, names[i]);
    }

    // --- sprite'y oraz linia i piksele ---
    sprite(star, 30, 438, false, 1);
    sprite(star, 50, 428, false, 2);
    sprite(star, 80, 414, false, 3);
    sprite(star, 120, 414, true, 3);
    label(30, 462, "sprite(s, x, y, odbicie, powiekszenie)");

    sprite(hero_png, 400, 398, false, 2);   // PNG 32x32 rysowany x2 = 64x64
    label(400, 462, "load_image(\"hero32.png\"), x2");

    line(680, 400, 780, 440, ORANGE);
    line(680, 440, 780, 400, ORANGE);
    for (int i = 0; i < 13; i++) pixel(680 + i * 8, 388, WHITE);
    label(680, 448, "line(...)");
    label(680, 464, "pixel(...)");

    watch("frame", frame_count());
}

}  // namespace

CONSOLE_ADD_GAME(api_demo, "Plakat API", "Wszystkie funkcje rysowania na jednym ekranie")
