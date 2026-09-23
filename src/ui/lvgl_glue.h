// Spiecie LVGL z plotnem gry i warstwa platformy.
//
// LVGL rysuje do wlasnego, malego bufora roboczego, a gotowe fragmenty sa kopiowane na plotno
// konsoli (800x480 RGB565, natywna rozdzielczosc - czcionki bez schodkow). Wyswietlanie (obrot) robi platform::present().
//
// Jak pokazac UI NA TLE gry: gra renderuje klatke, app kopiuje ja do osobnego bufora i podaje
// przez set_background_frame(). LVGL dostaje wtedy nieprzezroczyste tlo w postaci widgetu canvas
// i poprawnie sklada na nim swoje widgety. Proba rysowania widgetow wprost na pikselach gry
// (przezroczyste tlo ekranu) nie dziala - LVGL nie wie, co bylo w buforze wczesniej.
#pragma once

#include <stdint.h>

#include "input/pad.h"
#include "lvgl.h"

namespace ui {

// Podpina LVGL pod bufor plotna. Wolac raz, po platform::init().
bool init(uint16_t* canvas_buf);

// Ustawia nieruchoma klatke gry jako tlo UI (bufor musi zyc tak dlugo, jak tlo jest widoczne).
// nullptr = usun tlo.
void set_background_frame(const uint16_t* frame);

// Podaje LVGL stan klawiatury konsoli. Wolac przed tick(), gdy UI jest widoczne:
// GORA/DOL przenosza zaznaczenie, A albo START zatwierdzaja, B albo SELECT cofaja.
void feed_keys(const input::PadState& keys);

// Grupa nawigacyjna - ekrany UI dodaja do niej swoje przyciski, zeby dalo sie je wybrac klawiszami.
lv_group_t* nav_group();

// Wolac raz na klatke, gdy UI jest widoczne: przekazuje dotyk do LVGL i rysuje widgety na plotno.
void tick();

}  // namespace ui
