// SZABLON GRY - punkt startowy kazdej lekcji.
//
// Jak zrobic z niego nowa gre:
//   1. Skopiuj caly katalog 00_szablon i nadaj kopii nazwe, np. 05_moja_gra.
//   2. W ostatniej linii zmien  LAKE_GAME(szablon, ...)  na  LAKE_GAME(moja_gra, "Moja gra", "opis").
//   3. W pliku src/games/lekcje/lista.h dopisz linie  LEKCJA(moja_gra).
//   4. Otworz gra.cpp i wcisnij F6 (albo Ctrl+Shift+B) - gra pojawi sie w oknie i w menu konsoli.
#include "lake/lake.h"

namespace {   // pudelko na twoja gre - nie usuwaj tej linii ani zamykajacej klamry na dole

// --- tutaj beda twoje zmienne ---

// Wykonuje sie RAZ, na starcie gry (jak "kiedy klikniete zielona flage" w Scratchu).
void setup()
{
}

// Wykonuje sie 60 razy na sekunde. Kazde wywolanie rysuje jedna klatke od nowa.
void frame()
{
    clear(BLACK);
    const char* napis = "TU BEDZIE TWOJA GRA";
    text((screen_width() - text_width(napis, 2)) / 2, 110, napis, WHITE, 2);
}

}  // namespace - koniec pudelka

LAKE_GAME(szablon, "Szablon", "Pusta gra do skopiowania")
