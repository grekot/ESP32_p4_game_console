// Licznik klatek na sekunde - wspolny dla plytki i emulatora.
#pragma once

#include <stdint.h>

namespace engine {

// Wolane raz na klatke przez petle aplikacji.
void stats_tick(int64_t now_us);

// Srednia z ostatniego okresu pomiarowego (~2 s).
float current_fps();

}  // namespace engine
