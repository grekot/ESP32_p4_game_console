// LEKCJA 01 - WIZYTOWKA: zmienne, stale i wspolrzedne na ekranie.
// Zadania i wyjasnienia sa w README.md obok tego pliku.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj

// --- STALE: wartosci, ktore sie nie zmieniaja (slowo const) ---
const int   SIZE       = 80;        // bok kwadratu w pikselach
const Color BACKGROUND = DARK_BLUE; // kolor tla

// --- ZMIENNE: pudelka na liczby, ktore mozna zmieniac w trakcie gry ---
int square_x = 120;     // polozenie kwadratu (lewy gorny rog)
int square_y = 240;
int circle_x = 600;     // srodek kola
int circle_y = 280;
int radius   = 60;      // promien kola

const char* name = "Twoje imie";   // napis - zmien na swoje imie (bez polskich znakow)

void setup()
{
    // Na razie nic - wszystkie wartosci startowe sa nadane wyzej.
}

void frame()
{
    clear(BACKGROUND);

    // Napisy: text(x, y, "tekst", kolor, rozmiar 1-4)
    text(40, 40, "Wizytowka:", WHITE, 2);
    text(40, 80, name, YELLOW, 4);

    // Figury: prostokat (x, y, szerokosc, wysokosc, kolor), kolo (srodek_x, srodek_y, promien, kolor)
    rect(square_x, square_y, SIZE, SIZE, RED);
    circle(circle_x, circle_y, radius, GREEN);

    // Linia od (x0, y0) do (x1, y1)
    line(0, 460, screen_width() - 1, 460, WHITE);

    // Podglad zmiennej w rogu ekranu (i w sladzie emulatora --trace)
    watch("square_x", square_x);
}

}  // namespace

LAKE_GAME(wizytowka, "Wizytowka", "Lekcja 01: zmienne i figury")
