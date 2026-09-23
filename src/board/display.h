// Wyswietlacz: ST7701S 480x800 przez MIPI-DSI, podwojny bufor ramki w PSRAM.
//
// Panel jest fizycznie pionowy (480 szer. x 800 wys.). Gra rysuje w orientacji poziomej
// (800x480 lub jej calkowita podwielokrotnosc, np. 400x240), a present() skaluje i obraca
// obraz sprzetowo (PPA - Pixel Processing Accelerator) do tylnego bufora DPI, po czym
// przelacza bufory na najblizszym VSYNC.
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifndef CONSOLE_DISPLAY_ROTATION
#define CONSOLE_DISPLAY_ROTATION 90   // 90 albo 270
#endif
#if CONSOLE_DISPLAY_ROTATION != 90 && CONSOLE_DISPLAY_ROTATION != 270
#error "CONSOLE_DISPLAY_ROTATION musi byc 90 albo 270"
#endif

namespace board::display {

constexpr int PANEL_W  = 480;      // natywna szerokosc panelu (pion)
constexpr int PANEL_H  = 800;      // natywna wysokosc panelu
constexpr int SCREEN_W = PANEL_H;  // logiczny ekran gry (poziom)
constexpr int SCREEN_H = PANEL_W;

// Inicjalizacja LDO, magistrali DSI, panelu, buforow ramki i PPA. Wlacza podswietlenie.
esp_err_t init();

// Wyswietla klatke RGB565 o rozmiarze src_w x src_h (orientacja pozioma).
// Wymagane: SCREEN_W % src_w == 0 oraz SCREEN_W / src_w == SCREEN_H / src_h (calkowite skalowanie).
// Bufor src powinien byc wyrownany do 128 B (linia cache L2). Funkcja blokuje do VSYNC (~16 ms).
esp_err_t present(const uint16_t* src, int src_w, int src_h);

void set_backlight(bool on);

// Statystyki: ile razy nie doczekano sie sygnalu VSYNC (diagnostyka)
uint32_t vsync_timeouts();

}  // namespace board::display
