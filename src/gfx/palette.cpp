#include "gfx/palette.h"

#include "core/log.h"

namespace gfx {

const PaletteEntry DEFAULT_PALETTE[] = {
    {'k', pal::BLACK},  {'w', pal::WHITE},      {'e', pal::GRAY},   {'E', pal::DARK_GRAY},
    {'r', pal::RED},    {'R', pal::DARK_RED},   {'o', pal::ORANGE},
    {'y', pal::YELLOW}, {'Y', pal::DARK_YELLOW},
    {'g', pal::GREEN},  {'G', pal::DARK_GREEN},
    {'b', pal::BROWN},  {'B', pal::DARK_BROWN}, {'t', pal::BEIGE},  {'s', pal::SKIN},
    {'u', pal::BLUE},   {'U', pal::DARK_BLUE},
    {'p', pal::PURPLE}, {'P', pal::DARK_PURPLE},
};

const int DEFAULT_PALETTE_N = (int)(sizeof(DEFAULT_PALETTE) / sizeof(DEFAULT_PALETTE[0]));

uint16_t default_palette_lookup(char key)
{
    for (int i = 0; i < DEFAULT_PALETTE_N; ++i) {
        if (DEFAULT_PALETTE[i].key == key) return DEFAULT_PALETTE[i].color;
    }
    LAKE_LOGW("palette", "brak koloru '%c' w palecie domyslnej", key);
    return TRANSPARENT;
}

}  // namespace gfx
