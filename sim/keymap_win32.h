// Mapowanie klawiszy konsoli na klawisze klawiatury PC (tylko emulator).
#pragma once

#include "input/keys.h"

namespace sim {

// Wczytuje mapowanie: najpierw wartosci domyslne, potem plik (jesli istnieje).
// explicit_path = nullptr -> szukaj keymap.cfg obok exe, potem sim/keymap.cfg, potem keymap.cfg.
void keymap_load(const char* explicit_path);

// Kod wirtualny Windows przypisany do klawisza konsoli.
int keymap_vk(input::Key key);

// Numer przycisku pada USB (1..32, jak w "Kontrolery gier" Windows) przypisany do klawisza konsoli; 0 = brak.
// Wpisy PAD_A = 1 w keymap.cfg. Domyslnie uklad Xbox przez winmm: A=1 B=2 X=3 Y=4 Back=7 Start=8.
int keymap_pad_button(input::Key key);

}  // namespace sim
