// Mapowanie klawiszy konsoli na klawisze klawiatury PC (tylko emulator).
#pragma once

#include "input/keys.h"

namespace sim {

// Wczytuje mapowanie: najpierw wartosci domyslne, potem plik (jesli istnieje).
// explicit_path = nullptr -> szukaj keymap.cfg obok exe, potem sim/keymap.cfg, potem keymap.cfg.
void keymap_load(const char* explicit_path);

// Kod wirtualny Windows przypisany do klawisza konsoli.
int keymap_vk(input::Key key);

}  // namespace sim
