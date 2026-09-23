// Dotyk pojemnosciowy GT911 (do 5 punktow) przez I2C.
// Wspolrzedne zwracane sa w LOGICZNYM ukladzie ekranu gry (poziom, SCREEN_W x SCREEN_H),
// zgodnie z CONSOLE_DISPLAY_ROTATION. Przeliczenie na wspolrzedne plotna robi platform_esp.cpp.
#pragma once

#include <stdint.h>
#include "esp_err.h"

namespace board::touch {

struct Point {
    int16_t x;
    int16_t y;
};

constexpr int MAX_POINTS = 5;

esp_err_t init();

// Odczyt aktualnych punktow dotyku. Zwraca ich liczbe (0 = brak dotyku).
int read(Point* out, int max_points);

bool available();

}  // namespace board::touch
